#pragma once
#include "pch.h"
#include "framework.h"
#include <vector>
#include <list>
#include "Packet.h"

typedef void(*SOCKET_CALLBACK)(void* arg, int status,std::list<CPacket>&, CPacket&);

class CServerSocket//单例模式
{
public:
		static CServerSocket* GetInstance() {//静态函数没有this指针，只能访问静态成员变量
			if (m_instance == nullptr) {
				m_instance = new CServerSocket();
			}
			return m_instance;
		}
		int Run(SOCKET_CALLBACK callback, void* arg, short port = 9527)
		{
			bool ret = InitSocket(port);
			if (ret == false) return -1;
			std::list<CPacket> lstPackets;
			int count = 0;
			m_callback = callback;
			m_arg = arg;
			while (true) {
				if (AcceptClient() == false) {
					if (count >= 3) {
						return -2;
					}
					count++;
				}

				int cmdRet = DealCommand();
				if (cmdRet > 0) {
					m_callback(m_arg, cmdRet, lstPackets,m_packet);
					while (lstPackets.size() > 0) {
						Send(lstPackets.front());
						lstPackets.pop_front();
					}
				}
				CloseClient();
			}
			return 0;
		}
protected:
		bool InitSocket(short port) {
			if (m_sock == -1) return false;

			sockaddr_in serv_adr;
			memset(&serv_adr, 0, sizeof(serv_adr));
			serv_adr.sin_family = AF_INET;
			serv_adr.sin_addr.s_addr = INADDR_ANY;
			serv_adr.sin_port = htons(port);

			if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {// 绑定套接字；(套接字，地址结构体指针，结构体大小)
				return false;
			}
			if (listen(m_sock, 1) == -1) { // 监听连接；(套接字，等待队列大小)
				return false;
			}
			return true;
		}
		bool AcceptClient() {
			sockaddr_in client_adr;
			int cli_sz = sizeof(client_adr);
			m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz); // 接受连接；(套接字，地址结构体指针，结构体大小指针)
			TRACE("客户端已连接\n");
			if(m_client == -1) return false;

			return true;
		}
#define BUFFER_SIZE 4096
		int DealCommand() {
			if (m_client == -1) return -1;

			if (m_buffer.size() < 4096) m_buffer.resize(4096);

			while (true) {
				size_t parse_len = m_index;
				m_packet = CPacket((BYTE*)m_buffer.data(), parse_len);

				if (parse_len > 0) {
					memmove(m_buffer.data(), m_buffer.data() + parse_len, m_index - parse_len);
					m_index -= parse_len;
					return m_packet.sCmd;
				}

				if (m_index >= m_buffer.size()) {
					m_buffer.resize(m_buffer.size() * 2);
				}

				int len_received = recv(m_client, m_buffer.data() + m_index, m_buffer.size() - m_index, 0);

				if (len_received <= 0) {
					return -1;
				}

				m_index += len_received;
			}
			return -1;
		}
		bool Send(const char* pData, int nSize) {
			if (m_client == -1) return false;
			return send(m_client, pData, nSize, 0)>0;
		}
		bool Send(const CPacket& packet) {// 发送CPacket数据包
			if (m_client == -1) return false;
			size_t nSize = 6 + packet.nLength; // 包头(2) + 长度(4) + 命令(2) + 数据 + 校验(2)
			BYTE* pData = new BYTE[nSize];
			// ① 包头
			*(WORD*)(pData) = packet.sHead;
			// ② 长度
			*(DWORD*)(pData + 2) = packet.nLength;
			// ③ 命令
			*(WORD*)(pData + 6) = packet.sCmd;
			// ④ 数据体
			if (packet.nLength > 4)
			{
				memcpy(pData + 8, packet.strData.c_str(), packet.nLength - 4);
			}
			// ⑤ 校验码
			*(WORD*)(pData + 4 + packet.nLength) = packet.sSum;
			bool bRet = send(m_client, (const char*)pData, nSize, 0) > 0;
			delete[] pData;
			return bRet;
		}
		/*bool GetFilePath(std::string& strPath) {
			if ((m_packet.sCmd >= 2 && m_packet.sCmd <= 4) ||m_packet.sCmd==9 ) {
				strPath = m_packet.strData;
				return true;
			}
			return false;
		}*/
		/*bool GetMouseEvent(MOUSEEV &MouseEv) {
			if (m_packet.sCmd == 5) {
				if (m_packet.strData.size() == sizeof(MOUSEEV)) {
					memcpy(&MouseEv, m_packet.strData.c_str(), sizeof(MOUSEEV));
					return true;
				}
			}
			return false;
		}*/
		CPacket& GetPacket() {
			return m_packet;
		}
		void CloseClient() {
			if (m_client != INVALID_SOCKET) {
				closesocket(m_client);
				m_client = INVALID_SOCKET;
			}
		}
	private:
		SOCKET m_sock = INVALID_SOCKET;
		SOCKET m_client = INVALID_SOCKET;
		CPacket m_packet;
		std::vector<char> m_buffer;
		size_t m_index = 0;
		SOCKET_CALLBACK m_callback;
		void* m_arg;
		CServerSocket& operator=(const CServerSocket& ss) {}
		CServerSocket(const CServerSocket& ss) {};
		CServerSocket() {
			if (InitSockEnv() == FALSE) {
				MessageBoxW(NULL, _T("Socket环境初始化失败!"), _T("初始化错误"), MB_OK | MB_ICONERROR);
				exit(0);
			}
			m_sock = socket(PF_INET, SOCK_STREAM, 0); // 创建套接字；(地址族，套接字类型，协议)
			m_buffer.resize(BUFFER_SIZE);
			m_index = 0;
		}
		~CServerSocket() {
			closesocket(m_sock);
			closesocket(m_client);
			WSACleanup();
		}
		bool InitSockEnv() {
			WSADATA data;
			if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
				return FALSE;
			}
			return TRUE;
		}
		static void ReleaserInstance() {
			if (m_instance != nullptr) {
				CServerSocket* tmp = m_instance;
				m_instance = nullptr;
				delete tmp;
			}
		}
		static CServerSocket* m_instance;
		class CHelper {
		public:
			CHelper() {
				CServerSocket::GetInstance();
			}
			~CHelper() {
				CServerSocket::ReleaserInstance();
			}
		};
		static CHelper m_helper;
};
 
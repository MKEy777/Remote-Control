#pragma once
#include "pch.h"
#include "framework.h"

class CPacket {
public:
	CPacket():sHead(0), nLength(0), sCmd(0), strData(""), sSum(0) {}
	CPacket& operator=(const CPacket& packet) {
		if (this != &packet) {
			sHead = packet.sHead;
			nLength = packet.nLength;
			sCmd = packet.sCmd;
			strData = packet.strData;
			sSum = packet.sSum;
		}
		return *this;
	}
	//数据包封包构造函数，用于把数据封装成数据包
	CPacket(WORD nCmd,const BYTE* pData, size_t& nSize) {
		
	}

	//数据包解包，将包中的数据分配到成员变量中
	CPacket(const BYTE* pData, size_t& nSize)
	{
		size_t i = 0;

		// ① 查找包头 0xFEFF
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) // 判断包头标识（两个字节）
			{
				sHead = *(WORD*)(pData + i);   // 保存包头
				i += 2;                        // 跳过包头字节
				break;
			}
		}
		// 长度+命令+校验和
		if (i + 4 + 2 + 2 > nSize)
		{
			nSize = 0;     // 表示包不完整
			return;        
		}

		// ② 读取包总长度（4字节）
		nLength = *(DWORD*)(pData + i); // 从当前偏移位置取 4 字节长度
		i += 4;
		if (nLength + i > nSize)        // 判断数据是否接收完整
		{
			nSize = 0;                  // 数据不完整，继续接收
			return;                     // 解析失败
		}

		// ③ 读取命令字段（2字节）
		sCmd = *(WORD*)(pData + i);     // 取命令字
		i += 2;

		// ④ 读取包体数据
		if (nLength > 4) // 有数据区
		{
			strData.resize(nLength - 2 - 2);                        // 设置 string 大小
			// void * memcpy(void *_Dst, const void *_Src, size_t _Size) 
			memcpy(&strData[0], pData + i, nLength - 4);
			i += nLength - 4;                                        // 移动偏移量
		}

		// ⑤ 读取校验码并计算校验
		sSum = *(WORD*)(pData + i); // 从数据末尾读取 2 字节校验码
		i += 2;

		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) // 对数据部分计算校验和
			sum += BYTE(strData[j]) & 0xFF;         // 仅取低 8 位累加

		if (sum == sSum)   // 校验成功
		{
			nSize = i;    // 返回包总长度（包头 + 长度字段 + 命令 + 数据 + 校验）
			return;
		}

		// 校验失败
		nSize = 0;
	}

	~CPacket() {
		
	}
	WORD sHead;         //包头
	DWORD nLength;      //包长度,从命令字段到校验码的总长度
	WORD sCmd;          //命令字
	std::string strData;//数据
	WORD sSum;          //校验和
	
};


class CServerSocket//单例模式
{
	public:
		static CServerSocket* GetInstance() {//静态函数没有this指针，只能访问静态成员变量
			if (m_instance == nullptr) {
				m_instance = new CServerSocket();
			}
			return m_instance;
		}
		bool InitSocket() {
			if (m_sock == -1) return false;

			sockaddr_in serv_adr;
			memset(&serv_adr, 0, sizeof(serv_adr));
			serv_adr.sin_family = AF_INET;
			serv_adr.sin_addr.s_addr = INADDR_ANY;
			serv_adr.sin_port = htons(9527);

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
			if(m_client == -1) return false;

			return true;
		}
#define BUFFER_SIZE 4096
		int DealCommand() {
			if (m_client == -1) return -1;
			char* buffer=new char[4096];
			memset(buffer, 0, 4096);
			int index = 0;//buffer中已接收数据的长度
			while (true)
			{
				int len = recv(m_client, buffer, BUFFER_SIZE-index, 0);
				if (len <= 0) return -1;
				index += len;
				len = index;
                m_packet=CPacket((BYTE*)buffer, (size_t&)len);
				if (len > 0) {
					memmove(buffer, buffer + len, sizeof(buffer) - len);//移除已处理的数据
					index -= len;
					return m_packet.sCmd;
				}

			}
			return 0;
		}
		bool Send(const char* pData, int nSize) {
			if (m_client == -1) return false;
			return send(m_client, pData, nSize, 0)>0;
		}
	private:
		SOCKET m_sock = INVALID_SOCKET;
		SOCKET m_client = INVALID_SOCKET;
		CPacket m_packet;
		CServerSocket& operator=(const CServerSocket& ss) {}
		CServerSocket(const CServerSocket& ss) {};
		/*
		CServerSocket(const CServerSocket& ss){
			m_sock = ss.m_sock;
			m_client = ss.m_client;
		}
		*/
		CServerSocket() {
			if (InitSockEnv() == FALSE) {
				MessageBoxW(NULL, _T("Socket环境初始化失败!"), _T("初始化错误"), MB_OK | MB_ICONERROR);
				exit(0);
			}
			m_sock = socket(PF_INET, SOCK_STREAM, 0); // 创建套接字；(地址族，套接字类型，协议)
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
 
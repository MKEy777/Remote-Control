#pragma once
#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <afxsock.h>

class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), strData(""), sSum(0) {}
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
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}
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

	int Size() {//包数据的大小
		return nLength + 6;
	}
	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)(pData) = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}
	WORD sHead;         //包头
	DWORD nLength;      //包长度,从命令字段到校验码的总长度
	WORD sCmd;          //命令字
	std::string strData;//数据
	WORD sSum;          //校验和
	std::string strOut;//整个包的数据
};
typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//点击、移动、双击
	WORD nButton;//左键、中键、右键
	POINT ptXY;//坐标
}MOUSEEV, * PMOUSEEV;
typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;//是否有效
	BOOL IsDirectory;//是否为目录 0 否 1 是
	BOOL HasNext;//是否还有后续 0 没有 1 有
	char szFileName[256];//文件名
}FILEINFO, * PFILEINFO;


std::string GetErrInfo(int wsaErrcode);

class CClientSocket//单例模式
{
public:
	static CClientSocket* GetInstance() {//静态函数没有this指针，只能访问静态成员变量
		if (m_instance == nullptr) {
			m_instance = new CClientSocket();
		}
		return m_instance;
	}
	bool InitSocket(const std::string& strIPAddress) {
		if (m_sock == -1) return false;

		sockaddr_in ser_adr;
		memset(&ser_adr, 0, sizeof(ser_adr));
		ser_adr.sin_family = AF_INET;
		ser_adr.sin_addr.s_addr = inet_addr(strIPAddress.c_str());
		ser_adr.sin_port = htons(9527);

		int ret = connect(m_sock, (sockaddr*)&ser_adr, sizeof(ser_adr)); // 连接服务器；(套接字，地址结构体指针，结构体大小)
		if (ret == -1) {
			AfxMessageBox("无法连接到服务器!");
			TRACE("无法连接到服务器!\n",WSAGetLastError,GetErrInfo(WSAGetLastError()).c_str());
			return false;	
		}
		m_index = 0;
		return true;
	}
	
#define BUFFER_SIZE 2048000
	int DealCommand() {
		if (m_sock == -1) return -1;
		char* buffer = m_buffer.data();
		if (buffer == NULL) {
			TRACE("内存不足！\r\n");
			return -2;
		}
		while (true) {
			int len_received = recv(m_sock, buffer + m_index, BUFFER_SIZE - m_index, 0);

			if (len_received <= 0) {
				m_index = 0;
				return -1;
			}

			// 数据接收正常，累加索引
			m_index += len_received;
			size_t parse_len = m_index; 
			m_packet = CPacket((BYTE*)buffer, parse_len); // CPacket 会修改 parse_len

			if (parse_len > 0) {
				memmove(buffer, buffer + parse_len, m_index - parse_len);				// memmove(目标, 源, 长度)
				m_index -= parse_len; // 更新剩余长度
				return m_packet.sCmd; // 返回命令
			}
			if (m_index >= BUFFER_SIZE) {
				m_index = 0; // 丢弃所有数据
				return -1;
			}
		}
		return -1;
	}
	bool Send(const char* pData, int nSize) {
		if (m_sock == -1) return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}
	bool Send(const CPacket& packet) {// 发送CPacket数据包
		if (m_sock == -1) return false;
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
		bool bRet = send(m_sock, (const char*)pData, nSize, 0) > 0;
		delete[] pData;
		return bRet;
	}
	bool GetFilePath(std::string& strPath) {
		if (m_packet.sCmd >= 2 && m_packet.sCmd <= 4) {
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent(MOUSEEV& MouseEv) {
		if (m_packet.sCmd == 5) {
			if (m_packet.strData.size() == sizeof(MOUSEEV)) {
				memcpy(&MouseEv, m_packet.strData.c_str(), sizeof(MOUSEEV));
				return true;
			}
		}
		return false;
	}
	CPacket& GetPacket() {
		return m_packet;
	}

private:
	std::vector<char> m_buffer;
	size_t m_index = 0; // 记录缓冲区当前有效数据长度
	SOCKET m_sock = INVALID_SOCKET;
	CPacket m_packet;
	CClientSocket& operator=(const CClientSocket& ss) {}
	CClientSocket(const CClientSocket& ss) {};
	CClientSocket() {
		if (InitSockEnv() == FALSE) {
			MessageBoxA(NULL, "无法初始化套接字环境,请检查网络设置", "初始化错误", MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
		m_index = 0;
		m_sock = INVALID_SOCKET;
	}
	~CClientSocket() {
		closesocket(m_sock);
		WSACleanup();
	}
	bool InitSockEnv() {
		if (!AfxSocketInit()) {
			return FALSE;
		}
		return TRUE;
	}
	static void ReleaserInstance() {
		if (m_instance != nullptr) {
			CClientSocket* tmp = m_instance;
			m_instance = nullptr;
			delete tmp;
		}
	}
	static CClientSocket* m_instance;
	class CHelper {
	public:
		CHelper() {
			CClientSocket::GetInstance();
		}
		~CHelper() {
			CClientSocket::ReleaserInstance();
		}
	};
	static CHelper m_helper;
};


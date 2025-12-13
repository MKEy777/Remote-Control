#pragma once
#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <afxsock.h>
#include <mutex>
#include <map>
#include <list>

#pragma pack(push)
#pragma pack(1)

#define WM_SEND_PACK (WM_USER+1) //发送包数据
#define WM_SEND_PACK_ACK (WM_USER+2) //发送包数据应答

class CPacket {
public:
	CPacket() = default;
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
			memcpy(&strData[0], pData, nSize);
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

	~CPacket() {}

	int Size() {//包数据的大小
		return nLength + 6;
	}
	const char* Data(std::string& strOut) const {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)(pData) = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}
	WORD sHead = 0;
	DWORD nLength = 0;
	WORD sCmd = 0;
	WORD sSum = 0;
	std::string strData;
	std::string strOut;
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

enum {
	CSM_AUTOCLOSE = 1,//CSM = Client Socket Mode 自动关闭模式
};

typedef struct PacketData {
	std::string strData;
	UINT nMode;//AUTOCLOSE
	WPARAM wParam;
	PacketData(const char* pData, size_t nLen, UINT mode, WPARAM nParam = 0) {
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);
		nMode = mode;
		wParam = nParam;
	}
	PacketData(const PacketData& data) {
		strData = data.strData;
		nMode = data.nMode;
		wParam = data.wParam;
	}
	PacketData& operator=(const PacketData& data) {
		if (this != &data) {
			strData = data.strData;
			nMode = data.nMode;
			wParam = data.wParam;
		}
		return *this;
	}
}PACKET_DATA;

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
	bool InitSocket() {
		if (m_sock != -1) CloseSocket();
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (m_sock == -1) return false;

		sockaddr_in ser_adr;
		memset(&ser_adr, 0, sizeof(ser_adr));
		ser_adr.sin_family = AF_INET;
		ser_adr.sin_addr.s_addr =m_nIP;
		ser_adr.sin_port = htons(m_nPort);

		int ret = connect(m_sock, (sockaddr*)&ser_adr, sizeof(ser_adr)); // 连接服务器；(套接字，地址结构体指针，结构体大小)
		if (ret == -1) {
			AfxMessageBox(_T("无法连接到服务器!"));
			TRACE("无法连接到服务器!\n",WSAGetLastError,GetErrInfo(WSAGetLastError()).c_str());
			return false;	
		}
		m_index = 0;
		return true;
	}
	
	 //在“接收线程”里不停解析服务器发来的数据包。
#define BUFFER_SIZE 20480000
	int DealCommand();

	//主要用于发送数据到服务端，然后接收从服务端返回的消息，并且SendMessage()到对应的界面回调函数中
	bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed = true, WPARAM wParam = 0);
	
	/*bool GetFilePath(std::string& strPath) {
		if (m_packet.sCmd >= 2 && m_packet.sCmd <= 4) {
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}*/
	/*bool GetMouseEvent(MOUSEEV& MouseEv) {
		if (m_packet.sCmd == 5) {
			if (m_packet.strData.size() == sizeof(MOUSEEV)) {
				memcpy(&MouseEv, m_packet.strData.c_str(), sizeof(MOUSEEV));
				return true;
			}
		}
		return false;
	}*/
	void CloseSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
	CPacket& GetPacket() {
		return m_packet;
	}
	void UpdateAddress(int nIP, int nPort) {
		if ((m_nIP != nIP) || (m_nPort != nPort)) {
			m_nIP = nIP;
			m_nPort = nPort;
		}
	}
private:
	HANDLE m_eventInvoke = NULL;
	UINT   m_nThreadID = 0;
	HANDLE m_hThread = INVALID_HANDLE_VALUE;

	int    m_nIP = INADDR_ANY;
	int    m_nPort = 0;
	bool   m_bAutoClose = true;

	size_t m_index = 0;// 记录缓冲区当前有效数据长度
	SOCKET m_sock = INVALID_SOCKET;
	std::map<HANDLE, std::list<CPacket>&> m_mapAck;//等待应答的包
	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);//指向类的非 static 成员函数
	std::map<UINT, MSGFUNC> m_mapFunc;
	std::vector<char> m_buffer;
	std::mutex m_lock;
	std::map<HANDLE, bool> m_mapAutoClosed;
	CPacket m_packet;
	CClientSocket& operator=(const CClientSocket& ss) = delete;
	CClientSocket(const CClientSocket& ss)= delete;
	CClientSocket();
	~CClientSocket() {
		if (m_hThread != INVALID_HANDLE_VALUE) {
			PostThreadMessage(m_nThreadID, WM_QUIT, 0, 0); // 告诉线程退出
			WaitForSingleObject(m_hThread, INFINITE);
			CloseHandle(m_hThread);
			m_hThread = INVALID_HANDLE_VALUE;
		}

		// 关闭事件句柄
		if (m_eventInvoke) {
			CloseHandle(m_eventInvoke);
			m_eventInvoke = NULL;
		}

		// 关闭套接字
		if (m_sock != INVALID_SOCKET) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
		}

		WSACleanup();
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
	//主要用于发送数据到服务端，然后接收从服务端返回的消息，并且SendMessage()到对应的界面回调函数中
	void SendPack(UINT nMsg, WPARAM wParam/*缓冲区的值*/, LPARAM lParam/*缓冲区的长度*/);
	static unsigned __stdcall threadEntry(void* arg);
	void threadFunc2();

	bool InitSockEnv() {
		if (!AfxSocketInit()) {
			return FALSE;
		}
		return TRUE;
	}
	static void ReleaseInstance() {
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
			CClientSocket::ReleaseInstance();
		}
	};
	static CHelper m_helper;
};


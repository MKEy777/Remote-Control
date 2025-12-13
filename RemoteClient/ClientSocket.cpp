#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::m_instance = nullptr;
CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket::CClientSocket()
	: m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET),
	m_bAutoClose(true), m_index(0), m_hThread(INVALID_HANDLE_VALUE),
	m_eventInvoke(NULL), m_nThreadID(0)
{
	// 初始化套接字环境
	if (!InitSockEnv()) {
		MessageBox(NULL, _T("无法初始化套接字环境!"), _T("错误"), MB_OK | MB_ICONERROR);
		throw std::runtime_error("InitSockEnv failed");
	}

	// 创建事件句柄（手动重置，初始状态为未触发）
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!m_eventInvoke) {
		TRACE("CreateEvent failed: %d\n", GetLastError());
		throw std::runtime_error("CreateEvent failed");
	}

	// 初始化接收缓冲区
	m_buffer.resize(BUFFER_SIZE);
	memset(m_buffer.data(), 0, BUFFER_SIZE);

	// 使用 vector 替代静态数组，避免成员函数指针对齐问题
	std::vector<std::pair<UINT, MSGFUNC>> funcs = {
		{ WM_SEND_PACK, &CClientSocket::SendPack }
	};

	for (auto& f : funcs) {
		if (!m_mapFunc.insert({ f.first, f.second }).second) {
			TRACE("插入失败 消息 %d\n", f.first);
		}
	}

	// 启动接收线程
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, threadEntry, this, 0, &m_nThreadID);
	if (!m_hThread) {
		TRACE("线程启动失败!\n");
		throw std::runtime_error("Thread start failed");
	}
	// 等待线程启动完成
	DWORD dw = WaitForSingleObject(m_eventInvoke, 1000); // 等 1 秒
	if (dw == WAIT_TIMEOUT) {
		TRACE("线程启动超时\n");
	}
}

std::string GetErrInfo(int wsaErrcode) {
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		wsaErrcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}

//typedef struct tagMSG {
//	HWND        hwnd;
//	UINT        message;
//	WPARAM      wParam;
//	LPARAM      lParam;
//	DWORD       time;
//	POINT       pt;
//} MSG
void CClientSocket::threadFunc2()
{
	if (m_eventInvoke) {
		SetEvent(m_eventInvoke); // 唤醒构造函数等待
	}
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		TRACE("Get Message :%08X \r\n", msg.message);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}

unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}

int CClientSocket::DealCommand()
{
	if (m_sock == -1) return -1;
	char* buffer = m_buffer.data();
	if (buffer == NULL) {
		TRACE("内存不足！\r\n");
		return -2;
	}
	while (true) {
		// 先看看缓冲区里现有的数据够不够拼成一个包
		size_t parse_len = m_index;// 把当前缓冲区数据长度传进去
		m_packet = CPacket((BYTE*)buffer, parse_len);

		if (parse_len > 0) {
			// 发现完整包，处理掉，把剩下的移到前面
			memmove(buffer, buffer + parse_len, m_index - parse_len);
			m_index -= parse_len;
			return m_packet.sCmd;
		}

		// 缓冲区数据不够，继续接收
		if (m_index >= m_buffer.size()) {
			size_t new_size = m_buffer.size() * 2;
			// 可以在这里加个上限，防止恶意攻击撑爆内存
			if (new_size > 1024 * 1024 * 50) return -1;
			m_buffer.resize(new_size);
			buffer = m_buffer.data();
		}

		int len_received = recv(m_sock, buffer + m_index, BUFFER_SIZE - m_index, 0);
		if (len_received <= 0) {
			// 只有当真正断开或出错时才复位
			return -1;
		}
		m_index += len_received;
	}
	return -1;
}

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM wParam)
{
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData, (LPARAM)hWnd);
	if (ret == false) {
		delete pData;
	}
	return ret;
}

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{//定义一个消息的数据结构(数据和数据长度，模式) 回调消息的的数据结构(HWND))
	PACKET_DATA data = *(PACKET_DATA*)wParam;
	delete (PACKET_DATA*)wParam;
	HWND hWnd = (HWND)lParam;
	size_t nTemp = data.strData.size();
	CPacket current((BYTE*)data.strData.c_str(), nTemp);
	if (InitSocket() == true) {
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret > 0) {
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);
			char* pBuffer = (char*)strBuffer.c_str();
			while (m_sock != INVALID_SOCKET) {
				int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
				if (length > 0 || (index > 0)) {
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, nLen);
					if (nLen > 0) {
						TRACE("ack pack %d to hWnd %08X %d %d\r\n", pack.sCmd, hWnd, index, nLen);
						TRACE("%04X\r\n", *(WORD*)(pBuffer + nLen));
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);
						if (data.nMode & CSM_AUTOCLOSE) {
							CloseSocket();
							return;
						}
						index -= nLen;
						memmove(pBuffer, pBuffer + nLen, index);
					}
				}else {//对方关闭了套接字，或者网络设备异常
					TRACE("recv failed length %d index %d cmd %d\r\n", length, index, current.sCmd);
					CloseSocket();
					::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(current.sCmd, NULL, 0), 1);
				}
			}
		}else {
			CloseSocket();
			//网络终止处理
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
		}
	}else {
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);
	}
}
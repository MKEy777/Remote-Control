#pragma once
#include "Thread.h"
#include <map>

class CClient {

};

enum IOCPOperator{
	None,
	Accept,
	Recv,
	Send,
	Error
};
class CServer;
class COverlapped {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;
	std::vector<char>m_buffer;
	ThreadWorker m_worker;//处理函数
	CServer* m_server;//服务器对象
	CClient* m_client;//对应的客户端
	WSABUF m_wsabuffer;
	virtual ~COverlapped() {
		m_client = NULL;
	}
};

template<IOCPOperator>
class AcceptOverlapped : public COverlapped,ThreadFuncBase {
public:
	AcceptOverlapped():m_operator(Accept) , m_worker(this,&AcceptOverlapped::AcceptWorker){
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}
	int AcceptWorker() {
		return 0;
	}
};
typedef AcceptOverlapped<Accept> ACCEPTOVERLAPPED;

template<IOCPOperator>
class RecvOverlapped : public COverlapped, ThreadFuncBase {
public:
	RecvOverlapped() :m_operator(Recv), m_worker(this, &RecvOverlapped::RecvWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024*256);
	}
	int RecvWorker() {
		return 0;
	}
};
typedef RecvOverlapped<Recv> RECVOVERLAPPED;

template<IOCPOperator>
class SendOverlapped : public COverlapped, ThreadFuncBase {
public:
	SendOverlapped() :m_operator(Send), m_worker(this, &SendOverlapped::SendWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024 * 256);
	}
	int SendWorker() {
		return 0;
	}
};
typedef SendOverlapped<Send> SENDOVERLAPPED;

template<IOCPOperator>
class ErrorOverlapped : public COverlapped, ThreadFuncBase {
public:
	ErrorOverlapped() :m_operator(Error), m_worker(this, &ErrorOverlapped::ErrorWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}
	int ErrorWorker() {
		return 0;
	}
};
typedef ErrorOverlapped<Accept> ERROROVERLAPPED;

class CSever : public ThreadFuncBase
{
public:
    CSever(const std::string& ip="0.0.0.0",short port =9527): m_pool(10), m_hIOCP(NULL), m_sock(INVALID_SOCKET) {
		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
		m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		int opt = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = inet_addr(ip.c_str());  
		if (bind(m_sock, (sockaddr*)&addr, sizeof(addr)) == -1) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return;
		}
		if (listen(m_sock, 3) == -1) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return;
		}
		m_hIOCP = CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, 0, 4);
		if (m_hIOCP == NULL) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return;
		}
		CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 4);
		m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&CSever::threadIocp));

	}
	~CSever() {
		std::map<SOCKET, CClient*>::iterator it = m_client.begin();
		for (; it != m_client.end(); it++) {
			delete it->second;
			it->second = NULL;
		}
		m_client.clear();
	}
private:
	int threadIocp() {
		DWORD transferred = 0;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* lpOverlapped = NULL;
		if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITE)) {
			if (transferred > 0 && CompletionKey != 0) {
				COverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, COverlapped, m_overlapped);
				switch (pOverlapped->m_operator) {
				case Accept: {
					ACCEPTOVERLAPPED* pAccept = (ACCEPTOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pAccept->m_worker);
				}
				break;
				case Recv: {
					RECVOVERLAPPED* pRecv = (RECVOVERLAPPED*)pOverlapped;
					if (pRecv->m_client != NULL) {
						// 处理接收的数据
						pRecv->m_buffer.resize(transferred);
						// 这里可以处理接收到的数据

						// 重新投递接收操作
						pRecv->m_buffer.resize(1024 * 256);
						m_pool.DispatchWorker(pRecv->m_worker);
					}
				}
				break;
				case Send: {
					SENDOVERLAPPED* pSend = (SENDOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pSend->m_worker);
				}
				break;
				case Error: 
				{
					ERROROVERLAPPED* pError = (ERROROVERLAPPED*)pOverlapped;
					if (pError->m_client != NULL) {
						// 处理错误，清理客户端连接
						std::map<SOCKET, CClient*>::iterator it = m_client.find(
							reinterpret_cast<SOCKET>(pError->m_client)
						);
						if (it != m_client.end()) {
							delete it->second;
							m_client.erase(it);
						}
					}
				}
				break;
				}
			}
			else {
				return -1;
			}
		}
		return 0;
	}
	CThreadPool m_pool;
    HANDLE m_hIOCP;
    SOCKET m_sock;
    std::map<SOCKET, CClient*> m_client;
};


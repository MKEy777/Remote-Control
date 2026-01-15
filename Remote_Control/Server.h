#pragma once
#include <MSWSock.h>
#include "Thread.h"
#include <map>
#include "Queue.h"
#include "Tool.h"

enum IOCPOperator {
	None,
	Accept,
	Recv,
	Send,
	Error
};

class CServer;
class CClient;
typedef std::shared_ptr<CClient> PCLIENT;

class COverlapped :public ThreadFuncBase {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;
	std::vector<char>m_buffer;
	ThreadWorker m_worker;//处理函数
	CServer* m_server;//服务器对象
	CClient* m_client;//对应的客户端
	WSABUF m_wsabuffer;//WSA缓冲区
	virtual ~COverlapped() {
		m_client = NULL;
	}
};
template<IOCPOperator>class AcceptOverlapped;
typedef AcceptOverlapped<Accept> ACCEPTOVERLAPPED;
template<IOCPOperator>class RecvOverlapped;
typedef RecvOverlapped<Recv> RECVOVERLAPPED;
template<IOCPOperator>class SendOverlapped;
typedef SendOverlapped<Send> SENDOVERLAPPED;
template<IOCPOperator>class ErrorOverlapped;
typedef ErrorOverlapped<Error> ERROROVERLAPPED;

class CClient :public ThreadFuncBase {
public:
	CClient();
	~CClient();

	void AddRef() {
		m_refCount++;
	}
	void Release() {
		// fetch_sub 返回修改前的值。如果返回 1，说明减完变成了 0
		if (m_refCount.fetch_sub(1) == 1) {
			delete this;
		}
	}

	operator SOCKET() {
		return m_sock;
	}
	operator PVOID() {
		return (PVOID)m_buffer.data();
	}
	operator LPOVERLAPPED();
	operator LPDWORD() {
		return &m_received;
	}
	void SetOverlapped(CClient* ptr);
	LPWSABUF RecvWSABuffer();
	LPOVERLAPPED RecvOverlapped();
	LPWSABUF SendWSABuffer();
	LPOVERLAPPED SendOverlapped();
	DWORD& flags() { return m_flags; }
	sockaddr_in* GetLocalAddr() { return &m_laddr; }
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }
	size_t GetBufferSize()const { return m_buffer.size(); }
	int Recv();
	int Send(void* buffer, size_t nSize);
	int SendData(std::vector<char>& data);

private:
	std::atomic<int> m_refCount;
	SOCKET m_sock;
	DWORD m_received;
	DWORD m_flags;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
	std::shared_ptr<RECVOVERLAPPED> m_recv;
	std::shared_ptr<SENDOVERLAPPED> m_send;
	std::vector<char> m_buffer;//client 自己的 buffer（AcceptEx 用到）
	size_t m_used;//已经使用的缓冲区大小
	sockaddr_in m_laddr;
	sockaddr_in m_raddr;
	bool m_isbusy;
	CSendQueue<CClient, std::vector<char>> m_vecSend;//发送数据队列
};

template<IOCPOperator op>
class AcceptOverlapped : public COverlapped {
public:
	AcceptOverlapped();
	virtual ~AcceptOverlapped() {}
	int AcceptWorker();
};

template<IOCPOperator>
class RecvOverlapped : public COverlapped {
public:
	RecvOverlapped();
	virtual ~RecvOverlapped() {}
	int RecvWorker() {
		int ret = m_client->Recv();
		if (ret < 0) {
			m_server->CloseClient(m_client);
			m_client->Release();
			return -1;
		}
		m_client->AddRef();

		DWORD flags = 0;
		DWORD recvBytes = 0;
		m_wsabuffer.len = 0;
		int res = WSARecv((SOCKET)*m_client, &m_wsabuffer, 1, &recvBytes, &flags, &m_overlapped, NULL);
		if (res == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
			m_client->Release();
			m_server->CloseClient(m_client);
			m_client->Release();
			return -1; 
		}
		m_client->Release();
		return ret;
	}
};

using SENDCALLBACK = CSendQueue<CClient, std::vector<char>>::Callback;

template<IOCPOperator>
class SendOverlapped : public COverlapped{
public:
	SendOverlapped();
	virtual ~SendOverlapped() {}
	int SendWorker() {
		return -1;
	}
};

template<IOCPOperator>
class ErrorOverlapped : public COverlapped{
public:
	ErrorOverlapped() :m_operator(Error), m_worker(this, &ErrorOverlapped::ErrorWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}
	virtual ~ErrorOverlapped() {}
	int ErrorWorker() {
		return -1;
	}
};

class CServer : public ThreadFuncBase
{
public:
	CServer(const std::string& ip = "0.0.0.0", short port = 9527);
	~CServer();

	//初始化监听 socket、IOCP、启动线程池、投递 AcceptEx
	bool StartService();
	//创建一个新 CClient，投递 AcceptEx 等待新连接
	bool NewAccept();
	void BindNewSocket(SOCKET s, ULONG_PTR nKey);
	void CloseClient(CClient* client);
	SOCKET GetListenSocket() const { return m_sock; }
private:
	void CreateSocket();
	//IOCP 线程主循环：GetQueuedCompletionStatus 取完成事件并分发
	int threadIocp();

	CThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	sockaddr_in m_addr;
	std::mutex m_clientLock;
	std::map<SOCKET, CClient*> m_client;
};


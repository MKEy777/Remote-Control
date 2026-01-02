#pragma once
#include "pch.h"
#include <atomic>
#include <list>
#include "Thread.h"
#pragma warning(disable:4407)

template<class T>
class CQueue
{//线程安全队列(利用IOCP)
public:
	typedef struct IocpParam {
		size_t nOperator;//操作
		T Data;//数据
		HANDLE hEvent;//pop时使用，用于通知调用线程

		IocpParam() :nOperator(-1) {}
		IocpParam(size_t op, const T& data, HANDLE hEve=NULL) :nOperator(op), Data(data), hEvent(hEve) {}
	}PPARAM;//IOCP参数结构体
	enum {
		QNone,
		QPush,
		QPop,
		QSize,
		QClear
	};
public:
	CQueue() {
		m_lock = false;
		m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		m_hThread = INVALID_HANDLE_VALUE;
		if(m_hCompletionPort!=nullptr)
			m_hThread = (HANDLE)_beginthread(&CQueue<T>::threadEntry, 0, this);
	}

	virtual ~CQueue() {
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);
		if (m_hCompletionPort != NULL) {
			HANDLE hTemp = m_hCompletionPort;
			m_hCompletionPort=NULL;
			CloseHandle(hTemp);
		}
	}

	bool PushBack(const T & data) {
		if (m_lock) return false;
		IocpParam* pParam = new IocpParam(QPush, data);
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (!ret) delete pParam;
		//printf("push to queue: %s\r\n", data.c_str());
		return ret;
	}

	virtual bool PopFront(T& data) {
		if (m_lock) return false;
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (hEvent == NULL) {
			return false;
		}
		IocpParam Param = IocpParam(QPop, data, hEvent);
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (m_lock) {
			CloseHandle(hEvent);
			return false;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		CloseHandle(hEvent);
		if (ret) data = Param.Data;
		return ret;
	}

	size_t Size() {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (hEvent == NULL) {
			return false;
		}
		IocpParam Param =IocpParam(QSize, T(), hEvent);
		if (m_lock) {
			if (hEvent)CloseHandle(hEvent);
			return -1;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (!ret) {
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		CloseHandle(hEvent);
		if (ret) return Param.nOperator;
		return -1;
	}

	bool Clear() {
		if (m_lock) return false;
		IocpParam* pParam = new IocpParam(QClear, T());
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (!ret) delete pParam;
		return ret;
	}
protected:
	virtual void DealParam(PPARAM* pParam) {
		switch (pParam->nOperator) {
		case QPush:
			m_lstData.push_back(pParam->Data);
			delete pParam;
			break;
		case QPop:
			if (!m_lstData.empty()) {
				pParam->Data = m_lstData.front();
				m_lstData.pop_front();
			}
			if (pParam->hEvent)
				SetEvent(pParam->hEvent);
			break;
		case QSize:
			pParam->nOperator = m_lstData.size();
			if (pParam->hEvent)
				SetEvent(pParam->hEvent);
			break;
		case QClear:
			m_lstData.clear();
			delete pParam;
			break;
		default:
			OutputDebugStringA("CQueue线程收到未知操作命令，退出线程!\r\n");
			break;
		}
	}
	static void threadEntry(void* arg) {
		CQueue<T>* thiz = (CQueue<T>*)arg;
		thiz->threadmain();
		_endthread();
	}
	virtual void threadmain() {
		PPARAM* pParam = nullptr;
		DWORD dwBytesTransferred = 0;
		ULONG_PTR CompletionKey = 0;
		LPOVERLAPPED pOverlapped = NULL;
		while (GetQueuedCompletionStatus(m_hCompletionPort, 
			&dwBytesTransferred, 
			&CompletionKey, 
			&pOverlapped, INFINITE)) {
			if (dwBytesTransferred == 0 && pOverlapped == NULL)
				break;
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		while (GetQueuedCompletionStatus(m_hCompletionPort,
			&dwBytesTransferred,
			&CompletionKey,
			&pOverlapped, 0)) {
			if (dwBytesTransferred == 0 && pOverlapped == NULL) {
				break;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
	}
protected:
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
	std::atomic<bool> m_lock;
};

template<class Obj, class T>
class CSendQueue : public CQueue<T>, public ThreadFuncBase
{
public:
	using Callback = int (Obj::*)(T& data);

	CSendQueue(Obj* obj, Callback callback)
		: CQueue<T>()
		, m_base(obj)
		, m_callback(callback)
	{
		m_running.store(true);
		m_count.store(0);
		// 事件：用于唤醒发送驱动线程（有新数据 / 需要重试）
		m_hWakeEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (!m_hWakeEvent){
			throw std::runtime_error("CreateEvent failed");
		}
		m_thread.Start();
		m_thread.UpdateWorker(::ThreadWorker(this,
			(FUNCTYPE)&CSendQueue<Obj, T>::threadTick));
	}

	virtual ~CSendQueue()
	{
		// 1) 阻止后续发送回调
		m_running.store(false);
		// 2) 停止并唤醒 tick 线程，让其退出（避免 UAF）
		if (m_hWakeEvent) ::SetEvent(m_hWakeEvent);
		// 3) 停止并等待 tick 线程结束
		m_thread.Stop();    // 你工程里可能叫 RequestStop/Stop/Cancel
		// 4) 再清空回调指针（此时不会再有线程使用它们）
		m_base = nullptr;
		m_callback = nullptr;
		// 5) 关闭事件句柄
		if (m_hWakeEvent) {
			::CloseHandle(m_hWakeEvent);
			m_hWakeEvent = nullptr;
		}
	}

protected:
	// 禁用外部 PopFront(T&)：外部仍然只能 Push
	virtual bool PopFront(T& data) override { return false; }

	// 向 IOCP 投递一个 QPop 命令（触发一次发送尝试）
	bool PostPopCommand()
	{
		typename CQueue<T>::IocpParam* param = new typename CQueue<T>::IocpParam(CQueue<T>::QPop, T());
		if (CQueue<T>::m_lock) {  // 基类已开始析构/停止
			delete param;
			return false;
		}
		bool ret = ::PostQueuedCompletionStatus(
			CQueue<T>::m_hCompletionPort,
			0,
			(ULONG_PTR)param,
			nullptr
		);
		if (!ret) {
			delete param;
			return false;
		}
		return true;
	}

	//等事件 -> 如果队列计数>0 就投递一次 QPop 命令
	int threadTick()
	{
		while (m_running.load())
		{
			// 队列为空就阻塞等待；非空也可以等事件触发
			::WaitForSingleObject(m_hWakeEvent, INFINITE);
			if (!m_running.load())
				break;
			// 只根据原子计数判断是否需要尝试 pop
			if (m_count.load() > 0) {
				PostPopCommand();
			}
		}
		return -1;
	}

	// IOCP 线程串行处理队列命令：这里读写 m_lstData 是安全的
	virtual void DealParam(typename CQueue<T>::PPARAM* pParam) override
	{
		switch (pParam->nOperator)
		{
		case CQueue<T>::QPush:
		{
			// 入队
			CQueue<T>::m_lstData.push_back(pParam->Data);
			auto old = m_count.fetch_add(1);
			// 队列从 0 -> 1 时唤醒 tick 线程（开始发送）
			if (old == 0 && m_hWakeEvent) {
				::SetEvent(m_hWakeEvent);
			}
			delete pParam;
			break;
		}

		case CQueue<T>::QPop:
		{
			// 停止后不再做发送回调，直接丢弃命令
			if (!m_running.load()) {
				delete pParam;
				break;
			}
			if (!CQueue<T>::m_lstData.empty()) {
				pParam->Data = CQueue<T>::m_lstData.front();
				// 回调：返回 0 表示发送成功才出队
				int ret = (m_base && m_callback) ? (m_base->*m_callback)(pParam->Data) : -1;
				if (ret == 0) {
					CQueue<T>::m_lstData.pop_front();
					m_count.fetch_sub(1);

					// 如果还有剩余数据，继续唤醒 tick 线程触发下一次发送
					if (m_count.load() > 0 && m_hWakeEvent) {
						::SetEvent(m_hWakeEvent);
					}
				}
				else {
					// 这里选择立即重试：SetEvent 触发下一次 QPop
					if (m_hWakeEvent) {
						::SetEvent(m_hWakeEvent);
					}
				}
			}
			delete pParam;
			break;
		}

		case CQueue<T>::QSize:
		{
			pParam->nOperator = CQueue<T>::m_lstData.size();
			if (pParam->hEvent) ::SetEvent(pParam->hEvent);
			break;
		}

		case CQueue<T>::QClear:
		{
			CQueue<T>::m_lstData.clear();
			m_count.store(0);
			delete pParam;
			break;
		}

		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}

private:
	Obj* m_base;
	Callback m_callback;
	CThread m_thread;

	// 控制与同步
	std::atomic<bool>   m_running{ false };  // 控制 tick 线程/回调是否继续
	std::atomic<size_t> m_count{ 0 };        // 队列元素计数（避免跨线程读 list）
	HANDLE m_hWakeEvent{ nullptr };          // 唤醒 tick 线程的事件（auto-reset）
};


	
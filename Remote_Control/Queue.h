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
		m_hCompeletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		m_hThread = INVALID_HANDLE_VALUE;
		if(m_hCompeletionPort!=nullptr)
			m_hThread = (HANDLE)_beginthread(&CQueue<T>::threadEntry, 0, this);
	}

	virtual ~CQueue() {
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompeletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);
		if (m_hCompeletionPort != NULL) {
			HANDLE hTemp = m_hCompeletionPort;
			m_hCompeletionPort=NULL;
			CloseHandle(hTemp);
		}
	}

		bool PushBack(const T & data) {
			if (m_lock) return false;
			IocpParam* pParam = new IocpParam(QPush, data);
			bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
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
			bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
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
			bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
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
			bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
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
			while (GetQueuedCompletionStatus(m_hCompeletionPort, 
				&dwBytesTransferred, 
				&CompletionKey, 
				&pOverlapped, INFINITE)) {
				if (dwBytesTransferred == 0 && pOverlapped == NULL)
					break;
				pParam = (PPARAM*)CompletionKey;
				DealParam(pParam);
			}
			while (GetQueuedCompletionStatus(m_hCompeletionPort,
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
		HANDLE m_hCompeletionPort;
		HANDLE m_hThread;
		std::atomic<bool> m_lock;
	};

	template<class Obj,class T>
	class CSendQueue :public CQueue<T>, public ThreadFuncBase
	{
	public:
		using Callback = int (Obj::*)(T& data);

		CSendQueue(Obj* obj, Callback callback)
			:CQueue<T>(), m_base(obj), m_callback(callback)
		{
			m_thread.Start();
			m_thread.UpdateWorker(::ThreadWorker(this, (FUNCTYPE)&CSendQueue<Obj, T>::threadTick));
		}
		virtual ~CSendQueue() {
			m_base = nullptr;
			m_callback = nullptr;
		}
	protected:
		virtual bool PopFront(T& data) {
			return false;
		}
		bool PopFront() {
			typename CQueue<T>::IocpParam* Param = new typename CQueue<T>::IocpParam(CQueue<T>::QPop, T());
			if (CQueue<T>::m_lock) {
				delete Param;
				return false;
			}
			bool ret = PostQueuedCompletionStatus(CQueue<T>::m_hCompeletionPort, sizeof(typename CQueue<T>::PPARAM), (ULONG_PTR)&Param, NULL);
			if (ret == false) {
				delete Param;
				return false;
			}
			return ret;
		}
		int threadTick() {
			if (CQueue<T>::m_lstData.size() > 0) {
				PopFront();
			}
			Sleep(1);
			return 0;
		}
		virtual void DealParam(typename CQueue<T>::PPARAM* pParam) {
			switch (pParam->nOperator)
			{
			case CQueue<T>::QPush:
				CQueue<T>::m_lstData.push_back(pParam->Data);
				delete pParam;
				//printf("delete %08p\r\n", (void*)pParam);
				break;
			case CQueue<T>::QPop:
				if (CQueue<T>::m_lstData.size() > 0) {
					pParam->Data = CQueue<T>::m_lstData.front();
					if ((m_base->*m_callback)(pParam->Data) == 0)
						CQueue<T>::m_lstData.pop_front();
				}
				delete pParam;
				break;
			case CQueue<T>::QSize:
				pParam->nOperator = CQueue<T>::m_lstData.size();
				if (pParam->hEvent != NULL)
					SetEvent(pParam->hEvent);
				break;
			case CQueue<T>::QClear:
				CQueue<T>::m_lstData.clear();
				delete pParam;
				//printf("delete %08p\r\n", (void*)pParam);
				break;
			default:
				OutputDebugStringA("unknown operator!\r\n");
				break;
			}
		}
	private:
		Obj* m_base;
		Callback m_callback;
		CThread m_thread;
	};

	
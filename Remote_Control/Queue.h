#pragma once
#include "pch.h"
#include <atomic>
#include <list>

template<class T>
class CQueue
{//线程安全队列(利用IOCP)
public:
	typedef struct IocpParam {
		size_t nOperator;//操作
		T strData;//数据
		HANDLE hEvent;//pop时使用，用于通知调用线程

		IocpParam() :nOperator(-1) {}
		IocpParam(size_t op, const T& data, HANDLE hEve=NULL) :nOperator(op), strData(data), hEvent(hEve) {}
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

	~CQueue() {
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

		bool PopFront(T& data) {
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
			if (ret) data = Param.strData;
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
	private:
		void DealParam(PPARAM* pParam) {
			switch (pParam->nOperator) {
			case QPush:
				m_lstData.push_back(pParam->strData);
				delete pParam;
				break;
			case QPop:
				if (!m_lstData.empty()) {
					pParam->strData = m_lstData.front();
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
		void threadmain() {
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

		std::list<T> m_lstData;
		HANDLE m_hCompeletionPort;
		HANDLE m_hThread;
		std::atomic<bool> m_lock;
	};
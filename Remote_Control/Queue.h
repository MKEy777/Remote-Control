#pragma once

template<class T>
class CQueue
{//线程安全队列(利用IOCP)
public:
	CQueue();
	~CQueue();
	bool PushBack(const T& data);
	bool PopFront(T& data);
	size_t Size();
	void Clear();
private:
	static void threadEntry(void* agr);
	void threadmain();
private:
	std::list<T> m_lstData;
	HANDLE m_hCompeletionPort;
	HANDLE m_hThread;
public:
	typedef struct IocpParam {
		int nOperator;//操作
		std::string strData;//数据
		HANDLE hEvent;//pop时使用，用于通知调用线程

		IocpParam() :nOperator(-1) {}
		IocpParam(int op, const std::string data) :nOperator(op), strData(data) {}
	}IOCP_PARAM;//IOCP参数结构体
	enum {
		ListPush,
		ListPop,
		Size,
		Clear
	};
};

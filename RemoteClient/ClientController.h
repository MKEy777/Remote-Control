#pragma once
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include <map>
#include "resource.h"
#include "Tool.h"

//#define WM_SEND_PACK (WM_USER + 1)//发送包数据
#define WM_SHOW_STATUS (WM_USER + 3)//展示状态
#define WM_SHOW_WATCH (WM_USER+4) //远程监控
#define WM_SEND_MESSAGE (WM_USER+0x1000) //自定义消息处理

class CClientController
{
public:
	//获取单例
	static CClientController* getInstance();
	//初始化
	int initController();
	//启动
	int Invoke(CWnd*& pMainWnd);
	void UpdateAddress(int nIP, int nPort) {
		CClientSocket::GetInstance()->UpdateAddress(nIP, nPort);
	}
	int DealCommand() {
		return CClientSocket::GetInstance()->DealCommand();
	}
	//发送消息
	bool SendCommandPacket(
		HWND hWnd,//数据包受到后，需要应答的窗口
		int nCmd,
		bool bAutoClose = true,
		BYTE* pData = NULL,
		size_t nLength = 0,
		WPARAM wParam = 0
	);

	int GetImage(CImage& image);
	int DownFile(CString strPath);
	void DownloadEnd();
	void StartWatchScreen();
protected:
	//线程函数
	void threadFunc();
	//线程入口
	static unsigned _stdcall threadEntry(void* arg);
	void threadWatchScreen();
	static void threadWatchScreen(void* arg);

	CClientController():m_statusDlg(&m_remoteDlg),m_watchDlg(&m_remoteDlg)
	{
		m_isClosed=true;
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
		m_hThreadWatch= INVALID_HANDLE_VALUE;;
	}
	~CClientController() {
		WaitForSingleObject(m_hThread, 100);
	}
	static void releaseInstance() {
		TRACE("CClientSocket has been called!\r\n");
		if (m_instance != NULL) {
			delete m_instance;
			m_instance = NULL;
			TRACE("CClientController has released!\r\n");
		}
	}

	//消息处理函数
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendPacket(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
private:
	//消息信息结构体 
	typedef struct MsgInfo {
		MSG msg;
		LRESULT result;
		MsgInfo(MSG m) {
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}
		MsgInfo(const MsgInfo& m) {
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));
		}
		MsgInfo& operator=(const MsgInfo& m) {
			if (this != &m) {
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));
			}
			return *this;
		}
	} MSGINFO;

	//消息函数指针定义
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);//(消息类型，无符号整型，长整型)
	static std::map<UINT, MSGFUNC> m_mapFunc;//消息映射表

	CWatchDialog m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;

	HANDLE m_hThread;//线程句柄
	unsigned m_nThreadID;//线程ID
	HANDLE m_hThreadWatch;//监视线程句柄
	bool m_isClosed;//监视是否关闭
	
	CString m_strRemote;//下载文件的远程路径
	CString m_strLocal;//下载文件的本地保存路径
	
	static CClientController* m_instance;
	class CHelper {
	public:
		CHelper() {
			//CClientController::getInstance();
		}
		~CHelper() {
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};


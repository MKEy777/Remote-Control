#pragma once
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include <map>
#include "resource.h"
#include "Tool.h"

#define WM_SEND_DATA (WM_USER + 2)//发送数据
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
	//发送消息
	LRESULT SendMessage(MSG msg);
	//更新网络服务器地址和端口
	void UpdateAddress(int nIP,int nPort) {
		CClientSocket::GetInstance()->UpdateAddress(nIP, nPort);
	}
	//处理命令
	int DealCommand() {
		return CClientSocket::GetInstance()->DealCommand();
	}
	//关闭套接字
	void CloseSocket() {
		CClientSocket::GetInstance()->CloseSocket();
	}
	bool SendPacket(const CPacket& pack) {
		CClientSocket* pClient = CClientSocket::GetInstance();
		if (pClient->InitSocket() == false) return false;
		pClient->Send(pack);
	}
	//1 查看磁盘分区
	//2 查看指定目录下的文件
	//3 打开文件
	//4 下载文件
	//9 删除文件
	//5 鼠标操作
	//6 发送屏幕内容
	//7 锁机
	//8 解锁
	//1981 测试连接
	//返回值：是命令号，如果小于0则是错误
	bool SendCommandPacket(
		HWND hWnd,//数据包受到后，需要应答的窗口
		int nCmd,
		bool bAutoClose = true,
		BYTE* pData = NULL,
		size_t nLength = 0,
		WPARAM wParam = 0); 
	int GetImage(CImage& image) {
		CClientSocket* pClient = CClientSocket::GetInstance();
		return Tool::Bytes2Image(image, pClient->GetPacket().strData);
	}
	void DownloadEnd();
	int DownFile(CString strPath);
	void StartWatchScreen();
protected:
	void threadWatchScreen();
	static void threadWatchScreen(void* arg);
	CClientController():m_statusDlg(&m_remoteDlg),m_watchDlg(&m_remoteDlg)
	{
		m_isClosed = true;
		m_hThreadWatch = INVALID_HANDLE_VALUE;
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
	}
	~CClientController() {
		WaitForSingleObject(m_hThread, 100);
	}
	//线程函数
	void threadFunc();
	//线程入口
	static unsigned _stdcall threadEntry(void* arg);
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
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT CClientController::OnSendPacket(UINT nMsg, WPARAM wParam, LPARAM lParam)
	{
		PACKET_DATA* pData = (PACKET_DATA*)wParam;
		if (pData != nullptr) {
			// --- 修改开始 ---
			CClientSocket* pClient = CClientSocket::GetInstance();
			// 确保连接有效。InitSocket() 会处理重连逻辑。
			if (pClient->InitSocket())
			{
				pClient->Send(pData->strData.c_str(), pData->strData.size());
			}
			else
			{
				TRACE("连接服务器失败，无法发送数据包\n");
			}
			// --- 修改结束 ---

			delete pData;
		}
		return 0;
	}
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
	CWatchDialog m_watchDlg;//远程监控对话框
	CRemoteClientDlg m_remoteDlg;//远程客户端对话框
	CStatusDlg m_statusDlg;//状态对话框
	HANDLE m_hThread;
	HANDLE m_hThreadWatch;
	bool m_isClosed;//监视是否关闭
	//下载文件的远程路径
	CString m_strRemote;
	//下载文件的本地保存路径
	CString m_strLocal;
	static CClientController* m_instance;
	unsigned m_nThreadID;

	class CHelper {
	public:
		CHelper() {
			CClientController::getInstance();
		}
		~CHelper() {
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};


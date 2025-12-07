#pragma once
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include <map>
#include "resource.h"
#include "Tool.h"
#include "ClientModel.h" 

// 消息定义
#define WM_SEND_PACKET (WM_USER + 1) 
#define WM_SEND_DATA (WM_USER + 2)
#define WM_SHOW_STATUS (WM_USER + 3)
#define WM_SHOW_WATCH (WM_USER+4) 
//#define WM_SEND_PACK (WM_USER + 5) 
#define WM_SEND_MESSAGE (WM_USER+0x1000)

// 消息信息结构体 (恢复定义)
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


class CClientController
{
public:
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	static CClientController* getInstance();
	int initController();
	int Invoke(CWnd*& pMainWnd);
	LRESULT SendMessage(MSG msg);

	// -------- Controller 职责：将输入转发给 Model --------
	void UpdateAddress(int nIP, int nPort) {
		m_pModel->UpdateAddress(nIP, nPort);
	}
	int DealCommand() {
		return m_pModel->DealCommand();
	}
	void CloseSocket() {
		m_pModel->CloseSocket();
	}
	bool SendPacket(const CPacket& pack) {
		// Controller 转发给 Model (修复 CClientModel 缺失)
		return m_pModel->SendPacket(pack);
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
	//返回值：是状态，true是成功 false是失败
	bool SendCommandPacket(
		HWND hWnd,
		int nCmd,
		bool bAutoClose = true,
		BYTE* pData = NULL,
		size_t nLength = 0,
		WPARAM wParam = 0)
	{
		return m_pModel->SendCommandPacket(hWnd, nCmd, bAutoClose, pData, nLength, wParam);
	}

	// 下载控制
	int StartDownload(CString strPath);

	// 文件操作 View -> Controller -> Model 
	int SafeCopyFileInfo(PFILEINFO pDestInfo, const std::string& packetData);
	int LoadDiskDrivers(CRemoteClientDlg* pView);
	int LoadDirectory(CRemoteClientDlg* pView, HTREEITEM hTree, CString strPath);
	int CClientController::RemoveFile(CString strPath, int nSelected, CRemoteClientDlg* pView);
	int RunFile(CString strPath);
	void DownloadEnd();

	// 监控控制
	void StartWatchScreen();
	void HandleMouseInput(int nAction, int nButton, CPoint point);

	// View 的图像获取和状态报告接口
	CImage& GetWatchImage() { return m_pModel->getCurrentImage(); }
	bool IsWatchClosed() { return m_pModel->IsWatchClosed(); }

protected:
	void threadWatchScreen();
	static void threadWatchScreen(void* arg);
	CClientController();
	~CClientController() {
		m_pModel->SetWatchStatus(true);
		WaitForSingleObject(m_hThreadWatch, 500);
		WaitForSingleObject(m_hThread, 100);
	}
	void threadFunc();
	static unsigned _stdcall threadEntry(void* arg);
	static void releaseInstance();

	// 消息处理函数
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT CClientController::OnSendPacket(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnDownloadEnd(UINT nMsg, WPARAM wParam, LPARAM lParam);


private:
	// 静态成员定义 (修复：恢复定义)
	static std::map<UINT, MSGFUNC> m_mapFunc;

	CClientModel* m_pModel;
	CWatchDialog m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;

	HANDLE m_hThread;
	HANDLE m_hThreadWatch;

	static CClientController* m_instance;
	unsigned m_nThreadID;

	class CHelper {
	public:
		CHelper() { CClientController::getInstance(); }
		~CHelper() { CClientController::releaseInstance(); }
	};
	static CHelper m_helper;
};
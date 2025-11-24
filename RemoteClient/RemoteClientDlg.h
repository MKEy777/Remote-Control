
// RemoteClientDlg.h : header file
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"

#define WM_SEND_PACKET (WM_USER + 1)  //发送数据包的消息 ①

// CRemoteClientDlg dialog
class CRemoteClientDlg : public CDialogEx
{
// Construction
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

public:
	bool isFull() const { return m_isFull; }
	CImage& getImage() { return m_image; }
private:
	CImage m_image;//缓存屏幕图像
	bool m_isFull; //缓存是否有数据

private:
	static void threadEntryForWatchData(void* arg);
	void threadWatchData();
	static void threadEntryForDownFile(void* arg);	
	void threadDownFile();
	void LoadFileInfo();
	void LoadFileCurrent();
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
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
	int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0);

// Implementation
protected:
	HICON m_hIcon;
	CStatusDlg m_dlgStatus;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnTest();
	DWORD m_serv_address;
	CString m_nPort;
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	CTreeCtrl m_Tree;
	CListCtrl m_List;
	
	afx_msg void OnBnClickedBtnFileinfo();
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnRunFile();
	afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM lParam);//定义自定义消息响应函数 ②
	afx_msg void OnBnClickedBtnStartWatch();
};

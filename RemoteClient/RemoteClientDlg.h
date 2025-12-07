// RemoteClientDlg.h : header file
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"


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

protected:
	HICON m_hIcon;
	CStatusDlg m_dlgStatus; // 状态对话框实例，它自身也是一个 View

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	CTreeCtrl m_Tree;
	CListCtrl m_List;
	void LoadFileCurrent();
	void Str2Tree(const std::string& drivers, CTreeCtrl& tree);
	void UpdateFileInfo(const FILEINFO& finfo, HTREEITEM hParent);
	// Connection & Test
	afx_msg void OnBnClickedBtnTest();
	DWORD m_serv_address;
	CString m_nPort;
	afx_msg void OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeEditPort();

	// File Explorer Controls
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	// File Operations (View -> Controller)
	afx_msg void OnBnClickedBtnFileinfo();
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnRunFile();

	// 远程监控
	afx_msg void OnBnClickedBtnStartWatch();

	// 自定义消息处理函数 (保留，但逻辑需要简化，仅作为 Controller 的转发入口)
	afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM lParam);
};
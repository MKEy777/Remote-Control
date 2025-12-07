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
	CStatusDlg m_dlgStatus; // 状态对话框实例

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	// -------------------------------------------------------------
	// 核心业务辅助函数
	// -------------------------------------------------------------

	// 获取树节点的完整路径
	CString GetPath(HTREEITEM hTree);

	// 删除指定树节点的子节点（用于刷新目录前清理）
	void DeleteTreeChildrenItem(HTREEITEM hTree);

	// 【修改】请求加载文件列表（异步发送请求，不等待）
	// 原 LoadFileCurrent 逻辑有误，现更名为 LoadFileInfo 并负责触发网络请求
	void LoadFileInfo();

	// 【新增】业务逻辑分发器：负责根据命令号 (nCmd) 处理解包后的数据
	void DealCommand(WORD nCmd, const std::string& strData, LPARAM lParam);

	// 解析磁盘分区字符串 (如 "C,D,E") 并填充 Tree
	void Str2Tree(const std::string& drivers, CTreeCtrl& tree);

	// 解析文件信息结构体并填充 Tree/List
	void UpdateFileInfo(const FILEINFO& finfo, HTREEITEM hParent);

	void UpdateDownloadFile(const std::string& strData, FILE* pFile);

	// -------------------------------------------------------------
	// 控件变量
	// -------------------------------------------------------------
	CTreeCtrl m_Tree;
	CListCtrl m_List;
	DWORD m_serv_address;
	CString m_nPort;

	// -------------------------------------------------------------
	// 消息响应函数
	// -------------------------------------------------------------

	// Connection & IP/Port settings
	afx_msg void OnBnClickedBtnTest();
	afx_msg void OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeEditPort();

	// File Explorer Interactions (Tree/List Events)
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);

	// File Operations Buttons (View -> Controller)
	afx_msg void OnBnClickedBtnFileinfo(); // 获取磁盘列表
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnRunFile();

	// Remote Screen Watch
	afx_msg void OnBnClickedBtnStartWatch();

	// 【新增】网络数据回调函数
	// 当 Socket 接收到完整数据包后，通过 PostMessage 触发此函数
	// wParam: CPacket* 指针, lParam: 上下文数据 (如 HTREEITEM)
	afx_msg LRESULT OnSendPackAck(WPARAM wParam, LPARAM lParam);

	// 旧的发送消息处理（如果 Controller 仍依赖此消息进行线程通信，则保留）
	afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM lParam);
};
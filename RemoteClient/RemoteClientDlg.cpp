// RemoteClientDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "ClientController.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	
END_MESSAGE_MAP()


// CRemoteClientDlg dialog



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_serv_address(0)
	, m_nPort(_T(""))
{
	//m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hIcon = NULL;
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_serv_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet, strTmp;
	do {
		strTmp = m_Tree.GetItemText(hTree);
		strRet = strTmp + '\\' + strRet;
		hTree = m_Tree.GetParentItem(hTree);
	} while (hTree != NULL);
	return strRet;
}

// 辅助函数保留：View 需要清理界面元素
void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_Tree.GetChildItem(hTree);
		if (hSub != NULL)m_Tree.DeleteItem(hSub);
	} while (hSub != NULL);
}
BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
END_MESSAGE_MAP()


// CRemoteClientDlg message handlers

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	if (m_hIcon == NULL)
	{
		m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	}
	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	UpdateData();
	m_serv_address = 0x0100007F;//0x6538A8C0 192.168.56.101 
	m_nPort = _T("9527");
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_serv_address, atoi((LPCTSTR)(m_nPort)));
	UpdateData(FALSE);
	//m_dlgStatus.Create(IDD_DLG_STATUS, this);
	//m_dlgStatus.ShowWindow(SW_HIDE);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	// 1. View 获取选中项的路径
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	if (hTree == NULL) return;
	CString strPath = GetPath(hTree);
    
    // 2. View 转发给 Controller
    // Controller 负责网络通信、解析数据、并指示 View 更新 m_List 和 m_Tree
	CClientController::getInstance()->LoadDirectory(this, hTree, strPath);
}

void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	// 与单击事件执行相同的逻辑
	OnNMClickTreeDir(pNMHDR, pResult);
}

// 列表控件右键事件 (View 触发，显示菜单)
void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	// 保持不变，View 仅负责菜单显示
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;
	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);
	int ListSelected = m_List.HitTest(ptList);
	if (ListSelected < 0)return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu* pPupup = menu.GetSubMenu(0);
	if (pPupup != NULL) {
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
	}
}

// 点击按钮查看磁盘分区 (View 转发命令给 Controller)
void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
    // View 转发，Controller 负责网络、解析和更新 m_Tree/m_List
	CClientController::getInstance()->LoadDiskDrivers(this);
}

//下载文件 (View 转发命令给 Controller)
void CRemoteClientDlg::OnDownloadFile()
{
	int nListSelected = m_List.GetSelectionMark();
	if (nListSelected < 0) return;
    
	CString strFile = m_List.GetItemText(nListSelected, 0);
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
    
    // 1. View 确定远程文件路径
	CString strRemotePath = GetPath(hSelected) + strFile;

	// 2. View 通知 Controller 开始下载
	int ret = CClientController::getInstance()->StartDownload(strRemotePath);
	if (ret != 0) {
		MessageBox(_T("下载失败！"));
		TRACE("下载失败 ret = %d\r\n", ret);
	}
}

//删除文件 (View 转发命令给 Controller)
void CRemoteClientDlg::OnDeleteFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	int nSelected = m_List.GetSelectionMark();
	if (nSelected < 0) return;
    
	CString strFile = m_List.GetItemText(nSelected, 0);
	CString strPath = GetPath(hSelected) + strFile;
    
    // View 通知 Controller 删除，Controller 负责网络通信和 View 更新（m_List.DeleteItem(nSelected)）
	int ret = CClientController::getInstance()->RemoveFile(strPath, nSelected, this);
	if (ret < 0)
	{
		MessageBox(_T("删除文件命令失败!"));
		TRACE("删除文件命令发送失败:ret=%d\r\n", ret);
		return;
	}
}

//运行文件 (View 转发命令给 Controller)
void CRemoteClientDlg::OnRunFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	int nSelected = m_List.GetSelectionMark();
	if (nSelected < 0) return;
    
	CString strFile = m_List.GetItemText(nSelected, 0);
	CString strPath = GetPath(hSelected) + strFile;

	// View 通知 Controller 运行文件
	int ret = CClientController::getInstance()->RunFile(strPath);
	if (ret < 0)
	{
		MessageBox(_T("运行文件命令失败!")); // 修复：使用 MessageBox
		TRACE("运行文件命令发送失败:ret=%d\r\n", ret);
		return;
	}
}

// 自定义消息响应函数 (View -> Controller 转发)
LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)
{
	// **此方法原先包含大量的命令号判断和逻辑，现在应简化为转发给 Controller**
	
	// 由于这个消息是线程间通信的桥梁，我们将转发给 Controller 的通用消息处理。
	// 但鉴于此处的实现依赖特定的结构体和命令号，我们暂时保持原有的 SendCommandPacket 调用，
	// 只是调用的是 CClientController 的 SendCommandPacket。

	int ret = 0;
	int cmd = wParam >> 1;
	// 所有的 SendCommandPacket 调用都转发到 Controller
	// Controller 内部会调用 Model 的 SendCommandPacket
	switch (cmd) {
	case 4: { // 下载
		CString strFile = (LPCSTR)lParam;
		ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), cmd, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	}
		  break;
	case 5: {// 鼠标操作
		ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), cmd, wParam & 1, (BYTE*)lParam, sizeof(MOUSEEV));
	}
		  break;
	case 6: // 请求屏幕
		ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), cmd, wParam & 1,NULL,0);
		break;
	case 7: // 锁机
	case 8: { // 解锁
		ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), cmd, wParam & 1);
	}
		  break;
	default:
		ret = -1;
	}

	return ret;
}

// 开始监控 (View -> Controller)
void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	// 移除 m_watchDlg = false; m_bStopWatch = false; 等状态管理，由 Controller 负责
	// 移除线程创建 threadEntryForWatchData，由 Controller 负责
    
	CClientController* pController = CClientController::getInstance();
    
    // View 启动 Watch 业务，Controller 负责线程和对话框模态
	pController->StartWatchScreen(); // 此方法在 CClientController 中启动了 CWatchDialog.DoModal()

	// 移除 WaitForSingleObject(hThread, 500)
}

void CRemoteClientDlg::OnBnClickedBtnTest()
{
	// View 转发命令 1981 (测试连接) 给 Controller
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1981);
}

// IP/Port 修改 (View -> Controller)
void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	UpdateData();
	CClientController* pController = CClientController::getInstance();
    // View 通知 Controller 更新 Model 状态
	pController->UpdateAddress(m_serv_address, atoi((LPCTSTR)(m_nPort)));
}

void CRemoteClientDlg::OnEnChangeEditPort()
{
	UpdateData();
	CClientController* pController = CClientController::getInstance();
    // View 通知 Controller 更新 Model 状态
	pController->UpdateAddress(m_serv_address, atoi((LPCTSTR)(m_nPort)));
}

// RemoteClientDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientController.h"
#include "CWatchDialog.h"

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
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_serv_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	m_List.DeleteAllItems();
	int nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::GetInstance()->GetPacket().strData.c_str();
	//CClientSocket* pClient = CClientSocket::GetInstance();
	while (pInfo->HasNext) {
		TRACE("[%s] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (!pInfo->IsDirectory) {
			m_List.InsertItem(0, pInfo->szFileName);
		}
		int cmd = CClientController::getInstance()->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0)break;
		pInfo = (PFILEINFO)CClientSocket::GetInstance()->GetPacket().strData.c_str();
	}
	//pClient->CloseSocket();
}

void CRemoteClientDlg::Str2Tree(const std::string& drivers, CTreeCtrl& tree)
{
	std::string dr;
	tree.DeleteAllItems();
	for (size_t i = 0; i < drivers.size(); i++)
	{
		if (drivers[i] == ',') {
			dr += ":";
			HTREEITEM hTemp = tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			tree.InsertItem("", hTemp, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}
	if (dr.size() > 0) {
		dr += ":";
		HTREEITEM hTemp = tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
		tree.InsertItem("", hTemp, TVI_LAST);
	}
}

void CRemoteClientDlg::UpdateFileInfo(const FILEINFO& finfo, HTREEITEM hParent)
{
	TRACE("hasnext %d isdirectory %d %s\r\n", finfo.HasNext, finfo.IsDirectory, finfo.szFileName);
	if (finfo.HasNext == FALSE)return;
	if (finfo.IsDirectory) {
		if (CString(finfo.szFileName) == "." || (CString(finfo.szFileName) == ".."))
			return;
		TRACE("hselected %08X %08X\r\n", hParent, m_Tree.GetSelectedItem());
		HTREEITEM hTemp = m_Tree.InsertItem(finfo.szFileName, hParent);
		m_Tree.InsertItem("", hTemp, TVI_LAST);
		m_Tree.Expand(hParent, TVE_EXPAND);
	}
	else {
		m_List.InsertItem(0, finfo.szFileName);
	}
}

void CRemoteClientDlg::UpdateDownloadFile(const std::string& strData, FILE* pFile)
{
	static LONGLONG length = 0, index = 0;
	TRACE("length %lld index %lld\r\n", length, index);

	if (length == 0) {
		if (strData.size() == 0) {
			TRACE("接收到结束包，忽略。\r\n");
			return;
		}

		length = *(long long*)strData.c_str();
		if (length == 0) {
			AfxMessageBox("文件长度为零或者无法读取文件！！！");
			if (pFile != nullptr) {
				fclose(pFile);
				pFile = nullptr;
			}
			CClientController::getInstance()->DownloadEnd();
		}
	}
	else if (length > 0 && (index >= length)) {
		// 正常下载完成
		fclose(pFile);
		length = 0;
		index = 0;
		CClientController::getInstance()->DownloadEnd();
	}
	else {
		fwrite(strData.c_str(), 1, strData.size(), pFile);
		index += strData.size();
		TRACE("index = %lld\r\n", index);
		if (index >= length) {
			// 刚好拼完最后一个包
			fclose(pFile);
			length = 0;
			index = 0;
			CClientController::getInstance()->DownloadEnd();
		}
	}
}

void CRemoteClientDlg::DealCommand(WORD nCmd, const std::string& strData, LPARAM lParam)
{
	switch (nCmd) {
	case 1://获取驱动信息
		Str2Tree(strData, m_Tree);
		break;
	case 2://获取文件信息
		UpdateFileInfo(*(PFILEINFO)strData.c_str(), (HTREEITEM)lParam);
		break;
	case 3:
		MessageBox("打开文件完成！", "操作完成", MB_ICONINFORMATION);
		break;
	case 4:
		UpdateDownloadFile(strData, (FILE*)lParam);
		break;
	case 9:
		MessageBox("删除文件完成！", "操作完成", MB_ICONINFORMATION);
		break;
	case 1981:
		MessageBox("连接测试成功！", "连接成功", MB_ICONINFORMATION);
		break;
	default:
		TRACE("unknow data received! %d\r\n", nCmd);
		break;
	}
}

void CRemoteClientDlg::InitUIData()
{
	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	UpdateData();
	m_serv_address = 0x0100007F;//0x7F000001;//0xC0A80167;//192.168.1.103
	m_nPort = _T("9527");
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_serv_address, atoi((LPCTSTR)m_nPort));
	UpdateData(FALSE);
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
}


void CRemoteClientDlg::threadEntryForDownFile(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadDownFile();
	_endthread();

}

void CRemoteClientDlg::threadDownFile()
{
	int nListSelected = m_List.GetSelectionMark();//获取选中项的索引
	CString strFile = m_List.GetItemText(nListSelected, 0);//文件名
	CFileDialog dlg(FALSE, NULL, strFile,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("All Files (*.*)|*.*||"), this);//保存文件对话框
	if (dlg.DoModal() == IDOK) {//如果点击了保存按钮
		FILE* pFile = nullptr;
		errno_t err = _tfopen_s(&pFile, dlg.GetPathName(), _T("wb+"));
		if (err != 0 || pFile == nullptr)
		{
			AfxMessageBox(_T("无权限或无法创建本地文件!"));
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();//隐藏等待光标
			return;
		}
		HTREEITEM hSelected = m_Tree.GetSelectedItem();//获取树控件选中项
		strFile = GetPath(hSelected) + strFile;
		TRACE("Download file:%s\r\n", (LPCTSTR)strFile);
		CClientSocket* pClient = CClientSocket::GetInstance();
		//int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
		int ret=SendMessage(WM_SEND_PACKET,4<<1|0,(LPARAM)(LPCTSTR)strFile);//发送下载文件命令，使用消息发送方式
		//避免多线程同时使用同一个Socket时出现问题)
		if (ret < 0) {
			AfxMessageBox(_T("下载文件命令发送失败!"));
			TRACE("执行下载失败：ret = %d\r\n", ret);
			fclose(pFile);
			pClient->CloseSocket();
		}
		long long nlength = *(long long*)pClient->GetPacket().strData.c_str();
		if (nlength <= 0)
		{
			AfxMessageBox(_T("文件长度为0或无法读取文件"));
			fclose(pFile);
			pClient->CloseSocket();
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();//隐藏等待光标
			return;
		}
		long long nCount = 0;

		while (nCount < nlength)//循环接收文件数据
		{
			ret = pClient->DealCommand();
			if (ret < 0)
			{
				AfxMessageBox(_T("下载文件过程中出现错误!"));
				TRACE("下载文件过程中出现错误:ret=%d\r\n", ret);
				break;
			}
			fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
			nCount += pClient->GetPacket().strData.size();
		}

		fclose(pFile);
		pClient->CloseSocket();
	}
	m_dlgStatus.ShowWindow(SW_HIDE);
	EndWaitCursor();//隐藏等待光标
	MessageBox("文件下载完成!");
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL)
		return;
	if (m_Tree.GetChildItem(hTreeSelected) == NULL)
		return;
	DeleteTreeChildrenItem(hTreeSelected);
	m_List.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength(), (WPARAM)hTreeSelected);
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
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CRemoteClientDlg::OnSendPackAck)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
END_MESSAGE_MAP()


// CRemoteClientDlg message handlers

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

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

	InitUIData();
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

////发送命令 + 同步等待服务器回复
//int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
//{
//	UpdateData();
//	CClientSocket* pClient = CClientSocket::GetInstance();
//	bool ret = pClient->InitSocket();
//
//	if (!ret) {
//		AfxMessageBox(_T("网络初始化失败!"));
//		return -1;
//	}
//
//	CPacket pack(nCmd, pData, nLength);
//	ret = pClient->Send(pack);
//	TRACE("Send ret %d\r\n", ret);
//	int cmd = pClient->DealCommand();
//	TRACE("ack:%d\r\n", cmd);
//	if (bAutoClose)
//		pClient->CloseSocket();
//	return cmd;
//}


void CRemoteClientDlg::OnBnClickedBtnTest()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1981);
}

// 树控件单击事件
void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}
// 树控件双击事件
void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}

// 列表控件右键事件
void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
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

// 点击按钮查看磁盘分区
void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1, false);
	if (ret == -1) {
		AfxMessageBox(_T("命令处理失败!!!"));
		return;
	}

	CClientSocket* pClient = CClientSocket::GetInstance();
	std::string drivers = pClient->GetPacket().strData;

	while (true)
	{
		int cmd = pClient->DealCommand();
		if (cmd < 0) break;

		drivers += pClient->GetPacket().strData;
	}

	pClient->CloseSocket();
	std::string dr;
	m_Tree.DeleteAllItems();
	m_List.DeleteAllItems();

	for (size_t i = 0; i < drivers.size(); i++)
	{
		if (drivers[i] == ',') {
			dr += ":";
			HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}
	// 循环结束后把最后一个也加上去
	if (!dr.empty()) {
		dr += ":";
		HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
		m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
	}
}

//下载文件
void CRemoteClientDlg::OnDownloadFile()
{
	int nListSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nListSelected, 0);
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	strFile = GetPath(hSelected) + strFile;
	int ret = CClientController::getInstance()->DownFile(strFile);
	if (ret != 0) {
		MessageBox(_T("下载失败！"));
		TRACE("下载失败 ret = %d\r\n", ret);
	}
	
}

//删除文件
void CRemoteClientDlg::OnDeleteFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();//获取树控件选中项
	int nSelected = m_List.GetSelectionMark();//获取列表控件选中项索引
	CString strFile = m_List.GetItemText(nSelected, 0);//文件名
	CString strPath = GetPath(hSelected) + strFile;
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 9, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
	if (ret < 0)
	{
		AfxMessageBox(_T("打开文件命令失败!"));
		TRACE("运行文件命令发送失败:ret=%d\r\n", ret);
		return;
	}
	LoadFileCurrent();
}

//运行文件
void CRemoteClientDlg::OnRunFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();//获取树控件选中项
	CString strFile = m_List.GetItemText(m_List.GetSelectionMark(), 0);//文件名
	CString strPath = GetPath(hSelected) + strFile;
	int nSelected = m_List.GetSelectionMark();//获取列表控件选中项索引
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),3, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
	if (ret < 0)
	{
		AfxMessageBox(_T("打开文件命令失败!"));
		TRACE("运行文件命令发送失败:ret=%d\r\n", ret);
	}
}


void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CClientController::getInstance()->StartWatchScreen();
}

LRESULT CRemoteClientDlg::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || (lParam == -2)) {
		TRACE("socket is error %d\r\n", lParam);
	}
	else if (lParam == 1) {
		//对方关闭了套接字
		TRACE("socket is closed!\r\n");
	}
	else {
		if (wParam != NULL) {
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;
			DealCommand(head.sCmd, head.strData, lParam);
		}
	}
	return 0;
}

void CRemoteClientDlg::OnEnChangeEditPort()
{
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_serv_address, atoi((LPCTSTR)m_nPort));
}

void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_serv_address, atoi((LPCTSTR)m_nPort));
}

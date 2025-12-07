#include "pch.h"
#include "ClientController.h"
#include "ClientModel.h" // 引用 Model
#include <process.h>
#include "ClientSocket.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;
CClientController::CHelper CClientController::m_helper;

// 构造函数：初始化 View 并注入 Model
CClientController::CClientController() :
	m_statusDlg(&m_remoteDlg),
	m_watchDlg(&m_remoteDlg)
{
	// 获取 Model 实例
	m_pModel = CClientModel::getInstance();

	// 状态初始化
	m_hThreadWatch = INVALID_HANDLE_VALUE;
	m_hThread = INVALID_HANDLE_VALUE;
	m_nThreadID = -1;
}


CClientController* CClientController::getInstance()
{
	if (m_instance == nullptr) {
		m_instance = new CClientController();
		TRACE("CClientController size is %d\r\n", sizeof(*m_instance));

		// 消息映射：新增对下载结束的响应
		struct { UINT nMsg; MSGFUNC func; }MsgFuncs[] = {
			{WM_SHOW_STATUS,&CClientController::OnShowStatus},
			{WM_SHOW_WATCH,&CClientController::OnShowWatcher},
			{WM_SEND_DATA,&CClientController::OnSendData},
			{WM_SEND_PACK, &CClientController::OnSendPacket},
			{(UINT)4, &CClientController::OnDownloadEnd}, // 4 是下载命令号，此处假设 Model 用命令号作为通知消息
			{(UINT)-1,NULL}
		};
		for (int i = 0; MsgFuncs[i].func != NULL; i++) {
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));
		}
	}
	return m_instance;
}

// 线程初始化：更新为使用 Model 接口
int CClientController::initController()
{
	m_hThread = (HANDLE)_beginthreadex(NULL, 0,
		&CClientController::threadEntry,
		this, 0, &m_nThreadID);//创建接收线程

	// Controller 调用 Model 设置线程ID
	m_pModel->SetControllerThreadID(m_nThreadID);

	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);
	return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}

LRESULT CClientController::SendMessage(MSG msg)
{
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);//创建事件对象
	if (hEvent == NULL) {
		return -2;
	}
	MSGINFO info(msg);// 要传过去的消息数据
	PostThreadMessage(m_nThreadID,
		WM_SEND_MESSAGE,
		(WPARAM)&info,
		(LPARAM)&hEvent);
	WaitForSingleObject(hEvent, -1);
	return info.result;
}

// ---------------------- Controller 业务逻辑实现 ----------------------

// 下载文件 (Controller：协调 View 和 Model)
int CClientController::StartDownload(CString strPath)
{
	// 1. View 交互：弹出文件选择对话框
	CFileDialog dlg(
		FALSE, NULL,
		strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		NULL, &m_remoteDlg);
	if (dlg.DoModal() == IDOK) {
		CString strLocal = dlg.GetPathName();
		FILE* pFile = nullptr;

		// 2. Model I/O：打开本地文件（此步骤仍涉及I/O，通常应在Model中，此处为重用代码）
		errno_t err = fopen_s(&pFile, strLocal, "wb+");
		if (err != 0 || pFile == nullptr) {
			m_remoteDlg.MessageBox(_T("本地没有权限保存该文件，或者文件无法创建！！！"));
			return -1;
		}

		// 3. Controller -> Model：设置状态并启动业务
		m_pModel->setLocalPath(strLocal);
		m_pModel->StartDownload(strPath, pFile); // Model 发送网络命令

		// 4. View 更新
		m_remoteDlg.BeginWaitCursor();
		m_statusDlg.m_info.SetWindowText(_T("命令正在执行中！"));
		m_statusDlg.ShowWindow(SW_SHOW);
		m_statusDlg.CenterWindow(&m_remoteDlg);
		m_statusDlg.SetActiveWindow();
	}
	return 0;
}

// 开始监控 (Controller：协调线程和 Model 状态)
void CClientController::StartWatchScreen()
{
	m_pModel->SetWatchStatus(false); // Model 状态：未关闭
	m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreen, 0, this);
	m_watchDlg.DoModal(); // View 显示

	// 对话框关闭后，执行清理
	m_pModel->SetWatchStatus(true); // Model 状态：关闭
	WaitForSingleObject(m_hThreadWatch, 500);
}

// 监控线程 (Controller 职责)
void CClientController::threadWatchScreen()
{
	Sleep(50);
	ULONGLONG nTick = GetTickCount64();
	while (!m_pModel->IsWatchClosed()) { // 检查 Model 状态
		if (m_watchDlg.isFull() == false) {
			if (GetTickCount64() - nTick < 200) {
				Sleep(200 - DWORD(GetTickCount64() - nTick));
			}
			nTick = GetTickCount64();
			// Controller 调用 Model 请求帧，并传入响应窗口句柄
			int ret = m_pModel->RequestScreenFrame(m_watchDlg.GetSafeHwnd());
			if (ret != 1) { // 检查发送是否成功
				TRACE("获取图片失败！ret = %d\r\n", ret);
			}
		}
		Sleep(1);
	}
	TRACE("thread end \r\n");
}

void CClientController::threadWatchScreen(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
	_endthread();
}

// 处理鼠标输入 (Controller：接收 View 原始数据，执行坐标转换业务逻辑，转发给 Model)
void CClientController::HandleMouseInput(int nAction, int nButton, CPoint point)
{
	// 1. Controller 获取 View 的尺寸信息
	CRect rectView;
	m_watchDlg.m_picture.GetClientRect(&rectView); // Picture Control 的显示区域

	// 2. Controller 获取 Model 的图像状态
	CImage& img = m_pModel->getCurrentImage();

	// 3. 业务逻辑：执行坐标转换 (依赖 View 尺寸和 Model 图像尺寸)

	CPoint ptControl = point;
	m_watchDlg.ClientToScreen(&ptControl);             // View 转成屏幕绝对坐标
	m_watchDlg.m_picture.ScreenToClient(&ptControl);   // 转成画面控件内的相对坐标

	if (img.IsNull() || rectView.Width() == 0 || rectView.Height() == 0) return;

	// 坐标映射：真实坐标 = 点击坐标 * (原始尺寸 / 视图尺寸)
	int x = (int)((float)ptControl.x * img.GetWidth() / rectView.Width() + 0.5f);
	int y = (int)((float)ptControl.y * img.GetHeight() / rectView.Height() + 0.5f);
	CPoint remotePt(x, y);

	// 4. Controller 封装数据
	MOUSEEV event; // MOUSEEV 定义在 ClientModel.h 中
	event.ptXY = remotePt;
	event.nButton = nButton;
	event.nAction = nAction;

	// 5. Controller 调用 Model 发送命令
	m_pModel->SendMouseCommand(event);
}

// ---------------------- 消息处理函数 ----------------------

void CClientController::threadFunc()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_SEND_MESSAGE) {
			MSGINFO* pmsg = (MSGINFO*)msg.wParam;//获取消息信息结构体指针
			HANDLE hEvent = (HANDLE)msg.lParam;//获取事件对象句柄
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(pmsg->msg.message); // 使用 pmsg->msg.message 查找
			if (it != m_mapFunc.end()) {
				pmsg->result = (this->*it->second)(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);
			}
			else {
				pmsg->result = -1;
			}
			SetEvent(hEvent);//通知发送消息的线程
		}
		else {
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				(this->*it->second)(
					msg.message,
					msg.wParam,
					msg.lParam);
			}
		}
	}
}

unsigned _stdcall CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadFunc();
	_endthreadex(0);
	return 0;
}

void CClientController::releaseInstance()
{
	TRACE("CClientController has been called!\r\n");
	if (m_instance != NULL) {
		delete m_instance;
		m_instance = NULL;
		TRACE("CClientController has released!\r\n");
	}
}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	// View 操作
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	// View 操作
	return m_watchDlg.DoModal();
}

LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	// Controller 调用 Model
	CClientSocket* pClient = CClientSocket::GetInstance();
	char* pBuffer = (char*)wParam;
	return pClient->Send(pBuffer, (int)lParam);
}

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

		delete pData;
	}
	return 0;
}


LRESULT CClientController::OnDownloadEnd(UINT nMsg, WPARAM wParam, LPARAM lParam)
{

	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("下载完成！！"), _T("完成"));
	m_pModel->DownloadEnd();

	return 0;
}
int CClientController::SafeCopyFileInfo(PFILEINFO pDestInfo, const std::string& packetData)
{
	if (packetData.size() < sizeof(FILEINFO)) {
		return -1;
	}

	// 1. 缓冲区安全复制
	memcpy(pDestInfo, packetData.c_str(), sizeof(FILEINFO));

	// 2. 字符串安全加固
	pDestInfo->szFileName[sizeof(pDestInfo->szFileName) - 1] = '\0';

	// 检查是否是服务器发送的结束标记包
	if (pDestInfo->HasNext == FALSE) {
		return -1; // -1 表示数据流结束或错误
	}

	return 0; // 成功获取有效数据
}
int CClientController::LoadDiskDrivers(CRemoteClientDlg* pView)
{
	// 1. Controller 调用 Model 发送请求 (命令 1)
	int ret = m_pModel->SendCommandPacket(pView->GetSafeHwnd(), 1, false); // Cmd: 1
	if (ret == -1) {
		pView->MessageBox(_T("命令处理失败!!!"));
		return -1;
	}

	// 2. Controller 调用 Model 处理网络数据，并执行 View 更新
	CClientSocket* pClient = CClientSocket::GetInstance(); // Model 依赖
	std::string drivers = pClient->GetPacket().strData;

	while (true)
	{
		// 核心逻辑：DealCommand 和 GetPacket() 仍需依赖 Model
		int cmd = m_pModel->DealCommand();
		if (cmd < 0) break;

		drivers += pClient->GetPacket().strData;
	}

	m_pModel->CloseSocket(); // 关闭 Socket

	// 3. Controller 指示 View 更新界面 (Tree/List)
	std::string dr;
	pView->m_Tree.DeleteAllItems();
	pView->m_List.DeleteAllItems();

	for (size_t i = 0; i < drivers.size(); i++)
	{
		if (drivers[i] == ',') {
			dr += ":";
			HTREEITEM hTemp = pView->m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			pView->m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}
	if (!dr.empty()) {
		dr += ":";
		HTREEITEM hTemp = pView->m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
		pView->m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
	}
	return 0;
}

// Controller：加载指定目录下的文件 (View -> Model -> View)
int CClientController::LoadDirectory(CRemoteClientDlg* pView, HTREEITEM hTree, CString strPath)
{
	// 1. Controller 清理 View 控件 
	pView->m_List.DeleteAllItems();
	if (hTree != NULL && pView->m_Tree.GetChildItem(hTree) != NULL) {
		pView->DeleteTreeChildrenItem(hTree); // 假设 DeleteTreeChildrenItem 存在
	}

	// 2. Controller 调用 Model 发送请求 (命令 2)
	int ret = m_pModel->SendCommandPacket(pView->GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	if (ret == -1) {
		pView->MessageBox(_T("命令处理失败!!!"));
		return -1;
	}

	CClientSocket* pClient = CClientSocket::GetInstance();

	// 【安全修复 1.1：引入本地安全结构体】
	FILEINFO currentInfo;
	PFILEINFO pInfo = &currentInfo;

	// 【安全修复 1.2：第一次数据获取 - 复制到本地并强制空终止符】
	const std::string& firstPacketData = pClient->GetPacket().strData;
	// 复制数据到本地安全内存
	memcpy(pInfo, firstPacketData.c_str(), sizeof(FILEINFO));
	// 强制空终止符，解决 CString 访问越界问题
	pInfo->szFileName[sizeof(pInfo->szFileName) - 1] = '\0';


	// PFILEINFO 结构体定义在 Packet.h
	while (pInfo->HasNext) {
		// CController 调用 Dlg 的 UpdateFileInfo 来处理 UI 逻辑
		pView->UpdateFileInfo(currentInfo, hTree);

		// Controller 调用 Model 继续处理下一个命令，这会覆盖共享缓冲区
		int cmd = m_pModel->DealCommand();
		if (cmd < 0) break;

		// 【安全修复 1.3：获取下一个包 - 复制到本地并强制空终止符】
		const std::string& nextPacketData = pClient->GetPacket().strData;
		// 复制数据到本地安全内存，替代原有的指针赋值
		memcpy(pInfo, nextPacketData.c_str(), sizeof(FILEINFO));
		// 强制空终止符
		pInfo->szFileName[sizeof(pInfo->szFileName) - 1] = '\0';
	}

	// 循环结束后，强制展开父节点以刷新 UI
	if (hTree != NULL) {
		pView->m_Tree.Expand(hTree, TVE_EXPAND);
	}

	m_pModel->CloseSocket(); // 关闭 Socket
	return 0;;
}

// Controller：删除文件 (View -> Model -> View)
int CClientController::RemoveFile(CString strPath, int nSelected, CRemoteClientDlg* pView) 
{
	// 1. Controller 调用 Model 发送删除请求 (命令 9)
	int ret = m_pModel->SendCommandPacket(pView->GetSafeHwnd(), 9, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
	if (ret < 0)
	{
		return -1;
	}
	// 2. Controller 指示 View 更新界面 (List)
	pView->m_List.DeleteItem(nSelected);
	return 0;
}

// Controller：运行文件 (View -> Model)
int CClientController::RunFile(CString strPath)
{
	// 1. Controller 调用 Model 发送运行请求 (命令 3)
	int ret = m_pModel->SendCommandPacket(m_remoteDlg.GetSafeHwnd(), 3, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
	if (ret < 0)
	{
		m_remoteDlg.MessageBox(_T("运行文件命令失败!"));
		return -1;
	}
	return 0;
}
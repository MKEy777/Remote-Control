#include "pch.h"
#include "ClientController.h"
#include "ClientSocket.h"

// 静态成员变量初始化
std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;
CClientController::CHelper CClientController::m_helper;

// ========================================================================
// 构造与析构
// ========================================================================

CClientController::CClientController()
    : m_statusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg) // 关键：传入主窗口指针作为父窗口
{
    m_isClosed = true;
    m_hThread = INVALID_HANDLE_VALUE;
    m_nThreadID = -1;
    m_hThreadWatch = INVALID_HANDLE_VALUE;
}

CClientController::~CClientController()
{
    // 等待主线程安全退出，超时100ms
    if (m_hThread != INVALID_HANDLE_VALUE) {
        WaitForSingleObject(m_hThread, 100);
        CloseHandle(m_hThread);
        m_hThread = INVALID_HANDLE_VALUE;
    }
}

void CClientController::releaseInstance()
{
    TRACE("CClientSocket has been called!\r\n");
    if (m_instance != NULL) {
        delete m_instance;
        m_instance = NULL;
        TRACE("CClientController has released!\r\n");
    }
}

// ========================================================================
// 单例与初始化
// ========================================================================

CClientController* CClientController::getInstance()
{
    if (m_instance == nullptr) {
        m_instance = new CClientController();
        TRACE("CClientController size is %d\r\n", sizeof(*m_instance));

        // 建立消息ID与成员函数的映射表
        struct { UINT nMsg; MSGFUNC func; } MsgFuncs[] = {
            {WM_SHOW_STATUS, &CClientController::OnShowStatus},
            {WM_SHOW_WATCH, &CClientController::OnShowWatcher},
            {(UINT)-1, NULL}
        };

        for (int i = 0; MsgFuncs[i].func != NULL; i++) {
            m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));
        }
    }
    return m_instance;
}

int CClientController::initController()
{
    // 创建后台消息分发线程
    m_hThread = (HANDLE)_beginthreadex(NULL, 0,
        &CClientController::threadEntry,
        this, 0, &m_nThreadID);

    // 创建状态对话框
    m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);
    return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
    // 绑定并模态显示主窗口
    pMainWnd = &m_remoteDlg;
    return m_remoteDlg.DoModal();
}

// ========================================================================
// 网络通信基础接口
// ========================================================================

bool CClientController::SendCommandPacket(HWND hWnd, int nCmd, bool bAutoClose, BYTE* pData, size_t nLength, WPARAM wParam)
{
    TRACE("cmd:%d %s start %lld \r\n", nCmd, __FUNCTION__, GetTickCount64());
    CClientSocket* pClient = CClientSocket::GetInstance();
    bool ret = pClient->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);
    return ret;
}

// ========================================================================
// 业务功能：文件下载
// ========================================================================

int CClientController::DownFile(CString strPath)
{
    // 弹出文件保存对话框
    CFileDialog dlg(
        FALSE, NULL,
        strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        NULL, &m_remoteDlg);

    if (dlg.DoModal() == IDOK) {
        m_strRemote = strPath;
        m_strLocal = dlg.GetPathName();

        // 尝试创建本地文件
        FILE* pFile = nullptr;
        if (fopen_s(&pFile, m_strLocal, "wb+") != 0)
        {
            AfxMessageBox(_T("无法创建文件！"));
            return -1;
        }

        // 发送下载请求，将本地文件句柄作为 wParam 传递
        SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);

        // 显示等待状态
        m_remoteDlg.BeginWaitCursor();
        m_statusDlg.m_info.SetWindowText(_T("命令正在执行中！"));
        m_statusDlg.ShowWindow(SW_SHOW);
        m_statusDlg.CenterWindow(&m_remoteDlg);
        m_statusDlg.SetActiveWindow();
    }
    return 0;
}

void CClientController::DownloadEnd()
{
    m_statusDlg.ShowWindow(SW_HIDE);
    m_remoteDlg.EndWaitCursor();
    m_remoteDlg.MessageBox(_T("下载完成！！"), _T("完成"));
}

// ========================================================================
// 业务功能：屏幕监控
// ========================================================================

void CClientController::StartWatchScreen()
{
    m_isClosed = false;
    // 启动屏幕监控数据请求线程
    m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreen, 0, this);

    // 显示监控窗口（模态），直到窗口关闭
    m_watchDlg.DoModal();

    // 窗口关闭后，标志线程结束并等待清理
    m_isClosed = true;
    WaitForSingleObject(m_hThreadWatch, 500);
}

void CClientController::threadWatchScreen()
{
    Sleep(50); // 初始缓冲
    ULONGLONG nTick = GetTickCount64();

    while (!m_isClosed) {
        // 如果监控窗口并没有处于"满"状态（即已准备好接收下一帧）
        if (m_watchDlg.isFull() == false) {
            // 控制帧率，确保至少间隔 30ms
            if (GetTickCount64() - nTick < 30) {
                Sleep(30 - DWORD(GetTickCount64() - nTick));
            }
            nTick = GetTickCount64();

            // 发送截屏请求 (命令号 6)
            int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6, true, NULL, 0);
            if (ret == 1) {
                m_watchDlg.SetImageStatus(true); // 标记正在等待数据
            }
            else {
                TRACE("获取图片失败！ret = %d\r\n", ret);
            }
        }
        Sleep(1); // 防止空转占用CPU
    }
    TRACE("thread end %d\r\n", m_isClosed);
}

void CClientController::threadWatchScreen(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadWatchScreen();
    _endthread();
}

// ========================================================================
// 后台消息分发线程
// ========================================================================

unsigned CClientController::threadEntry(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadFunc();
    _endthreadex(0);
    return 0;
}

void CClientController::threadFunc()
{
    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        // 处理自定义的跨线程消息
        if (msg.message == WM_SEND_MESSAGE) {
            // 使用头文件中定义的 MsgInfo 结构
            MsgInfo* pmsg = (MsgInfo*)msg.wParam; // 获取消息信息结构体指针
            HANDLE hEvent = (HANDLE)msg.lParam;   // 获取同步事件对象句柄

            std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(pmsg->msg.message);
            if (it != m_mapFunc.end()) {
                // 调用映射的成员函数
                pmsg->result = (this->*it->second)(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);
            }
            else {
                pmsg->result = -1;
            }
            SetEvent(hEvent); // 通知发送方处理完成
        }
        else {
            // 处理直接发送到线程消息队列的映射消息
            std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
            if (it != m_mapFunc.end()) {
                (this->*it->second)(msg.message, msg.wParam, msg.lParam);
            }
        }
    }
}

// ========================================================================
// 消息处理函数
// ========================================================================

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return m_watchDlg.DoModal();
}
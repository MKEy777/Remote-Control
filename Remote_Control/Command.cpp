#include "pch.h"     
#include "Command.h"
#include "Resource.h" 

// --- 全局静态变量  ---
HANDLE CCommand::g_hThread = NULL;
HWND   CCommand::g_hWnd = NULL;
bool   CCommand::g_bIsLock = false;

// 自定义消息
#define WM_APP_UNLOCK (WM_APP + 100)

CCommand::CCommand()
{
    struct
    {
        int nCmd;
        CMDFUNC func;
    } data[] = {
        {1, &CCommand::MakeDriverInfo},
        {2, &CCommand::MakeDirectoryInfo},
        {3, &CCommand::RunFile},
        {4, &CCommand::DownloadFile},
        {5, &CCommand::MouseEvent},
        {6, &CCommand::SendScreen},
        {7, &CCommand::LockMachine},
        {8, &CCommand::UnlockMachine},
        {9, &CCommand::DeleteLocalFile},
        {1981, &CCommand::TestConnect},
        {-1, NULL}
    };

    for (int i = 0; data[i].nCmd != -1; i++)
    {
        m_mapCmd.insert(std::make_pair(data[i].nCmd, data[i].func));
    }
}

// 执行命令
int CCommand::ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket)
{
    std::map<int, CMDFUNC>::iterator it = m_mapCmd.find(nCmd);
    if (it == m_mapCmd.end())
    {
        return -1; // 未找到对应的命令处理函数
    }
    return (this->*it->second)(lstPacket, inPacket);
}
void CCommand::RunCommand(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	CCommand* thiz = (CCommand*)arg;
    if (status > 0)
    {
        int ret = thiz->ExcuteCommand(status, lstPacket, inPacket);
        if(ret != 0)
        {
            OutputDebugString(_T("命令执行失败！！"));
		}
    }
    else {
		MessageBox(NULL, _T("无法正常接入队列，自动重连..."), _T("j接入用户失败"), MB_OK | MB_ICONERROR);
    }
}

int CCommand::MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {
    std::string result;
    for (int i = 1; i <= 26; i++) {
        if (_chdrive(i) == 0) {
            if (result.size() > 0)
                result += ',';
            result += 'A' + i - 1;
        }
    }
    lstPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
    return 0;
}
int CCommand::MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {
    std::string strPath= inPacket.strData;
    if (_chdir(strPath.c_str()) != 0) {
        FILEINFO finfo;
        finfo.HasNext = FALSE;
		lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
        OutputDebugString(_T("没有权限访问目录！！"));
        return -2;
    }
    _finddata_t fdata;
    intptr_t hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1) {
        OutputDebugString(_T("没有找到任何文件！！"));
        FILEINFO finfo;
        finfo.HasNext = FALSE;
        lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
        return -3;
    }
    int count = 0;
    do {
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
        count++;
    } while (!_findnext(hfind, &fdata));

    _findclose(hfind);

    FILEINFO finfo;
    finfo.HasNext = FALSE;
    lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
    return 0;
}
int CCommand::RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
    std::string strPath = inPacket.strData;
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    lstPacket.push_back(CPacket(3, NULL, 0));
    return 0;
}
int CCommand::DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
    std::string strPath = inPacket.strData;
    long long data = 0;
    FILE* pFile = NULL;

    CString strLog;
    strLog.Format(_T("[Debug] 尝试打开文件: %s\n"), CString(strPath.c_str()));
    OutputDebugString(strLog);

    errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
    if (err != 0) {
        strLog.Format(_T("[Error] 打开文件失败! Error Code: %d, Path: %s\n"), err, CString(strPath.c_str()));
        OutputDebugString(strLog);
        lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
    }
    if (pFile != NULL) {
        fseek(pFile, 0, SEEK_END);
        data = _ftelli64(pFile);

        strLog.Format(_T("[Debug] 文件打开成功，大小: %lld 字节\n"), data);
        OutputDebugString(strLog);

        lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
        fseek(pFile, 0, SEEK_SET);
        char buffer[1024] = "";
        size_t rlen = 0;
        do {
            rlen = fread(buffer, 1, 1024, pFile);
            lstPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));
        } while (rlen >= 1024);
        fclose(pFile);
    }
    lstPacket.push_back(CPacket(4, NULL, 0));
    return 0;
}
int CCommand::MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket) {
    MOUSEEV mouse;
    memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int packetW = screenW;
    int packetH = screenH;

    if (screenW > 1920) {
        packetW = 1920;
        packetH = screenH * 1920 / screenW;
    }
    long realX = (packetW > 0) ? (mouse.ptXY.x * screenW / packetW) : 0;
    long realY = (packetH > 0) ? (mouse.ptXY.y * screenH / packetH) : 0;

    DWORD nFlags = 0;
    switch (mouse.nButton) {
    case 0: nFlags = 1; break;
    case 1: nFlags = 2; break;
    case 2: nFlags = 4; break;
    case 4: nFlags = 8; break;
    }
    if (nFlags != 8) SetCursorPos(realX, realY);

    switch (mouse.nAction) {
    case 0: nFlags |= 0x10; break;
    case 1: nFlags |= 0x20; break;
    case 2: nFlags |= 0x40; break;
    case 3: nFlags |= 0x80; break;
    }

    switch (nFlags) {
    case 0x21: // 左双
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
    case 0x11: // 左单
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
        break;
    case 0x41: mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo()); break;
    case 0x81: mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo()); break;
    case 0x22: // 右双
        mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
        mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
    case 0x12: // 右单
        mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
        mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        break;
    case 0x42: mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo()); break;
    case 0x82: mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo()); break;
    case 0x24: // 中双
        mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
        mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
    case 0x14: // 中单
        mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
        mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        break;
    case 0x44: mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo()); break;
    case 0x84: mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo()); break;
    case 0x08: { // 移动
        int screenX = GetSystemMetrics(SM_CXSCREEN);
        int screenY = GetSystemMetrics(SM_CYSCREEN);
        LONG dx = realX * 65535 / (screenX - 1);
        LONG dy = realY * 65535 / (screenY - 1);
        mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE, dx, dy, 0, GetMessageExtraInfo());
        break;
    }
    }
    lstPacket.push_back(CPacket(5, NULL, 0));
    return 0;
}
int CCommand::SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket) {
    CImage screen;
    HDC hScreen = ::GetDC(NULL);
    int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
    int nWidth = GetDeviceCaps(hScreen, HORZRES);
    int nHeight = GetDeviceCaps(hScreen, VERTRES);

    int nDestWidth = nWidth;
    int nDestHeight = nHeight;
    if (nWidth > 1920) {
        nDestWidth = 1920;
        nDestHeight = nHeight * 1920 / nWidth;
    }

    screen.Create(nDestWidth, nDestHeight, nBitPerPixel);
    HDC hDestDC = screen.GetDC();
    SetStretchBltMode(hDestDC, HALFTONE);
    StretchBlt(hDestDC, 0, 0, nDestWidth, nDestHeight, hScreen, 0, 0, nWidth, nHeight, SRCCOPY);

    ReleaseDC(NULL, hScreen);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
    if (hMem == NULL) return -1;
    IStream* pStream = NULL;
    HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
    if (ret == S_OK) {
        screen.Save(pStream, Gdiplus::ImageFormatPNG);
        LARGE_INTEGER bg = { 0 };
        pStream->Seek(bg, STREAM_SEEK_SET, NULL);
        PBYTE pData = (PBYTE)GlobalLock(hMem);
        SIZE_T nSize = GlobalSize(hMem);
        lstPacket.push_back(CPacket(6, (BYTE*)pData, nSize));
        GlobalUnlock(hMem);
    }
    pStream->Release();
    GlobalFree(hMem);
    screen.ReleaseDC();
    return 0;
}
int CCommand::LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
    if (g_bIsLock) return 0;

    g_bIsLock = true;
    unsigned tid;
    g_hThread = (HANDLE)_beginthreadex(NULL, 0,
        (unsigned(__stdcall*)(void*)) & CCommand::threadLockDlg,
        NULL, 0, &tid);

    for (int i = 0; i < 100; i++) {
        if (g_hWnd && ::IsWindow(g_hWnd)) break;
        Sleep(10);
    }

    lstPacket.push_back(CPacket(7, NULL, 0));
    return 0;
}
int CCommand::UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
    if (!g_bIsLock) return 0;

    if (g_hWnd && ::IsWindow(g_hWnd)) {
        ::PostMessage(g_hWnd, WM_APP_UNLOCK, 0, 0);
    }

    lstPacket.push_back(CPacket(8, NULL, 0));
    return 0;
}
int CCommand::DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
    std::string strPath = inPacket.strData;
    DeleteFileA(strPath.c_str());
    lstPacket.push_back(CPacket(9, NULL, 0));
    return 0;
}
int CCommand::TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket) {
    lstPacket.push_back(CPacket(1981, NULL, 0));
    return 0;
}

// 线程函数：锁机窗口逻辑
unsigned __stdcall CCommand::threadLockDlg(void* arg)
{
    CLockDialog dlg;
    dlg.Create(IDD_DIALOG_INFO, NULL);
    g_hWnd = dlg.m_hWnd;

    CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
    if (pText) {
        CRect rtText;
        pText->GetWindowRect(rtText);
        int x = (GetSystemMetrics(SM_CXSCREEN) - rtText.Width()) / 2;
        int y = (GetSystemMetrics(SM_CYSCREEN) - rtText.Height()) / 2;
        pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
    }

    dlg.SetWindowPos(&CWnd::wndTopMost, 0, 0,
        GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_SHOWWINDOW);

    CRect rect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    ClipCursor(&rect);
    while (ShowCursor(FALSE) >= 0) {}

    HWND hTray = ::FindWindow(_T("Shell_TrayWnd"), NULL);
    if (hTray) ::ShowWindow(hTray, SW_HIDE);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_APP_UNLOCK) break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_KEYDOWN && msg.wParam == 0x41) break; // 按 'A' 键测试退出
    }

    ClipCursor(NULL);
    while (ShowCursor(TRUE) < 0) {}
    if (hTray) ::ShowWindow(hTray, SW_SHOW);
    dlg.DestroyWindow();

    g_bIsLock = false;
    g_hWnd = NULL;
    g_hThread = NULL;
    _endthreadex(0);
    return 0;
}
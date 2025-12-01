#include "pch.h"
#include "Command.h"
#include "Resource.h"

HANDLE CCommand::g_hLockThread = NULL;
HWND CCommand::g_hLockWnd = NULL;
bool CCommand::g_isLocked = false;

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
        {1981,&CCommand::TestConnect},
        {-1, NULL} 
    };
    for(int i = 0; data[i].nCmd != -1; i++)
    {
        m_mapCmd.insert(std::make_pair(data[i].nCmd, data[i].func));
    }
}

int CCommand::ExcuteCommand(int nCmd)
{
	std::map<int, CMDFUNC>::iterator it = m_mapCmd.find(nCmd);
    if(it==m_mapCmd.end())
    {
        return -1; // 未找到对应的命令处理函数
	}
    return (this->*it->second)();
}

unsigned __stdcall CCommand::threadLockDlg(void* arg)
{
	// 对话框对象放在线程里，避免跨线程访问
	CLockDialog dlg;

	// 创建窗口
	dlg.Create(IDD_DIALOG_INFO, NULL);

	// 发布句柄给主线程（通过 arg）
	HWND* pHwnd = (HWND*)arg;
	*pHwnd = dlg.m_hWnd;

	//把文字挪到屏幕正中间
	CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
	if (pText) {
		CRect rtText;
		pText->GetWindowRect(rtText);
		int nWidth = rtText.Width();
		int x = (GetSystemMetrics(SM_CXSCREEN) - nWidth) / 2; // 使用 rect.right (屏幕宽度) 计算居中 X
		int nHeight = rtText.Height();
		int y = (GetSystemMetrics(SM_CYSCREEN) - nHeight) / 2; // 使用 rect.bottom (屏幕高度) 计算居中 Y
		pText->MoveWindow(x, y, nWidth, nHeight);
	}

	// 全屏置顶
	dlg.SetWindowPos(&dlg.wndTopMost,
		0, 0,
		GetSystemMetrics(SM_CXSCREEN),
		GetSystemMetrics(SM_CYSCREEN),
		SWP_SHOWWINDOW);

	// 限制鼠标
	CRect rect(0, 0,
		GetSystemMetrics(SM_CXSCREEN),
		GetSystemMetrics(SM_CYSCREEN));
	//ClipCursor(rect);
	while (ShowCursor(FALSE) >= 0) {} // 保证隐藏

	// 隐藏任务栏
	HWND hTray = ::FindWindow(_T("Shell_TrayWnd"), NULL);
	if (hTray) ::ShowWindow(hTray, SW_HIDE);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (msg.message == WM_APP_UNLOCK)
			break;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_KEYDOWN) {
			if (msg.wParam == 0x41) {
				break;
			}
		}
	}

	// 恢复
	ClipCursor(NULL);
	while (ShowCursor(TRUE) < 0) {}
	if (hTray) ::ShowWindow(hTray, SW_SHOW);

	dlg.DestroyWindow();

	g_isLocked = false;
	g_hLockThread = NULL;

	_endthreadex(0);
	return 0;
}

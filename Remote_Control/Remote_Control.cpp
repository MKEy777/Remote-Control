// Remote_Control.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "Remote_Control.h"
#include "ServerSocket.h"
#include <io.h>
#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//#pragma comment(linker, "/subsystem:windows /entry:WinMainCRTStartup")
//#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")
//#pragma comment(linker, "/subsystem:console /entry:mainCRTStartup")
//#pragma comment(linker, "/subsystem:console /entry:WinMainCRTStartup")aaa

CWinApp theApp;

using namespace std;

void Dump(BYTE* pData, size_t nSize) {//调试输出十六进制数据
	std::string out;
    for (size_t i = 0;i < nSize;i++) {
        char buf[8] = "";
        sprintf_s(buf, sizeof(buf), "%02X ", pData[i]);//把一个字节格式化成可读的十六进制字符串
        out += buf;
    }
	out += '\n';
	OutputDebugStringA(out.c_str());//将字符串输出到 Visual Studio 的“输出”窗口（Debug 模式下可见）
}

//查看本地磁盘分区，并把信息打包成数据包
int MakeDriverInfo() {
	std::string result;
	for (int i = 1; i <= 26; i++) {
		if (_chdrive(i) == 0) {
			if (result.size() > 0)
				result += ',';
			result += 'A' + i - 1;
		}
	}

	CPacket pack(1, (BYTE*)result.c_str(), result.size());
	Dump((BYTE*)pack.Data(), pack.Size());
	CServerSocket::GetInstance()->Send(pack);
	return 0;
}

//发送指定目录的文件信息
int MakeDirectoryInfo() {
	std::string strPath;
	//std::list<FILEINFO> lstFileInfos;
	if (CServerSocket::GetInstance()->GetFilePath(strPath) == false) {
		OutputDebugString(_T("当前的命令，不是获取文件列表，命令解析错误！！"));
		return -1;
	}
	if (_chdir(strPath.c_str()) != 0) {
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::GetInstance()->Send(pack);
		OutputDebugString(_T("没有权限访问目录！！"));
		return -2;
	}
	_finddata_t fdata;
	int hfind = 0;
	if ((hfind = _findfirst("*", &fdata)) == -1) {
		OutputDebugString(_T("没有找到任何文件！！"));
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::GetInstance()->Send(pack);
		return -3;
	}
	int count = 0;
	do {
		FILEINFO finfo;
		finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
		memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
		TRACE("%s\r\n", finfo.szFileName);
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::GetInstance()->Send(pack);
		count++;
	} while (!_findnext(hfind, &fdata));
	TRACE("server: count = %d\r\n", count);
	_findclose(hfind);
	//发送信息到控制端
	FILEINFO finfo;
	finfo.HasNext = FALSE;
	CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
	CServerSocket::GetInstance()->Send(pack);
	return 0;
}

//运行指定的文件
int RunFile() {
	std::string strPath;
	CServerSocket::GetInstance()->GetFilePath(strPath);
	ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
	CPacket pack(3, NULL, 0);
	CServerSocket::GetInstance()->Send(pack);
	return 0;
}

//下载指定的文件
#pragma warning(disable:4966) // fopen sprintf strcpy strstr 
int DownloadFile() {
	std::string strPath;
	CServerSocket::GetInstance()->GetFilePath(strPath);
	long long data = 0;
	FILE* pFile = NULL;
	errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
	if (err != 0) {
		CPacket  pack(4, (BYTE*)&data, 8);
		CServerSocket::GetInstance()->Send(pack);
		return -1;
	}
	if (pFile != NULL) {
		fseek(pFile, 0, SEEK_END);
		data = _ftelli64(pFile);//获取文件大小
		CPacket head(4, (BYTE*)&data, 8);
		CServerSocket::GetInstance()->Send(head);
		fseek(pFile, 0, SEEK_SET);
		char buffer[1024] = "";
		size_t rlen = 0;
		do {
			rlen = fread(buffer, 1, 1024, pFile);
			CPacket pack(4, (BYTE*)buffer, rlen);
			CServerSocket::GetInstance()->Send(pack);
		} while (rlen >= 1024);
		fclose(pFile);
	}
	CPacket pack(4, NULL, 0);
	CServerSocket::GetInstance()->Send(pack);
	return 0;
}

//处理鼠标事件
int MouseEvent()
{
	MOUSEEV mouse;
	if (CServerSocket::GetInstance()->GetMouseEvent(mouse)) {
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
		if (realX < 0) realX = 0; if (realX >= screenW) realX = screenW - 1;
		if (realY < 0) realY = 0; if (realY >= screenH) realY = screenH - 1;

		DWORD nFlags = 0;
		switch (mouse.nButton) {
		case 0://左键
			nFlags = 1;
			break;
		case 1://右键
			nFlags = 2;
			break;
		case 2://中键
			nFlags = 4;
			break;
		case 4://没有按键
			nFlags = 8;
			break;
		}
		if (nFlags != 8)SetCursorPos(realX, realY);
		switch (mouse.nAction)
		{
		case 0://单击
			nFlags |= 0x10;
			break;
		case 1://双击
			nFlags |= 0x20;
			break;
		case 2://按下
			nFlags |= 0x40;
			break;
		case 3://放开
			nFlags |= 0x80;
			break;
		default:
			break;
		}
		TRACE("mouse event : %08X x %d y %d\r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
		switch (nFlags)
		{
		case 0x21://左键双击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x11://左键单击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x41://左键按下
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x81://左键放开
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x22://右键双击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x12://右键单击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x42://右键按下
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x82://右键放开
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x24://中键双击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x14://中键单击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x44://中键按下
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x84://中键放开
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x08:{//单纯的鼠标移动
			int screenX = GetSystemMetrics(SM_CXSCREEN);
			int screenY = GetSystemMetrics(SM_CYSCREEN);

			LONG dx = mouse.ptXY.x * 65535 / (screenX - 1);
			LONG dy = mouse.ptXY.y * 65535 / (screenY - 1);

			mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE, dx, dy, 0, GetMessageExtraInfo());
			break;
		}
		}
		CPacket pack(5, NULL, 0);
		CServerSocket::GetInstance()->Send(pack);
	}
	else {
		OutputDebugString(_T("获取鼠标操作参数失败！！"));
		return -1;
	}
	return 0;
}


//发送屏幕截图
#include <atlimage.h>
int SendScreen()
{
	CImage screen;//GDI
	HDC hScreen = ::GetDC(NULL);//获取整个屏幕的设备上下文。
	int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);//24   ARGB8888 32bit RGB888 24bit RGB565  RGB444
	int nWidth = GetDeviceCaps(hScreen, HORZRES);
	int nHeight = GetDeviceCaps(hScreen, VERTRES);

	// --- 增加缩放逻辑，限制最大宽度为 1920 ---
	int nDestWidth = nWidth;
	int nDestHeight = nHeight;
	if (nWidth > 1920) {
		nDestWidth = 1920;
		nDestHeight = nHeight * 1920 / nWidth;
	}

	screen.Create(nDestWidth, nDestHeight, nBitPerPixel);

	HDC hDestDC = screen.GetDC();
	SetStretchBltMode(hDestDC, HALFTONE); // 设置缩放模式，防止画质劣化
	StretchBlt(hDestDC, 0, 0, nDestWidth, nDestHeight, hScreen, 0, 0, nWidth, nHeight, SRCCOPY);//把屏幕内容复制到 CImage 对象内部的 DC

	ReleaseDC(NULL, hScreen);
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//分配一个可移动的全局内存块
	if (hMem == NULL)return -1;
	IStream* pStream = NULL;
	HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);//把全局内存包装成一个 COM 流 (IStream)
	if (ret == S_OK) {
		screen.Save(pStream, Gdiplus::ImageFormatPNG);//把 CImage 以 JPEG 格式写入这块内存
		LARGE_INTEGER bg = { 0 };
		pStream->Seek(bg, STREAM_SEEK_SET, NULL);
		PBYTE pData = (PBYTE)GlobalLock(hMem);//获取指针
		SIZE_T nSize = GlobalSize(hMem);//获取 JPEG 数据大小
		CPacket pack(6, pData, nSize);
		CServerSocket::GetInstance()->Send(pack);
		GlobalUnlock(hMem);
	}
	pStream->Release();
	GlobalFree(hMem);
	screen.ReleaseDC();
	return 0;
}
/*screen.Save(_T("test2020.png"), Gdiplus::ImageFormatPNG);
	//TRACE("png %d\r\n", GetTickCount64() - tick);
	for (int i = 0; i < 10; i++) {
		DWORD tick = GetTickCount64();
		screen.Save(_T("test2020.png"), Gdiplus::ImageFormatPNG);
		TRACE("png %d\r\n", GetTickCount64() - tick);
		tick = GetTickCount64();
		screen.Save(_T("test2020.jpg"), Gdiplus::ImageFormatJPEG);
		TRACE("jpg %d\r\n", GetTickCount64() - tick) ;
	}*/

#include "LockDialog.h"
#include <process.h>

static HANDLE g_hLockThread = NULL;
HWND g_hLockWnd = NULL;
static bool   g_isLocked = false;

// 解锁消息（直接发到窗口）
#define WM_APP_UNLOCK   (WM_APP + 100)

unsigned __stdcall threadLockDlg(void* arg)
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

int LockMachine()
{
	if (g_isLocked)
		return 0;   // 已经锁定了，避免重复创建线程

	g_isLocked = true;

	// 把窗口句柄存这里
	extern HWND g_hLockWnd;

	// 创建线程并把 &g_hLockWnd 传进去，让线程写回窗口句柄
	unsigned tid;
	g_hLockThread = (HANDLE)_beginthreadex(NULL, 0, threadLockDlg, &g_hLockWnd, 0, &tid);

	// 等待窗口创建（最多等待 1000ms）
	for (int i = 0; i < 100; i++)
	{
		if (g_hLockWnd)
			break;
		Sleep(10);
	}

	// 发送包
	CPacket pack(7, NULL, 0);
	CServerSocket::GetInstance()->Send(pack);

	return 0;
}

int UnlockMachine()
{
	if (!g_isLocked)
		return 0;

	// 直接发消息给窗口，而不是发给线程
	extern HWND g_hLockWnd;
	if (g_hLockWnd && IsWindow(g_hLockWnd))
		PostMessage(g_hLockWnd, WM_APP_UNLOCK, 0, 0);

	CPacket pack(8, NULL, 0);
	CServerSocket::GetInstance()->Send(pack);

	return 0;
}

int TestConnect()
{

	CPacket pack(1981, NULL, 0);
	bool ret = CServerSocket::GetInstance()->Send(pack);
	TRACE("Send ret = %d\r\n", ret);
	return 0;
}

int DeleteLocalFile() {
	std::string strPath;
	CServerSocket::GetInstance()->GetFilePath(strPath);
	TCHAR sPath[MAX_PATH] = _T("");//定义宽字符数组
	MultiByteToWideChar(
		CP_ACP, 0, strPath.c_str(), strPath.size(), sPath,
		sizeof(sPath) / sizeof(TCHAR));
	DeleteFileA(strPath.c_str());
	CPacket pack(9, NULL, 0);
	bool ret=CServerSocket::GetInstance()->Send(pack);
	TRACE("Send ret = %d\r\n", ret);
	return 0;
}

int ExcuteCommand(int nCmd) {
	int ret = 0;
	switch (nCmd) {
	case 1:
		ret=MakeDriverInfo();
		break;
	case 2:
		ret = MakeDirectoryInfo();
		break;
	case 3:
		ret = RunFile();
		break;
	case 4:
		ret = DownloadFile();
		break;
	case 5:
		ret = MouseEvent();
		break;
	case 6:
		ret = SendScreen();
		break;
	case 7:
		ret = LockMachine();
		break;
	case 8:
		ret = UnlockMachine();
		break;
	case 9:
		ret = DeleteLocalFile();
		break;
	case 1981:
		ret = TestConnect();
		break;
	}
	return ret;
}

int main()
{
    int nRetCode = 0;
    
    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
		else
		{	//全局静态变量初始化
			CServerSocket* pserver = CServerSocket::GetInstance();
			int count = 0;
			if (pserver->InitSocket() == false) {
				MessageBox(NULL, _T("网络初始化异常，未能成功初始hi，请检查网络状态！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
				exit(0);
			}
			while (CServerSocket::GetInstance() != NULL) {
				if (pserver->AcceptClient() == false) {
					if (count >= 3) {
						MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
						exit(0);
					}
					MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
					count++;
				}
				TRACE("AcceptClient return true\r\n");
				int ret = pserver->DealCommand();
				TRACE("DealCommand ret %d\r\n", ret);
				if (ret > 0) {
					ret = ExcuteCommand(ret);
					if (ret != 0) {
						TRACE("执行命令失败：%d ret=%d\r\n", pserver->GetPacket().sCmd, ret);
					}
					pserver->CloseClient();
					TRACE("Command has done!\r\n");
				}
			}
		}
    }else{
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}

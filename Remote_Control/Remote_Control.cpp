// Remote_Control.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "Remote_Control.h"
#include "ServerSocket.h"
#include "Tool.h"
#include "Command.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//#pragma comment(linker, "/subsystem:windows /entry:WinMainCRTStartup")
//#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")
//#pragma comment(linker, "/subsystem:console /entry:mainCRTStartup")
//#pragma comment(linker, "/subsystem:console /entry:WinMainCRTStartup")

#define INVOKE_PATH _T("C:\\Users\\edoyun\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")

CWinApp theApp;

using namespace std;

//bool ChooseAutoInvoke(const CString& strPath) {
//	TCHAR wcsSystem[MAX_PATH] = _T("");
//	if (PathFileExists(strPath)) {
//		return true;
//	}
//	CString strInfo = _T("该程序只允许用于合法的用途！\n");
//	strInfo += _T("继续运行该程序，将使得这台机器处于被监控状态！\n");
//	strInfo += _T("如果你不希望这样，请按“取消”按钮，退出程序。\n");
//	strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行！\n");
//	strInfo += _T("按下“否”按钮，程序只运行一次，不会在系统内留下任何东西！\n");
//	int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
//	if (ret == IDYES) {
//		//WriteRegisterTable(strPath);
//		if (!CTool::WriteStartupDir(strPath))
//		{
//			MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
//			return false;
//		}
//	}
//	else if (ret == IDCANCEL) {
//		return false;
//	}
//	return true;
//}
//
//int main()
//{
//    int nRetCode = 0;
//    
//    HMODULE hModule = ::GetModuleHandle(nullptr);
//
//	if (!CTool::Init())
//	{
//		if (!CTool::Init())return 1;
//		if (CTool::IsAdmin()) {
//			if (!CTool::Init())return 1;
//			if (ChooseAutoInvoke(INVOKE_PATH)) {
//				CCommand cmd;
//				int ret = CServerSocket::GetInstance()->Run(&CCommand::RunCommand, &cmd);
//				switch (ret) {
//				case -1:
//					MessageBox(NULL, _T("网络初始化异常，未能成功初始hi，请检查网络状态！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
//					break;
//				case -2:
//					MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
//					break;
//				}
//			}
//		}
//		else {
//			if (CTool::RunAsAdmin() == false) {
//				CTool::ShowError();
//				return 1;
//			}
//		}
//	}
//	return 0;
//}
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
		{
			CCommand cmd;
			//全局静态变量初始化
			CServerSocket* pserver = CServerSocket::GetInstance();
			int ret = pserver->Run(CCommand::RunCommand, &cmd);
			switch (ret)
			{
			case -1:
				MessageBox(NULL, _T("网络初始化异常，未能成功初始hi，请检查网络状态！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
				exit(0);
				break;
			case -2:
				MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
				exit(0);
				break;
			default:
				break;
			}
		}
	}
	else {
		// TODO: 更改错误代码以符合需要
		wprintf(L"错误: GetModuleHandle 失败\n");
		nRetCode = 1;
	}

	return nRetCode;
}

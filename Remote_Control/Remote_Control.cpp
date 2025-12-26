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
//			if (CTool::ChooseAutoInvoke(INVOKE_PATH)) {
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
	if (!CTool::Init()) return 1;
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
	return 0;
}

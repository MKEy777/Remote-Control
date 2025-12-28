// Remote_Control.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "Remote_Control.h"
#include "ServerSocket.h"
#include "Tool.h"
#include "Command.h"
#include <conio.h>
#include "Queue.h"
#include <MSWSock.h>

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

class COverlapped {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;
	char m_buffer[4096];
	COverlapped() {
		m_operator = 0;
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		memset(m_buffer, 0, sizeof(m_buffer));
	}
};

void IOCP()
{
	SOCKET sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (sock == INVALID_SOCKET)
	{
		CTool::ShowError();
		return;
	}
	HANDLE hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, sock, 4);
	SOCKET client = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	CreateIoCompletionPort((HANDLE)sock, hIOCP, 0, 0);
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(9527);
	if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) return;
	if (listen(sock, 5) == SOCKET_ERROR) return;

	COverlapped overlapped;
	overlapped.m_operator = 1;//accept
	memset(&overlapped.m_overlapped, 0, sizeof(OVERLAPPED));
	DWORD receved = 0;
	if (AcceptEx(sock, client, overlapped.m_buffer, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &receved, &overlapped.m_overlapped) == FALSE) {
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING) {
			CTool::ShowError();
			return;
		}
	}

	overlapped.m_operator = 2;//send
	//WSASend();
	overlapped.m_operator = 3;//recv
	//WSARecv();
	//开启线程处理IOCP
	while (true) {//代表一个线程
		DWORD transferred = 0;
		ULONG_PTR Key = 0;
		LPOVERLAPPED pOverlapped = NULL;
		if (GetQueuedCompletionStatus(hIOCP, &transferred, &Key, &pOverlapped, INFINITE)) {
			COverlapped* pO = CONTAINING_RECORD(pOverlapped, COverlapped, m_overlapped);
			switch (pO->m_operator) {
			case 1://accept
			{
			}
			}
		}
	}
}

int main()
{
	if (!CTool::Init()) return 1;
	IOCP();
}
//CCommand cmd;
	////全局静态变量初始化
	//CServerSocket* pserver = CServerSocket::GetInstance();
	//int ret = pserver->Run(CCommand::RunCommand, &cmd);
	//switch (ret)
	//{
	//case -1:
	//	MessageBox(NULL, _T("网络初始化异常，未能成功初始hi，请检查网络状态！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
	//	exit(0);
	//	break;
	//case -2:
	//	MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
	//	exit(0);
	//	break;
	//default:
	//	break;
	//}

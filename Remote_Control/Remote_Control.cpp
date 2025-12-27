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



#define IOCP_LIST_EMPTY 0
#define IOCP_LIST_PUSH  1
#define IOCP_LIST_POP   2
enum {
	IocpListEmpty,
	IocpListPush,
	IocpListPop
};
struct IOCP_DATA {
	int nOperator;
	std::string strData;
	_beginthreadex_proc_type callback;

	IOCP_DATA() :nOperator(-1){}
	IOCP_DATA(int op, const std::string data, _beginthreadex_proc_type cb=nullptr) :nOperator(op), strData(data), callback(cb) {}
};
void threadmain(HANDLE hIOCP) {
	std::list<std::string> lstString;
	DWORD dwTransferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* Overlapped = nullptr;
	while (GetQueuedCompletionStatus(hIOCP, &dwTransferred, &CompletionKey, &Overlapped, INFINITE)) {
		if (dwTransferred == 0 && CompletionKey == NULL && Overlapped == nullptr) {
			printf("IOCP thread exit\r\n");
			break;
		}
		IOCP_DATA* pParam = (IOCP_DATA*)CompletionKey;
		if (pParam->nOperator == IocpListPush) {
			lstString.push_back(pParam->strData);
		}
		else if (pParam->nOperator == IocpListPop) {
			std::string* pStr = nullptr;
			if (!lstString.empty()) {
				pStr = new std::string(lstString.front());
				lstString.pop_front();
			}
			if (pParam->callback) {
				pParam->callback(pStr);
			}
		}
		else if (pParam->nOperator == IocpListEmpty) {
			lstString.clear();
		}
		delete pParam;
	}
	lstString.clear();
}
void threadQueueEntry(HANDLE hIOCP)
{
	threadmain(hIOCP);
	_endthread();
}
unsigned __stdcall func(void* arg) {
	std::string* pstr = (std::string*)arg;
	if (pstr != NULL) {
		printf("pop from list: %s\r\n", pstr->c_str());
		delete pstr;
	}
	else {
		printf("pop from list: null\r\n");
	}
	return 0;
}

void test() {
	printf("press any key to exit...\r\n");

	CQueue<std::string> lstStrings;
	ULONGLONG total = GetTickCount64();
	ULONGLONG tick = GetTickCount64();
	ULONGLONG tick0 = GetTickCount64();
	while (GetTickCount64() - total < 1000) {
		//if (GetTickCount64() - tick0 >20)
		{
			lstStrings.PushBack("hello world");
			tick0 = GetTickCount64();
		}
	}
	printf("exit begin,size %d\r\n", lstStrings.Size());
	total = GetTickCount64();
	while (GetTickCount64() - total < 1000) {
		//if (GetTickCount64() - tick > 20)
		{
			std::string str;
			lstStrings.PopFront(str);
			tick = GetTickCount64();
		}
	}
	printf("exit done,size %d\r\n", lstStrings.Size());
	lstStrings.Clear();

}
int main()
{
	if (!CTool::Init()) return 1;
	for (int i = 0; i < 10; ++i) {
		test();
	}
	return 0;
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

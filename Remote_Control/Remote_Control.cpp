// Remote_Control.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "Remote_Control.h"
#include "ServerSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(linker, "/subsystem:windows /entry:WinMainCRTStartup")
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")
#pragma comment(linker, "/subsystem:console /entry:mainCRTStartup")
#pragma comment(linker, "/subsystem:console /entry:WinMainCRTStartup")

CWinApp theApp;

using namespace std;

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
			CServerSocket* pserver = CServerSocket::GetInstance();
			int count = 0;
            while (CServerSocket::GetInstance() != nullptr) {
                if (pserver->InitSocket() == false) {
					MessageBox(nullptr, L"初始化套接字失败", L"错误", MB_OK|MB_ICONERROR);
					exit(0);
                }
                if (pserver->AcceptClient() == false) {
                    if (count >= 3) {
                        MessageBox(nullptr, L"接受客户端连接失败，程序即将退出", L"错误", MB_OK | MB_ICONERROR);
						exit(0);
                    }
                    MessageBox(nullptr, L"接受客户端连接失败", L"错误", MB_OK | MB_ICONERROR);
					count++;
                }
				int ret = pserver->DealCommand();
            }
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}

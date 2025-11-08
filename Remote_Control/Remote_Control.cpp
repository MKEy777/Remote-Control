// Remote_Control.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "Remote_Control.h"
#include "ServerSocket.h"
#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(linker, "/subsystem:windows /entry:WinMainCRTStartup")
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")
#pragma comment(linker, "/subsystem:console /entry:mainCRTStartup")
#pragma comment(linker, "/subsystem:console /entry:WinMainCRTStartup")

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
    for (int i = 1; i <= 26; i++)//遍历磁盘符
    {
        if (_chdrive(i) == 0)//切换成功，利用切换磁盘分区的方法来遍历磁盘符
        {
            if (result.size() > 0)
                result += ',';
            result += 'A' + i - 1;//得到盘符
        }
    }

    CPacket pack(1, (BYTE*)result.c_str(), result.size());//打包

    //调试与发送
    Dump((BYTE*)pack.Data(), pack.Size());
    CServerSocket::GetInstance()->Send(pack);
    return 0;
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
        {
            MakeDriverInfo();
			/*CServerSocket* pserver = CServerSocket::GetInstance();
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
            }*/

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

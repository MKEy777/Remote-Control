#pragma once
#include <map>
#include <list>
#include <string>
#include <atlimage.h>
#include <process.h>
#include <direct.h>
#include <io.h>
#include "framework.h"
#include "LockDialog.h"
#include "ServerSocket.h"
#include "Tool.h"

class CCommand
{
public:
    CCommand();
    // 执行命令的入口
    int ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket);
    static void RunCommand(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket);

protected:
    static HANDLE g_hThread;
    static HWND   g_hWnd;
    static bool   g_bIsLock;

    int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket);
    int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket);
    int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket);
    int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket);
    int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket);
    int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket);
    int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket);
    int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket);
    int DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket);
    int TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket);
    // 线程函数声明 
    static unsigned __stdcall threadLockDlg(void* arg);

    // 命令函数指针定义
    typedef int(CCommand::* CMDFUNC)(std::list<CPacket>& lstPacket, CPacket& inPacket);
    // 命令映射表
    std::map<int, CMDFUNC> m_mapCmd;
};
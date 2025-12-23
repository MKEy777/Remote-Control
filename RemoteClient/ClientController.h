#pragma once
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include <map>
#include "resource.h"
#include "Tool.h"

#define WM_SHOW_STATUS (WM_USER + 3)//展示状态
#define WM_SHOW_WATCH (WM_USER+4) //远程监控
#define WM_SEND_MESSAGE (WM_USER+0x1000) //自定义消息处理

class CClientController
{
public:
    // ========================================================================
    // 单例模式与生命周期管理
    // ========================================================================

    // 获取单例对象指针
    static CClientController* getInstance();

    // 初始化控制器（创建后台消息线程等）
    int initController();

    // 启动主业务流程，加载主窗口
    int Invoke(CWnd*& pMainWnd);

    // ========================================================================
    // 网络通信与命令控制接口
    // ========================================================================

    // 更新服务器连接信息（IP与端口）
    void UpdateAddress(int nIP, int nPort) {
        CClientSocket::GetInstance()->UpdateAddress(nIP, nPort);
    }

    // 处理接收到的网络命令
    int DealCommand() {
        return CClientSocket::GetInstance()->DealCommand();
    }

    // 发送命令数据包
    // hWnd: 接收应答消息的窗口句柄
    // nCmd: 命令字
    bool SendCommandPacket(
        HWND hWnd,
        int nCmd,
        bool bAutoClose = true,
        BYTE* pData = NULL,
        size_t nLength = 0,
        WPARAM wParam = 0
    );

    // ========================================================================
    // 具体业务功能接口
    // ========================================================================

    // 请求下载远程文件
    int DownFile(CString strPath);

    // 下载完成后的处理逻辑
    void DownloadEnd();

    // 启动远程屏幕监控
    void StartWatchScreen();

protected:
    // ========================================================================
    // 构造与析构
    // ========================================================================
    CClientController();
    ~CClientController();

    // 释放单例资源
    static void releaseInstance();

    // ========================================================================
    // 线程处理函数
    // ========================================================================

    // 主后台线程：负责消息分发
    static unsigned _stdcall threadEntry(void* arg);
    void threadFunc();

    // 屏幕监控线程：负责循环获取屏幕数据
    static void threadWatchScreen(void* arg);
    void threadWatchScreen();

    // ========================================================================
    // 消息映射与处理
    // ========================================================================

    // 消息处理函数指针类型定义
    typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);

    // 各类自定义消息的处理实现
    LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
    // ========================================================================
    // 内部数据结构与成员变量
    // ========================================================================

    // 跨线程传递的消息封装结构
    struct MsgInfo {
        MSG msg;
        LRESULT result;
        MsgInfo(MSG m) {
            result = 0;
            memcpy(&msg, &m, sizeof(MSG));
        }
        MsgInfo(const MsgInfo& m) {
            result = m.result;
            memcpy(&msg, &m.msg, sizeof(MSG));
        }
        MsgInfo& operator=(const MsgInfo& m) {
            if (this != &m) {
                result = m.result;
                memcpy(&msg, &m.msg, sizeof(MSG));
            }
            return *this;
        }
    };

    // 窗口成员变量
    CRemoteClientDlg m_remoteDlg;  // 主控制窗口
    CWatchDialog     m_watchDlg;   // 远程监视窗口
    CStatusDlg       m_statusDlg;  // 状态显示窗口

    // 线程相关句柄与状态
    HANDLE   m_hThread;       // 主逻辑线程句柄
    unsigned m_nThreadID;     // 主逻辑线程ID
    HANDLE   m_hThreadWatch;  // 监视线程句柄
    bool     m_isClosed;      // 线程关闭标志位

    // 业务缓存数据
    CString m_strRemote;      // 远程文件路径
    CString m_strLocal;       // 本地文件路径

    // 静态成员：单例指针与消息映射表
    static CClientController* m_instance;
    static std::map<UINT, MSGFUNC> m_mapFunc;

    // 自动资源释放辅助类（RAII）
    class CHelper {
    public:
        CHelper() {}
        ~CHelper() {
            CClientController::releaseInstance();
        }
    };
    static CHelper m_helper;
};

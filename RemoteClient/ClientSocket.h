#pragma once

#include "pch.h"
#include "framework.h"

#include <afxsock.h>
#include <list>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#pragma pack(push, 1)

#define WM_SEND_PACK      (WM_USER + 1) // 发送包数据
#define WM_SEND_PACK_ACK  (WM_USER + 2) // 发送包数据应答

// ============================ CPacket ============================
class CPacket {
public:
    CPacket() = default;

    CPacket& operator=(const CPacket& packet) {
        if (this != &packet) {
            sHead = packet.sHead;
            nLength = packet.nLength;
            sCmd = packet.sCmd;
            strData = packet.strData;
            sSum = packet.sSum;
        }
        return *this;
    }

    // 封包构造：把数据封装成数据包
    CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
        sHead = 0xFEFF;
        nLength = nSize + 4;
        sCmd = nCmd;

        if (nSize > 0) {
            strData.resize(nSize);
            memcpy(&strData[0], pData, nSize);
        }
        else {
            strData.clear();
        }

        sSum = 0;
        for (size_t j = 0; j < strData.size(); j++) {
            sSum += BYTE(strData[j]) & 0xFF;
        }
    }

    // 解包构造：将包中的数据分配到成员变量中
    // nSize: 输入为缓冲区长度；输出为成功解析消耗的字节数；失败/不完整返回 0
    CPacket(const BYTE* pData, size_t& nSize) {
        size_t i = 0;

        // ① 查找包头 0xFEFF
        for (; i < nSize; i++) {
            if (*(WORD*)(pData + i) == 0xFEFF) {
                sHead = *(WORD*)(pData + i);
                i += 2;
                break;
            }
        }

        // 长度 + 命令 + 校验和
        if (i + 4 + 2 + 2 > nSize) {
            nSize = 0;
            return;
        }

        // ② 读取包总长度（4字节）
        nLength = *(DWORD*)(pData + i);
        i += 4;

        if (nLength + i > nSize) { // 判断数据是否接收完整
            nSize = 0;
            return;
        }

        // ③ 读取命令字段（2字节）
        sCmd = *(WORD*)(pData + i);
        i += 2;

        // ④ 读取包体数据
        if (nLength > 4) {
            strData.resize(nLength - 2 - 2);
            memcpy(&strData[0], pData + i, nLength - 4);
            i += nLength - 4;
        }

        // ⑤ 读取校验码并计算校验
        sSum = *(WORD*)(pData + i);
        i += 2;

        WORD sum = 0;
        for (size_t j = 0; j < strData.size(); j++)
            sum += BYTE(strData[j]) & 0xFF;

        if (sum == sSum) {
            nSize = i;
            return;
        }

        // 校验失败
        nSize = 0;
    }

    ~CPacket() {}

    int Size() { // 包数据大小
        return nLength + 6;
    }

    //CPacket实例打包成一段连续的二进制字节流
    const char* Data(std::string& strOut) const {
        strOut.resize(nLength + 6);

        BYTE* pData = (BYTE*)strOut.c_str();
        *(WORD*)pData = sHead;    pData += 2;
        *(DWORD*)pData = nLength; pData += 4;
        *(WORD*)pData = sCmd;     pData += 2;

        memcpy(pData, strData.c_str(), strData.size());
        pData += strData.size();

        *(WORD*)pData = sSum;
        return strOut.c_str();
    }

public:
    WORD        sHead = 0;
    DWORD       nLength = 0;
    WORD        sCmd = 0;
    std::string strData;//业务数据
    WORD        sSum = 0;
	std::string strOut;//完整包数据
};

// ============================ Structs ============================
typedef struct MouseEvent {
    MouseEvent() {
        nAction = 0;
        nButton = -1;
        ptXY.x = 0;
        ptXY.y = 0;
    }

    WORD  nAction; // 点击、移动、双击
    WORD  nButton; // 左键、中键、右键
    POINT ptXY;    // 坐标
} MOUSEEV, * PMOUSEEV;

typedef struct file_info {
    file_info() {
        IsInvalid = FALSE;
        IsDirectory = -1;
        HasNext = TRUE;
        memset(szFileName, 0, sizeof(szFileName));
    }

    BOOL IsInvalid;        // 是否有效
    BOOL IsDirectory;      // 是否为目录 0 否 1 是
    BOOL HasNext;          // 是否还有后续 0 没有 1 有
    char szFileName[256];  // 文件名
} FILEINFO, * PFILEINFO;

// ============================ Enums / Modes ============================
enum {
    CSM_AUTOCLOSE = 1, // CSM = Client Socket Mode 自动关闭模式
};

// ============================ PACKET_DATA ============================
typedef struct PacketData {
    std::string strData;
    UINT        nMode;   // AUTOCLOSE
    WPARAM      wParam;

    PacketData(const char* pData, size_t nLen, UINT mode, WPARAM nParam = 0) {
        strData.resize(nLen);
        memcpy((char*)strData.c_str(), pData, nLen);
        nMode = mode;
        wParam = nParam;
    }

    PacketData(const PacketData& data) {
        strData = data.strData;
        nMode = data.nMode;
        wParam = data.wParam;
    }

    PacketData& operator=(const PacketData& data) {
        if (this != &data) {
            strData = data.strData;
            nMode = data.nMode;
            wParam = data.wParam;
        }
        return *this;
    }
} PACKET_DATA;

std::string GetErrInfo(int wsaErrcode);

// ============================ CClientSocket ============================
class CClientSocket // 单例模式
{
public:
    static CClientSocket* GetInstance() {
        if (m_instance == nullptr) {
            m_instance = new CClientSocket();
        }
        return m_instance;
    }

public:
    bool InitSocket();

    // 在“接收线程”里不停解析服务器发来的数据包
#define BUFFER_SIZE 20480000
    int DealCommand();

    // 发送数据到服务端，然后接收返回消息，并 SendMessage() 到界面回调函数
    bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed = true, WPARAM wParam = 0);

    void CloseSocket() {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
    }

    CPacket& GetPacket() {
        return m_packet;
    }

    void UpdateAddress(int nIP, int nPort) {
        if ((m_nIP != nIP) || (m_nPort != nPort)) {
            m_nIP = nIP;
            m_nPort = nPort;
        }
    }

private:
    HANDLE m_eventInvoke = NULL;
    UINT   m_nThreadID = 0;
    HANDLE m_hThread = INVALID_HANDLE_VALUE;

    int  m_nIP = INADDR_ANY;
    int  m_nPort = 0;
    bool m_bAutoClose = true;

    size_t m_index = 0; // 记录缓冲区当前有效数据长度
    SOCKET m_sock = INVALID_SOCKET;

    typedef void (CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
    std::map<UINT, MSGFUNC> m_mapFunc;

    std::vector<char> m_buffer;
    std::mutex        m_lock;

    CPacket m_packet;

private:
    CClientSocket& operator=(const CClientSocket& ss) = delete;
    CClientSocket(const CClientSocket& ss) = delete;

    CClientSocket();
    ~CClientSocket();

private:
    // 发送数据到服务端，然后接收返回消息，并 SendMessage() 到界面回调函数
    void SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);

    static unsigned __stdcall threadEntry(void* arg);
    void threadFunc2();

    bool InitSockEnv() {
        if (!AfxSocketInit()) {
            return FALSE;
        }
        return TRUE;
    }

    static void ReleaseInstance() {
        if (m_instance != nullptr) {
            CClientSocket* tmp = m_instance;
            m_instance = nullptr;
            delete tmp;
        }
    }

private:
    static CClientSocket* m_instance;

    class CHelper {
    public:
        CHelper() { CClientSocket::GetInstance(); }
        ~CHelper() { CClientSocket::ReleaseInstance(); }
    };

    static CHelper m_helper;
};

#pragma pack(pop)

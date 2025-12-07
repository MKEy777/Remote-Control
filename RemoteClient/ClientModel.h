#pragma once
#include "pch.h"
#include <map>
#include <atlimage.h>
#include "ClientSocket.h" // Model需要与Socket直接交互
#include "Tool.h"         // 用于图像转换等工具函数


// 鼠标事件结构体 (从 CWatchDialog.cpp 移动到 Model 命名空间)
//typedef struct MouseEvent {
//	MouseEvent() {
//		nAction = 0;
//		nButton = -1;
//		ptXY.x = 0;
//		ptXY.y = 0;
//	}
//	WORD nAction;//点击、移动、双击
//	WORD nButton;//左键、中键、右键
//	POINT ptXY;//坐标
//}MOUSEEV, * PMOUSEEV;


class CClientModel
{
public:
	// 获取单例
	static CClientModel* getInstance();
	static void releaseInstance();

	// 核心业务方法 (从 CClientController 剥离)
	void UpdateAddress(int nIP, int nPort);
	int DealCommand();
	void CloseSocket();
	// 新增：用于在 Controller 启动时设置线程ID
	void SetControllerThreadID(unsigned nThreadID);
	bool SendPacket(const CPacket& pack) {
		CClientSocket* pClient = CClientSocket::GetInstance();
		if (pClient->InitSocket() == false) return false;
		return pClient->Send(pack);
	}
	// 发送数据包的业务逻辑
	bool SendCommandPacket(
		HWND hWnd,
		int nCmd,
		bool bAutoClose = true,
		BYTE* pData = NULL,
		size_t nLength = 0,
		WPARAM wParam = 0);

	// 下载文件业务逻辑
	int StartDownload(const CString& strRemote, FILE* pFile);
	void DownloadEnd();

	// 屏幕监控业务逻辑
	void SetWatchStatus(bool isClosed) { m_isClosed = isClosed; }
	bool IsWatchClosed() const { return m_isClosed; }
	int RequestScreenFrame(HWND hWnd);
	int GetImage(CImage& image); // 获取当前屏幕图像数据

	// 鼠标控制业务逻辑 (此处仅为接口，实际坐标转换在Controller中完成)
	// **注意：此处的实现我们已决定转移到 Controller，但为了匹配 .cpp 签名暂时保留**
	CPoint ConvertPointToRemote(const CPoint& viewPoint);
	bool SendMouseCommand(const MOUSEEV& event);

	// 状态数据访问器
	CImage& getCurrentImage() { return m_currentImage; }
	const CString& getRemotePath() const { return m_strRemote; }
	const CString& getLocalPath() const { return m_strLocal; }
	void setLocalPath(const CString& strLocal) { m_strLocal = strLocal; }

private:
	CClientModel();
	~CClientModel();

	// 状态数据
	bool m_isClosed;
	CString m_strRemote;    // 下载文件的远程路径
	CString m_strLocal;     // 下载文件的本地保存路径
	CImage m_currentImage;  // 最新的屏幕图像数据

	// Model 单例
	static CClientModel* m_instance;

	// 辅助类，用于单例模式的自动释放
	class CHelper {
	public:
		CHelper() { CClientModel::getInstance(); }
		~CHelper() { CClientModel::releaseInstance(); }
	};
	static CHelper m_helper;
};
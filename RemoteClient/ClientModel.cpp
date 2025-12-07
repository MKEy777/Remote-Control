#include "pch.h"
#include "ClientModel.h"
#include "ClientSocket.h" // 确保 ClientSocket 的头文件被包含

// 静态成员初始化
CClientModel* CClientModel::m_instance = NULL;
CClientModel::CHelper CClientModel::m_helper;

// 构造函数
CClientModel::CClientModel()
{
	m_isClosed = true;
}

// 析构函数
CClientModel::~CClientModel()
{
	// 释放资源
	if (!m_currentImage.IsNull()) {
		m_currentImage.Destroy();
	}
}

// 单例模式：获取实例
CClientModel* CClientModel::getInstance()
{
	if (m_instance == nullptr) {
		m_instance = new CClientModel();
		// Model 初始化逻辑
	}
	return m_instance;
}

// 单例模式：释放实例
void CClientModel::releaseInstance()
{
	TRACE("CClientModel has been called!\r\n");
	if (m_instance != NULL) {
		delete m_instance;
		m_instance = NULL;
		TRACE("CClientModel has released!\r\n");
	}
}

// ---------------------- 封装 ClientSocket 接口 ----------------------

void CClientModel::UpdateAddress(int nIP, int nPort) {
	CClientSocket::GetInstance()->UpdateAddress(nIP, nPort);
}

int CClientModel::DealCommand() {
	return CClientSocket::GetInstance()->DealCommand();
}

void CClientModel::CloseSocket() {
	CClientSocket::GetInstance()->CloseSocket();
}

// Model 封装 Socket 线程 ID 设置（供 Controller 初始化时调用）
void CClientModel::SetControllerThreadID(unsigned nThreadID) {
	CClientSocket::GetInstance()->SetThreadID(nThreadID);
}

// ---------------------- 核心业务逻辑 ----------------------

// 核心业务逻辑：发送数据包（与网络直接交互，属于Model职责）
bool CClientModel::SendCommandPacket(HWND hWnd, int nCmd, bool bAutoClose, BYTE* pData, size_t nLength, WPARAM wParam)
{
	TRACE("cmd:%d %s start %lld \r\n", nCmd, __FUNCTION__, GetTickCount64());
	CClientSocket* pClient = CClientSocket::GetInstance();
	// 原逻辑中 CClientSocket 是单例，我们沿用此实现。
	bool ret = pClient->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);
	return ret;
}

// 屏幕图像获取 (从 Socket 接收数据并存入 Model 状态)
int CClientModel::GetImage(CImage& image) {
	CClientSocket* pClient = CClientSocket::GetInstance();
	// 使用 Model 的私有 CImage m_currentImage 来存储数据
	return Tool::Bytes2Image(m_currentImage, pClient->GetPacket().strData);
}

// 业务逻辑：下载结束 (应通知Controller，由Controller通知View，此处仅作占位)
void CClientModel::DownloadEnd()
{
	// 原有显示/提示逻辑应移至Controller或View，此处仅保留与数据/状态相关的清理
	// 假设通知 Controller 的机制已建立（此处省略）
}

// 业务逻辑：请求屏幕帧
int CClientModel::RequestScreenFrame(HWND hWnd)
{
	// 6 是发送屏幕内容命令
	return SendCommandPacket(hWnd, 6, true, NULL, 0);
}

// 业务逻辑：开始下载（仅负责发送命令和记录路径）
int CClientModel::StartDownload(const CString& strRemote, FILE* pFile)
{
	// 4 是下载文件命令
	// Model 记录远程路径 (本地路径由 Controller 在此方法调用前通过 setLocalPath 设置)
	m_strRemote = strRemote;
	// 转发命令，文件句柄 pFile 通过 wParam 传递。
	SendCommandPacket(NULL, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
	return 0;
}

// 业务逻辑：将 View 坐标转换为远程屏幕坐标 (依赖 Model 的 CImage 尺寸状态)
CPoint CClientModel::ConvertPointToRemote(const CPoint& viewPoint)
{
	// View 矩形（需要 Controller 或 View 传入）
	// 注意：在纯 Model 中进行坐标转换逻辑是不合理的，因为 Model 不应该依赖 View 的尺寸。
	// 此处逻辑已在 Controller 中实现，Controller 会同时利用 View 的尺寸和 Model 的图像尺寸进行计算。

	if (m_currentImage.IsNull()) {
		return CPoint(0, 0);
	}

	// 暂时返回 (0, 0)，因为 View 尺寸信息缺失。
	return CPoint(0, 0);
}

// 业务逻辑：发送鼠标控制命令
bool CClientModel::SendMouseCommand(const MOUSEEV& event)
{
	// 5 是鼠标操作命令
	return SendCommandPacket(NULL, 5, true, (BYTE*)&event, sizeof(event));
}
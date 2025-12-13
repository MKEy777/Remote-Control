// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "ClientController.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	m_bFirstFrame = true;
	m_lastPoint = CPoint(0, 0);
	m_nObjWidth=-1;
	m_nObjHeight=-1;
	m_isFull = false;
}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	//ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDBLCLK()
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CWatchDialog::OnSendPackAck)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_isFull = false;
	// TODO:  在此添加额外的初始化
	SetTimer(0, 50, NULL);//50ms刷新一次

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

BOOL CWatchDialog::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN)
		{
			return TRUE; // 拦截回车键，防止触发默认按钮（锁机）
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}

LRESULT CWatchDialog::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || (lParam == -2)) {
		//TODO:错误处理
	}
	else if (lParam == 1) {
		//对方关闭了套接字
	}
	else {
		CPacket* pPacket = (CPacket*)wParam;
		if (pPacket != NULL) {
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;
			switch (head.sCmd) {
			case 6:
			{
				// 1. 将字节流转换为图片
				Tool::Bytes2Image(m_image, head.strData);

				// 2. 检查图片是否有效
				if (m_image.IsNull() == false) {
					m_nObjWidth = m_image.GetWidth();
					m_nObjHeight = m_image.GetHeight();

					// 如果是第一帧，调整窗口大小以适应远程屏幕分辨率
					if (m_bFirstFrame)
					{
						int nToolBarHeight = 80; // 预留工具栏高度，根据实际布局调整
						int nWidth = m_nObjWidth;
						int nHeight = m_nObjHeight;

						// 计算包含边框和标题栏的窗口大小
						CRect rectWindow(0, 0, nWidth, nHeight + nToolBarHeight);
						CalcWindowRect(&rectWindow);// 调整为窗口大小

						// 设置主窗口大小
						SetWindowPos(NULL, 0, 0, rectWindow.Width(), rectWindow.Height(), SWP_NOMOVE | SWP_NOZORDER);

						// 调整 Picture Control 大小，使其位于工具栏下方
						if (m_picture.GetSafeHwnd()) {
							m_picture.MoveWindow(0, nToolBarHeight, nWidth, nHeight);
						}

						CenterWindow();
						m_bFirstFrame = false;
					}
					// 获取 Picture Control 的设备上下文
					CClientDC dc(&m_picture);
					CRect rect;
					m_picture.GetClientRect(rect); // 获取控件客户区大小

					// 使用 StretchBlt 进行拉伸绘制，适应控件大小
					m_image.StretchBlt(dc.GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
					// 销毁图片对象，释放资源
					m_image.Destroy();
				}
				// 重置标志位 
				m_isFull = false;
				break;
			}
			case 5:
				TRACE("远程端应答了鼠标操作\r\n");
				break;
			case 7:
			case 8:
			default:
				break;
			}

		}
	}

	return 0;
}

void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//if (nIDEvent == 0) {
	//	if (m_isFull)
	//	{
	//		if (m_bFirstFrame)
	//		{
	//			int nToolBarHeight = 80;
	//			int nWidth = m_image.GetWidth();
	//			int nHeight = m_image.GetHeight();
	//			CRect rectWindow(0, 0, nWidth, nHeight + nToolBarHeight);
	//			CalcWindowRect(&rectWindow);
	//			SetWindowPos(NULL, 0, 0, rectWindow.Width(), rectWindow.Height(), SWP_NOMOVE | SWP_NOZORDER);

	//			// 调整 Picture Control 大小
	//			if (m_picture.GetSafeHwnd()) {
	//				m_picture.MoveWindow(0, nToolBarHeight, nWidth, nHeight);
	//			}

	//			CenterWindow();
	//			m_bFirstFrame = false;
	//		}
	//		CClientDC dc(&m_picture);

	//		CRect rect;
	//		m_picture.GetClientRect(rect); // 获取控件客户区大小

	//		// 将成员变量 m_image 绘制到控件上
	//		m_image.StretchBlt(dc.GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);

	//		m_image.Destroy();

	//		// 重置标志位
	//		m_isFull = false;
	//	}
	//}
	CDialog::OnTimer(nIDEvent);
}

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
void CWatchDialog::SendMouseEvent(int nAction, int nButton, CPoint point) {
	CPoint remotePt = UserPoint2RemoteScreenPoint(point, false);
	if (remotePt.x == -1 && remotePt.y == -1) return;
	// 2. 封装数据包结构体
	MOUSEEV event;
	event.ptXY = remotePt;
	event.nButton = nButton; // 0左键, 1右键, 2中键, 4无按键
	event.nAction = nAction; // 0单击, 1双击, 2按下, 3放开

	CClientController* pController = CClientController::getInstance();
	if (pController) {
		pController->SendCommandPacket(
			GetSafeHwnd(),
			5,
			true,
			(BYTE*)&event,
			sizeof(MOUSEEV)
		);
	}
}

CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen)
{
	if (m_nObjWidth <= 0 || m_nObjHeight <= 0) {
		return CPoint(-1, -1);
	}
	if (!isScreen) {
		ClientToScreen(&point); 
	}
	m_picture.ScreenToClient(&point);

	CRect clientRect;
	m_picture.GetClientRect(&clientRect);

	if (clientRect.Width() == 0 || clientRect.Height() == 0) {
		return CPoint(-1, -1);
	}

	int x = (int)((float)point.x * m_nObjWidth / clientRect.Width() + 0.5f);
	int y = (int)((float)point.y * m_nObjHeight / clientRect.Height() + 0.5f);

	return CPoint(x, y);
}

void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	SendMouseEvent(2, 0, point); // 2=按下, 0=左键
	CDialog::OnLButtonDown(nFlags, point);
}

void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	SendMouseEvent(3, 0, point); // 3=放开, 0=左键
	CDialog::OnLButtonUp(nFlags, point);
}

void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	SendMouseEvent(2, 1, point); // 2=按下, 1=右键
	CDialog::OnRButtonDown(nFlags, point);
}

void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	SendMouseEvent(3, 1, point); // 3=放开, 1=右键
	CDialog::OnRButtonUp(nFlags, point);
}

void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	SendMouseEvent(1, 0, point); // 1=双击, 0=左键
	CDialog::OnLButtonDblClk(nFlags, point);
}

void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	if ((abs(point.x - m_lastPoint.x) > 2) || (abs(point.y - m_lastPoint.y) > 2))
	{
		SendMouseEvent(0, 4, point); // 0=移动, 4=无按键
		m_lastPoint = point;
	}
	CDialog::OnMouseMove(nFlags, point);
}

//void CWatchDialog::OnSize(UINT nType, int cx, int cy)
//{
//	CDialog::OnSize(nType, cx, cy);
//
//	// 窗口大小改变时，同步调整 Picture Control 的大小
//	if (m_picture.GetSafeHwnd())
//	{
//		m_picture.MoveWindow(0, 0, cx, cy);
//	}
//}
void CWatchDialog::OnBnClickedBtnLock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 7);
}

void CWatchDialog::OnBnClickedBtnUnlock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),8);
}

void CWatchDialog::OnOK()
{
	// 按回车键时对话框不会关闭
	//CDialog::OnOK();
}

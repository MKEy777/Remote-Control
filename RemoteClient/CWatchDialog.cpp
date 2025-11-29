// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	m_bFirstFrame = true;
	m_lastPoint = CPoint(0, 0);

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
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序



BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

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

void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0) {
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();//获取父窗口指针
		if(pParent->isFull())
		{
			CImage& img = pParent->getImage();//获取图像引用
			if (m_bFirstFrame)
			{
				int nToolBarHeight = 80;
				// 1. 获取受控端屏幕的真实大小
				int nWidth = img.GetWidth();
				int nHeight = img.GetHeight();
				// 2. 计算窗口边框大小
				CRect rectWindow(0, 0, nWidth, nHeight + nToolBarHeight);
				CalcWindowRect(&rectWindow);
				// 3. 调整对话框窗口大小
				SetWindowPos(NULL, 0, 0, rectWindow.Width(), rectWindow.Height(), SWP_NOMOVE | SWP_NOZORDER);
				// 4. 调整内部 Picture Control 控件的大小，让它填满客户区
				if (m_picture.GetSafeHwnd()) {
					m_picture.MoveWindow(0, 0, nWidth, nHeight);
				}
				if (m_picture.GetSafeHwnd()) {
					m_picture.MoveWindow(0, nToolBarHeight, nWidth, nHeight);
				}
				// 5. 居中显示并在调整后标记为 false
				CenterWindow();
				m_bFirstFrame = false;
			}
			CRect rect;
			m_picture.GetWindowRect(rect);//获取静态控件的矩形区域
			HDC hdc = m_picture.GetDC()->GetSafeHdc();
	
			img.StretchBlt(hdc, 0, 0, rect.Width(), rect.Height(),SRCCOPY);//绘制图像
			//m_picture.InvalidateRect(NULL);//使静态控件无效，准备重绘
			//img.BitBlt(hdc, 0, 0, SRCCOPY);//绘制图像
			img.Destroy();
			pParent->SetImageStatus(false);
			
		}
	}
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
	CPoint remotePt = UserPoint2RemotePoint(point);

	// 2. 封装数据包结构体
	MOUSEEV event;
	event.ptXY = remotePt;
	event.nButton = nButton; // 0左键, 1右键, 2中键, 4无按键
	event.nAction = nAction; // 0单击, 1双击, 2按下, 3放开

	// 3. 发送消息给父窗口 (RemoteClientDlg) 进行网络发送
	// 参数说明：
	// Msg: WM_SEND_PACKET (你在 RemoteClientDlg.h 定义的消息)
	// wParam: 5 << 1 | 1 (5是命令号, 左移1位预留位, |1 表示长连接不关闭)
	// lParam: 结构体的指针
	CWnd* pParent = GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (LPARAM)&event);
}

CPoint CWatchDialog::UserPoint2RemotePoint(CPoint& point)
{
	CPoint ptControl = point;
	ClientToScreen(&ptControl);             // 1. 先转成屏幕绝对坐标
	m_picture.ScreenToClient(&ptControl);   // 2. 再转成画面控件内的相对坐标
	
	CRect rect;
	m_picture.GetClientRect(&rect);
	// 计算比例
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	CImage& img = pParent->getImage();

	if (img.IsNull()) {
		return CPoint(0, 0);
	}

	int nImgWidth = img.GetWidth();
	int nImgHeight = img.GetHeight();
	// 控件当前尺寸
	int nViewWidth = rect.Width();
	int nViewHeight = rect.Height();

	// 防止除以0
	if (nViewWidth == 0 || nViewHeight == 0) return CPoint(0, 0);

	// 坐标映射：真实坐标 = 点击坐标 * (原始尺寸 / 视图尺寸)
	int x = (int)((float)ptControl.x * nImgWidth / nViewWidth + 0.5f);
	int y = (int)((float)ptControl.y * nImgHeight / nViewHeight + 0.5f);

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
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 7 << 1 | 1);//发送锁机命令
}

void CWatchDialog::OnBnClickedBtnUnlock()
{
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 8 << 1 | 1);//发送解锁命令
}

void CWatchDialog::OnOK()
{
	// 按回车键时对话框不会关闭
	//CDialog::OnOK();
}

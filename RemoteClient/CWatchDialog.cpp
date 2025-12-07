// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
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
	// View 层的显示逻辑
	if (nIDEvent == 0) {
		CClientController* pController = CClientController::getInstance();

		// View 根据 Controller/Model 状态决定是否重绘
		if (m_isFull) // 检查是否已收到 Model 新帧的通知
		{
			// 从 Controller 获取 Model 中的图像引用
			CImage& img = pController->GetWatchImage();

			if (m_bFirstFrame)
			{
				int nToolBarHeight = 80;
				// 1. 获取 Model 中存储的受控端屏幕的真实大小
				int nWidth = img.GetWidth();
				int nHeight = img.GetHeight();
				// 2. 计算窗口边框大小
				CRect rectWindow(0, 0, nWidth, nHeight + nToolBarHeight);
				CalcWindowRect(&rectWindow);
				// 3. 调整对话框窗口大小
				SetWindowPos(NULL, 0, 0, rectWindow.Width(), rectWindow.Height(), SWP_NOMOVE | SWP_NOZORDER);
				// 4. 调整内部 Picture Control 控件的大小
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

			// 绘制图像
			img.StretchBlt(hdc, 0, 0, rect.Width(), rect.Height(), SRCCOPY);
			// 注意：不应在这里调用 img.Destroy()，因为 CImage 实例属于 Model 状态

			m_isFull = false; // View 标记已重绘
		}
	}
	CDialog::OnTimer(nIDEvent);
}

void CWatchDialog::SendMouseEvent(int nAction, int nButton, CPoint point) {
	// View 只负责捕获原始输入 (point) 并转发给 Controller
	CClientController::getInstance()->HandleMouseInput(nAction, nButton, point);
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

// 锁机/解锁 (View 转发给 Controller)
void CWatchDialog::OnBnClickedBtnLock()
{
	// Controller 转发命令给 Model (SendCommandPacket(..., 7))
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 7);
}

void CWatchDialog::OnBnClickedBtnUnlock()
{
	// Controller 转发命令给 Model (SendCommandPacket(..., 8))
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 8);
}

void CWatchDialog::OnOK()
{
	// 按回车键时对话框不会关闭
	//CDialog::OnOK();
}

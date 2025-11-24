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

void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0) {
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();//获取父窗口指针
		if(pParent->isFull())
		{
			CRect rect;
			m_picture.GetWindowRect(rect);//获取静态控件的矩形区域
			CImage& img = pParent->getImage();//获取图像引用
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

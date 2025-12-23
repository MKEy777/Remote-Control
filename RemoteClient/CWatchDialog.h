#pragma once
#include "afxdialogex.h"

// CWatchDialog 对话框

class CWatchDialog : public CDialog
{
	DECLARE_DYNAMIC(CWatchDialog)

public:
	// ---------------------------------------------------------
	// 构造与析构 (Construction & Destruction)
	// ---------------------------------------------------------
	CWatchDialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CWatchDialog();

	// 对话框数据 (Dialog Data)
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_WATCH };
#endif

public:
	// ---------------------------------------------------------
	//业务逻辑辅助函数 
	// ---------------------------------------------------------
	// 将用户在控件上的点击坐标转换为远程屏幕的绝对坐标
	CPoint UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen);

	// 封装并发送鼠标事件到服务端
	void SendMouseEvent(int nAction, int nButton, CPoint point);

	// ---------------------------------------------------------
	// 公共接口与状态管理 
	// ---------------------------------------------------------
	void SetImageStatus(bool isFull = false) {
		m_isFull = isFull;
	}
	bool isFull() const {
		return m_isFull;
	}

public:
	// ---------------------------------------------------------
	// 公共成员变量 (Public Members)
	// ---------------------------------------------------------
	int m_nObjWidth;        // 远程屏幕/图片宽度
	int m_nObjHeight;       // 远程屏幕/图片高度
	CPoint m_lastPoint;     // 上一次鼠标位置（用于防抖）
	CStatic m_picture;      // Picture Control 控件变量
	CImage m_image;

protected:
	// ---------------------------------------------------------
	// MFC 虚函数重写 (MFC Overrides)
	// ---------------------------------------------------------
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();                        // 初始化
	virtual BOOL PreTranslateMessage(MSG* pMsg);        // 消息预处理(拦截回车等)
	virtual void OnOK();                                // 覆盖默认的回车关闭行为

	// ---------------------------------------------------------
	// 消息响应函数 (Message Handlers)
	// ---------------------------------------------------------
	afx_msg LRESULT OnSendPackAck(WPARAM wParam, LPARAM lParam); // 自定义网络包确认消息

	// 鼠标交互消息
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);   // 左键按下
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);     // 左键弹起
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point); // 左键双击
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);   // 右键按下
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);     // 右键弹起
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);     // 鼠标移动

	// 按钮点击消息
	afx_msg void OnBnClickedBtnLock();   // 锁机
	afx_msg void OnBnClickedBtnUnlock(); // 解锁

	DECLARE_MESSAGE_MAP()

private:
	bool m_isFull;//缓存是否有数据 
	bool m_bFirstFrame;  // 标记是否是第一帧（用于自动调整窗口大小）
};
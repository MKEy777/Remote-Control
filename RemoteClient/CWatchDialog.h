#pragma once
#include "afxdialogex.h"


// CWatchDialog 对话框

class CWatchDialog : public CDialog
{
	DECLARE_DYNAMIC(CWatchDialog)

public:
	CWatchDialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CWatchDialog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_WATCH };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CPoint m_lastPoint;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL OnInitDialog();
	CStatic m_picture;
	//afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point); // 左键按下
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);   // 左键弹起
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point); // 右键按下
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);   // 右键弹起
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);   // 鼠标移动
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point); // 左键双击

	void SendMouseEvent(int nAction, int nButton, CPoint point);

	CPoint UserPoint2RemotePoint(CPoint& point);

private:
	bool m_bFirstFrame;
};

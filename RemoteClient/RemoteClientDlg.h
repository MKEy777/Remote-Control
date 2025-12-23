// CRemoteClientDlg dialog
// RemoteClientDlg.h : header file
#pragma once

#include "ClientSocket.h"
#include "StatusDlg.h"

#define WM_SEND_PACKET (WM_USER + 2)

// CRemoteClientDlg dialog
class CRemoteClientDlg : public CDialogEx
{
public:
    explicit CRemoteClientDlg(CWnd* pParent = nullptr); // standard constructor

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support

    // -------------------- Private methods --------------------
private:
    void DealCommand(WORD nCmd, const std::string& strData, LPARAM lParam);
    void InitUIData();

    // 文件管理相关
    void LoadFileInfo();
    void LoadFileCurrent();
    void Str2Tree(const std::string& drivers, CTreeCtrl& tree);
    void UpdateFileInfo(const FILEINFO& finfo, HTREEITEM hParent);
    void UpdateDownloadFile(const std::string& strData, FILE* pFile);

    // Tree/List 辅助
    CString GetPath(HTREEITEM hTree);
    void DeleteTreeChildrenItem(HTREEITEM hTree);

    // -------------------- Implementation --------------------
protected:
    HICON      m_hIcon = nullptr;
    CStatusDlg m_dlgStatus;

    // Generated message map functions
    virtual BOOL OnInitDialog() override;
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()

    // -------------------- UI controls & handlers --------------------
public:
    // 控件/数据
    DWORD   m_serv_address = 0;
    CString m_nPort;
    CTreeCtrl m_Tree;
    CListCtrl m_List;

    // 消息/事件处理
    afx_msg void OnBnClickedBtnTest();
    afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT OnSendPackAck(WPARAM wParam, LPARAM lParam);

    afx_msg void OnBnClickedBtnFileinfo();
    afx_msg void OnDownloadFile();
    afx_msg void OnDeleteFile();
    afx_msg void OnRunFile();
    afx_msg void OnBnClickedBtnStartWatch();

    afx_msg void OnEnChangeEditPort();
    afx_msg void OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult);
};

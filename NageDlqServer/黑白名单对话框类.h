// BwListDlg.h
#pragma once
#include "afxdialogex.h"

// CBwListDlg 对话框
class CBwListDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CBwListDlg)

public:
    CBwListDlg(CWnd* pParent = nullptr);   // 标准构造函数
    virtual ~CBwListDlg();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_BWLIST_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

public:
    CListBox    m_blackList;
    CListBox    m_whiteList;

    // 消息处理函数
    afx_msg void OnBnClickedSaveBw();
    afx_msg void OnBnClickedCloseBw();
    afx_msg void OnLbnDblclkBlistBwlist();
    afx_msg void OnLbnDblclkWlistBwlist();

private:
    void LoadIPLists();
    void SaveIPLists();
    BOOL EditListBoxItem(CListBox& listBox, int nIndex);
    CString ValidateIPAddress(const CString& ip);
};
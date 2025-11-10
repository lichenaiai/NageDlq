#pragma once
#include "afxdialogex.h"

// 输入对话框类
class 输入对话框类 : public CDialogEx
{
    DECLARE_DYNAMIC(输入对话框类)

public:
    输入对话框类(const CString& 提示文本, const CString& 初始值, CWnd* pParent = nullptr);
    virtual ~输入对话框类();

    CString 获取输入文本() const { return 输入文本; }

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_INPUT_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    CString 提示文本;
    CString 输入文本;
    CEdit 输入编辑框;

    DECLARE_MESSAGE_MAP()
    afx_msg void OnBnClickedOk();
};

// 黑白名单对话框类
class 黑白名单对话框类 : public CDialogEx
{
    DECLARE_DYNAMIC(黑白名单对话框类)

public:
    黑白名单对话框类(CWnd* pParent = nullptr);
    virtual ~黑白名单对话框类();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_BWLIST_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    CListBox 黑名单列表框;
    CListBox 白名单列表框;

    void 加载IP列表();
    void 保存IP列表();
    BOOL 编辑列表框项(CListBox& 列表框, int 项索引);
    CString 验证IP地址(const CString& IP地址);

    DECLARE_MESSAGE_MAP()
    afx_msg void OnBnClickedSaveBw();
    afx_msg void OnBnClickedCloseBw();
    afx_msg void OnLbnDblclkBlistBwlist();      //黑名单双击
    afx_msg void OnLbnDblclkWlistBwlist();      //白名单双击
};
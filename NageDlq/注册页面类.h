// 注册页面类.h
#pragma once
#include "pch.h"
#include "afxdialogex.h"

// 注册页面类
class 注册页面类 : public CDialogEx
{
    DECLARE_DYNAMIC(注册页面类)

public:
    注册页面类(CWnd* pParent = nullptr);   // 标准构造函数
    virtual ~注册页面类();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_PAGE_REGISTER };
#endif

protected:
    // 添加颜色变量和消息处理
    COLORREF 状态文本颜色;
    BOOL 需要设置颜色;

    // 添加消息处理函数
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

public:
    // 控件变量
    CEdit 账号编辑框;
    CEdit 密码编辑框;
    CEdit 确认密码编辑框;
    CEdit 邮箱编辑框;
    CButton 注册按钮;
    CStatic 注册状态标签;

    // 最后注册时间记录
    CTime 最后注册时间;

    // 消息处理函数
    afx_msg void OnBnClickedButtonRegConfirm();   // 注册按钮点击
    afx_msg void OnTimer(UINT_PTR nIDEvent);  // 添加定时器消息处理

    // 验证函数
    BOOL 验证输入();
    BOOL 验证账号规则(const CString& 账号);
    BOOL 验证密码规则(const CString& 密码);
    BOOL 验证邮箱规则(const CString& 邮箱);
    BOOL 检查注册时间限制();

    // 注册请求
    void 发送注册请求();

    // 清空输入框
    void 清空输入框();
    
    void 显示注册状态(const CString& 状态信息, BOOL 成功);
};
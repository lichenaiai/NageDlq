#pragma once
#include "pch.h"
#include "afxdialogex.h"
#include "NageDlqDlg.h"  // 包含主对话框头文件

// 注册页面类
class 注册页面类 : public CDialogEx
{
    DECLARE_DYNAMIC(注册页面类)

public:
    注册页面类(CWnd* pParent = nullptr);   // 标准构造函数
    virtual ~注册页面类();

    // 对话框数据
    enum { IDD = IDD_REGISTER_DIALOG };

protected:
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
    CButton 取消按钮;

    // 最后注册时间记录
    CTime 最后注册时间;

    // 主对话框指针
    NageDlqDlg* 主对话框指针;

    // 消息处理函数
    afx_msg void OnBnClickedButtonRegister();   // 注册按钮点击
    afx_msg void OnBnClickedButtonCancel();     // 取消按钮点击

    // 验证函数
    BOOL 验证输入();
    BOOL 验证账号规则(const CString& 账号);
    BOOL 验证密码规则(const CString& 密码);
    BOOL 验证邮箱规则(const CString& 邮箱);
    BOOL 检查注册时间限制();

    // 注册请求
    void 发送注册请求(const CString& 账号, const CString& 密码, const CString& 邮箱);
};

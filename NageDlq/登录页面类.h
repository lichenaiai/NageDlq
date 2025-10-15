#pragma once
#include "afxdialogex.h"


// 登录页面类 对话框

class 登录页面类 : public CDialogEx
{
	DECLARE_DYNAMIC(登录页面类)

public:
	登录页面类(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~登录页面类();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PAGE_LOGIN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CEdit 用户名编辑框;
	CEdit 密码编辑框;
	CButton 登录按钮;
	CButton 启动按钮;
	CButton 窗口1280;
};

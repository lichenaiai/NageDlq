#pragma once
#include "afxdialogex.h"

// 设置对话框类 对话框
class 设置对话框类 : public CDialogEx
{
	DECLARE_DYNAMIC(设置对话框类)

public:
	设置对话框类(CWnd* pParent = nullptr);
	virtual ~设置对话框类();

	enum { IDD = IDD_SETTINGS_DIALOG };

public:
	CString 数据库用户名;       // 数据库用户名
	CString 数据库密码;        // 数据库密码
	CString 数据库名称;        // 数据库名称

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButtonTestConnection();  // 测试连接按钮点击事件

private:
	// 控件变量 - 使用中文
	CEdit 数据库用户编辑框;
	CEdit 数据库密码编辑框;
	CEdit 数据库名称编辑框;
	CButton 测试连接按钮;
};

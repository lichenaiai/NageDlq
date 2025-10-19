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
	CString 数据库用户名;
	CString 数据库密码;
	CString 数据库名称;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnEraseBkgnd(CDC* pDC);
	virtual void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButtonTestConnection();
	afx_msg LRESULT OnUpdateStatus(WPARAM wParam, LPARAM lParam);

private:
	// 控件变量
	CEdit 数据库用户编辑框;
	CEdit 数据库密码编辑框;
	CEdit 数据库名称编辑框;
	CButton 测试连接按钮;
	CStatic 连接状态标签;

	// 状态相关
	BOOL 连接成功状态;
	COLORREF 状态文本颜色;

	// 线程相关
	static UINT 测试连接线程函数(LPVOID pParam);
	HANDLE 测试连接线程句柄;

	struct 线程参数 {
		设置对话框类* 对话框指针;
		CString 连接字符串;
	};
};

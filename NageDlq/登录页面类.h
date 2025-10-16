#pragma once

#include "afxdialogex.h"
#include <windows.h>
#include <tlhelp32.h>
#include <thread>

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

protected:
	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();	//初始化对话框
	afx_msg void OnBnClickedButtonLogin();		//点击登录按钮
	afx_msg void OnBnClickedButtonStart();		//点击启动按钮

public:
	CEdit 用户名编辑框;
	CEdit 密码编辑框;
	CButton 登录按钮;
	CButton 窗口1280;
	CButton 启动按钮;
	CStatic 背景图片;

private:
	bool 已登录;
	bool 窗口1280选中状态;

	// 注入相关函数
	void 注入窗口大小修改代码();
	void 注入IP修改代码();

	// 进程操作函数
	DWORD 获取进程ID(const wchar_t* 进程名);
	void 启动游戏进程();

	// 钩子安装函数
	void 等待并安装窗口大小钩子(const wchar_t* 监控进程名);
	void 等待并安装IP钩子(const wchar_t* 监控进程名);
	void 等待并安装UI钩子(const wchar_t* 监控进程名);
};
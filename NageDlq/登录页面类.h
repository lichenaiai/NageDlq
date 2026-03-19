// 登录页面类.h
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
	afx_msg void OnBnClickedButtonRelogin();  // 重新连接按钮点击事件
	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();	//初始化对话框
	afx_msg void OnBnClickedButtonLogin();		//点击登录按钮
	afx_msg void OnBnClickedButtonStart();		//点击启动按钮

	// 处理登录响应
	void 处理登录响应(const CString& 响应数据);

public:
	CEdit 用户名编辑框;
	CEdit 密码编辑框;
	CButton 登录按钮;
	CButton 窗口1280复选框;
	CButton 启动按钮;
	CStatic 背景图片;

private:
	
	bool 窗口1280选中状态;

	// 注入相关函数
	void 注入窗口大小修改代码();
	void 注入IP修改代码();

	// 进程操作函数
	DWORD 获取进程ID(const wchar_t* 进程名);
	void 启动游戏进程();

	// 钩子安装函数
	void 等待并安装窗口大小钩子(const wchar_t* 监控进程名);
	void 登录页面类::等待并安装汇编钩子(const wchar_t* 监控进程名);
	void 等待并安装IP钩子(const wchar_t* 监控进程名);
	void 等待并安装UI钩子(const wchar_t* 监控进程名);

	

	// 状态变量
	BOOL 客户端重新连接标志;    // 客户端重新连接标志
	BOOL 账号重新连接标志;   // 账号重新连接标志

	void OnTimer(UINT_PTR nIDEvent);

public:
	bool 已登录;

	// 登录页面显示密钥权限状态
	CStatic 权限状态;

	// 进程检测相关
	HANDLE 游戏进程句柄;      // 游戏进程句柄
	DWORD 游戏进程ID;    // 游戏进程ID
	BOOL 游戏运行中;        // 游戏是否正在运行
	BOOL 关闭游戏进程();        // 关闭游戏进程

	// 重新连接相关方法
	void 执行重新连接客户端();
	void 执行重新连接账号();

	void 退出登录状态();  // 退出当前登录状态

	CString 获取当前用户名();

	// 按钮控制函数
	void 启用重新连接按钮(BOOL 启用);  // 启用或禁用重新连接按钮
};
#pragma once
#include "pch.h"
#include "framework.h"
#include "NageDlqServer.h"
#include "NageDlqServerDlg.h"
#include "afxdialogex.h"
#include "设置对话框类.h" 
#include <map>
#include <vector> 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CNageDlqServerDlg 对话框
class CNageDlqServerDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CNageDlqServerDlg)

public:
	CNageDlqServerDlg(CWnd* pParent = nullptr);
	virtual ~CNageDlqServerDlg();

	enum { IDD = IDD_NAGEDLQSERVER_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	// 按钮点击事件 - 保持英文函数名
	afx_msg void OnBnClickedButtonStart();        // 启动服务器
	afx_msg void OnBnClickedButtonStop();         // 停止服务器
	afx_msg void OnBnClickedButtonUpdateClient(); // 推送登录器更新
	afx_msg void OnBnClickedButtonUpdateHook();  // 推送HOOK更新
	afx_msg void OnBnClickedButtonSettings();    // 设置

private:
	// 控件变量 - 使用中文
	// 控件变量 - 使用中文
	CButton 启动服务器按钮;
	CButton 停止服务器按钮;
	CButton 推送登录器更新按钮;
	CButton 推送HOOK更新按钮;
	CButton 设置按钮;
	CEdit 信息显示编辑框;
	CStatic 权限状态标签;
	CStatic 当前版本号标签;
	CStatic 连接数量标签;

	// 服务器状态
	BOOL 服务器运行状态;
	SOCKET 监听套接字;
	HANDLE 服务器线程句柄;

	// 客户端连接管理
	std::map<SOCKET, CString> 客户端连接列表;
	CRITICAL_SECTION 客户端列表锁;

	// 数据库配置 - 使用ODBC连接SQL Server
	CString 数据库用户名;
	CString 数据库密码;
	CString 数据库名称;

	// ODBC连接句柄
	SQLHENV SQL环境句柄;
	SQLHDBC SQL连接句柄;
	SQLHSTMT SQL语句句柄;
	BOOL 数据库连接状态;

	// 服务器信息
	CString 当前密钥;
	CString 当前版本号;
	int 客户端连接数量;

	// Hook功能列表
	struct Hook功能结构 {
		CString 功能名称;
		CString 功能代码;
		CString 功能描述;
	};
	std::vector<Hook功能结构> Hook功能列表;

public:
	// 线程函数
	static UINT 服务器线程函数(LPVOID pParam);  // 服务器线程
	static UINT 客户端线程函数(LPVOID pParam);  // 客户端线程

	// 服务器功能
	BOOL 启动服务器();
	BOOL 停止服务器();
	BOOL 加载配置();
	BOOL 保存配置();
	BOOL 连接数据库();
	BOOL 加载Hook功能();

	// 数据库操作
	CString 获取用户密钥(const CString& 用户名, const CString& 密码);
	CString 获取最新版本号();
	void 更新服务器信息();

	// 网络通信
	BOOL 发送到客户端(SOCKET 客户端套接字, const CString& 数据);
	CString 从客户端接收(SOCKET 客户端套接字);

	// 客户端管理
	void 添加客户端连接(SOCKET 客户端套接字, const CString& 客户端IP);
	void 移除客户端连接(SOCKET 客户端套接字);
	int 获取连接数量();

	// 信息显示
	void 添加信息显示(const CString& 信息);
	void 更新状态显示();
};

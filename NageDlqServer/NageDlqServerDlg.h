//NageDlqServerDlg.h
#pragma once
#include "pch.h"
#include "framework.h"
#include "NageDlqServer.h"
#include "NageDlqServerDlg.h"
#include "afxdialogex.h"
#include "设置对话框类.h" 
#include "端口转发.h"  
#include <map>
#include <vector> 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>


#ifndef INCLUDED_端口转发
#define INCLUDED_端口转发
#include "端口转发.h"
#endif

class 黑白名单对话框类;

// CNageDlqServerDlg 对话框
class NageDlqServerDlg : public CDialogEx
{
	DECLARE_DYNAMIC(NageDlqServerDlg)

public:
	NageDlqServerDlg(CWnd* pParent = nullptr);
	virtual ~NageDlqServerDlg();

	enum { IDD = IDD_NAGEDLQSERVER_DIALOG };
	BOOL 已初始化显示;

	

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT On延迟加载端口转发数据(WPARAM wParam, LPARAM lParam);
	afx_msg void On自定义绘制列表(NMHDR* pNMHDR, LRESULT* pResult);
public:
	// 按钮点击事件 - 保持英文函数名
	afx_msg void OnBnClickedButtonStart();        // 启动服务器
	afx_msg void OnBnClickedButtonStop();         // 停止服务器
	afx_msg void OnBnClickedButtonUpdateClient(); // 推送登录器更新
	afx_msg void OnBnClickedButtonUpdateHook();  // 推送HOOK更新
	afx_msg void OnBnClickedButtonSettings();    // 设置按钮
	afx_msg void OnBnClickedButtonBwlist();		//黑白名单按钮

private:
	// 控件变量
	CButton 启动服务器按钮;
	CButton 停止服务器按钮;
	CButton 推送登录器更新按钮;
	CButton 推送HOOK更新按钮;
	CButton 设置按钮;
	CListBox 信息显示编辑框;
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

	// 日志文件相关
	CStdioFile 日志文件;
	CString 当前日志文件名;
	BOOL 日志文件已打开;

	// 日志文件方法
	BOOL 初始化日志文件();
	BOOL 创建日志文件();
	void 关闭日志文件();
	CString 生成日志文件名();
	void 写入日志文件(const CString& 信息);

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

	// 端口转发
	int 当前编辑行 = -1;
	int 当前编辑列 = -1;
	CEdit 编辑控件;
	BOOL 正在编辑 = FALSE;
	CMenu 右键菜单;

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
	CString 获取客户端密钥();
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

	// 验证用户登录
	BOOL 验证用户登录(const CString& 用户名, const CString& 密码);

	// 注册处理函数
	BOOL 处理用户注册(const CString& 用户名, const CString& 密码, const CString& 邮箱);

	// 转生和加点相关函数
	BOOL 处理角色转生(const CString& 用户名, const CString& 角色名);
	BOOL 处理角色加点(const CString& 用户名, const CString& 角色名, int 力量, int 敏捷, int 意念, int 灵力);
	BOOL 检测账号是否在线(const CString& 用户名);
	void 获取职业初始属性(int 职业代码, int 累计等级, int& Lv, int& Exp, int& HP, int& SP, int& STM,
		int& Str, int& Dex, int& Esp, int& Spt, int& cmap, int& lvpoint, int& relvC);
	BOOL 检测角色是否在线(const CString& 角色名);
	void 获取用户角色列表(const CString& 用户名, CStringArray& 角色列表);

	// IP黑白名单相关
	std::vector<CString> 黑名单列表;
	std::vector<CString> 白名单列表;

	// 版本管理函数
	int 比较版本号(const CString& 版本1, const CString& 版本2);
	BOOL 检查IP权限(const CString& IP地址);
	void 加载黑白名单();

	CButton 黑白名单按钮;

public:
	// 端口转发相关成员
	端口转发管理类 端口转发管理器;

	// 端口转发控件
	CListCtrl 端口转发列表控件;
	CButton 启动转发按钮;
	CButton 停止转发按钮;

	// 端口转发相关方法
	void 初始化端口转发界面();
	void 刷新端口转发列表();
	BOOL 启动端口转发();
	BOOL 停止端口转发();
	void 添加默认转发规则();
	BOOL 保存端口转发配置();  
	BOOL 加载端口转发配置();
	BOOL 安全启动端口转发();
	BOOL 安全停止端口转发();
	BOOL 验证IP地址(const CString& IP地址);
	void 开始编辑单元格(int 行, int 列);
	void 结束编辑单元格(BOOL 保存更改 = TRUE);
	void 更新规则数据(int 行, int 列, const CString& 新值);
	void 添加新规则行();
	void 处理新增规则(int 行);
	void 添加空白行();
	void 设置空白行默认值(int 行索引);
	BOOL 修复日志文件编码(const CString& 文件名);

	// 消息处理函数
	afx_msg void On启动转发按钮点击();
	afx_msg void On停止转发按钮点击();
	afx_msg void On列表项双击(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void On列表结束编辑(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void On列表单击(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg	void On编辑框失去焦点();
	//afx_msg void On编辑框回车();
	afx_msg	void On编辑框内容改变();
	afx_msg void On右键菜单(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void On删除规则();
};

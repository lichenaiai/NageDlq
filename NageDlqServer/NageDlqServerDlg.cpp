#include "pch.h"
#include "framework.h"
#include "NageDlqServer.h"
#include "NageDlqServerDlg.h"
#include "afxdialogex.h"
#include "设置对话框类.h"

// 添加ODBC头文件
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// NageDlqServerDlg 对话框
IMPLEMENT_DYNAMIC(NageDlqServerDlg, CDialogEx)

NageDlqServerDlg::NageDlqServerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_NAGEDLQSERVER_DIALOG, pParent)
	, 服务器运行状态(FALSE)
	, 监听套接字(INVALID_SOCKET)
	, 服务器线程句柄(NULL)
	, SQL环境句柄(NULL)
	, SQL连接句柄(NULL)
	, SQL语句句柄(NULL)
	, 数据库连接状态(FALSE)
	, 当前密钥(_T(""))
	, 当前版本号(_T("1.0.0"))
	, 客户端连接数量(0)
	, 已初始化显示(FALSE)		// 添加初始化标志
{
	// 数据库配置 - 使用SQL Server默认设置
	数据库用户名 = _T("sa");           // SQL Server默认管理员
	数据库密码 = _T("your_password");  // 修改为您的密码
	数据库名称 = _T("nagelogin");      // 您的数据库名

	InitializeCriticalSection(&客户端列表锁);
}

NageDlqServerDlg::~NageDlqServerDlg()
{
	// 释放ODBC资源
	if (SQL语句句柄) SQLFreeHandle(SQL_HANDLE_STMT, SQL语句句柄);
	if (SQL连接句柄) SQLDisconnect(SQL连接句柄);
	if (SQL连接句柄) SQLFreeHandle(SQL_HANDLE_DBC, SQL连接句柄);
	if (SQL环境句柄) SQLFreeHandle(SQL_HANDLE_ENV, SQL环境句柄);

	DeleteCriticalSection(&客户端列表锁);
}

void NageDlqServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_START, 启动服务器按钮);
	DDX_Control(pDX, IDC_BUTTON_STOP, 停止服务器按钮);
	DDX_Control(pDX, IDC_BUTTON_UPDATE_CLIENT, 推送登录器更新按钮);
	DDX_Control(pDX, IDC_BUTTON_UPDATE_HOOK, 推送HOOK更新按钮);
	DDX_Control(pDX, IDC_BUTTON_SETTINGS, 设置按钮);
	DDX_Control(pDX, IDC_EDIT_INFO, 信息显示编辑框);
	DDX_Control(pDX, IDC_STATIC_KEY, 权限状态标签);
	DDX_Control(pDX, IDC_STATIC_VERSION, 当前版本号标签);
	DDX_Control(pDX, IDC_STATIC_CONNECTIONS, 连接数量标签);
}

BEGIN_MESSAGE_MAP(NageDlqServerDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_START, &NageDlqServerDlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &NageDlqServerDlg::OnBnClickedButtonStop)
	ON_BN_CLICKED(IDC_BUTTON_UPDATE_CLIENT, &NageDlqServerDlg::OnBnClickedButtonUpdateClient)
	ON_BN_CLICKED(IDC_BUTTON_UPDATE_HOOK, &NageDlqServerDlg::OnBnClickedButtonUpdateHook)
	ON_BN_CLICKED(IDC_BUTTON_SETTINGS, &NageDlqServerDlg::OnBnClickedButtonSettings)
END_MESSAGE_MAP()

BOOL NageDlqServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置初始状态
	停止服务器按钮.EnableWindow(FALSE);
	推送登录器更新按钮.EnableWindow(FALSE);
	推送HOOK更新按钮.EnableWindow(FALSE);


	// 初始化Winsock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		添加信息显示(_T("Winsock初始化失败"));
		return FALSE;
	}

	// 加载配置
	加载配置();

	添加信息显示(_T("程序已初始化"));
	return TRUE;
}

// 启动服务器按钮
void NageDlqServerDlg::OnBnClickedButtonStart()
{
	if (!服务器运行状态)
	{
		// 标记为已初始化
		已初始化显示 = TRUE;

		// 连接数据库
		if (连接数据库())
		{
			添加信息显示(_T("数据库连接成功"));
			权限状态标签.SetWindowText(_T("权限状态: 查询中..."));
			当前版本号标签.SetWindowText(_T("当前版本号: 查询中..."));
			// 查询数据库
			更新服务器信息();
		}
		else
		{
			添加信息显示(_T("数据库连接失败"));
			权限状态标签.SetWindowText(_T("权限状态: 数据库未连接"));
			当前版本号标签.SetWindowText(_T("当前版本号: 数据库未连接"));
			return;  // 如果数据库连接失败，不启动服务器
		}

		// 加载Hook功能
		if (加载Hook功能())
		{
			CString 信息;
			信息.Format(_T("加载了 %d 个Hook功能"), Hook功能列表.size());
			添加信息显示(信息);
		}
		else
		{
			添加信息显示(_T("加载Hook功能失败或未找到Hook文件"));
		}

		if (启动服务器())
		{
			服务器运行状态 = TRUE;
			启动服务器按钮.EnableWindow(FALSE);
			停止服务器按钮.EnableWindow(TRUE);
			推送登录器更新按钮.EnableWindow(TRUE);
			推送HOOK更新按钮.EnableWindow(TRUE);
			添加信息显示(_T("服务器启动成功"));
		}
	}
}

// 停止服务器按钮
void NageDlqServerDlg::OnBnClickedButtonStop()
{
	if (服务器运行状态)
	{
		停止服务器();
		服务器运行状态 = FALSE;
		启动服务器按钮.EnableWindow(TRUE);
		停止服务器按钮.EnableWindow(FALSE);
		推送登录器更新按钮.EnableWindow(FALSE);
		推送HOOK更新按钮.EnableWindow(FALSE);
		添加信息显示(_T("服务器已停止"));

		// 停止时清空显示
		权限状态标签.SetWindowText(_T("权限状态："));
		当前版本号标签.SetWindowText(_T("当前版本号："));
		连接数量标签.SetWindowText(_T("连接数量："));
	}
}

// 推送登录器更新按钮
void NageDlqServerDlg::OnBnClickedButtonUpdateClient()
{
	添加信息显示(_T("开始推送登录器更新..."));

	// 向所有连接的客户端发送更新通知
	EnterCriticalSection(&客户端列表锁);
	for (const auto& 客户端 : 客户端连接列表)
	{
		发送到客户端(客户端.first, _T("UPDATE_CLIENT:新版本登录器可用"));
	}
	LeaveCriticalSection(&客户端列表锁);

	添加信息显示(_T("登录器更新推送完成"));
}

// 推送HOOK更新按钮
void NageDlqServerDlg::OnBnClickedButtonUpdateHook()
{
	添加信息显示(_T("开始推送HOOK更新..."));

	// 向所有连接的客户端发送HOOK更新通知
	EnterCriticalSection(&客户端列表锁);
	for (const auto& 客户端 : 客户端连接列表)
	{
		发送到客户端(客户端.first, _T("UPDATE_HOOK:新的HOOK功能可用"));
	}
	LeaveCriticalSection(&客户端列表锁);

	添加信息显示(_T("HOOK更新推送完成"));
}

// 设置按钮
void NageDlqServerDlg::OnBnClickedButtonSettings()
{
	设置对话框类 设置对话框;
	设置对话框.数据库用户名 = 数据库用户名;
	设置对话框.数据库密码 = 数据库密码;
	设置对话框.数据库名称 = 数据库名称;

	if (设置对话框.DoModal() == IDOK)
	{
		数据库用户名 = 设置对话框.数据库用户名;
		数据库密码 = 设置对话框.数据库密码;
		数据库名称 = 设置对话框.数据库名称;

		保存配置();
		添加信息显示(_T("配置已更新"));

		// 重新连接数据库
		if (SQL连接句柄)
		{
			SQLDisconnect(SQL连接句柄);
			SQLFreeHandle(SQL_HANDLE_DBC, SQL连接句柄);
			SQL连接句柄 = NULL;
		}
		连接数据库();
		更新服务器信息();
	}
}

// 服务器线程函数
UINT NageDlqServerDlg::服务器线程函数(LPVOID pParam)
{
	NageDlqServerDlg* 对话框指针 = (NageDlqServerDlg*)pParam;

	// 创建监听socket
	对话框指针->监听套接字 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (对话框指针->监听套接字 == INVALID_SOCKET)
	{
		对话框指针->添加信息显示(_T("创建socket失败"));
		return 1;
	}

	// 绑定地址
	sockaddr_in 服务器地址;
	服务器地址.sin_family = AF_INET;
	服务器地址.sin_port = htons(9896);
	服务器地址.sin_addr.s_addr = INADDR_ANY;

	if (bind(对话框指针->监听套接字, (sockaddr*)&服务器地址, sizeof(服务器地址)) == SOCKET_ERROR)
	{
		对话框指针->添加信息显示(_T("绑定端口失败"));
		closesocket(对话框指针->监听套接字);
		return 1;
	}

	// 开始监听
	if (listen(对话框指针->监听套接字, 10) == SOCKET_ERROR)
	{
		对话框指针->添加信息显示(_T("监听失败"));
		closesocket(对话框指针->监听套接字);
		return 1;
	}

	对话框指针->添加信息显示(_T("开始监听端口 9896"));

	// 接受客户端连接
	while (对话框指针->服务器运行状态)
	{
		sockaddr_in 客户端地址;
		int 客户端地址长度 = sizeof(客户端地址);
		SOCKET 客户端套接字 = accept(对话框指针->监听套接字, (sockaddr*)&客户端地址, &客户端地址长度);

		if (客户端套接字 != INVALID_SOCKET)
		{
			char 客户端IP缓冲区[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(客户端地址.sin_addr), 客户端IP缓冲区, INET_ADDRSTRLEN);

			CString 客户端IP = CString(客户端IP缓冲区);
			对话框指针->添加客户端连接(客户端套接字, 客户端IP);
			对话框指针->添加信息显示(客户端IP + _T(" 已连接"));

			// 为每个客户端创建线程
			HANDLE 客户端线程句柄 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)客户端线程函数,
				(LPVOID)客户端套接字, 0, NULL);
			if (客户端线程句柄)
			{
				CloseHandle(客户端线程句柄);
			}
		}
		else
		{
			Sleep(100);
		}
	}

	closesocket(对话框指针->监听套接字);
	return 0;
}

// 客户端线程函数
UINT NageDlqServerDlg::客户端线程函数(LPVOID pParam)
{
	SOCKET 客户端套接字 = (SOCKET)pParam;
	NageDlqServerDlg* 对话框指针 = (NageDlqServerDlg*)AfxGetApp()->GetMainWnd();

	// 接收客户端请求
	CString 客户端请求 = 对话框指针->从客户端接收(客户端套接字);
	if (客户端请求.IsEmpty())
	{
		CString 客户端IP;
		对话框指针->移除客户端连接(客户端套接字);
		closesocket(客户端套接字);
		return 1;
	}

	// 记录客户端请求
	CString 客户端IP;
	EnterCriticalSection(&对话框指针->客户端列表锁);
	auto it = 对话框指针->客户端连接列表.find(客户端套接字);
	if (it != 对话框指针->客户端连接列表.end())
	{
		客户端IP = it->second;
	}
	LeaveCriticalSection(&对话框指针->客户端列表锁);

	if (!客户端IP.IsEmpty())
	{
		对话框指针->添加信息显示(客户端IP + _T(" 请求: ") + 客户端请求);
	}

	// 解析请求
	if (客户端请求.Find(_T("LOGIN:")) == 0)
	{
		// 处理登录请求
		CString 请求数据 = 客户端请求.Mid(6);
		int 分隔符位置 = 请求数据.Find(':');
		if (分隔符位置 != -1)
		{

			// 查询数据库
			CString 客户端密钥 = 对话框指针->获取客户端密钥();
			CString 最新版本号 = 对话框指针->获取最新版本号();

			// 发送响应
			CString 响应数据;
			if (!客户端密钥.IsEmpty())
			{
				响应数据.Format(_T("SUCCESS:%s:%s"), 客户端密钥, 最新版本号);
				对话框指针->添加信息显示(客户端IP + _T(" 登录成功"));
			}
			else
			{
				响应数据 = _T("FAILED:用户名或密码错误");
				对话框指针->添加信息显示(客户端IP + _T(" 登录失败"));
			}

			对话框指针->发送到客户端(客户端套接字, 响应数据);
		}
	}
	else if (客户端请求 == _T("GET_HOOKS"))
	{
		// 发送Hook功能列表
		CString 响应数据 = _T("HOOKS_LIST:");
		for (const auto& hook : 对话框指针->Hook功能列表)
		{
			响应数据 += hook.功能名称 + _T("|") + hook.功能描述 + _T(";");
		}
		对话框指针->发送到客户端(客户端套接字, 响应数据);
		对话框指针->添加信息显示(客户端IP + _T(" 请求HOOK列表"));
	}
	else if (客户端请求.Find(_T("GET_HOOK_CODE:")) == 0)
	{
		// 获取特定Hook的代码
		CString Hook名称 = 客户端请求.Mid(14);
		CString Hook代码;

		for (const auto& hook : 对话框指针->Hook功能列表)
		{
			if (hook.功能名称 == Hook名称)
			{
				Hook代码 = hook.功能代码;
				break;
			}
		}

		if (!Hook代码.IsEmpty())
		{
			对话框指针->发送到客户端(客户端套接字, _T("HOOK_CODE:") + Hook代码);
			对话框指针->添加信息显示(客户端IP + _T(" 获取HOOK代码: ") + Hook名称);
		}
		else
		{
			对话框指针->发送到客户端(客户端套接字, _T("HOOK_CODE_NOT_FOUND"));
			对话框指针->添加信息显示(客户端IP + _T(" 请求的HOOK不存在: ") + Hook名称);
		}
	}
}
// 启动服务器
BOOL NageDlqServerDlg::启动服务器()
{
	服务器线程句柄 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)服务器线程函数,
		(LPVOID)this, 0, NULL);
	return 服务器线程句柄 != NULL;
}

// 停止服务器
BOOL NageDlqServerDlg::停止服务器()
{
	服务器运行状态 = FALSE;
	if (服务器线程句柄)
	{
		WaitForSingleObject(服务器线程句柄, 5000);
		CloseHandle(服务器线程句柄);
		服务器线程句柄 = NULL;
	}

	// 关闭所有客户端连接
	EnterCriticalSection(&客户端列表锁);
	for (const auto& 客户端 : 客户端连接列表)
	{
		closesocket(客户端.first);
	}
	客户端连接列表.clear();
	客户端连接数量 = 0;
	LeaveCriticalSection(&客户端列表锁);

	更新状态显示();
	return TRUE;
}

// 连接数据库 (ODBC方式连接SQL Server)
BOOL NageDlqServerDlg::连接数据库()
{
	SQLRETURN retcode;

	// 如果已经连接，先释放资源
	if (SQL语句句柄) {
		SQLFreeHandle(SQL_HANDLE_STMT, SQL语句句柄);
		SQL语句句柄 = NULL;
	}
	if (SQL连接句柄) {
		SQLDisconnect(SQL连接句柄);
		SQLFreeHandle(SQL_HANDLE_DBC, SQL连接句柄);
		SQL连接句柄 = NULL;
	}
	if (SQL环境句柄) {
		SQLFreeHandle(SQL_HANDLE_ENV, SQL环境句柄);
		SQL环境句柄 = NULL;
	}

	// 分配环境句柄
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &SQL环境句柄);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
		数据库连接状态 = FALSE;
		return FALSE;
	}

	// 设置ODBC版本
	retcode = SQLSetEnvAttr(SQL环境句柄, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
		数据库连接状态 = FALSE;
		return FALSE;
	}

	// 分配连接句柄
	retcode = SQLAllocHandle(SQL_HANDLE_DBC, SQL环境句柄, &SQL连接句柄);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
		数据库连接状态 = FALSE;
		return FALSE;
	}

	// 连接字符串
	CString 连接字符串;
	连接字符串.Format(_T("DRIVER={SQL Server};SERVER=47.116.167.99;DATABASE=%s;UID=%s;PWD=%s;"),
		数据库名称, 数据库用户名, 数据库密码);

	SQLWCHAR* wszConnStr = (SQLWCHAR*)连接字符串.GetBuffer();
	SQLSMALLINT cbConnStrOut;

	// 连接到数据库
	retcode = SQLDriverConnectW(SQL连接句柄, NULL, wszConnStr, SQL_NTS,
		NULL, 0, &cbConnStrOut, SQL_DRIVER_NOPROMPT);

	连接字符串.ReleaseBuffer();

	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
		数据库连接状态 = FALSE;
		return FALSE;
	}

	// 分配语句句柄
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, SQL连接句柄, &SQL语句句柄);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
		数据库连接状态 = FALSE;
		return FALSE;
	}

	数据库连接状态 = TRUE;
	return TRUE;
}

// 加载Hook功能
BOOL NageDlqServerDlg::加载Hook功能()
{
	Hook功能列表.clear();  // 清空列表
	CString Hook目录 = _T("C:\\nagehook\\");
	CString 说明文件 = Hook目录 + _T("ps.txt");

	// 读取说明文件
	CStdioFile 文件;
	if (文件.Open(说明文件, CFile::modeRead))
	{
		CString 行内容;
		while (文件.ReadString(行内容))
		{
			int 分隔符位置 = 行内容.Find(':');
			if (分隔符位置 != -1)
			{
				Hook功能结构 Hook功能;
				Hook功能.功能名称 = 行内容.Left(分隔符位置);
				Hook功能.功能描述 = 行内容.Mid(分隔符位置 + 1);

				// 读取对应的Hook代码文件
				CString 代码文件 = Hook目录 + Hook功能.功能名称 + _T(".txt");
				CStdioFile 代码文件对象;
				if (代码文件对象.Open(代码文件, CFile::modeRead))
				{
					CString 代码内容;
					CString 代码行;
					while (代码文件对象.ReadString(代码行))
					{
						代码内容 += 代码行 + _T("\n");
					}
					Hook功能.功能代码 = 代码内容;
					代码文件对象.Close();
				}

				Hook功能列表.push_back(Hook功能);
			}
		}
		文件.Close();
	}

	CString 信息;
	信息.Format(_T("加载了 %d 个Hook功能"), Hook功能列表.size());
	添加信息显示(信息);

	return !Hook功能列表.empty();
}

// 获取客户端密钥
CString NageDlqServerDlg::获取客户端密钥()
{
	SQLRETURN retcode;
	CString 查询语句 = _T("SELECT TOP 1 pw FROM my ORDER BY [index] DESC");
	
	// 执行SQL查询
	retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)查询语句.GetString(), SQL_NTS);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
		// 获取错误信息
		SQLWCHAR sqlState[6], message[SQL_MAX_MESSAGE_LENGTH];
		SQLINTEGER nativeError;
		SQLSMALLINT msgLen;

		SQLGetDiagRecW(SQL_HANDLE_STMT, SQL语句句柄, 1, sqlState, &nativeError,
			message, SQL_MAX_MESSAGE_LENGTH, &msgLen);

		CString 错误信息;
		错误信息.Format(_T("查询用户密钥失败: %s"), CString(message));
		添加信息显示(错误信息);

		return _T("");
	}
	
	// 获取结果
	retcode = SQLFetch(SQL语句句柄);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		SQLWCHAR 客户端密钥[256], 版本号[256];
		SQLLEN 密钥长度, 版本长度;

		SQLGetData(SQL语句句柄, 1, SQL_C_WCHAR, 客户端密钥, sizeof(客户端密钥), &密钥长度);

		SQLCloseCursor(SQL语句句柄);
		return CString(客户端密钥);
	}
	else if (retcode == SQL_NO_DATA) {
		添加信息显示(_T("未找到客户端密钥"));
	}
	else {
		// 获取错误信息
		SQLWCHAR sqlState[6], message[SQL_MAX_MESSAGE_LENGTH];
		SQLINTEGER nativeError;
		SQLSMALLINT msgLen;

		SQLGetDiagRecW(SQL_HANDLE_STMT, SQL语句句柄, 1, sqlState, &nativeError,
			message, SQL_MAX_MESSAGE_LENGTH, &msgLen);

		CString 错误信息;
		错误信息.Format(_T("获取客户端密钥失败: %s"), CString(message));
		添加信息显示(错误信息);
	}

	SQLCloseCursor(SQL语句句柄);
	return _T("");
}

// 获取最新版本号
CString NageDlqServerDlg::获取最新版本号()
{
	SQLRETURN retcode;
	CString 查询语句 = _T("SELECT TOP 1 v FROM my ORDER BY [index] DESC");  

	// 执行SQL查询
	retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)查询语句.GetString(), SQL_NTS);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
		return _T("1.0.0");
	}

	// 获取结果
	retcode = SQLFetch(SQL语句句柄);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		SQLWCHAR 版本号[256];
		SQLLEN 版本长度;

		SQLGetData(SQL语句句柄, 1, SQL_C_WCHAR, 版本号, sizeof(版本号), &版本长度);

		SQLCloseCursor(SQL语句句柄);
		return CString(版本号);
	}

	SQLCloseCursor(SQL语句句柄);
	return _T("1.0.0");
}

// 更新服务器信息
void NageDlqServerDlg::更新服务器信息()
{
	if (!数据库连接状态)
	{
		添加信息显示(_T("数据库未连接，无法更新信息"));
		return;
	}

	SQLRETURN retcode;
	// 修改：一次性查询密钥和版本号
	CString 查询语句 = _T("SELECT TOP 1 pw, v FROM my ORDER BY [index] DESC");

	// 执行SQL查询
	retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)查询语句.GetString(), SQL_NTS);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
		// 获取详细的错误信息
		SQLWCHAR sqlState[6], message[SQL_MAX_MESSAGE_LENGTH];
		SQLINTEGER nativeError;
		SQLSMALLINT msgLen;

		SQLGetDiagRecW(SQL_HANDLE_STMT, SQL语句句柄, 1, sqlState, &nativeError,
			message, SQL_MAX_MESSAGE_LENGTH, &msgLen);

		CString 错误信息;
		错误信息.Format(_T("查询失败: %s (错误代码: %s)"), CString(message), CString(sqlState));
		添加信息显示(错误信息);

		当前密钥 = _T("查询失败");
		当前版本号 = _T("查询失败");
		更新状态显示();
		return;
	}

	// 获取结果
	retcode = SQLFetch(SQL语句句柄);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		SQLWCHAR 客户端密钥[256] = { 0 };
		SQLWCHAR 版本号[256] = { 0 };
		SQLLEN 密钥长度 = 0, 版本长度 = 0;

		// 清空缓冲区
		memset(客户端密钥, 0, sizeof(客户端密钥));
		memset(版本号, 0, sizeof(版本号));

		SQLGetData(SQL语句句柄, 1, SQL_C_WCHAR, 客户端密钥, sizeof(客户端密钥), &密钥长度);
		SQLGetData(SQL语句句柄, 2, SQL_C_WCHAR, 版本号, sizeof(版本号), &版本长度);

		当前密钥 = CString(客户端密钥);
		当前版本号 = CString(版本号);

		SQLCloseCursor(SQL语句句柄);

		// 添加成功信息
		CString 成功信息;
		成功信息.Format(_T("成功查询到密钥和版本号"));
		添加信息显示(成功信息);
	}
	else {
		// 获取具体的错误信息
		SQLWCHAR sqlState[6], message[SQL_MAX_MESSAGE_LENGTH];
		SQLINTEGER nativeError;
		SQLSMALLINT msgLen;

		SQLGetDiagRecW(SQL_HANDLE_STMT, SQL语句句柄, 1, sqlState, &nativeError,
			message, SQL_MAX_MESSAGE_LENGTH, &msgLen);

		CString 错误信息;
		错误信息.Format(_T("获取数据失败: %s (错误代码: %s)"), CString(message), CString(sqlState));
		添加信息显示(错误信息);

		当前密钥 = _T("获取失败");
		当前版本号 = _T("获取失败");
		SQLCloseCursor(SQL语句句柄);
	}

	更新状态显示();
}

// 加载配置
BOOL NageDlqServerDlg::加载配置()
{
	// 从注册表读取配置
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\NageServer"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		DWORD dwType;
		DWORD dwSize = 256;
		TCHAR szValue[256];
		
		if (RegQueryValueEx(hKey, _T("DBUser"), NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
		{
			数据库用户名 = szValue;
		}
		
		dwSize = 256;
		if (RegQueryValueEx(hKey, _T("DBPassword"), NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
		{
			数据库密码 = szValue;
		}
		
		dwSize = 256;
		if (RegQueryValueEx(hKey, _T("DBName"), NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
		{
			数据库名称 = szValue;
		}
		
		RegCloseKey(hKey);
	}
	
	return TRUE;
}

// 保存配置
BOOL NageDlqServerDlg::保存配置()
{
	// 保存配置到注册表
	HKEY hKey;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\NageServer"), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey, _T("DBUser"), 0, REG_SZ, (const BYTE*)(LPCTSTR)数据库用户名, (数据库用户名.GetLength() + 1) * sizeof(TCHAR));
		RegSetValueEx(hKey, _T("DBPassword"), 0, REG_SZ, (const BYTE*)(LPCTSTR)数据库密码, (数据库密码.GetLength() + 1) * sizeof(TCHAR));
		RegSetValueEx(hKey, _T("DBName"), 0, REG_SZ, (const BYTE*)(LPCTSTR)数据库名称, (数据库名称.GetLength() + 1) * sizeof(TCHAR));
		RegCloseKey(hKey);
		添加信息显示(_T("已保存配置"));
		return TRUE;
	}
	添加信息显示(_T("保存配置失败"));
	return FALSE;
}

// 添加客户端连接
void NageDlqServerDlg::添加客户端连接(SOCKET 客户端套接字, const CString& 客户端IP)
{
	EnterCriticalSection(&客户端列表锁);
	客户端连接列表[客户端套接字] = 客户端IP;
	客户端连接数量 = 客户端连接列表.size();
	LeaveCriticalSection(&客户端列表锁);
	
	更新状态显示();
}

// 移除客户端连接
void NageDlqServerDlg::移除客户端连接(SOCKET 客户端套接字)
{
	EnterCriticalSection(&客户端列表锁);
	auto it = 客户端连接列表.find(客户端套接字);
	if (it != 客户端连接列表.end())
	{
		CString 客户端IP = it->second;
		添加信息显示(客户端IP + _T(" 已断开连接"));
		客户端连接列表.erase(it);
		客户端连接数量 = 客户端连接列表.size();
	}
	LeaveCriticalSection(&客户端列表锁);
	
	更新状态显示();
}

// 获取连接数量
int NageDlqServerDlg::获取连接数量()
{
	return 客户端连接数量;
}

// 添加信息显示
void NageDlqServerDlg::添加信息显示(const CString& 信息)
{
	CString 时间信息 = CTime::GetCurrentTime().Format(_T("%H:%M:%S"));
	CString 完整信息 = 时间信息 + _T(" - ") + 信息;
	
	// 添加到List Box
	CListBox* pListBox = (CListBox*)GetDlgItem(IDC_EDIT_INFO);
	if (pListBox)
	{
		pListBox->AddString(完整信息);
		pListBox->SetCurSel(pListBox->GetCount() - 1); // 滚动到最后
	}
}

// 更新状态显示
void NageDlqServerDlg::更新状态显示()
{
	// 更新权限状态标签
	CString 密钥信息;
	if (当前密钥.IsEmpty())
		密钥信息 = _T("权限状态: 未查询");
	else if (当前密钥 == _T("查询失败") || 当前密钥 == _T("获取失败"))
		密钥信息 = _T("权限状态: 查询失败");
	else if (当前密钥 == _T("无数据"))
		密钥信息 = _T("权限状态: 无数据");
	else
		密钥信息.Format(_T("权限状态: %s"), 当前密钥);

	权限状态标签.SetWindowText(密钥信息);

	// 更新版本号标签
	CString 版本信息;
	if (当前版本号.IsEmpty())
		版本信息 = _T("当前版本号: 未查询");
	else if (当前版本号 == _T("查询失败") || 当前版本号 == _T("获取失败"))
		版本信息 = _T("当前版本号: 查询失败");
	else if (当前版本号 == _T("无数据"))
		版本信息 = _T("当前版本号: 无数据");
	else
		版本信息.Format(_T("当前版本号: %s"), 当前版本号);

	当前版本号标签.SetWindowText(版本信息);

	// 更新连接数量标签
	CString 连接信息;
	连接信息.Format(_T("连接数量: %d"), 客户端连接数量);
	连接数量标签.SetWindowText(连接信息);
}

// 发送到客户端
BOOL NageDlqServerDlg::发送到客户端(SOCKET 客户端套接字, const CString& 数据)
{
	int 数据长度 = 数据.GetLength() * sizeof(TCHAR);
	return send(客户端套接字, (const char*)数据.GetString(), 数据长度, 0) != SOCKET_ERROR;
}

// 从客户端接收
CString NageDlqServerDlg::从客户端接收(SOCKET 客户端套接字)
{
	char 缓冲区[1024];
	int 接收长度 = recv(客户端套接字, 缓冲区, sizeof(缓冲区) - 1, 0);
	if (接收长度 > 0)
	{
		缓冲区[接收长度] = '\0';
		return CString(缓冲区);
	}
	return _T("");
}

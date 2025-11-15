//NageDlqServerDlg.cpp
#include "pch.h"
#include "framework.h"
#include "NageDlqServer.h"
#include "NageDlqServerDlg.h"
#include "afxdialogex.h"
#include "设置对话框类.h"
#include "黑白名单对话框类.h"

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
	, 当前编辑框(nullptr)  
	, 当前编辑项(-1)      
	, 当前编辑列(-1)
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
	
	安全停止端口转发();
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
	DDX_Control(pDX, IDC_BUTTON_BWLIST, 黑白名单按钮);

	// 添加端口转发控件的 DDX
	DDX_Control(pDX, IDC_LIST_FORWARD, 端口转发列表控件);
	DDX_Control(pDX, IDC_BUTTON_START_FORWARD, 启动转发按钮);
	DDX_Control(pDX, IDC_BUTTON_STOP_FORWARD, 停止转发按钮);
}

//消息映射
BEGIN_MESSAGE_MAP(NageDlqServerDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_START, &NageDlqServerDlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &NageDlqServerDlg::OnBnClickedButtonStop)
	ON_BN_CLICKED(IDC_BUTTON_UPDATE_CLIENT, &NageDlqServerDlg::OnBnClickedButtonUpdateClient)
	ON_BN_CLICKED(IDC_BUTTON_UPDATE_HOOK, &NageDlqServerDlg::OnBnClickedButtonUpdateHook)
	ON_BN_CLICKED(IDC_BUTTON_SETTINGS, &NageDlqServerDlg::OnBnClickedButtonSettings)
	ON_BN_CLICKED(IDC_BUTTON_BWLIST, &NageDlqServerDlg::OnBnClickedButtonBwlist)
	ON_BN_CLICKED(IDC_BUTTON_START_FORWARD, &NageDlqServerDlg::On启动转发按钮点击)
	ON_BN_CLICKED(IDC_BUTTON_STOP_FORWARD, &NageDlqServerDlg::On停止转发按钮点击)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_FORWARD, &NageDlqServerDlg::On列表项双击)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST_FORWARD, &NageDlqServerDlg::On列表结束编辑)
	ON_WM_DESTROY()		//销毁处理
	ON_WM_KILLFOCUS(1000, &NageDlqServerDlg::On编辑框失去焦点)      // 添加焦点丢失处理
END_MESSAGE_MAP()

//销毁处理函数
void NageDlqServerDlg::OnDestroy()
{
	if (当前编辑框)
	{
		当前编辑框->DestroyWindow();
		delete 当前编辑框;
		当前编辑框 = nullptr;
	}

	CDialogEx::OnDestroy();
}

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

	加载配置();
	添加信息显示(_T("程序已初始化"));

	初始化端口转发界面();
	加载端口转发配置();
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

//黑白名单按钮点击处理
void NageDlqServerDlg::OnBnClickedButtonBwlist()
{
	TRACE(_T("=== 打开黑白名单管理界面 ===\n"));

	// 创建并显示黑白名单对话框
	黑白名单对话框类 黑白名单对话框;
	黑白名单对话框.DoModal();

	// 对话框关闭后重新加载黑白名单
	黑名单列表.clear();
	白名单列表.clear();
	加载黑白名单();

	TRACE(_T("=== 黑白名单管理界面关闭 ===\n"));
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

	// 设置socket为非阻塞模式
	u_long 非阻塞模式 = 1;
	ioctlsocket(对话框指针->监听套接字, FIONBIO, &非阻塞模式);

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
			// 非阻塞模式下，如果没有连接会立即返回，需要适当延迟
			Sleep(100);
		}
	}

	closesocket(对话框指针->监听套接字);
	return 0;
}

// 客户端线程函数
UINT NageDlqServerDlg::客户端线程函数(LPVOID pParam)
{
	TRACE(_T("=== 客户端线程函数开始 ===\n"));
	SOCKET 客户端套接字 = (SOCKET)pParam;
	NageDlqServerDlg* 对话框指针 = (NageDlqServerDlg*)AfxGetApp()->GetMainWnd();

	if (!对话框指针)
	{
		closesocket(客户端套接字);
		return 1;
	}

	// 设置socket为阻塞模式
	u_long 阻塞模式 = 0;
	ioctlsocket(客户端套接字, FIONBIO, &阻塞模式);

	CString 客户端IP;
	EnterCriticalSection(&对话框指针->客户端列表锁);
	auto it = 对话框指针->客户端连接列表.find(客户端套接字);
	if (it != 对话框指针->客户端连接列表.end())
	{
		客户端IP = it->second;
	}
	LeaveCriticalSection(&对话框指针->客户端列表锁);

	// 持续处理客户端请求
	while (对话框指针->服务器运行状态)
	{
		TRACE(_T("=== chixuchulikehuduanqingqiu ===\n"));
		// 接收客户端请求
		CString 客户端请求 = 对话框指针->从客户端接收(客户端套接字);

		// 检查连接是否关闭或出错
		if (客户端请求.IsEmpty())
		{
			// 检查是否是真正的连接关闭
			char 测试缓冲区[1];
			int 测试结果 = recv(客户端套接字, 测试缓冲区, 1, MSG_PEEK);
			if (测试结果 == 0)
			{
				// 连接已关闭
				对话框指针->添加信息显示(客户端IP + _T(" 连接已关闭"));
				break;
			}
			else if (测试结果 == SOCKET_ERROR)
			{
				// 连接错误
				int 错误码 = WSAGetLastError();
				if (错误码 != WSAEWOULDBLOCK)
				{
					CString 错误信息;
					错误信息.Format(_T(" 连接错误，错误码: %d"), 错误码);
					对话框指针->添加信息显示(客户端IP + 错误信息);
					break;
				}
			}
			// 如果是空数据但不是错误，继续等待
			continue;
		}

		// 记录客户端请求
		if (!客户端IP.IsEmpty())
		{
			CString 完整请求信息;
			完整请求信息.Format(_T(" 请求: [%s], 长度: %d"), 客户端请求, 客户端请求.GetLength());
			//对话框指针->添加信息显示(客户端IP + 完整请求信息);
		}

		// 解析请求
		//连接请求
		TRACE(_T("开始解析请求: %s\n"), 客户端请求);  // 添加这行
		if (客户端请求.Find(_T("CONNECT:")) == 0)
		{
			TRACE(_T("=== 处理连接请求开始 ===\n"));

			// 处理连接验证请求 - 格式: CONNECT:客户端版本号:客户端IP
			CString 连接数据 = 客户端请求.Mid(8); // 去掉"CONNECT:"
			TRACE(_T("连接数据: %s\n"), 连接数据);

			int 分隔符位置 = 连接数据.Find(':');
			if (分隔符位置 != -1)
			{
				CString 客户端版本号 = 连接数据.Left(分隔符位置);
				CString 客户端IP = 连接数据.Mid(分隔符位置 + 1);

				TRACE(_T("客户端版本: %s, IP: %s\n"), 客户端版本号, 客户端IP);

				// 检查IP黑白名单
				if (!对话框指针->检查IP权限(客户端IP))  // 修复：通过指针调用
				{
					TRACE(_T("IP不在白名单或存在于黑名单中\n"));
					对话框指针->发送到客户端(客户端套接字, _T("CONNECT_FAILED:IP访问受限"));
					对话框指针->添加信息显示(客户端IP + _T(" IP访问受限"));
					closesocket(客户端套接字);
					return 0;
				}

				// 查询数据库获取密钥和最新版本号
				CString 客户端密钥 = 对话框指针->获取客户端密钥();
				CString 最新版本号 = 对话框指针->获取最新版本号();

				TRACE(_T("查询到密钥: %s, 版本: %s\n"), 客户端密钥, 最新版本号);

				// 检查客户端版本
				if (对话框指针->比较版本号(客户端版本号, 最新版本号) < 0)  // 修复：通过指针调用
				{
					TRACE(_T("客户端版本过时\n"));
					对话框指针->发送到客户端(客户端套接字, _T("VERSION_OUTDATED"));
					对话框指针->添加信息显示(客户端IP + _T(" 版本过时，已断开连接"));
					closesocket(客户端套接字);
					return 0;
				}

				// 发送响应 - 确保包含密钥和版本号
				CString 响应数据;
				if (!客户端密钥.IsEmpty() && !最新版本号.IsEmpty())
				{
					响应数据.Format(_T("CONNECT_SUCCESS:%s:%s"), 客户端密钥, 最新版本号);
					对话框指针->添加信息显示(客户端IP + _T(" 连接验证成功"));
					TRACE(_T("发送带密钥的连接成功响应: %s\n"), 响应数据);
				}
				else
				{
					响应数据 = _T("CONNECT_FAILED:服务端配置错误");
					对话框指针->添加信息显示(客户端IP + _T(" 连接验证失败"));
					TRACE(_T("发送连接失败响应\n"));
				}

				// 发送响应
				BOOL 发送结果 = 对话框指针->发送到客户端(客户端套接字, 响应数据);
				TRACE(_T("发送响应结果: %d\n"), 发送结果);
			}
			else
			{
				TRACE(_T("连接数据格式错误\n"));
				对话框指针->发送到客户端(客户端套接字, _T("CONNECT_FAILED:无效的连接数据格式"));
				对话框指针->添加信息显示(客户端IP + _T(" 连接数据格式错误"));
			}
			TRACE(_T("=== 处理连接请求结束 ===\n"));
		}
		//登录请求
		else if (客户端请求.Find(_T("LOGIN:")) == 0)
		{
			TRACE(_T("=== 登录请求开始 ===\n"));
			// 先记录请求，再处理
			if (!客户端IP.IsEmpty())
			{
				CString 完整请求信息;
				完整请求信息.Format(_T(" 请求: [%s], 长度: %d"), 客户端请求, 客户端请求.GetLength());
				//对话框指针->添加信息显示(客户端IP + 完整请求信息);
			}

			// 然后处理登录请求
			CString 登录数据 = 客户端请求.Mid(6);
			登录数据.TrimRight(_T("\r\n")); // 去掉末尾的换行符和回车符
			TRACE(_T("处理后的登录数据: %s\n"), 登录数据);

			CStringArray 参数数组;
			int 起始位置 = 0;
			CString 参数 = 登录数据.Tokenize(_T(":"), 起始位置);

			while (!参数.IsEmpty())
			{
				参数数组.Add(参数);
				参数 = 登录数据.Tokenize(_T(":"), 起始位置);
			}

			if (参数数组.GetSize() == 2)
			{
				CString 用户名 = 参数数组[0];
				CString 密码 = 参数数组[1];

				TRACE(_T("解析参数 - 用户: %s, 密码: %s\n"), 用户名, 密码);

				// 验证用户名和密码
				BOOL 登录结果 = 对话框指针->验证用户登录(用户名, 密码);
				TRACE(_T("登录验证结果: %d\n"), 登录结果);

				if (登录结果)
				{
					对话框指针->发送到客户端(客户端套接字, _T("LOGIN_SUCCESS:登录成功"));
					TRACE(_T("=== 登录成功 ===\n"));
					// 成功日志放在最后
					对话框指针->添加信息显示(客户端IP + _T(" 登录成功 - 用户名: ") + 用户名);
				}
				else
				{
					对话框指针->发送到客户端(客户端套接字, _T("LOGIN_FAILED:用户名或密码错误"));
					TRACE(_T("=== 账号密码错误 ===\n"));
					// 失败日志放在最后
					对话框指针->添加信息显示(客户端IP + _T(" 登录失败 - 用户名: ") + 用户名);
				}
			}
			else
			{
				对话框指针->发送到客户端(客户端套接字, _T("LOGIN_FAILED:无效的登录数据格式"));
				对话框指针->添加信息显示(客户端IP + _T(" 登录数据格式错误"));
				TRACE(_T("=== 登录失败 ===\n"));
			}
		}
		//注册请求
		else if (客户端请求.Find(_T("REGISTER:")) == 0)
		{
			TRACE(_T("=== 注册请求开始 ===\n"));
			// 处理注册请求 - 格式: REGISTER:username:password:email
			CString 注册数据 = 客户端请求.Mid(9); // 去掉"REGISTER:"
			TRACE(_T("注册数据: %s\n"), 注册数据);

			// 只保留请求日志
			if (!客户端IP.IsEmpty())
			{
				CString 完整请求信息;
				完整请求信息.Format(_T(" 请求: [%s], 长度: %d"), 客户端请求, 客户端请求.GetLength());
				对话框指针->添加信息显示(客户端IP + 完整请求信息);
			}

			CStringArray 参数数组;
			int 起始位置 = 0;
			CString 参数 = 注册数据.Tokenize(_T(":"), 起始位置);

			while (!参数.IsEmpty())
			{
				参数数组.Add(参数);
				参数 = 注册数据.Tokenize(_T(":"), 起始位置);
			}

			if (参数数组.GetSize() >= 3)
			{
				CString 用户名 = 参数数组[0];
				CString 密码 = 参数数组[1];
				CString 邮箱 = 参数数组[2];

				TRACE(_T("解析参数 - 用户: %s, 密码: %s, 邮箱: %s\n"), 用户名, 密码, 邮箱);

				// 处理注册
				BOOL 注册结果 = 对话框指针->处理用户注册(用户名, 密码, 邮箱);
				TRACE(_T("注册处理结果: %d\n"), 注册结果);

				if (注册结果)
				{
					CString 响应数据 = _T("REGISTER_SUCCESS:注册成功");
					TRACE(_T("发送注册成功响应: %s\n"), 响应数据);
					BOOL 发送结果 = 对话框指针->发送到客户端(客户端套接字, 响应数据);
					TRACE(_T("发送响应结果: %d\n"), 发送结果);
					// 只保留最终结果日志
					对话框指针->添加信息显示(客户端IP + _T(" 注册成功 - 用户名: ") + 用户名);
				}
				else
				{
					CString 响应数据 = _T("REGISTER_FAILED:注册失败");
					TRACE(_T("发送注册失败响应: %s\n"), 响应数据);
					BOOL 发送结果 = 对话框指针->发送到客户端(客户端套接字, 响应数据);
					TRACE(_T("发送响应结果: %d\n"), 发送结果);
					// 只保留最终结果日志
					对话框指针->添加信息显示(客户端IP + _T(" 注册失败 - 用户名: ") + 用户名);
				}
			}
			else
			{
				CString 错误信息;
				错误信息.Format(_T("注册数据格式错误，参数数量: %d"), 参数数组.GetSize());
				TRACE(_T("注册数据格式错误: %s\n"), 错误信息);
				CString 响应数据 = _T("REGISTER_FAILED:") + 错误信息;
				BOOL 发送结果 = 对话框指针->发送到客户端(客户端套接字, 响应数据);
				TRACE(_T("发送错误响应结果: %d\n"), 发送结果);
				// 只保留最终结果日志
				对话框指针->添加信息显示(客户端IP + _T(" ") + 错误信息);
			}
			TRACE(_T("=== 处理注册请求结束 ===\n"));
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
		else if (客户端请求.Find(_T("REBORN:")) == 0)
		{
			// 处理转生请求 - 格式: REBORN:username:charname
			CString 转生数据 = 客户端请求.Mid(7); // 去掉"REBORN:"
			int 分隔符 = 转生数据.Find(':');

			if (分隔符 != -1)
			{
				CString 用户名 = 转生数据.Left(分隔符);
				CString 角色名 = 转生数据.Mid(分隔符 + 1);

				BOOL 转生结果 = 对话框指针->处理角色转生(用户名, 角色名);

				if (转生结果)
				{
					对话框指针->发送到客户端(客户端套接字, _T("REBORN_SUCCESS:转生成功"));
					对话框指针->添加信息显示(客户端IP + _T(" 角色转生成功: ") + 角色名);
				}
				else
				{
					对话框指针->发送到客户端(客户端套接字, _T("REBORN_FAILED:转生失败"));
					对话框指针->添加信息显示(客户端IP + _T(" 角色转生失败: ") + 角色名);
				}
			}
			else
			{
				对话框指针->发送到客户端(客户端套接字, _T("REBORN_FAILED:无效的转生数据格式"));
				对话框指针->添加信息显示(客户端IP + _T(" 转生数据格式错误"));
			}
		}
		else if (客户端请求.Find(_T("ADD_POINTS:")) == 0)
		{
			// 处理加点请求 - 格式: ADD_POINTS:username:charname:str:dex:esp:spt
			CString 加点数据 = 客户端请求.Mid(11); // 去掉"ADD_POINTS:"

			CStringArray 参数数组;
			int 起始位置 = 0;
			CString 临时字符串 = 加点数据.Tokenize(_T(":"), 起始位置);
			while (!临时字符串.IsEmpty())
			{
				参数数组.Add(临时字符串);
				临时字符串 = 加点数据.Tokenize(_T(":"), 起始位置);
			}

			if (参数数组.GetSize() == 6)
			{
				CString 用户名 = 参数数组[0];
				CString 角色名 = 参数数组[1];
				int 力量 = _ttoi(参数数组[2]);
				int 敏捷 = _ttoi(参数数组[3]);
				int 意念 = _ttoi(参数数组[4]);
				int 灵力 = _ttoi(参数数组[5]);

				BOOL 加点结果 = 对话框指针->处理角色加点(用户名, 角色名, 力量, 敏捷, 意念, 灵力);

				if (加点结果)
				{
					对话框指针->发送到客户端(客户端套接字, _T("ADD_POINTS_SUCCESS:加点成功"));
					对话框指针->添加信息显示(客户端IP + _T(" 角色加点成功: ") + 角色名);
				}
				else
				{
					对话框指针->发送到客户端(客户端套接字, _T("ADD_POINTS_FAILED:加点失败"));
					对话框指针->添加信息显示(客户端IP + _T(" 角色加点失败: ") + 角色名);
				}
			}
			else
			{
				对话框指针->发送到客户端(客户端套接字, _T("ADD_POINTS_FAILED:无效的加点数据格式"));
				对话框指针->添加信息显示(客户端IP + _T(" 加点数据格式错误"));
			}
		}
		else if (客户端请求.Find(_T("GET_CHAR_INFO:")) == 0)
		{
			// 获取角色信息 - 格式: GET_CHAR_INFO:username:charname
			CString 查询数据 = 客户端请求.Mid(14); // 去掉"GET_CHAR_INFO:"
			int 分隔符 = 查询数据.Find(':');

			if (分隔符 != -1)
			{
				CString 用户名 = 查询数据.Left(分隔符);
				CString 角色名 = 查询数据.Mid(分隔符 + 1);

				// 查询角色信息
				CString 查询语句;
				查询语句.Format(_T("SELECT baseskill, Lv, lv + relvC AS total_lv, lvpoint, Str, Dex, Esp, Spt FROM CharInfo WHERE charname = '%s'"), 角色名);

				SQLRETURN retcode = SQLExecDirectW(对话框指针->SQL语句句柄, (SQLWCHAR*)查询语句.GetString(), SQL_NTS);
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
				{
					retcode = SQLFetch(对话框指针->SQL语句句柄);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
					{
						SQLINTEGER 职业代码, 战斗等级, 累计等级, 剩余点数, 力量, 敏捷, 意念, 灵力;

						SQLGetData(对话框指针->SQL语句句柄, 1, SQL_C_LONG, &职业代码, sizeof(职业代码), NULL);
						SQLGetData(对话框指针->SQL语句句柄, 2, SQL_C_LONG, &战斗等级, sizeof(战斗等级), NULL);
						SQLGetData(对话框指针->SQL语句句柄, 3, SQL_C_LONG, &累计等级, sizeof(累计等级), NULL);
						SQLGetData(对话框指针->SQL语句句柄, 4, SQL_C_LONG, &剩余点数, sizeof(剩余点数), NULL);
						SQLGetData(对话框指针->SQL语句句柄, 5, SQL_C_LONG, &力量, sizeof(力量), NULL);
						SQLGetData(对话框指针->SQL语句句柄, 6, SQL_C_LONG, &敏捷, sizeof(敏捷), NULL);
						SQLGetData(对话框指针->SQL语句句柄, 7, SQL_C_LONG, &意念, sizeof(意念), NULL);
						SQLGetData(对话框指针->SQL语句句柄, 8, SQL_C_LONG, &灵力, sizeof(灵力), NULL);

						CString 响应数据;
						响应数据.Format(_T("CHAR_INFO:%d:%d:%d:%d:%d:%d:%d:%d"),
							职业代码, 战斗等级, 累计等级, 剩余点数, 力量, 敏捷, 意念, 灵力);

						对话框指针->发送到客户端(客户端套接字, 响应数据);
						对话框指针->添加信息显示(客户端IP + _T(" 查询角色信息: ") + 角色名);
					}
					else
					{
						对话框指针->发送到客户端(客户端套接字, _T("CHAR_INFO_FAILED:角色不存在"));
					}
					SQLCloseCursor(对话框指针->SQL语句句柄);
				}
				else
				{
					对话框指针->发送到客户端(客户端套接字, _T("CHAR_INFO_FAILED:查询失败"));
				}
			}
		}
		else
		{
			TRACE(_T("=== weizhiqingqiu ===\n"));
			// 未知请求
			对话框指针->发送到客户端(客户端套接字, _T("UNKNOWN_COMMAND"));
			对话框指针->添加信息显示(客户端IP + _T(" 未知请求: ") + 客户端请求);
		}
	}

	// 只有在连接出错或服务器停止时才关闭连接
	对话框指针->移除客户端连接(客户端套接字);
	closesocket(客户端套接字);

	return 0;
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
	SQLWCHAR sqlState[6];
	SQLWCHAR message[SQL_MAX_MESSAGE_LENGTH];

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
		// 错误处理代码应该在这里
		SQLINTEGER nativeError;
		SQLSMALLINT msgLen;
		SQLGetDiagRecW(SQL_HANDLE_DBC, SQL连接句柄, 1, sqlState, &nativeError,
			message, SQL_MAX_MESSAGE_LENGTH, &msgLen);

		CString 错误信息;
		错误信息.Format(_T("数据库连接失败: %s (错误代码: %s)"), CString(message), CString(sqlState));
		添加信息显示(错误信息);

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
		return _T(""); // 返回空字符串表示失败
	}
	
	// 获取结果
	retcode = SQLFetch(SQL语句句柄);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		SQLWCHAR 客户端密钥[256];
		SQLLEN 密钥长度;

		SQLGetData(SQL语句句柄, 1, SQL_C_WCHAR, 客户端密钥, sizeof(客户端密钥), &密钥长度);

		SQLCloseCursor(SQL语句句柄);
		return CString(客户端密钥); // 返回实际的密钥值，如"chenge"
		TRACE(_T("获取客户端密钥: %s\n"), 客户端密钥);
	}

	SQLCloseCursor(SQL语句句柄);
	return _T(""); // 返回空字符串表示失败
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
	// 转换为UTF-8
	int 字节长度 = WideCharToMultiByte(CP_UTF8, 0, 数据, -1, NULL, 0, NULL, NULL);
	if (字节长度 > 0)
	{
		char* 字节缓冲区 = new char[字节长度];
		WideCharToMultiByte(CP_UTF8, 0, 数据, -1, 字节缓冲区, 字节长度, NULL, NULL);

		int 发送结果 = send(客户端套接字, 字节缓冲区, 字节长度 - 1, 0);  // -1 去掉null终止符

		delete[] 字节缓冲区;
		return 发送结果 != SOCKET_ERROR;
	}
	return FALSE;
}

// 从客户端接收
CString NageDlqServerDlg::从客户端接收(SOCKET 客户端套接字)
{
	char 缓冲区[4096];
	memset(缓冲区, 0, sizeof(缓冲区));

	// 移除超时设置，使用阻塞模式正常接收
	// struct timeval 超时;
	// 超时.tv_sec = 5;
	// 超时.tv_usec = 0;
	// setsockopt(客户端套接字, SOL_SOCKET, SO_RCVTIMEO, (char*)&超时, sizeof(超时));

	int 接收长度 = recv(客户端套接字, 缓冲区, sizeof(缓冲区) - 1, 0);

	if (接收长度 > 0)
	{
		缓冲区[接收长度] = '\0';

		// 调试信息
		TRACE(_T("接收到的原始数据(长度%d): "), 接收长度);
		for (int i = 0; i < 接收长度; i++) {
			TRACE(_T("%02x "), (unsigned char)缓冲区[i]);
		}
		TRACE(_T("\n"));
		TRACE(_T("接收到的文本: %hs\n"), 缓冲区);

		// 尝试UTF-8转换
		int 宽字符长度 = MultiByteToWideChar(CP_UTF8, 0, 缓冲区, 接收长度, NULL, 0);
		if (宽字符长度 > 0)
		{
			wchar_t* 宽字符缓冲区 = new wchar_t[宽字符长度 + 1];
			MultiByteToWideChar(CP_UTF8, 0, 缓冲区, 接收长度, 宽字符缓冲区, 宽字符长度);
			宽字符缓冲区[宽字符长度] = L'\0';

			CString 结果(宽字符缓冲区);
			delete[] 宽字符缓冲区;

			TRACE(_T("UTF-8转换后的数据: %s\n"), 结果);
			return 结果;
		}
		else
		{
			// 如果UTF-8转换失败，尝试ANSI
			CString 结果(缓冲区);
			TRACE(_T("UTF-8转换失败，使用ANSI: %s\n"), 结果);
			return 结果;
		}
	}
	else if (接收长度 == 0)
	{
		TRACE(_T("客户端正常关闭连接\n"));
		return _T("");
	}
	else
	{
		int 错误码 = WSAGetLastError();
		TRACE(_T("接收数据错误，错误码: %d\n"), 错误码);

		// 如果是阻塞操作被中断，继续等待
		if (错误码 == WSAEWOULDBLOCK) {
			return _T(""); // 返回空但不视为错误
		}

		return _T(""); // 其他错误返回空
	}
}

// 处理用户注册函数
BOOL NageDlqServerDlg::处理用户注册(const CString& 用户名, const CString& 密码, const CString& 邮箱)
{
	// 验证用户名规则：只能包含字母和数字，最长12位
	if (用户名.GetLength() > 12 || 用户名.IsEmpty())
	{
		return FALSE;
	}

	for (int i = 0; i < 用户名.GetLength(); i++)
	{
		TCHAR c = 用户名[i];
		if (!((c >= _T('a') && c <= _T('z')) ||
			(c >= _T('A') && c <= _T('Z')) ||
			(c >= _T('0') && c <= _T('9'))))
		{
			return FALSE;
		}
	}

	// 验证密码长度
	if (密码.GetLength() > 12 || 密码.IsEmpty())
	{
		return FALSE;
	}

	SQLRETURN retcode;

	try
	{
		// 检查账号是否已存在
		CString 检查语句;
		检查语句.Format(_T("SELECT COUNT(*) FROM Chr_Log_Info WHERE id_loginid = '%s'"), 用户名);

		retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)检查语句.GetString(), SQL_NTS);
		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			SQLCloseCursor(SQL语句句柄);
			return FALSE;
		}

		retcode = SQLFetch(SQL语句句柄);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			SQLINTEGER 数量;
			SQLGetData(SQL语句句柄, 1, SQL_C_LONG, &数量, sizeof(数量), NULL);

			if (数量 > 0)
			{
				SQLCloseCursor(SQL语句句柄);
				return FALSE;
			}
		}
		SQLCloseCursor(SQL语句句柄);

		// 获取最大的id_idx
		CString 最大ID语句 = _T("SELECT MAX(id_idx) FROM Chr_Log_Info");
		retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)最大ID语句.GetString(), SQL_NTS);

		int 最大ID = 0;
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			retcode = SQLFetch(SQL语句句柄);
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				SQLINTEGER 当前最大ID;
				SQLGetData(SQL语句句柄, 1, SQL_C_LONG, &当前最大ID, sizeof(当前最大ID), NULL);
				最大ID = 当前最大ID;
			}
		}
		SQLCloseCursor(SQL语句句柄);

		// 计算新的propid
		int 新的propid = 1000 + 最大ID + 1;

		// 插入新用户
		CString 插入语句;
		插入语句.Format(_T("INSERT INTO Chr_Log_Info (id_loginid, id_passwd, propid, id_mail) VALUES ('%s', '%s', %d, '%s')"),
			用户名, 密码, 新的propid, 邮箱);

		retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)插入语句.GetString(), SQL_NTS);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			// 提交事务
			SQLEndTran(SQL_HANDLE_DBC, SQL连接句柄, SQL_COMMIT);
			return TRUE;
		}
		else
		{
			// 获取错误信息
			SQLWCHAR sqlState[6], message[SQL_MAX_MESSAGE_LENGTH];
			SQLINTEGER nativeError;
			SQLSMALLINT msgLen;

			SQLGetDiagRecW(SQL_HANDLE_STMT, SQL语句句柄, 1, sqlState, &nativeError,
				message, SQL_MAX_MESSAGE_LENGTH, &msgLen);

			CString 错误信息;
			错误信息.Format(_T("注册失败: %s"), CString(message));

			// 回滚事务
			SQLEndTran(SQL_HANDLE_DBC, SQL连接句柄, SQL_ROLLBACK);
			return FALSE;
		}
	}
	catch (...)
	{
		return FALSE;
	}
}

// 检测账号是否在线
BOOL NageDlqServerDlg::检测账号是否在线(const CString& 用户名)
{
	SQLRETURN retcode;
	CString 查询语句;
	查询语句.Format(_T("SELECT COUNT(*) FROM Chr_Log_Info WHERE id_loginid = '%s' AND online = 1"), 用户名);

	retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)查询语句.GetString(), SQL_NTS);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		retcode = SQLFetch(SQL语句句柄);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			SQLINTEGER 在线数量;
			SQLGetData(SQL语句句柄, 1, SQL_C_LONG, &在线数量, sizeof(在线数量), NULL);
			SQLCloseCursor(SQL语句句柄);
			return 在线数量 > 0;
		}
		SQLCloseCursor(SQL语句句柄);
	}

	return FALSE;
}

// 获取职业初始属性
void NageDlqServerDlg::获取职业初始属性(int 职业代码, int 累计等级, int& Lv, int& Exp, int& HP, int& SP, int& STM,
	int& Str, int& Dex, int& Esp, int& Spt, int& cmap, int& lvpoint, int& relvC)
{
	// 设置默认值
	Lv = 1;
	Exp = 100;
	cmap = 1;
	Str = 0;
	Dex = 0;
	Esp = 0;
	Spt = 0;

	// 根据职业设置初始属性
	switch (职业代码)
	{
	case 6:  // 超能
		HP = 33;
		SP = 88;
		STM = 33;
		Esp = 23;
		Spt = 22;
		break;
	case 7:  // 枪手
		HP = 44;
		SP = 44;
		STM = 33;
		Esp = 23;
		Spt = 22;
		break;
	case 0:  // 格斗
	case 2:  // 舞械
		HP = 46;
		SP = 33;
		STM = 51;
		Str = 23;
		Dex = 22;
		break;
	default:
		HP = 0;
		SP = 0;
		STM = 0;
		break;
	}

	// 计算点数
	lvpoint = 累计等级 * 3 - 3;
	relvC = 累计等级;
}

// 处理角色转生
BOOL NageDlqServerDlg::处理角色转生(const CString& 用户名, const CString& 角色名)
{
	// 检测账号是否在线
	if (检测账号是否在线(用户名))
	{
		添加信息显示(_T("转生失败: 账号在线 - ") + 用户名);
		return FALSE;
	}

	SQLRETURN retcode;

	try
	{
		// 查询角色信息
		CString 查询语句;
		查询语句.Format(_T("SELECT Lv, baseskill, relvCtime, lv + relvC AS total_lv, recount FROM CharInfo WHERE charname = '%s'"), 角色名);

		retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)查询语句.GetString(), SQL_NTS);
		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			添加信息显示(_T("转生失败: 查询角色信息错误"));
			SQLCloseCursor(SQL语句句柄);
			return FALSE;
		}

		retcode = SQLFetch(SQL语句句柄);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			SQLINTEGER 当前等级, 职业代码, 累计等级, 转生次数;
			TIMESTAMP_STRUCT 上次转生时间;
			SQLLEN 时间指示器;

			SQLGetData(SQL语句句柄, 1, SQL_C_LONG, &当前等级, sizeof(当前等级), NULL);
			SQLGetData(SQL语句句柄, 2, SQL_C_LONG, &职业代码, sizeof(职业代码), NULL);
			SQLGetData(SQL语句句柄, 3, SQL_C_TYPE_TIMESTAMP, &上次转生时间, sizeof(上次转生时间), &时间指示器);
			SQLGetData(SQL语句句柄, 4, SQL_C_LONG, &累计等级, sizeof(累计等级), NULL);
			SQLGetData(SQL语句句柄, 5, SQL_C_LONG, &转生次数, sizeof(转生次数), NULL);

			SQLCloseCursor(SQL语句句柄);

			// 检查等级
			if (当前等级 < 130)
			{
				添加信息显示(_T("转生失败: 等级不足130"));
				return FALSE;
			}

			// 检查转生时间（7天限制）
			if (时间指示器 != SQL_NULL_DATA)
			{
				SYSTEMTIME 系统时间;
				GetLocalTime(&系统时间);

				// 计算天数差
				FILETIME 当前文件时间, 上次文件时间;
				SystemTimeToFileTime(&系统时间, &当前文件时间);

				SYSTEMTIME 上次系统时间 = { 0 };
				上次系统时间.wYear = 上次转生时间.year;
				上次系统时间.wMonth = 上次转生时间.month;
				上次系统时间.wDay = 上次转生时间.day;
				上次系统时间.wHour = 上次转生时间.hour;
				上次系统时间.wMinute = 上次转生时间.minute;
				上次系统时间.wSecond = 上次转生时间.second;

				SystemTimeToFileTime(&上次系统时间, &上次文件时间);

				ULARGE_INTEGER 当前时间值, 上次时间值;
				当前时间值.LowPart = 当前文件时间.dwLowDateTime;
				当前时间值.HighPart = 当前文件时间.dwHighDateTime;
				上次时间值.LowPart = 上次文件时间.dwLowDateTime;
				上次时间值.HighPart = 上次文件时间.dwHighDateTime;

				ULONGLONG 时间差 = 当前时间值.QuadPart - 上次时间值.QuadPart;
				int 天数差 = (int)(时间差 / 10000000 / 60 / 60 / 24);

				if (天数差 < 7)
				{
					添加信息显示(_T("转生失败: 距离上次转生不足7天"));
					return FALSE;
				}
			}

			// 获取初始属性
			int Lv, Exp, HP, SP, STM, Str, Dex, Esp, Spt, cmap, lvpoint, relvC;
			获取职业初始属性(职业代码, 累计等级, Lv, Exp, HP, SP, STM, Str, Dex, Esp, Spt, cmap, lvpoint, relvC);

			// 更新角色数据
			CString 更新语句;
			更新语句.Format(_T("UPDATE CharInfo SET Lv = %d, Exp = %d, HP = %d, SP = %d, STM = %d, ")
				_T("Str = %d, Dex = %d, Esp = %d, Spt = %d, cmap = %d, lvpoint = %d, ")
				_T("relvC = %d, relvCtime = GETDATE(), recount = %d WHERE charname = '%s'"),
				Lv, Exp, HP, SP, STM, Str, Dex, Esp, Spt, cmap, lvpoint, relvC, 转生次数 + 1, 角色名);

			retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)更新语句.GetString(), SQL_NTS);
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				SQLEndTran(SQL_HANDLE_DBC, SQL连接句柄, SQL_COMMIT);
				添加信息显示(_T("转生成功: ") + 角色名);
				return TRUE;
			}
			else
			{
				SQLEndTran(SQL_HANDLE_DBC, SQL连接句柄, SQL_ROLLBACK);
				添加信息显示(_T("转生失败: 更新数据库错误"));
				return FALSE;
			}
		}
		else
		{
			SQLCloseCursor(SQL语句句柄);
			添加信息显示(_T("转生失败: 角色不存在"));
			return FALSE;
		}
	}
	catch (...)
	{
		添加信息显示(_T("转生失败: 发生未知错误"));
		return FALSE;
	}
}

// 处理角色加点
BOOL NageDlqServerDlg::处理角色加点(const CString& 用户名, const CString& 角色名, int 力量, int 敏捷, int 意念, int 灵力)
{
	// 检测账号是否在线
	if (检测账号是否在线(用户名))
	{
		添加信息显示(_T("加点失败: 账号在线 - ") + 用户名);
		return FALSE;
	}

	SQLRETURN retcode;

	try
	{
		// 查询角色的剩余点数
		CString 查询语句;
		查询语句.Format(_T("SELECT lvpoint, baseskill FROM CharInfo WHERE charname = '%s'"), 角色名);

		retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)查询语句.GetString(), SQL_NTS);
		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			添加信息显示(_T("加点失败: 查询角色信息错误"));
			SQLCloseCursor(SQL语句句柄);
			return FALSE;
		}

		retcode = SQLFetch(SQL语句句柄);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			SQLINTEGER 剩余点数, 职业代码;
			SQLGetData(SQL语句句柄, 1, SQL_C_LONG, &剩余点数, sizeof(剩余点数), NULL);
			SQLGetData(SQL语句句柄, 2, SQL_C_LONG, &职业代码, sizeof(职业代码), NULL);
			SQLCloseCursor(SQL语句句柄);

			// 计算总加点数
			int 总点数 = 力量 + 敏捷 + 意念 + 灵力;

			if (总点数 > 剩余点数)
			{
				添加信息显示(_T("加点失败: 点数不足"));
				return FALSE;
			}

			// 根据职业验证加点合法性
			if (职业代码 == 0 || 职业代码 == 2)  // 格斗和舞械
			{
				if (意念 != 0 || 灵力 != 0)
				{
					添加信息显示(_T("加点失败: 该职业不能加意念和灵力"));
					return FALSE;
				}
			}
			else if (职业代码 == 6 || 职业代码 == 7)  // 超能和枪手
			{
				if (力量 != 0)
				{
					添加信息显示(_T("加点失败: 该职业不能加力量"));
					return FALSE;
				}
			}

			// 更新角色属性
			CString 更新语句;
			更新语句.Format(_T("UPDATE CharInfo SET Str = Str + %d, Dex = Dex + %d, Esp = Esp + %d, Spt = Spt + %d, lvpoint = lvpoint - %d WHERE charname = '%s'"),
				力量, 敏捷, 意念, 灵力, 总点数, 角色名);

			retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)更新语句.GetString(), SQL_NTS);
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				SQLEndTran(SQL_HANDLE_DBC, SQL连接句柄, SQL_COMMIT);
				添加信息显示(_T("加点成功: ") + 角色名);
				return TRUE;
			}
			else
			{
				SQLEndTran(SQL_HANDLE_DBC, SQL连接句柄, SQL_ROLLBACK);
				添加信息显示(_T("加点失败: 更新数据库错误"));
				return FALSE;
			}
		}
		else
		{
			SQLCloseCursor(SQL语句句柄);
			添加信息显示(_T("加点失败: 角色不存在"));
			return FALSE;
		}
	}
	catch (...)
	{
		添加信息显示(_T("加点失败: 发生未知错误"));
		return FALSE;
	}
}

// 验证用户登录
BOOL NageDlqServerDlg::验证用户登录(const CString& 用户名, const CString& 密码)
{
	TRACE(_T("=== 验证用户登录开始 ===\n"));
	TRACE(_T("验证用户: %s, 密码: %s\n"), 用户名, 密码);

	SQLRETURN retcode;

	// 先检查用户是否存在
	CString 检查用户语句;
	检查用户语句.Format(_T("SELECT COUNT(*) FROM Chr_Log_Info WHERE id_loginid = '%s'"), 用户名);

	TRACE(_T("执行检查用户SQL: %s\n"), 检查用户语句);

	retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)检查用户语句.GetString(), SQL_NTS);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
		TRACE(_T("检查用户数据库查询错误\n"));
		SQLCloseCursor(SQL语句句柄);
		return FALSE;
	}

	SQLINTEGER 用户数量 = 0;
	retcode = SQLFetch(SQL语句句柄);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		SQLGetData(SQL语句句柄, 1, SQL_C_LONG, &用户数量, sizeof(用户数量), NULL);
		TRACE(_T("用户数量: %d\n"), 用户数量);
	}
	SQLCloseCursor(SQL语句句柄);

	if (用户数量 == 0)
	{
		TRACE(_T("用户不存在\n"));
		return FALSE;
	}

	// 验证用户名和密码
	CString 验证语句;
	验证语句.Format(_T("SELECT COUNT(*) FROM Chr_Log_Info WHERE id_loginid = '%s' AND id_passwd = '%s'"),
		用户名, 密码);

	TRACE(_T("执行验证SQL: %s\n"), 验证语句);

	retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)验证语句.GetString(), SQL_NTS);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
		TRACE(_T("验证数据库查询错误\n"));
		SQLCloseCursor(SQL语句句柄);
		return FALSE;
	}

	SQLINTEGER 验证数量 = 0;
	retcode = SQLFetch(SQL语句句柄);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		SQLGetData(SQL语句句柄, 1, SQL_C_LONG, &验证数量, sizeof(验证数量), NULL);
		TRACE(_T("验证结果数量: %d\n"), 验证数量);
	}
	SQLCloseCursor(SQL语句句柄);

	BOOL 结果 = (验证数量 > 0);
	TRACE(_T("最终验证结果: %d\n"), 结果);
	TRACE(_T("=== 验证用户登录结束 ===\n"));

	return 结果;
}

//版本比较函数
int NageDlqServerDlg::比较版本号(const CString& 版本1, const CString& 版本2)
{
	CStringArray 版本1数组, 版本2数组;

	// 分割版本号
	int 位置 = 0;
	CString 部分 = 版本1.Tokenize(_T("."), 位置);
	while (!部分.IsEmpty())
	{
		版本1数组.Add(部分);
		部分 = 版本1.Tokenize(_T("."), 位置);
	}

	位置 = 0;
	部分 = 版本2.Tokenize(_T("."), 位置);
	while (!部分.IsEmpty())
	{
		版本2数组.Add(部分);
		部分 = 版本2.Tokenize(_T("."), 位置);
	}

	// 比较每个部分
	int 最大长度 = max(版本1数组.GetSize(), 版本2数组.GetSize());
	for (int i = 0; i < 最大长度; i++)
	{
		int 数字1 = (i < 版本1数组.GetSize()) ? _ttoi(版本1数组[i]) : 0;
		int 数字2 = (i < 版本2数组.GetSize()) ? _ttoi(版本2数组[i]) : 0;

		if (数字1 < 数字2) return -1;
		if (数字1 > 数字2) return 1;
	}

	return 0; // 版本相同
}

//IP权限检查函数
BOOL NageDlqServerDlg::检查IP权限(const CString& IP地址)
{
	TRACE(_T("=== 检查IP权限开始 ===\n"));
	TRACE(_T("检查IP: %s\n"), IP地址);

	// 从注册表或文件加载黑白名单
	加载黑白名单();

	// 先检查黑名单
	for (const auto& 黑名单IP : 黑名单列表)
	{
		if (黑名单IP == IP地址)
		{
			TRACE(_T("IP在黑名单中: %s\n"), IP地址);
			return FALSE;
		}
	}

	// 如果白名单不为空，检查白名单
	if (!白名单列表.empty())
	{
		BOOL 在白名单中 = FALSE;
		for (const auto& 白名单IP : 白名单列表)
		{
			if (白名单IP == IP地址)
			{
				在白名单中 = TRUE;
				break;
			}
		}

		if (!在白名单中)
		{
			TRACE(_T("IP不在白名单中: %s\n"), IP地址);
			return FALSE;
		}
	}
	else
	{
		TRACE(_T("白名单为空，允许所有非黑名单IP访问\n"));
	}

	TRACE(_T("IP允许访问: %s\n"), IP地址);
	return TRUE;
}

//加载黑白名单
void NageDlqServerDlg::加载黑白名单()
{
	// 如果已经加载过，直接返回
	static BOOL 已加载 = FALSE;
	if (已加载) return;

	黑名单列表.clear();
	白名单列表.clear();

	// 从注册表加载黑白名单
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\NageServer\\IPLists"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		DWORD dwType, dwSize;
		TCHAR szValue[4096];

		// 加载黑名单
		dwSize = sizeof(szValue);
		if (RegQueryValueEx(hKey, _T("BlackList"), NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
		{
			CString 黑名单数据(szValue);
			int 位置 = 0;
			CString IP = 黑名单数据.Tokenize(_T(";"), 位置);
			while (!IP.IsEmpty())
			{
				IP.Trim();
				if (!IP.IsEmpty())
				{
					黑名单列表.push_back(IP);
					TRACE(_T("加载黑名单IP: %s\n"), IP);
				}
				IP = 黑名单数据.Tokenize(_T(";"), 位置);
			}
		}

		// 加载白名单
		dwSize = sizeof(szValue);
		if (RegQueryValueEx(hKey, _T("WhiteList"), NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
		{
			CString 白名单数据(szValue);
			int 位置 = 0;
			CString IP = 白名单数据.Tokenize(_T(";"), 位置);
			while (!IP.IsEmpty())
			{
				IP.Trim();
				if (!IP.IsEmpty())
				{
					白名单列表.push_back(IP);
					TRACE(_T("加载白名单IP: %s\n"), IP);
				}
				IP = 白名单数据.Tokenize(_T(";"), 位置);
			}
		}

		RegCloseKey(hKey);
	}

	已加载 = TRUE;
	TRACE(_T("加载黑白名单完成，黑名单数量: %d, 白名单数量: %d\n"), 黑名单列表.size(), 白名单列表.size());
}

// 初始化端口转发界面
void NageDlqServerDlg::初始化端口转发界面()
{
	
	// 获取控件
	//端口转发列表控件.SubclassDlgItem(IDC_LIST_FORWARD, this);
	// 获取控件 - 使用 Attach 而不是 SubclassDlgItem
	//端口转发列表控件.Attach(::GetDlgItem(GetSafeHwnd(), IDC_LIST_FORWARD));
	//启动转发按钮.Attach(::GetDlgItem(GetSafeHwnd(), IDC_BUTTON_START_FORWARD));
	//停止转发按钮.Attach(::GetDlgItem(GetSafeHwnd(), IDC_BUTTON_STOP_FORWARD));

	// 设置列表样式 - 添加 LVS_EDITLABELS 支持编辑
	DWORD 样式 = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER;
	端口转发列表控件.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
	
	// 添加列
	端口转发列表控件.InsertColumn(0, _T("状态"), LVCFMT_LEFT, 60);
	端口转发列表控件.InsertColumn(1, _T("序号"), LVCFMT_LEFT, 40);
	端口转发列表控件.InsertColumn(2, _T("输入IP"), LVCFMT_LEFT, 100);
	端口转发列表控件.InsertColumn(3, _T("输入端口"), LVCFMT_LEFT, 60);
	端口转发列表控件.InsertColumn(4, _T("输出IP"), LVCFMT_LEFT, 100);
	端口转发列表控件.InsertColumn(5, _T("输出端口"), LVCFMT_LEFT, 60);
	端口转发列表控件.InsertColumn(6, _T("连接数"), LVCFMT_LEFT, 60);

	// 添加默认规则
	添加默认转发规则();
}

// 启动转发按钮点击处理
void NageDlqServerDlg::On启动转发按钮点击()
{
	if (端口转发管理器.获取规则列表().empty())
	{
		添加信息显示(_T("请先添加转发规则"));
		return;
	}

	if (启动端口转发())
	{
		启动转发按钮.EnableWindow(FALSE);
		停止转发按钮.EnableWindow(TRUE);
		添加信息显示(_T("端口转发已启动"));
	}
	else
	{
		添加信息显示(_T("端口转发启动失败"));
	}
}

// 停止转发按钮点击处理
void NageDlqServerDlg::On停止转发按钮点击()  // 修改函数名
{
	if (停止端口转发())
	{
		启动转发按钮.EnableWindow(TRUE);
		停止转发按钮.EnableWindow(FALSE);
		添加信息显示(_T("端口转发已停止"));
	}
}

// 启动端口转发
BOOL NageDlqServerDlg::启动端口转发()
{
	try
	{
		if (端口转发管理器.启动所有转发())
		{
			刷新端口转发列表();
			return TRUE;
		}
	}
	catch (const std::exception& e)
	{
		TRACE(_T("启动端口转发时发生异常: %s\n"), CString(e.what()));
		添加信息显示(_T("启动端口转发时发生异常"));
	}
	catch (...)
	{
		TRACE(_T("启动端口转发时发生未知异常\n"));
		添加信息显示(_T("启动端口转发时发生未知异常"));
	}

	return FALSE;
}

// 停止端口转发
BOOL NageDlqServerDlg::停止端口转发()
{
	try
	{
		if (端口转发管理器.停止所有转发())
		{
			刷新端口转发列表();
			return TRUE;
		}
	}
	catch (const std::exception& e)
	{
		TRACE(_T("停止端口转发时发生异常: %s\n"), CString(e.what()));
		添加信息显示(_T("停止端口转发时发生异常"));
	}
	catch (...)
	{
		TRACE(_T("停止端口转发时发生未知异常\n"));
		添加信息显示(_T("停止端口转发时发生未知异常"));
	}

	return FALSE;
}

// 列表项双击编辑处理
/*
void NageDlqServerDlg::On列表项双击(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNMItemActivate)
	{
		int 选中项 = pNMItemActivate->iItem;
		int 选中列 = pNMItemActivate->iSubItem;

		// 只允许编辑IP和端口列（第2、3、4、5列）
		if (选中项 >= 0 && 选中列 >= 2 && 选中列 <= 5)
		{
			// 开始编辑
			CEdit* pEdit = 端口转发列表控件.EditLabel(选中项);
			if (pEdit)
			{
				// 获取当前文本
				CString 当前文本 = 端口转发列表控件.GetItemText(选中项, 选中列);

				// 设置编辑框的初始文本
				pEdit->SetWindowText(当前文本);
			}

			*pResult = 0;
		}
	}
}*/
void NageDlqServerDlg::On列表项双击(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNMItemActivate)
	{
		int 选中项 = pNMItemActivate->iItem;
		int 选中列 = pNMItemActivate->iSubItem;

		TRACE(_T("列表项双击: 项=%d, 列=%d\n"), 选中项, 选中列);

		// 只允许编辑IP和端口列（第2、3、4、5列）
		if (选中项 >= 0 && 选中列 >= 2 && 选中列 <= 5)
		{
			if (当前编辑框)
			{
				当前编辑框->DestroyWindow();
				delete 当前编辑框;
				当前编辑框 = nullptr;
			}
			// 手动创建编辑框
			CRect 矩形;
			端口转发列表控件.GetSubItemRect(选中项, 选中列, LVIR_LABEL, 矩形);

			// 创建编辑框
			当前编辑框 = new CEdit();
			DWORD 样式 = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
			当前编辑框->Create(样式, 矩形, &端口转发列表控件, 1); // 使用固定ID 1

			// 获取当前文本
			CString 当前文本 = 端口转发列表控件.GetItemText(选中项, 选中列);
			当前编辑框->SetWindowText(当前文本);
			当前编辑框->SetFocus();
			当前编辑框->SetSel(0, -1); // 全选文本

			// 保存编辑框指针和位置信息
			当前编辑项 = 选中项;
			当前编辑列 = 选中列;

			TRACE(_T("创建编辑框: 项=%d, 列=%d, 文本=%s\n"), 选中项, 选中列, 当前文本);
		}
		else
		{
			TRACE(_T("不允许编辑的列: 项=%d, 列=%d\n"), 选中项, 选中列);
		}

		*pResult = 0;
	}
}

// 列表结束编辑处理
/*
void NageDlqServerDlg::On列表结束编辑(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	if (pDispInfo->item.pszText != NULL)
	{
		int 选中项 = pDispInfo->item.iItem;
		int 选中列 = pDispInfo->item.iSubItem;
		CString 新文本 = pDispInfo->item.pszText;

		CString 序号文本 = 端口转发列表控件.GetItemText(选中项, 1);
		int 规则序号 = _ttoi(序号文本);

		// 获取规则列表
		auto 规则列表 = 端口转发管理器.获取规则列表();

		if (选中项 >= 0 && 选中项 < (int)规则列表.size())
		{
			// 使用指针访问
			auto* 当前规则 = 规则列表[选中项];

			// 缓存当前值用于更新
			CString 当前输入IP = 端口转发列表控件.GetItemText(选中项, 2);
			CString 当前输出IP = 端口转发列表控件.GetItemText(选中项, 4);

			// 更新数据
			switch (选中列)
			{
			case 2: // 输入IP
			{
				// 检查是否需要自动填充后续行
				if (选中项 == 0) // 第一行
				{
					// 如果编辑的是输入IP，尝试自动填充后续行
					if (!新文本.IsEmpty())
					{
						// 自动填充后续行的输入IP
						for (int i = 选中项 + 1; i < 端口转发列表控件.GetItemCount(); i++)
						{
							CString 当前行输入IP = 端口转发列表控件.GetItemText(i, 2);
							if (当前行输入IP.IsEmpty() || 当前行输入IP == _T("双击填写"))
							{
								端口转发列表控件.SetItemText(i, 2, 新文本);

								// 同时更新对应规则的数据
								if (i < (int)规则列表.size())
								{
									auto* 后续规则 = 规则列表[i];
									端口转发管理器.更新转发规则(后续规则->序号, 新文本, 后续规则->输入端口,
										后续规则->输出IP, 后续规则->输出端口);
								}
							}
						}
					}
				}
				// 更新当前规则 - 使用指针访问
				端口转发管理器.更新转发规则(当前规则->序号, 新文本, 当前规则->输入端口,
					当前规则->输出IP, 当前规则->输出端口);
			}
			break;

			case 4: // 输出IP
			{
				// 检查是否需要自动填充后续行
				if (选中项 == 0) // 第一行
				{
					// 自动填充后续行的输出IP
					for (int i = 选中项 + 1; i < 端口转发列表控件.GetItemCount(); i++)
					{
						CString 当前行输出IP = 端口转发列表控件.GetItemText(i, 4);
						if (当前行输出IP.IsEmpty() || 当前行输出IP == _T("双击填写"))
						{
							端口转发列表控件.SetItemText(i, 4, 新文本);

							// 同时更新对应规则的数据
							if (i < (int)规则列表.size())
							{
								auto* 后续规则 = 规则列表[i];
								端口转发管理器.更新转发规则(后续规则->序号, 后续规则->输入IP, 后续规则->输入端口,
									新文本, 后续规则->输出端口);
							}
						}
					}
				}
				// 更新当前规则 - 使用指针访问
				端口转发管理器.更新转发规则(当前规则->序号, 当前规则->输入IP, 当前规则->输入端口,
					新文本, 当前规则->输出端口);
			}
			break;

			case 3: // 输入端口
			{
				int 新端口 = _ttoi(新文本);
				if (新端口 > 0 && 新端口 <= 65535)
				{
					// 使用指针访问
					端口转发管理器.更新转发规则(当前规则->序号, 当前输入IP, 新端口,
						当前输出IP, 当前规则->输出端口);
				}
				else
				{
					// 端口号无效，恢复原值
					CString 原端口文本;
					原端口文本.Format(_T("%d"), 当前规则->输入端口);
					端口转发列表控件.SetItemText(选中项, 选中列, 原端口文本);
					添加信息显示(_T("端口号无效，请输入1-65535之间的数字"));
				}
			}
			break;

			case 5: // 输出端口
			{
				int 新端口 = _ttoi(新文本);
				if (新端口 > 0 && 新端口 <= 65535)
				{
					// 使用指针访问
					端口转发管理器.更新转发规则(当前规则->序号, 当前输入IP, 当前规则->输入端口,
						当前输出IP, 新端口);
				}
				else
				{
					// 端口号无效，恢复原值
					CString 原端口文本;
					原端口文本.Format(_T("%d"), 当前规则->输出端口);
					端口转发列表控件.SetItemText(选中项, 选中列, 原端口文本);
					添加信息显示(_T("端口号无效，请输入1-65535之间的数字"));
				}
			}
			break;

			default:
				// 其他列不允许编辑
				break;
			}

			// 保存配置
			if (保存端口转发配置())
			{
				TRACE(_T("端口转发配置保存成功\n"));
			}
			else
			{
				TRACE(_T("端口转发配置保存失败\n"));
				添加信息显示(_T("端口转发配置保存失败"));
			}

			// 自动刷新显示
			刷新端口转发列表();

			// 添加成功提示
			CString 成功信息;
			switch (选中列)
			{
			case 2:
				成功信息.Format(_T("规则 %d 输入IP已更新为: %s"), 规则序号, 新文本);
				break;
			case 3:
				成功信息.Format(_T("规则 %d 输入端口已更新为: %s"), 规则序号, 新文本);
				break;
			case 4:
				成功信息.Format(_T("规则 %d 输出IP已更新为: %s"), 规则序号, 新文本);
				break;
			case 5:
				成功信息.Format(_T("规则 %d 输出端口已更新为: %s"), 规则序号, 新文本);
				break;
			default:
				成功信息.Format(_T("规则 %d 已更新"), 规则序号);
				break;
			}
			添加信息显示(成功信息);
		}
		else
		{
			// 选中项超出范围
			TRACE(_T("选中项超出范围: %d, 规则列表大小: %d\n"), 选中项, 规则列表.size());
			添加信息显示(_T("编辑失败: 选中的规则不存在"));
		}
	}
	else
	{
		// 编辑取消
		TRACE(_T("编辑取消\n"));
	}

	*pResult = 0;
}*/
void NageDlqServerDlg::On列表结束编辑(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	TRACE(_T("=== 列表结束编辑开始 ===\n"));

	if (pDispInfo->item.pszText != NULL)
	{
		int 选中项 = pDispInfo->item.iItem;
		int 选中列 = pDispInfo->item.iSubItem;
		CString 新文本 = pDispInfo->item.pszText;

		TRACE(_T("编辑完成: 项=%d, 列=%d, 新文本=%s\n"), 选中项, 选中列, 新文本);

		CString 序号文本 = 端口转发列表控件.GetItemText(选中项, 1);
		int 规则序号 = _ttoi(序号文本);

		TRACE(_T("规则序号: %d\n"), 规则序号);

		// 获取规则列表
		auto 规则列表 = 端口转发管理器.获取规则列表();

		TRACE(_T("规则列表大小: %d\n"), 规则列表.size());

		if (选中项 >= 0 && 选中项 < (int)规则列表.size())
		{
			// 使用指针访问
			auto* 当前规则 = 规则列表[选中项];

			// 缓存当前值用于更新
			CString 当前输入IP = 端口转发列表控件.GetItemText(选中项, 2);
			CString 当前输出IP = 端口转发列表控件.GetItemText(选中项, 4);

			TRACE(_T("当前输入IP: %s, 当前输出IP: %s\n"), 当前输入IP, 当前输出IP);

			// 更新数据
			switch (选中列)
			{
			case 2: // 输入IP
			{
				TRACE(_T("更新输入IP: %s\n"), 新文本);
				// 检查是否需要自动填充后续行
				if (选中项 == 0) // 第一行
				{
					// 如果编辑的是输入IP，尝试自动填充后续行
					if (!新文本.IsEmpty())
					{
						// 自动填充后续行的输入IP
						for (int i = 选中项 + 1; i < 端口转发列表控件.GetItemCount(); i++)
						{
							CString 当前行输入IP = 端口转发列表控件.GetItemText(i, 2);
							if (当前行输入IP.IsEmpty() || 当前行输入IP == _T("双击填写"))
							{
								端口转发列表控件.SetItemText(i, 2, 新文本);

								// 同时更新对应规则的数据
								if (i < (int)规则列表.size())
								{
									auto* 后续规则 = 规则列表[i];
									端口转发管理器.更新转发规则(后续规则->序号, 新文本, 后续规则->输入端口,
										后续规则->输出IP, 后续规则->输出端口);
								}
							}
						}
					}
				}
				// 更新当前规则 - 使用指针访问
				端口转发管理器.更新转发规则(当前规则->序号, 新文本, 当前规则->输入端口,
					当前规则->输出IP, 当前规则->输出端口);
			}
			break;

			case 4: // 输出IP
			{
				TRACE(_T("更新输出IP: %s\n"), 新文本);
				// 检查是否需要自动填充后续行
				if (选中项 == 0) // 第一行
				{
					// 自动填充后续行的输出IP
					for (int i = 选中项 + 1; i < 端口转发列表控件.GetItemCount(); i++)
					{
						CString 当前行输出IP = 端口转发列表控件.GetItemText(i, 4);
						if (当前行输出IP.IsEmpty() || 当前行输出IP == _T("双击填写"))
						{
							端口转发列表控件.SetItemText(i, 4, 新文本);

							// 同时更新对应规则的数据
							if (i < (int)规则列表.size())
							{
								auto* 后续规则 = 规则列表[i];
								端口转发管理器.更新转发规则(后续规则->序号, 后续规则->输入IP, 后续规则->输入端口,
									新文本, 后续规则->输出端口);
							}
						}
					}
				}
				// 更新当前规则 - 使用指针访问
				端口转发管理器.更新转发规则(当前规则->序号, 当前规则->输入IP, 当前规则->输入端口,
					新文本, 当前规则->输出端口);
			}
			break;

			case 3: // 输入端口
			{
				int 新端口 = _ttoi(新文本);
				TRACE(_T("更新输入端口: %d\n"), 新端口);
				if (新端口 > 0 && 新端口 <= 65535)
				{
					// 使用指针访问
					端口转发管理器.更新转发规则(当前规则->序号, 当前输入IP, 新端口,
						当前输出IP, 当前规则->输出端口);
				}
				else
				{
					// 端口号无效，恢复原值
					CString 原端口文本;
					原端口文本.Format(_T("%d"), 当前规则->输入端口);
					端口转发列表控件.SetItemText(选中项, 选中列, 原端口文本);
					添加信息显示(_T("端口号无效，请输入1-65535之间的数字"));
				}
			}
			break;

			case 5: // 输出端口
			{
				int 新端口 = _ttoi(新文本);
				TRACE(_T("更新输出端口: %d\n"), 新端口);
				if (新端口 > 0 && 新端口 <= 65535)
				{
					// 使用指针访问
					端口转发管理器.更新转发规则(当前规则->序号, 当前输入IP, 当前规则->输入端口,
						当前输出IP, 新端口);
				}
				else
				{
					// 端口号无效，恢复原值
					CString 原端口文本;
					原端口文本.Format(_T("%d"), 当前规则->输出端口);
					端口转发列表控件.SetItemText(选中项, 选中列, 原端口文本);
					添加信息显示(_T("端口号无效，请输入1-65535之间的数字"));
				}
			}
			break;

			default:
				TRACE(_T("不允许编辑的列: %d\n"), 选中列);
				break;
			}

			// 保存配置
			if (保存端口转发配置())
			{
				TRACE(_T("端口转发配置保存成功\n"));
			}
			else
			{
				TRACE(_T("端口转发配置保存失败\n"));
				添加信息显示(_T("端口转发配置保存失败"));
			}

			// 自动刷新显示
			刷新端口转发列表();

			// 添加成功提示
			CString 成功信息;
			switch (选中列)
			{
			case 2:
				成功信息.Format(_T("规则 %d 输入IP已更新为: %s"), 规则序号, 新文本);
				break;
			case 3:
				成功信息.Format(_T("规则 %d 输入端口已更新为: %s"), 规则序号, 新文本);
				break;
			case 4:
				成功信息.Format(_T("规则 %d 输出IP已更新为: %s"), 规则序号, 新文本);
				break;
			case 5:
				成功信息.Format(_T("规则 %d 输出端口已更新为: %s"), 规则序号, 新文本);
				break;
			default:
				成功信息.Format(_T("规则 %d 已更新"), 规则序号);
				break;
			}
			添加信息显示(成功信息);
		}
		else
		{
			// 选中项超出范围
			TRACE(_T("选中项超出范围: %d, 规则列表大小: %d\n"), 选中项, 规则列表.size());
			添加信息显示(_T("编辑失败: 选中的规则不存在"));
		}
	}
	else
	{
		// 编辑取消
		TRACE(_T("编辑取消\n"));
	}

	TRACE(_T("=== 列表结束编辑结束 ===\n"));
	*pResult = 0;
}

// 添加默认转发规则
void NageDlqServerDlg::添加默认转发规则()
{
	// 添加一条默认规则
	端口转发管理器.添加转发规则(_T("127.0.0.1"), 9896, _T("192.168.100.1"), 9896);

	// 刷新列表
	刷新端口转发列表();
}

// 刷新端口转发列表
void NageDlqServerDlg::刷新端口转发列表()
{
	端口转发列表控件.DeleteAllItems();

	const auto& 规则列表 = 端口转发管理器.获取规则列表();

	for (size_t i = 0; i < 规则列表.size(); i++)
	{
		const auto* 规则 = 规则列表[i];  // 使用指针

		int 索引 = 端口转发列表控件.InsertItem(0, 规则->状态);  // 使用 -> 操作符

		CString 序号文本;
		序号文本.Format(_T("%d"), 规则->序号);  // 使用 -> 操作符
		端口转发列表控件.SetItemText(索引, 1, 序号文本);
		端口转发列表控件.SetItemText(索引, 2, 规则->输入IP);  // 使用 -> 操作符

		CString 端口文本;
		端口文本.Format(_T("%d"), 规则->输入端口);  // 使用 -> 操作符
		端口转发列表控件.SetItemText(索引, 3, 端口文本);
		端口转发列表控件.SetItemText(索引, 4, 规则->输出IP);  // 使用 -> 操作符

		端口文本.Format(_T("%d"), 规则->输出端口);  // 使用 -> 操作符
		端口转发列表控件.SetItemText(索引, 5, 端口文本);

		端口文本.Format(_T("%d"), 规则->连接数);  // 使用 -> 操作符
		端口转发列表控件.SetItemText(索引, 6, 端口文本);
	}

	for (int i = 0; i < 7; i++) {
		端口转发列表控件.SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
	}
}

// 安全的端口转发启动
BOOL NageDlqServerDlg::安全启动端口转发()
{
	try
	{
		return 启动端口转发();
	}
	catch (const std::exception& e)
	{
		添加信息显示(_T("端口转发启动过程中发生异常，但程序继续运行"));
		TRACE(_T("端口转发启动异常: %s\n"), CString(e.what()));
		return FALSE;
	}
	catch (...)
	{
		添加信息显示(_T("端口转发启动过程中发生未知异常，但程序继续运行"));
		return FALSE;
	}
}

// 安全的端口转发停止
BOOL NageDlqServerDlg::安全停止端口转发()
{
	try
	{
		return 停止端口转发();
	}
	catch (const std::exception& e)
	{
		添加信息显示(_T("端口转发停止过程中发生异常，但程序继续运行"));
		TRACE(_T("端口转发停止异常: %s\n"), CString(e.what()));
		return FALSE;
	}
	catch (...)
	{
		添加信息显示(_T("端口转发停止过程中发生未知异常，但程序继续运行"));
		return FALSE;
	}
}

// 保存端口转发配置函数
BOOL NageDlqServerDlg::保存端口转发配置()
{
	try
	{
		if (端口转发管理器.保存配置())
		{
			添加信息显示(_T("端口转发配置保存成功"));
			return TRUE;
		}
	}
	catch (const std::exception& e)
	{
		TRACE(_T("保存端口转发配置时发生异常: %s\n"), CString(e.what()));
		添加信息显示(_T("端口转发配置保存失败"));
	}
	catch (...)
	{
		TRACE(_T("保存端口转发配置时发生未知异常\n"));
		添加信息显示(_T("端口转发配置保存失败"));
	}

	return FALSE;
}

// 加载端口转发配置函数
BOOL NageDlqServerDlg::加载端口转发配置()
{
	try
	{
		if (端口转发管理器.加载配置())
		{
			添加信息显示(_T("端口转发配置加载成功"));
			return TRUE;
		}
	}
	catch (const std::exception& e)
	{
		TRACE(_T("加载端口转发配置时发生异常: %s\n"), CString(e.what()));
		添加信息显示(_T("端口转发配置加载失败或没有配置"));
	}
	catch (...)
	{
		TRACE(_T("加载端口转发配置时发生未知异常\n"));
		添加信息显示(_T("端口转发配置加载失败"));
	}

	return FALSE;
}

// 编辑框失去焦点处理
void NageDlqServerDlg::On编辑框失去焦点(CWnd* pNewWnd)
{
	// 检查是否是编辑框失去焦点
	//if (当前编辑框 && pNewWnd != 当前编辑框)
	if (当前编辑框)
	{
		CString 新文本;
		当前编辑框->GetWindowText(新文本);

		// 调用编辑完成处理
		if (当前编辑项 >= 0 && 当前编辑列 >= 0)
		{
			TRACE(_T("编辑完成: 项=%d, 列=%d, 文本=%s\n"), 当前编辑项, 当前编辑列, 新文本);

			// 更新列表显示
			端口转发列表控件.SetItemText(当前编辑项, 当前编辑列, 新文本);

			// 这里添加规则更新逻辑
			CString 序号文本 = 端口转发列表控件.GetItemText(当前编辑项, 1);
			int 规则序号 = _ttoi(序号文本);

			auto 规则列表 = 端口转发管理器.获取规则列表();
			if (当前编辑项 >= 0 && 当前编辑项 < (int)规则列表.size())
			{
				auto* 当前规则 = 规则列表[当前编辑项];

				switch (当前编辑列)
				{
				case 2: // 输入IP
					端口转发管理器.更新转发规则(当前规则->序号, 新文本, 当前规则->输入端口,
						当前规则->输出IP, 当前规则->输出端口);
					break;
				case 3: // 输入端口
				{
					int 新端口 = _ttoi(新文本);
					if (新端口 > 0 && 新端口 <= 65535)
					{
						CString 当前输入IP = 端口转发列表控件.GetItemText(当前编辑项, 2);
						端口转发管理器.更新转发规则(当前规则->序号, 当前输入IP, 新端口,
							当前规则->输出IP, 当前规则->输出端口);
					}
				}
				break;
				case 4: // 输出IP
					端口转发管理器.更新转发规则(当前规则->序号, 当前规则->输入IP, 当前规则->输入端口,
						新文本, 当前规则->输出端口);
					break;
				case 5: // 输出端口
				{
					int 新端口 = _ttoi(新文本);
					if (新端口 > 0 && 新端口 <= 65535)
					{
						CString 当前输出IP = 端口转发列表控件.GetItemText(当前编辑项, 4);
						端口转发管理器.更新转发规则(当前规则->序号, 当前规则->输入IP, 当前规则->输入端口,
							当前输出IP, 新端口);
					}
				}
				break;
				}

				// 保存配置
				保存端口转发配置();
				刷新端口转发列表();
			}
		}

		// 销毁编辑框
		当前编辑框->DestroyWindow();
		delete 当前编辑框;
		当前编辑框 = nullptr;
		当前编辑项 = -1;
		当前编辑列 = -1;
	}
}
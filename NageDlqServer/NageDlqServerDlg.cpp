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
			对话框指针->添加信息显示(客户端IP + 完整请求信息);
		}

		// 解析请求
		if (客户端请求.Find(_T("CONNECT:")) == 0)
		{
			// 处理连接验证请求 - 格式: CONNECT:客户端版本号:客户端IP
			CString 连接数据 = 客户端请求.Mid(8); // 去掉"CONNECT:"
			int 分隔符位置 = 连接数据.Find(':');
			if (分隔符位置 != -1)
			{
				CString 客户端版本号 = 连接数据.Left(分隔符位置);
				CString 客户端IP = 连接数据.Mid(分隔符位置 + 1);

				// 查询数据库获取密钥和最新版本号
				CString 客户端密钥 = 对话框指针->获取客户端密钥();
				CString 最新版本号 = 对话框指针->获取最新版本号();

				// 发送响应
				CString 响应数据;
				if (!客户端密钥.IsEmpty())
				{
					响应数据.Format(_T("CONNECT_SUCCESS:%s:%s"), 客户端密钥, 最新版本号);
					对话框指针->添加信息显示(客户端IP + _T(" 连接验证成功，版本: ") + 客户端版本号);
				}
				else
				{
					响应数据 = _T("CONNECT_FAILED:服务端配置错误");
					对话框指针->添加信息显示(客户端IP + _T(" 连接验证失败"));
				}

				对话框指针->发送到客户端(客户端套接字, 响应数据);
			}
			else
			{
				对话框指针->发送到客户端(客户端套接字, _T("CONNECT_FAILED:无效的连接数据格式"));
				对话框指针->添加信息显示(客户端IP + _T(" 连接数据格式错误"));
			}
		}
		else if (客户端请求.Find(_T("LOGIN:")) == 0)
		{
			// 先记录请求，再处理
			if (!客户端IP.IsEmpty())
			{
				CString 完整请求信息;
				完整请求信息.Format(_T(" 请求: [%s], 长度: %d"), 客户端请求, 客户端请求.GetLength());
				对话框指针->添加信息显示(客户端IP + 完整请求信息);
			}

			// 然后处理登录请求
			CString 登录数据 = 客户端请求.Mid(6);

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

				// 验证用户名和密码
				BOOL 登录结果 = 对话框指针->验证用户登录(用户名, 密码);

				if (登录结果)
				{
					对话框指针->发送到客户端(客户端套接字, _T("LOGIN_SUCCESS:登录成功"));
					// 成功日志放在最后
					对话框指针->添加信息显示(客户端IP + _T(" 登录成功 - 用户名: ") + 用户名);
				}
				else
				{
					对话框指针->发送到客户端(客户端套接字, _T("LOGIN_FAILED:用户名或密码错误"));
					// 失败日志放在最后
					对话框指针->添加信息显示(客户端IP + _T(" 登录失败 - 用户名: ") + 用户名);
				}
			}
			else
			{
				对话框指针->发送到客户端(客户端套接字, _T("LOGIN_FAILED:无效的登录数据格式"));
				对话框指针->添加信息显示(客户端IP + _T(" 登录数据格式错误"));
			}
		}
		else if (客户端请求.Find(_T("REGISTER:")) == 0)
		{
			// 处理注册请求 - 格式: REGISTER:username:password:email
			CString 注册数据 = 客户端请求.Mid(9); // 去掉"REGISTER:"

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

				// 处理注册
				BOOL 注册结果 = 对话框指针->处理用户注册(用户名, 密码, 邮箱);

				if (注册结果)
				{
					对话框指针->发送到客户端(客户端套接字, _T("REGISTER_SUCCESS:注册成功"));
					对话框指针->添加信息显示(客户端IP + _T(" 注册成功 - 用户名: ") + 用户名);
				}
				else
				{
					对话框指针->发送到客户端(客户端套接字, _T("REGISTER_FAILED:注册失败"));
					对话框指针->添加信息显示(客户端IP + _T(" 注册失败 - 用户名: ") + 用户名);
				}
			}
			else
			{
				CString 错误信息;
				错误信息.Format(_T("注册数据格式错误，参数数量: %d"), 参数数组.GetSize());
				对话框指针->添加信息显示(客户端IP + _T(" ") + 错误信息);
				对话框指针->发送到客户端(客户端套接字, _T("REGISTER_FAILED:") + 错误信息);
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
	SQLRETURN retcode;
	CString 查询语句;
	查询语句.Format(_T("SELECT COUNT(*) FROM Chr_Log_Info WHERE id_loginid = '%s' AND id_passwd = '%s'"),
		用户名, 密码);

	retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)查询语句.GetString(), SQL_NTS);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
		添加信息显示(_T("登录验证失败: 数据库查询错误"));
		SQLCloseCursor(SQL语句句柄);
		return FALSE;
	}

	retcode = SQLFetch(SQL语句句柄);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		SQLINTEGER 数量;
		SQLGetData(SQL语句句柄, 1, SQL_C_LONG, &数量, sizeof(数量), NULL);
		SQLCloseCursor(SQL语句句柄);

		return 数量 > 0;
	}

	SQLCloseCursor(SQL语句句柄);
	return FALSE;
}
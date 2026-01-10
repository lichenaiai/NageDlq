//NageDlqServerDlg.cpp
#include "pch.h"
#include "framework.h"
#include "NageDlqServer.h"
#include "NageDlqServerDlg.h"
#include "afxdialogex.h"
#include "设置对话框类.h"
#include "黑白名单对话框类.h"
#include "WebSocket处理类.h"

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
	, 日志文件已打开(FALSE)
{
	// 数据库配置 - 使用SQL Server默认设置
	数据库用户名 = _T("sa");           // SQL Server默认管理员
	数据库密码 = _T("您的数据库密码");  // 修改为您的密码
	数据库名称 = _T("nagelogin");      // 您的数据库名

	InitializeCriticalSection(&客户端列表锁);
	
	初始化日志文件();
}

NageDlqServerDlg::~NageDlqServerDlg()
{
	停止所有后台操作();

	WSACleanup();     // 清理Winsock

	// 释放ODBC资源
	if (SQL语句句柄) SQLFreeHandle(SQL_HANDLE_STMT, SQL语句句柄);
	if (SQL连接句柄) SQLDisconnect(SQL连接句柄);
	if (SQL连接句柄) SQLFreeHandle(SQL_HANDLE_DBC, SQL连接句柄);
	if (SQL环境句柄) SQLFreeHandle(SQL_HANDLE_ENV, SQL环境句柄);

	关闭日志文件();

	DeleteCriticalSection(&客户端列表锁);
	
	安全停止端口转发();

	// 清除定时器
	if (端口转发刷新定时器 != 0)
	{
		KillTimer(端口转发刷新定时器);
	}
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
	ON_NOTIFY(NM_CLICK, IDC_LIST_FORWARD, &NageDlqServerDlg::On列表单击)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST_FORWARD, &NageDlqServerDlg::On列表结束编辑)
	ON_MESSAGE(WM_USER + 100, &NageDlqServerDlg::On延迟加载端口转发数据)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_FORWARD, &NageDlqServerDlg::On自定义绘制列表)
	ON_EN_KILLFOCUS(IDC_EDIT_CONTROL, &NageDlqServerDlg::On编辑框失去焦点)
	ON_EN_CHANGE(IDC_EDIT_CONTROL, &NageDlqServerDlg::On编辑框内容改变)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FORWARD, &NageDlqServerDlg::On右键菜单)
	ON_COMMAND(ID_MENU_DELETE_RULE, &NageDlqServerDlg::On删除规则)
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL NageDlqServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	加载配置();

	// 设置初始状态
	停止服务器按钮.EnableWindow(FALSE);
	推送登录器更新按钮.EnableWindow(FALSE);
	推送HOOK更新按钮.EnableWindow(FALSE);
	停止转发按钮.EnableWindow(FALSE);

	// 初始化Winsock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		添加信息显示(_T("Winsock初始化失败"));
		return FALSE;
	}


	// 只初始化界面，不加载数据
	初始化端口转发界面();

	// 立即加载配置，不依赖延迟消息
	if (加载端口转发配置())
	{
		TRACE(_T("配置加载成功\n"));
		//添加信息显示(_T("端口转发配置加载成功"));
	}
	else
	{
		TRACE(_T("配置加载失败，添加默认规则\n"));
		添加信息显示(_T("使用默认端口转发规则"));
		添加默认转发规则();
	}

	// 刷新显示
	刷新端口转发列表();

	if (初始化日志文件())
	{
		TRACE(_T("日志文件初始化成功\n"));
	}
	else
	{
		TRACE(_T("日志文件初始化失败\n"));
	}


	添加信息显示(_T("程序已初始化"));

	// 启动定时器，每1秒刷新一次连接数
	端口转发刷新定时器 = SetTimer(1000, 1000, NULL); // ID=1000, 间隔1秒

	return TRUE;
}

void NageDlqServerDlg::OnClose()
{
	TRACE(_T("=== OnClose开始 ===\n"));

	// 停止所有后台操作
	停止所有后台操作();

	// 保存配置
	TRACE(_T("保存配置...\n"));
	保存配置();
	保存端口转发配置();

	// 调用父类的OnClose
	CDialogEx::OnClose();

	TRACE(_T("=== OnClose完成 ===\n"));
}

void NageDlqServerDlg::OnDestroy()
{
	TRACE(_T("=== OnDestroy开始 ===\n"));

	// 确保所有后台操作已停止
	停止所有后台操作();

	// 清理控件关联
	端口转发列表控件.DeleteAllItems();

	// 调用父类的OnDestroy
	CDialogEx::OnDestroy();

	TRACE(_T("=== OnDestroy完成 ===\n"));
}

void NageDlqServerDlg::PostNcDestroy()
{
	TRACE(_T("=== PostNcDestroy开始 ===\n"));

	// 确保所有资源已释放

	// 调用父类的PostNcDestroy
	CDialogEx::PostNcDestroy();

	TRACE(_T("=== PostNcDestroy完成 ===\n"));
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

	// 显示模态对话框
	if (设置对话框.DoModal() == IDOK)
	{
		// 获取新的配置
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

// 定时器处理函数
void NageDlqServerDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1000) // 定时器ID
	{
		// 刷新端口转发列表的连接数显示
		for (int i = 0; i < 端口转发列表控件.GetItemCount(); i++)
		{
			CString 序号文本 = 端口转发列表控件.GetItemText(i, 1);
			int 规则序号 = _ttoi(序号文本);

			if (规则序号 > 0)
			{
				// 获取规则列表
				auto 规则列表 = 端口转发管理器.获取规则列表();
				for (const auto* 规则 : 规则列表)
				{
					if (规则 && 规则->序号 == 规则序号)
					{
						CString 连接数文本;
						连接数文本.Format(_T("%d"), 规则->获取连接数());
						端口转发列表控件.SetItemText(i, 6, 连接数文本);
						break;
					}
				}
			}
		}
	}

	CDialogEx::OnTimer(nIDEvent);
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

	// 为websocket增加的代码。记录客户端类型：0=未知，1=普通TCP客户端，2=WebSocket客户端
	int 客户端类型 = 0;
	bool 已进行WebSocket握手 = false;

	// 为websocket增加的代码。首先尝试接收数据来确定客户端类型
	char 初始缓冲区[4096];
	memset(初始缓冲区, 0, sizeof(初始缓冲区));

	// 为websocket增加的代码。设置接收超时
	struct timeval 接收超时;
	接收超时.tv_sec = 5;
	接收超时.tv_usec = 0;
	setsockopt(客户端套接字, SOL_SOCKET, SO_RCVTIMEO, (char*)&接收超时, sizeof(接收超时));
	int 初始接收长度 = recv(客户端套接字, 初始缓冲区, sizeof(初始缓冲区) - 1, 0);

	//u_long 阻塞模式 = 0;
	u_long 非阻塞模式 = 1;
	ioctlsocket(客户端套接字, FIONBIO, &非阻塞模式);

	// 为websocket增加的代码。
	if (初始接收长度 > 0)
	{
		初始缓冲区[初始接收长度] = '\0';
		std::string 初始请求(初始缓冲区, 初始接收长度);

		TRACE(_T("收到初始数据，长度: %d\n"), 初始接收长度);
		TRACE(_T("初始数据: %s\n"), CString(初始请求.c_str()));

		// 检查是否是WebSocket握手请求
		if (WebSocket处理器::是WebSocket握手请求(初始请求))
		{
			TRACE(_T("检测到WebSocket握手请求\n"));

			// 生成WebSocket握手响应
			std::string 握手响应 = WebSocket处理器::生成握手响应(初始请求);

			if (!握手响应.empty())
			{
				// 发送握手响应
				send(客户端套接字, 握手响应.c_str(), 握手响应.length(), 0);
				TRACE(_T("已发送WebSocket握手响应\n"));

				客户端类型 = 2; // WebSocket客户端
				已进行WebSocket握手 = true;

				// 清除缓冲区，握手后的数据需要按WebSocket帧解析
				memset(初始缓冲区, 0, sizeof(初始缓冲区));
				初始接收长度 = 0;
			}
			else
			{
				TRACE(_T("WebSocket握手响应生成失败\n"));
				closesocket(客户端套接字);
				return 1;
			}
		}
		else
		{
			// 普通TCP客户端
			客户端类型 = 1; // 普通TCP客户端
			TRACE(_T("检测到普通TCP客户端\n"));
		}
	}
	else if (初始接收Length == 0)
	{
		TRACE(_T("客户端在握手前关闭连接\n"));
		closesocket(客户端套接字);
		return 1;
	}
	else
	{
		// 接收超时或错误，按普通TCP客户端处理
		TRACE(_T("初始接收超时，按普通TCP客户端处理\n"));
		客户端类型 = 1;
	}

	CString 客户端IP;
	EnterCriticalSection(&对话框指针->客户端列表锁);
	auto it = 对话框指针->客户端连接列表.find(客户端套接字);
	if (it != 对话框指针->客户端连接列表.end())
	{
		客户端IP = it->second;
	}
	LeaveCriticalSection(&对话框指针->客户端列表锁);

	// 记录最后活动时间，用于超时检测
	DWORD 最后活动时间 = GetTickCount();
	const DWORD 连接超时时间 = 300000; // 5分钟超时

	// 为websocket增加的代码。用于WebSocket的缓冲区
	std::vector<char> WebSocket接收缓冲区;

	// 持续处理客户端请求
	while (对话框指针->服务器运行状态)
	{
		// 检查连接是否超时
		DWORD 当前时间 = GetTickCount();
		if (当前时间 - 最后活动时间 > 连接超时时间)
		{
			TRACE(_T("客户端 %s 连接超时（5分钟无活动）\n"), 客户端IP);
			对话框指针->添加信息显示(客户端IP + _T(" 连接超时，自动断开"));
			break;
		}

		// 接收客户端请求
		CString 客户端请求;

		if (客户端类型 == 2) // WebSocket客户端
		{
			// WebSocket数据接收处理
			char WebSocket临时缓冲区[4096];
			memset(WebSocket临时缓冲区, 0, sizeof(WebSocket临时缓冲区));

			int WebSocket接收长度 = recv(客户端套接字, WebSocket临时缓冲区, sizeof(WebSocket临时缓冲区), 0);

			if (WebSocket接收Length > 0)
			{
				// 添加到缓冲区
				WebSocket接收缓冲区.insert(WebSocket接收缓冲区.end(),
					WebSocket临时缓冲区,
					WebSocket临时缓冲区 + WebSocket接收Length);

				// 尝试解析WebSocket帧
				std::string WebSocket消息 = WebSocket处理器::解析WebSocket帧(WebSocket接收缓冲区);

				if (!WebSocket消息.empty())
				{
					// 成功解析到完整消息
					客户端请求 = CString(WebSocket消息.c_str());
					// 清除已处理的数据（简化处理：清空整个缓冲区）
					WebSocket接收缓冲区.clear();

					// 更新最后活动时间
					最后活动时间 = GetTickCount();
				}
				else if (WebSocket消息.empty() && WebSocket接收缓冲区.size() > 0)
				{
					// 空字符串表示关闭帧或其他控制帧
					if (WebSocket接收Buffer.size() >= 2)
					{
						unsigned char* 数据 = reinterpret_cast<unsigned char*>(WebSocket接收缓冲区.data());
						if ((数据[0] & 0x0F) == 0x8) // 关闭帧
						{
							TRACE(_T("收到WebSocket关闭帧\n"));
							break;
						}
						// 其他控制帧，继续等待数据
						Sleep(10);
						continue;
					}
				}
			}
			else if (WebSocket接收Length == 0)
			{
				// 连接关闭
				TRACE(_T("WebSocket客户端关闭连接\n"));
				break;
			}
			else
			{
				int 错误码 = WSAGetLastError();
				if (错误码 != WSAEWOULDBLOCK)
				{
					TRACE(_T("WebSocket接收错误: %d\n"), 错误码);
					break;
				}
				// 没有数据，休眠等待
				Sleep(10);
				continue;
			}
		}
		else // 普通TCP客户端
		{
			// 使用原有的接收逻辑
			客户端请求 = 对话框指针->从客户端接收(客户端套接字);

			if (!客户端请求.IsEmpty())
			{
				客户端请求 = 对话框指针->清理请求(客户端请求);
				最后活动时间 = GetTickCount();
			}
		}

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
			// 如果是空数据但不是错误，休眠等待，避免忙等待，降低CPU占用
			Sleep(50);
			continue;
		}

		// 记录客户端请求
		if (!客户端IP.IsEmpty())
		{
			CString 完整请求信息;
			完整请求信息.Format(_T(" 请求: [%s], 长度: %d"), 客户端请求, 客户端请求.GetLength());
			//对话框指针->添加信息显示(客户端IP + 完整请求信息);
		}

		// 保存响应字符串
		CString 响应数据;
		BOOL 需要发送响应 = TRUE;

		// 解析连接请求
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
					响应数据 = _T("CONNECT_FAILED:IP访问受限"));
					对话框指针->添加信息显示(客户端IP + _T(" IP访问受限"));
					closesocket(客户端套接字);
					return 0;
				}

				// 查询数据库获取密钥和最新版本号
				CString 客户端密钥 = 对话框指针->获取客户端密钥();
				CString 最新版本号 = 对话框指针->获取最新版本号();

				TRACE(_T("查询到密钥: %s, 版本: %s\n"), 客户端密钥, 最新版本号);

				// 检查客户端版本
				if (对话框指针->比较版本号(客户端版本号, 最新版本号) < 0) 
				{
					TRACE(_T("客户端版本过时\n"));
					// 发送版本过时消息，并带上最新版本号
					CString 响应数据;
					响应数据.Format(_T("VERSION_OUTDATED:%s"), 最新版本号);
					对话框指针->发送到客户端(客户端套接字, 响应数据);
					//对话框指针->添加信息显示(客户端IP + _T(" 版本过时，已断开连接"));

					// 等待一段时间让客户端收到消息
					//Sleep(1000);

					//closesocket(客户端套接字);
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

				// 清理用户名和角色名
				用户名.Remove(_T('\r'));
				用户名.Remove(_T('\n'));
				用户名.Trim();
				角色名.Remove(_T('\r'));
				角色名.Remove(_T('\n'));
				角色名.Trim();

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
			CString 加点数据 = 客户端请求.Mid(11); 

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

				// 清理用户名和角色名
				用户名.Remove(_T('\r'));
				用户名.Remove(_T('\n'));
				用户名.Trim();
				角色名.Remove(_T('\r'));
				角色名.Remove(_T('\n'));
				角色名.Trim();

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
			CString 查询数据 = 客户端请求.Mid(14); 
			int 分隔符 = 查询数据.Find(':');

			if (分隔符 != -1)
			{
				CString 用户名 = 查询数据.Left(分隔符);
				CString 角色名 = 查询数据.Mid(分隔符 + 1);

				// 清理用户名和角色名
				用户名.Remove(_T('\r'));
				用户名.Remove(_T('\n'));
				用户名.Trim();
				角色名.Remove(_T('\r'));
				角色名.Remove(_T('\n'));
				角色名.Trim();

				TRACE(_T("查询角色信息，用户: %s, 角色: %s\n"), 用户名, 角色名);

				// 跨数据库查询：从nage数据库的CharInfo表获取角色详细信息
				CString 查询语句;
				查询语句.Format(_T("SELECT baseskill, Lv, lv + relvC AS total_lv, recount, lvpoint, Str, Dex, Esp, Spt FROM nage.dbo.CharInfo WHERE charName = '%s'"), 角色名);

				TRACE(_T("执行角色信息SQL: %s\n"), 查询语句);

				SQLRETURN retcode = SQLExecDirectW(对话框指针->SQL语句句柄, (SQLWCHAR*)查询语句.GetString(), SQL_NTS);
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
				{
					retcode = SQLFetch(对话框指针->SQL语句句柄);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
					{
						SQLINTEGER 职业代码, 战斗等级, 累计等级, 转生次数, 剩余点数, 力量, 敏捷, 意念, 灵力;

						SQLGetData(对话框指针->SQL语句句柄, 1, SQL_C_LONG, &职业代码, sizeof(职业代码), NULL);
						SQLGetData(对话框指针->SQL语句句柄, 2, SQL_C_LONG, &战斗等级, sizeof(战斗等级), NULL);
						SQLGetData(对话框指针->SQL语句句柄, 3, SQL_C_LONG, &累计等级, sizeof(累计等级), NULL);
						SQLGetData(对话框指针->SQL语句句柄, 4, SQL_C_LONG, &转生次数, sizeof(转生次数), NULL);
						SQLGetData(对话框指针->SQL语句句柄, 5, SQL_C_LONG, &剩余点数, sizeof(剩余点数), NULL);
						SQLGetData(对话框指针->SQL语句句柄, 6, SQL_C_LONG, &力量, sizeof(力量), NULL);
						SQLGetData(对话框指针->SQL语句句柄, 7, SQL_C_LONG, &敏捷, sizeof(敏捷), NULL);
						SQLGetData(对话框指针->SQL语句句柄, 8, SQL_C_LONG, &意念, sizeof(意念), NULL);
						SQLGetData(对话框指针->SQL语句句柄, 9, SQL_C_LONG, &灵力, sizeof(灵力), NULL);

						CString 响应数据;
						响应数据.Format(_T("CHAR_INFO:%d:%d:%d:%d:%d:%d:%d:%d:%d"),
							职业代码, 战斗等级, 累计等级, 转生次数, 剩余点数, 力量, 敏捷, 意念, 灵力);

						对话框指针->发送到客户端(客户端套接字, 响应数据);
						对话框指针->添加信息显示(客户端IP + _T(" 查询角色信息: ") + 角色名);
						TRACE(_T("角色信息查询成功: %s\n"), 响应数据);
					}
					else
					{
						对话框指针->发送到客户端(客户端套接字, _T("CHAR_INFO_FAILED:角色不存在"));
						TRACE(_T("角色不存在: %s\n"), 角色名);
					}
					SQLCloseCursor(对话框指针->SQL语句句柄);
				}
				else
				{
					对话框指针->发送到客户端(客户端套接字, _T("CHAR_INFO_FAILED:查询失败"));
					TRACE(_T("角色信息查询失败\n"));
				}
			}
		}
		//角色列表处理
		else if (客户端请求.Find(_T("GET_ROLES:")) == 0)
		{
			// 处理获取角色列表请求 - 格式: GET_ROLES:username
			CString 用户名 = 客户端请求.Mid(10); 

			// 清理用户名
			用户名.Remove(_T('\r'));
			用户名.Remove(_T('\n'));
			用户名.Trim();

			TRACE(_T("获取角色列表请求，用户名: %s\n"), 用户名);

			// 获取角色列表
			CStringArray 角色列表;
			对话框指针->获取用户角色列表(用户名, 角色列表);

			// 构建响应
			CString 响应数据 = _T("ROLES_LIST:");
			for (int i = 0; i < 角色列表.GetSize(); i++)
			{
				if (i > 0)
					响应数据 += _T(";");
				响应数据 += 角色列表[i];
			}

			TRACE(_T("发送角色列表: %s\n"), 响应数据);
			对话框指针->发送到客户端(客户端套接字, 响应数据);
			对话框指针->添加信息显示(客户端IP + _T(" 请求角色列表 - 用户名: ") + 用户名);
			}
		else if (客户端请求.Find(_T("CHECK_ACCOUNT_ONLINE:")) == 0)
		{
			// 处理检查账号在线状态请求
			CString 前缀 = _T("CHECK_ACCOUNT_ONLINE:");
			CString 用户名 = 客户端请求.Mid(前缀.GetLength());

			用户名.Trim(); // 清理空格
			用户名.Remove(_T('\r')); // 移除回车
			用户名.Remove(_T('\n')); // 移除换行

			TRACE(_T("检查账号在线状态，用户名: [%s]\n"), 用户名);

			// 检查账号在线状态
			BOOL 在线状态 = 对话框指针->检测账号是否在线(用户名);

			// 构建响应
			CString 响应数据;
			if (在线状态)
			{
				响应数据 = _T("ACCOUNT_ONLINE:1");
				TRACE(_T("账号在线: %s\n"), 用户名);
			}
			else
			{
				响应数据 = _T("ACCOUNT_ONLINE:0");
				TRACE(_T("账号离线: %s\n"), 用户名);
			}

			对话框指针->发送到客户端(客户端套接字, 响应数据);
			对话框指针->添加信息显示(客户端IP + _T(" 检查账号在线状态 - 用户: ") + 用户名 + (在线状态 ? _T(" 在线") : _T(" 离线")));
		}
		else if (客户端请求.Find(_T("GET_RANKING:")) == 0)
		{
			// 处理获取排行榜请求 - 格式: GET_RANKING:数量
			CString 数量文本 = 客户端请求.Mid(12); // 去掉"GET_RANKING:"
			int 数量 = _ttoi(数量文本);
			if (数量 <= 0) 数量 = 20; // 默认20个

			TRACE(_T("获取排行榜请求，数量: %d\n"), 数量);

			// 获取排行榜数据
			CString 排行榜数据 = 对话框指针->获取排行榜数据(数量);

			// 构建响应
			CString 响应数据 = _T("RANKING_DATA:") + 排行榜数据;

			TRACE(_T("发送排行榜数据: %s\n"), 响应数据);
			对话框指针->发送到客户端(客户端套接字, 响应数据);
			对话框指针->添加信息显示(客户端IP + _T(" 请求排行榜数据"));
		}
		//网页处理
		else if (客户端请求.Find(_T("WEB_LOGIN:")) == 0)
		{
			TRACE(_T("=== 网页登录请求开始 ===\n"));

			// 处理网页登录请求 - 格式: WEB_LOGIN:username:password
			CString 登录数据 = 客户端请求.Mid(10);
			登录数据.TrimRight(_T("\r\n"));
			TRACE(_T("网页登录数据: %s\n"), 登录数据);

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

				TRACE(_T("网页登录 - 用户: %s, 密码: %s\n"), 用户名, 密码);

				// 验证用户名和密码
				BOOL 登录结果 = 对话框指针->验证用户登录(用户名, 密码);
				TRACE(_T("网页登录验证结果: %d\n"), 登录结果);

				if (登录结果)
				{
					// 获取用户角色列表
					CStringArray 角色列表;
					对话框指针->获取用户角色列表(用户名, 角色列表);

					// 构建响应 - 包含角色列表
					CString 响应数据 = _T("WEB_LOGIN_SUCCESS:");
					for (int i = 0; i < 角色列表.GetSize(); i++)
					{
						if (i > 0)
							响应数据 += _T(";");
						响应数据 += 角色列表[i];
					}

					// 获取用户余额
					int 余额 = 对话框指针->获取用户余额(用户名);
					CString 余额信息;
					余额信息.Format(_T("|%d"), 余额);
					响应数据 += 余额信息;

					对话框指针->发送到客户端(客户端套接字, 响应数据);
					TRACE(_T("网页登录成功，发送角色列表和余额\n"));
					对话框指针->添加信息显示(客户端IP + _T(" 网页登录成功 - 用户名: ") + 用户名);
				}
				else
				{
					对话框指针->发送到客户端(客户端套接字, _T("WEB_LOGIN_FAILED:用户名或密码错误"));
					TRACE(_T("网页登录失败\n"));
				}
			}
			else
			{
				对话框指针->发送到客户端(客户端套接字, _T("WEB_LOGIN_FAILED:无效的登录数据格式"));
				对话框指针->添加信息显示(客户端IP + _T(" 网页登录数据格式错误"));
			}
			TRACE(_T("=== 网页登录请求结束 ===\n"));
			}
		else if (客户端请求.Find(_T("WEB_PURCHASE:")) == 0)
		{
			TRACE(_T("=== 网页购买请求开始 ===\n"));

			// 处理网页购买请求 - 格式: WEB_PURCHASE:username:role:itemId:itemName:price
			CString 购买数据 = 客户端请求.Mid(12);
			购买数据.TrimRight(_T("\r\n"));
			TRACE(_T("网页购买数据: %s\n"), 购买数据);

			CStringArray 参数数组;
			int 起始位置 = 0;
			CString 参数 = 购买数据.Tokenize(_T(":"), 起始位置);

			while (!参数.IsEmpty())
			{
				参数数组.Add(参数);
				参数 = 购买数据.Tokenize(_T(":"), 起始位置);
			}

			if (参数数组.GetSize() >= 5)
			{
				CString 用户名 = 参数数组[0];
				CString 角色名 = 参数数组[1];
				int 物品ID = _ttoi(参数数组[2]);
				CString 物品名称 = 参数数组[3];
				int 价格 = _ttoi(参数数组[4]);

				TRACE(_T("网页购买 - 用户: %s, 角色: %s, 物品ID: %d, 物品: %s, 价格: %d\n"),
					用户名, 角色名, 物品ID, 物品名称, 价格);

				// 处理购买逻辑
				BOOL 购买结果 = 对话框指针->处理网页购买(用户名, 角色名, 物品ID, 物品名称, 价格, 客户端IP);

				if (购买结果)
				{
					对话框指针->发送到客户端(客户端套接字, _T("WEB_PURCHASE_SUCCESS:购买成功"));
					TRACE(_T("网页购买成功\n"));
				}
				else
				{
					对话框指针->发送到客户端(客户端套接字, _T("WEB_PURCHASE_FAILED:购买失败"));
					TRACE(_T("网页购买失败\n"));
				}
			}
			else
			{
				对话框指针->发送到客户端(客户端套接字, _T("WEB_PURCHASE_FAILED:无效的购买数据格式"));
				对话框指针->添加信息显示(客户端IP + _T(" 网页购买数据格式错误"));
			}
			TRACE(_T("=== 网页购买请求结束 ===\n"));
			}
		else if (客户端请求.Find(_T("WEB_GET_BALANCE:")) == 0)
		{
			TRACE(_T("=== 获取余额请求开始 ===\n"));

			// 获取用户余额 - 格式: WEB_GET_BALANCE:username
			CString 用户名 = 客户端请求.Mid(16);
			用户名.TrimRight(_T("\r\n"));
			TRACE(_T("获取余额 - 用户: %s\n"), 用户名);

			int 余额 = 对话框指针->获取用户余额(用户名);
			CString 响应数据;
			响应数据.Format(_T("WEB_BALANCE:%d"), 余额);

			对话框指针->发送到客户端(客户端套接字, 响应数据);
			TRACE(_T("发送余额: %d\n"), 余额);
			TRACE(_T("=== 获取余额请求结束 ===\n"));
}
		
		else
		{
			TRACE(_T("=== 未知请求 ===\n"));
			响应数据 = _T("UNKNOWN_COMMAND");
			对话框指针->添加信息显示(客户端IP + _T(" 未知请求: ") + 客户端请求);
		}

		// 发送响应
		if (需要发送响应 && !响应数据.IsEmpty())
		{
			if (客户端类型 == 2) // WebSocket客户端
			{
				// 创建WebSocket帧并发送
				std::string 响应文本 = CT2A(响应数据.GetString());
				std::vector<char> WebSocket帧 = WebSocket处理器::创建WebSocket帧(响应文本);

				if (!WebSocket帧.empty())
				{
					send(客户端套接字, WebSocket帧.data(), WebSocket帧.size(), 0);
					TRACE(_T("已发送WebSocket响应\n"));
				}
			}
			else // 普通TCP客户端
			{
				// 使用原有的发送方式
				对话框指针->发送到客户端(客户端套接字, 响应数据);
			}
		}

		//处理完请求后短暂休眠，避免过于频繁的循环
		Sleep(10);
	}

	// 清理工作
	对话框指针->移除客户端连接(客户端套接字);

	if (客户端类型 == 2) // WebSocket客户端，发送关闭帧
	{
		std::vector<char> 关闭帧 = WebSocket处理器::创建关闭帧();
		if (!关闭帧.empty())
		{
			send(客户端套接字, 关闭帧.data(), 关闭帧.size(), 0);
		}
	}

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
	连接字符串.Format(_T("DRIVER={SQL Server};SERVER=124.220.82.87;DATABASE=%s;UID=%s;PWD=%s;"),
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
		
		dwSize = sizeof(szValue);
		if (RegQueryValueEx(hKey, _T("DBUser"), NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
		{
			数据库用户名 = szValue;
		}
		
		dwSize = sizeof(szValue);
		if (RegQueryValueEx(hKey, _T("DBPassword"), NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
		{
			数据库密码 = szValue;
		}
		
		dwSize = sizeof(szValue);
		if (RegQueryValueEx(hKey, _T("DBName"), NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
		{
			数据库名称 = szValue;
		}
		
		RegCloseKey(hKey);
		return TRUE;
	}
	return FALSE;
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
	// 使用完整的时间格式，包含日期和时间
	CString 时间信息 = CTime::GetCurrentTime().Format(_T("%H:%M:%S"));
	CString 完整信息 = 时间信息 + _T(" - ") + 信息;

	// 添加到List Box
	CListBox* pListBox = (CListBox*)GetDlgItem(IDC_EDIT_INFO);
	if (pListBox)
	{
		// 始终在末尾添加，使用SetCurSel确保滚动到最后
		int 索引 = pListBox->AddString(完整信息);
		pListBox->SetCurSel(索引); // 滚动到最新添加的项目

		// 如果项目太多，删除最旧的项目,保持最多200条
		const int 最大消息数量 = 200;
		while (pListBox->GetCount() > 最大消息数量)
		{
			pListBox->DeleteString(0); // 删除最旧的消息
		}
	}

	// 写入日志文件
	写入日志文件(信息);
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
	// 检查这个socket是否是WebSocket客户端
	// 这里需要维护一个客户端类型映射表
	// 简化处理：尝试按两种方式发送

	// 先尝试按WebSocket发送
	std::string 文本数据 = CT2A(数据.GetString());
	std::vector<char> WebSocket帧 = WebSocket处理器::创建WebSocket帧(文本数据);

	if (!WebSocket帧.empty())
	{
		int 发送结果 = send(客户端套接字, WebSocket帧.data(), WebSocket帧.size(), 0);
		if (发送结果 != SOCKET_ERROR)
		{
			return TRUE;
		}
	}

	// 如果WebSocket发送失败，尝试普通TCP发送
	return 发送原始数据到客户端(客户端套接字, 数据);
}

// 添加辅助函数
BOOL NageDlqServerDlg::发送原始数据到客户端(SOCKET 客户端套接字, const CString& 数据)
{
	// 原有的发送逻辑
	int 字节长度 = WideCharToMultiByte(CP_UTF8, 0, 数据, -1, NULL, 0, NULL, NULL);
	if (字节长度 > 0)
	{
		char* 字节缓冲区 = new char[字节长度];
		WideCharToMultiByte(CP_UTF8, 0, 数据, -1, 字节缓冲区, 字节长度, NULL, NULL);

		int 发送结果 = send(客户端套接字, 字节缓冲区, 字节长度 - 1, 0);

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

	struct timeval 超时;
	超时.tv_sec = 30;       // 0秒
	超时.tv_usec = 0; // 微秒（0.1秒）
	//setsockopt(客户端套接字, SOL_SOCKET, SO_RCVTIMEO, (char*)&超时, sizeof(超时));

	// 使用select检查是否有数据可读
	fd_set 读集合;
	FD_ZERO(&读集合);
	FD_SET(客户端套接字, &读集合);

	int 选择结果 = select(0, &读集合, NULL, NULL, &超时);

	if (选择结果 == SOCKET_ERROR)
	{
		int 错误码 = WSAGetLastError();
		TRACE(_T("select错误，错误码: %d\n"), 错误码);
		return _T("");
	}
	else if (选择结果 == 0)
	{
		// 超时，没有数据可读
		return _T("");
	}

	int 接收长度 = recv(客户端套接字, 缓冲区, sizeof(缓冲区) - 1, 0);

	if (接收长度 > 0)
	{
		缓冲区[接收长度] = '\0';

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
		if (错误码 == WSAEWOULDBLOCK) 
		{
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

		// 计算新的propid   GM的ID段无法显示角色名称所以新建ID+20000
		int 新的propid = 20000 + 最大ID + 1;

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
	查询语句.Format(_T("SELECT status FROM nagelogin.dbo.Chr_Log_Info WHERE id_loginid = '%s'"), 用户名);

	retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)查询语句.GetString(), SQL_NTS);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		retcode = SQLFetch(SQL语句句柄);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			SQLINTEGER 在线状态;
			SQLGetData(SQL语句句柄, 1, SQL_C_LONG, &在线状态, sizeof(在线状态), NULL);
			SQLCloseCursor(SQL语句句柄);

			TRACE(_T("账号 %s 在线状态: %d\n"), 用户名, 在线状态);
			return (在线状态 == 1); // 1为在线，0为离线
		}
		SQLCloseCursor(SQL语句句柄);
	}

	TRACE(_T("无法获取账号 %s 的在线状态，默认认为在线\n"), 用户名);
	return TRUE; // 如果查询失败，默认认为在线，禁止使用功能
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
	lvpoint = 累计等级 * 1 - 1;
	relvC = 累计等级;
}

// 添加一个函数来处理角色名的空格填充
CString NageDlqServerDlg::处理角色名空格(const CString& 角色名)
{
	TRACE(_T("=== 处理角色名空格开始 ===\n"));
	TRACE(_T("原始角色名: [%s], 长度: %d\n"), 角色名, 角色名.GetLength());

	// 计算字符数（注意：中文字符算2个字节，但在这里我们按Unicode字符计算）
	int 字符数 = 角色名.GetLength();

	// 数据库字段是char(16)，但我们需要按字节计算
	// 在SQL Server中，char(16)表示16个字节，中文字符通常占2个字节

	// 简单处理：如果长度小于16，补充空格
	CString 处理后的角色名 = 角色名;

	// 去除首尾空格
	处理后的角色名.Trim();

	TRACE(_T("去除空格后的角色名: [%s], 长度: %d\n"), 处理后的角色名, 处理后的角色名.GetLength());

	// 计算需要补充的空格数
	// 这里假设中文字符占2个字节，英文字符占1个字节
	int 字节数 = 0;
	for (int i = 0; i < 处理后的角色名.GetLength(); i++)
	{
		TCHAR c = 处理后的角色名[i];
		// 判断是否为中文字符（Unicode范围）
		if (c >= 0x4E00 && c <= 0x9FFF) // 常用汉字范围
		{
			字节数 += 2;
			TRACE(_T("字符[%d]: U+%04X (中文字符，占2字节)\n"), i, (int)c);
		}
		else if (c >= 0x3400 && c <= 0x4DBF) // 扩展A区汉字
		{
			字节数 += 2;
			TRACE(_T("字符[%d]: U+%04X (扩展汉字，占2字节)\n"), i, (int)c);
		}
		else
		{
			字节数 += 1;
			TRACE(_T("字符[%d]: U+%04X '%c' (英文字符，占1字节)\n"), i, (int)c, c);
		}
	}

	TRACE(_T("角色名总字节数: %d\n"), 字节数);

	// 如果字节数小于16，补充空格
	if (字节数 < 16)
	{
		int 需要空格数 = 16 - 字节数;
		TRACE(_T("需要补充 %d 个空格\n"), 需要空格数);

		for (int i = 0; i < 需要空格数; i++)
		{
			处理后的角色名 += _T(' ');
		}
	}
	else if (字节数 > 16)
	{
		TRACE(_T("警告：角色名字节数(%d)超过16，可能会被截断\n"), 字节数);
		// 可以在这里进行截断处理
		处理后的角色名 = 处理后的角色名.Left(8); // 简单截断，可能需要更复杂的处理
		TRACE(_T("截断后的角色名: [%s]\n"), 处理后的角色名);
	}

	TRACE(_T("处理后的角色名: [%s], 显示长度: %d\n"), 处理后的角色名, 处理后的角色名.GetLength());

	// 打印处理后的字符（用于调试）
	TRACE(_T("处理后的字符详情:\n"));
	for (int i = 0; i < 处理后的角色名.GetLength(); i++)
	{
		TCHAR c = 处理后的角色名[i];
		if (c == _T(' '))
			TRACE(_T("  字符[%d]: 空格 (0x%04X)\n"), i, (int)c);
		else
			TRACE(_T("  字符[%d]: U+%04X '%c'\n"), i, (int)c, c);
	}

	TRACE(_T("=== 处理角色名空格结束 ===\n"));
	return 处理后的角色名;
}

// 处理角色转生
BOOL NageDlqServerDlg::处理角色转生(const CString& 用户名, const CString& 角色名)
{
	// 处理角色名空格问题
	CString 处理后的角色名 = 处理角色名空格(角色名);
	TRACE(_T("处理后的角色名: [%s]\n"), 处理后的角色名);

	// 检测账号是否在线
	if (检测账号是否在线(用户名))
	{
		添加信息显示(_T("转生失败: 账号在线或状态检查失败 - ") + 用户名);
		return FALSE;
	}

	SQLRETURN retcode;

	try
	{
		// 查询角色信息
		CString 查询语句;
		查询语句.Format(_T("SELECT Lv, baseskill, relvCtime, lv + relvC AS total_lv, recount FROM nage.dbo.CharInfo WHERE charName = '%s'"), 处理后的角色名);

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

			int 需求等级 = 110 + 转生次数 * 10;

			// 检查等级
			if (当前等级 < 需求等级)
			{
				CString 错误信息;
				错误信息.Format(_T("转生失败: 等级不足%d级（第%d次转生需要%d级）"),
					需求等级, 转生次数 + 1, 需求等级);
				添加信息显示(错误信息);
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
			更新语句.Format(_T("UPDATE nage.dbo.CharInfo SET Lv = %d, Exp = %d, HP = %d, SP = %d, STM = %d, ")
				_T("Str = %d, Dex = %d, Esp = %d, Spt = %d, cmap = %d, lvpoint = %d, ")
				_T("relvC = %d, relvCtime = GETDATE(), recount = %d, Hero = 0 WHERE charName = '%s'"),
				Lv, Exp, HP, SP, STM, Str, Dex, Esp, Spt, cmap, lvpoint, relvC, 转生次数 + 1, 处理后的角色名);

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
	TRACE(_T("加点数值: 力量=%d, 敏捷=%d, 意念=%d, 灵力=%d\n"), 力量, 敏捷, 意念, 灵力);

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
		查询语句.Format(_T("SELECT lvpoint, baseskill FROM nage.dbo.CharInfo WHERE charName = '%s'"), 角色名);

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
			更新语句.Format(_T("UPDATE nage.dbo.CharInfo SET Str = Str + %d, Dex = Dex + %d, Esp = Esp + %d, Spt = Spt + %d, lvpoint = lvpoint - %d WHERE charName = '%s'"),
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
	if (!端口转发列表控件.GetSafeHwnd())
	{
		return;
	}

	// 设置列表控件样式
	端口转发列表控件.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// 清空现有项
	端口转发列表控件.DeleteAllItems();

	// 删除所有列
	int 列数 = 端口转发列表控件.GetHeaderCtrl()->GetItemCount();
	for (int i = 列数 - 1; i >= 0; i--)
	{
		端口转发列表控件.DeleteColumn(i);
	}

	// 添加列
	端口转发列表控件.InsertColumn(0, _T("状态"), LVCFMT_LEFT, 0);
	端口转发列表控件.InsertColumn(1, _T("序号"), LVCFMT_LEFT, 40);
	端口转发列表控件.InsertColumn(2, _T("输入IP"), LVCFMT_LEFT, 100);
	端口转发列表控件.InsertColumn(3, _T("输入端口"), LVCFMT_LEFT, 60);
	端口转发列表控件.InsertColumn(4, _T("输出IP"), LVCFMT_LEFT, 100);
	端口转发列表控件.InsertColumn(5, _T("输出端口"), LVCFMT_LEFT, 60);
	端口转发列表控件.InsertColumn(6, _T("连接数"), LVCFMT_LEFT, 60);

	端口转发列表控件.EnableScrollBar(SB_BOTH, ESB_ENABLE_BOTH);
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

// 双击处理 - 只在特定列允许编辑
void NageDlqServerDlg::On列表项双击(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if (pNMItemActivate)
	{
		int 选中项 = pNMItemActivate->iItem;
		int 选中列 = pNMItemActivate->iSubItem;

		TRACE(_T("双击列表项: 行=%d, 列=%d\n"), 选中项, 选中列);

		// 检查是否在有效范围内
		if (选中项 >= 0 && 选中项 < 端口转发列表控件.GetItemCount())
		{
			// 检查是否是空白行（没有状态的行）
			CString 状态 = 端口转发列表控件.GetItemText(选中项, 0);
			BOOL 是空白行 = 状态.IsEmpty();

			// 只允许编辑IP和端口列（第2、3、4、5列）
			if (选中列 >= 2 && 选中列 <= 5)
			{
				// 如果是空白行，先设置默认值
				if (是空白行)
				{
					设置空白行默认值(选中项);
				}

				开始编辑单元格(选中项, 选中列);
			}
		}
	}


	*pResult = 0;

}

// 设置空白行的默认值
void NageDlqServerDlg::设置空白行默认值(int 行索引)
{
	// 设置默认状态
	端口转发列表控件.SetItemText(行索引, 0, _T("已停止"));

	// 设置默认IP和端口
	端口转发列表控件.SetItemText(行索引, 2, _T("127.0.0.1"));
	端口转发列表控件.SetItemText(行索引, 3, _T("9896"));
	端口转发列表控件.SetItemText(行索引, 4, _T("192.168.100.1"));
	端口转发列表控件.SetItemText(行索引, 5, _T("9896"));
	端口转发列表控件.SetItemText(行索引, 6, _T("0"));
}

// 单击处理 - 防止误操作
void NageDlqServerDlg::On列表单击(NMHDR* pNMHDR, LRESULT* pResult)
{
	// 如果正在编辑，结束编辑
	if (正在编辑)
	{
		结束编辑单元格(TRUE); // 保存更改
	}
	*pResult = 0;
}

// 开始编辑单元格
void NageDlqServerDlg::开始编辑单元格(int 行, int 列)
{
	// 如果已经在编辑，先结束之前的编辑
	if (正在编辑)
	{
		结束编辑单元格(TRUE);
	}

	// 保存当前编辑位置
	当前编辑行 = 行;
	当前编辑列 = 列;
	正在编辑 = TRUE;

	// 获取单元格位置
	CRect 单元格矩形;
	if (列 == 0) // 第一列
	{
		端口转发列表控件.GetItemRect(行, &单元格矩形, LVIR_BOUNDS);
	}
	else // 其他列
	{
		端口转发列表控件.GetSubItemRect(行, 列, LVIR_BOUNDS, 单元格矩形);
	}

	// 调整矩形，避免覆盖边框
	单元格矩形.DeflateRect(2, 2);

	// 创建编辑控件
	if (编辑控件.GetSafeHwnd())
	{
		编辑控件.DestroyWindow();
	}

	编辑控件.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
		单元格矩形, &端口转发列表控件, IDC_EDIT_CONTROL);

	// 设置字体
	CFont* 字体 = 端口转发列表控件.GetFont();
	编辑控件.SetFont(字体);

	// 设置初始文本
	CString 初始文本 = 端口转发列表控件.GetItemText(行, 列);
	编辑控件.SetWindowText(初始文本);

	// 全选文本
	编辑控件.SetSel(0, -1);

	// 显示编辑框
	编辑控件.ShowWindow(SW_SHOW);
	编辑控件.SetFocus();

	TRACE(_T("开始编辑单元格: 行=%d, 列=%d, 文本=%s\n"), 行, 列, 初始文本);
}

// 结束编辑单元格
void NageDlqServerDlg::结束编辑单元格(BOOL 保存更改)
{
	if (正在编辑 && 编辑控件.GetSafeHwnd())
	{
		if (保存更改)
		{
			CString 新文本;
			编辑控件.GetWindowText(新文本);
			新文本.Trim();

			TRACE(_T("结束编辑并保存: 行=%d, 列=%d, 新文本=%s\n"), 当前编辑行, 当前编辑列, 新文本);

			if (!新文本.IsEmpty())
			{
				// 更新列表显示
				端口转发列表控件.SetItemText(当前编辑行, 当前编辑列, 新文本);

				// 更新规则数据
				更新规则数据(当前编辑行, 当前编辑列, 新文本);
			}
		}

		// 销毁编辑控件
		编辑控件.DestroyWindow();

		// 重置状态
		正在编辑 = FALSE;
		当前编辑行 = -1;
		当前编辑列 = -1;
	}
}

// 编辑框失去焦点处理
void NageDlqServerDlg::On编辑框失去焦点()
{
	TRACE(_T("编辑框失去焦点\n"));
	结束编辑单元格(TRUE); // 保存更改
}

// 处理编辑框内容改变（包括回车）
void NageDlqServerDlg::On编辑框内容改变()
{
	// 检查是否按下了回车键
	CString 当前文本;
	编辑控件.GetWindowText(当前文本);

	// 如果文本包含换行符，说明按了回车
	if (当前文本.Find(_T('\r')) != -1 || 当前文本.Find(_T('\n')) != -1)
	{
		// 移除换行符
		当前文本.Remove(_T('\r'));
		当前文本.Remove(_T('\n'));
		当前文本.Trim();

		// 设置清理后的文本
		编辑控件.SetWindowText(当前文本);

		// 结束编辑（相当于单击其他地方）
		结束编辑单元格(TRUE);
	}
}

// 列表结束编辑处理
void NageDlqServerDlg::On列表结束编辑(NMHDR* pNMHDR, LRESULT* pResult)
{
	// 我们使用自定义的编辑控件，所以这里可以留空或做其他处理
	*pResult = 0;
}

// 验证IP地址
BOOL NageDlqServerDlg::验证IP地址(const CString& IP地址)
{
	if (IP地址.IsEmpty())
		return FALSE;

	// 允许localhost
	if (IP地址 == _T("localhost") || IP地址 == _T("127.0.0.1"))
		return TRUE;

	// 简单的IP验证
	CStringArray 部分;
	int 位置 = 0;
	CString 部分文本 = IP地址.Tokenize(_T("."), 位置);

	while (!部分文本.IsEmpty())
	{
		部分.Add(部分文本);
		部分文本 = IP地址.Tokenize(_T("."), 位置);
	}

	if (部分.GetSize() != 4)
		return FALSE;

	for (int i = 0; i < 4; i++)
	{
		int 数字 = _ttoi(部分[i]);
		if (数字 < 0 || 数字 > 255)
			return FALSE;
	}

	return TRUE;
}

// 添加默认转发规则
void NageDlqServerDlg::添加默认转发规则()
{
	// 先检查是否已经有规则
	auto 规则列表 = 端口转发管理器.获取规则列表();
	if (!规则列表.empty())
	{
		return;
	}

	// 添加一条默认规则
	端口转发管理器.添加转发规则(_T("127.0.0.1"), 9896, _T("192.168.100.1"), 9896);

	// 保存配置
	保存端口转发配置();
}

// 刷新端口转发列表
void NageDlqServerDlg::刷新端口转发列表()
{
	if (!端口转发列表控件.GetSafeHwnd())
	{
		return;
	}

	// 结束正在进行的编辑
	if (正在编辑)
	{
		结束编辑单元格(TRUE);
	}

	// 禁用重绘以提高性能
	端口转发列表控件.SetRedraw(FALSE);

	// 清空现有项
	端口转发列表控件.DeleteAllItems();

	// 获取规则列表
	const auto& 规则列表 = 端口转发管理器.获取规则列表();

	// 添加规则到列表
	for (size_t i = 0; i < 规则列表.size(); i++)
	{
		const auto* 规则 = 规则列表[i];

		if (!规则)
		{
			continue;
		}

		// 插入新项
		int 索引 = 端口转发列表控件.InsertItem(i, 规则->状态);

		// 设置各项文本
		CString 文本;
		文本.Format(_T("%d"), 规则->序号);
		端口转发列表控件.SetItemText(索引, 1, 文本);

		端口转发列表控件.SetItemText(索引, 2, 规则->输入IP);

		文本.Format(_T("%d"), 规则->输入端口);
		端口转发列表控件.SetItemText(索引, 3, 文本);

		端口转发列表控件.SetItemText(索引, 4, 规则->输出IP);

		文本.Format(_T("%d"), 规则->输出端口);
		端口转发列表控件.SetItemText(索引, 5, 文本);

		文本.Format(_T("%d"), 规则->连接数);
		端口转发列表控件.SetItemText(索引, 6, 文本);

		// 使用线程安全方法获取连接数
		文本.Format(_T("%d"), 规则->获取连接数());
		端口转发列表控件.SetItemText(索引, 6, 文本);
	}

	// 始终添加一个空白行用于新增
	添加空白行();

	// 启用重绘
	端口转发列表控件.SetRedraw(TRUE);
	端口转发列表控件.Invalidate();
	端口转发列表控件.UpdateWindow();
}

// 添加空白行用于新增规则
void NageDlqServerDlg::添加空白行()
{
	// 获取当前行数
	int 当前行数 = 端口转发列表控件.GetItemCount();

	// 插入空白行
	int 空白行索引 = 端口转发列表控件.InsertItem(当前行数, _T(""));

	// 设置空白行的序号
	CString 序号文本;
	序号文本.Format(_T("%d"), 当前行数 + 1);
	端口转发列表控件.SetItemText(空白行索引, 1, 序号文本);

	// 其他列留空
	端口转发列表控件.SetItemText(空白行索引, 2, _T(""));
	端口转发列表控件.SetItemText(空白行索引, 3, _T(""));
	端口转发列表控件.SetItemText(空白行索引, 4, _T(""));
	端口转发列表控件.SetItemText(空白行索引, 5, _T(""));
	端口转发列表控件.SetItemText(空白行索引, 6, _T(""));
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
		TRACE(_T("=== 调用端口转发管理器加载配置 ===\n"));

		if (端口转发管理器.加载配置())
		{
			int 规则数量 = 端口转发管理器.获取规则列表().size();
			TRACE(_T("配置加载成功，规则数量: %d\n"), 规则数量);

			CString 信息;
			信息.Format(_T("端口转发配置加载成功，%d 条规则"), 规则数量);
			添加信息显示(信息);

			return TRUE;
		}
		else
		{
			TRACE(_T("配置加载失败\n"));
			添加信息显示(_T("端口转发配置加载失败或没有配置"));
			return FALSE;
		}
	}
	catch (const std::exception& e)
	{
		TRACE(_T("加载端口转发配置时发生异常: %s\n"), CString(e.what()));
		添加信息显示(_T("端口转发配置加载异常"));
		return FALSE;
	}
	catch (...)
	{
		TRACE(_T("加载端口转发配置时发生未知异常\n"));
		添加信息显示(_T("端口转发配置加载未知异常"));
		return FALSE;
	}
}

LRESULT NageDlqServerDlg::On延迟加载端口转发数据(WPARAM wParam, LPARAM lParam)
{
	// 检查是否已经有数据
	/*
	std::vector<端口转发规则*> 规则列表副本;
	if (端口转发管理器.获取规则列表副本(规则列表副本))
	{
		if (规则列表副本.empty())
		{
			// 添加默认规则
			添加默认转发规则();
		}
	}

	// 刷新显示
	刷新端口转发列表();
	*/
	TRACE(_T("=== 延迟加载端口转发数据（备用） ===\n"));

	// 作为备用方案，再次确保数据加载
	刷新端口转发列表();
	return 0;
}

void NageDlqServerDlg::On自定义绘制列表(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
	*pResult = CDRF_DODEFAULT;

	switch (pLVCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		*pResult = CDRF_NOTIFYITEMDRAW;
		break;

	case CDDS_ITEMPREPAINT:
		*pResult = CDRF_NOTIFYSUBITEMDRAW;
		break;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
	{
		int 项索引 = static_cast<int>(pLVCD->nmcd.dwItemSpec);
		int 列索引 = pLVCD->iSubItem;

		// 只对状态列（第0列）进行特殊绘制
		if (列索引 == 0)
		{
			CString 状态文本 = 端口转发列表控件.GetItemText(项索引, 0);

			if (状态文本 == _T("运行中"))
			{
				// 绿色背景表示正常运行
				pLVCD->clrText = RGB(0, 0, 0);  // 黑色文字
				pLVCD->clrTextBk = RGB(200, 255, 200);  // 浅绿色背景
			}
			else if (状态文本 == _T("已停止"))
			{
				// 红色背景表示停止
				pLVCD->clrText = RGB(0, 0, 0);  // 黑色文字
				pLVCD->clrTextBk = RGB(255, 200, 200);  // 浅红色背景
			}
		}
		*pResult = CDRF_NEWFONT;
		break;
	}
	}
}

// 更新规则数据
void NageDlqServerDlg::更新规则数据(int 行, int 列, const CString& 新值)
{
	// 获取规则序号
	CString 序号文本 = 端口转发列表控件.GetItemText(行, 1);
	int 规则序号 = _ttoi(序号文本);

	// 获取当前规则的所有字段
	CString 输入IP = 端口转发列表控件.GetItemText(行, 2);
	CString 输入端口文本 = 端口转发列表控件.GetItemText(行, 3);
	CString 输出IP = 端口转发列表控件.GetItemText(行, 4);
	CString 输出端口文本 = 端口转发列表控件.GetItemText(行, 5);

	int 输入端口 = _ttoi(输入端口文本);
	int 输出端口 = _ttoi(输出端口文本);

	// 根据编辑的列更新相应字段
	BOOL 更新成功 = FALSE;
	CString 错误信息;

	switch (列)
	{
	case 2: // 输入IP
		if (验证IP地址(新值))
		{
			输入IP = 新值;
			更新成功 = TRUE;
		}
		else
		{
			错误信息 = _T("IP地址格式无效");
		}
		break;

	case 3: // 输入端口
	{
		int 新端口 = _ttoi(新值);
		if (新端口 > 0 && 新端口 <= 65535)
		{
			输入端口 = 新端口;
			更新成功 = TRUE;
		}
		else
		{
			错误信息 = _T("端口号必须在1-65535之间");
		}
		break;
	}

	case 4: // 输出IP
		if (验证IP地址(新值))
		{
			输出IP = 新值;
			更新成功 = TRUE;
		}
		else
		{
			错误信息 = _T("IP地址格式无效");
		}
		break;

	case 5: // 输出端口
	{
		int 新端口 = _ttoi(新值);
		if (新端口 > 0 && 新端口 <= 65535)
		{
			输出端口 = 新端口;
			更新成功 = TRUE;
		}
		else
		{
			错误信息 = _T("端口号必须在1-65535之间");
		}
		break;
	}
	}

	if (更新成功)
	{
		// 检查是更新现有规则还是新增规则
		auto 规则列表 = 端口转发管理器.获取规则列表();
		BOOL 是现有规则 = (行 < (int)规则列表.size());


		if (是现有规则 && 行 >= 0)
		{
			// 更新现有规则
			端口转发管理器.更新转发规则(规则序号, 输入IP, 输入端口, 输出IP, 输出端口);

			CString 成功信息;
			成功信息.Format(_T("规则 %d 更新成功"), 规则序号);
			添加信息显示(成功信息);
		}
		else
		{
			// 新增规则
			if (端口转发管理器.添加转发规则(输入IP, 输入端口, 输出IP, 输出端口))
			{
				添加信息显示(_T("新规则添加成功"));
				// 刷新显示以确保序号正确
				刷新端口转发列表();
			}
			else
			{
				添加信息显示(_T("新规则添加失败"));
			}
		}

		// 保存配置
		保存端口转发配置();
	}
	else if (!错误信息.IsEmpty())
	{
		// 恢复原值
		CString 原值 = 端口转发列表控件.GetItemText(行, 列);
		端口转发列表控件.SetItemText(行, 列, 原值);
		添加信息显示(错误信息);
	}
}

// 添加新规则行
void NageDlqServerDlg::添加新规则行()
{
	// 结束任何正在进行的编辑
	结束编辑单元格(TRUE);

	// 获取当前行数
	int 当前行数 = 端口转发列表控件.GetItemCount();

	// 插入新行
	int 新行索引 = 端口转发列表控件.InsertItem(当前行数, _T("已停止"));

	// 设置序号
	CString 序号文本;
	序号文本.Format(_T("%d"), 当前行数 + 1);
	端口转发列表控件.SetItemText(新行索引, 1, 序号文本);

	// 设置默认值
	端口转发列表控件.SetItemText(新行索引, 2, _T("127.0.0.1"));
	端口转发列表控件.SetItemText(新行索引, 3, _T("9896"));
	端口转发列表控件.SetItemText(新行索引, 4, _T("192.168.100.1"));
	端口转发列表控件.SetItemText(新行索引, 5, _T("9896"));
	端口转发列表控件.SetItemText(新行索引, 6, _T("0"));

	// 自动开始编辑第一列可编辑的字段（输入IP）
	开始编辑单元格(新行索引, 2);
}

// 处理新增规则
void NageDlqServerDlg::处理新增规则(int 行)
{
	// 获取行数据
	CString 输入IP = 端口转发列表控件.GetItemText(行, 2);
	CString 输入端口文本 = 端口转发列表控件.GetItemText(行, 3);
	CString 输出IP = 端口转发列表控件.GetItemText(行, 4);
	CString 输出端口文本 = 端口转发列表控件.GetItemText(行, 5);

	int 输入端口 = _ttoi(输入端口文本);
	int 输出端口 = _ttoi(输出端口文本);

	// 验证数据
	if (!验证IP地址(输入IP) || !验证IP地址(输出IP) ||
		输入端口 <= 0 || 输入端口 > 65535 ||
		输出端口 <= 0 || 输出端口 > 65535)
	{
		添加信息显示(_T("新增规则数据无效，请检查IP和端口"));

		// 删除无效行
		端口转发列表控件.DeleteItem(行);
		return;
	}

	// 添加到管理器
	if (端口转发管理器.添加转发规则(输入IP, 输入端口, 输出IP, 输出端口))
	{
		// 保存配置
		保存端口转发配置();
		添加信息显示(_T("新规则添加成功"));

		// 刷新显示
		刷新端口转发列表();
	}
	else
	{
		添加信息显示(_T("新规则添加失败"));
		端口转发列表控件.DeleteItem(行);
	}
}

// 右键菜单功能
void NageDlqServerDlg::On右键菜单(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	// 检查是否点击在有效项上
	if (pNMItemActivate->iItem >= 0)
	{
		// 结束任何正在进行的编辑
		结束编辑单元格(TRUE);

		// 创建右键菜单
		if (右键菜单.GetSafeHmenu())
			右键菜单.DestroyMenu();

		右键菜单.CreatePopupMenu();
		右键菜单.AppendMenu(MF_STRING, ID_MENU_DELETE_RULE, _T("删除规则"));

		// 获取鼠标位置并显示菜单
		CPoint 鼠标位置;
		GetCursorPos(&鼠标位置);

		// 设置当前选中项
		端口转发列表控件.SetItemState(pNMItemActivate->iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		端口转发列表控件.SetSelectionMark(pNMItemActivate->iItem);

		// 显示菜单
		右键菜单.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, 鼠标位置.x, 鼠标位置.y, this);
	}

	*pResult = 0;
}

// 删除规则功能
void NageDlqServerDlg::On删除规则()
{
	// 获取当前选中项
	int 选中项 = 端口转发列表控件.GetSelectionMark();

	if (选中项 < 0)
	{
		// 如果没有选中项，尝试获取第一个选中项
		POSITION 位置 = 端口转发列表控件.GetFirstSelectedItemPosition();
		if (位置 == NULL)
		{
			AfxMessageBox(_T("请先选择要删除的规则"));
			return;
		}
		选中项 = 端口转发列表控件.GetNextSelectedItem(位置);
	}

	if (选中项 >= 0)
	{
		// 获取规则序号
		CString 序号文本 = 端口转发列表控件.GetItemText(选中项, 1);
		int 规则序号 = _ttoi(序号文本);

		// 获取规则详情用于确认对话框
		CString 输入IP = 端口转发列表控件.GetItemText(选中项, 2);
		CString 输入端口 = 端口转发列表控件.GetItemText(选中项, 3);
		CString 输出IP = 端口转发列表控件.GetItemText(选中项, 4);
		CString 输出端口 = 端口转发列表控件.GetItemText(选中项, 5);

		CString 确认信息;
		确认信息.Format(_T("确定要删除以下规则吗？\n\n规则 %d: %s:%s → %s:%s"),
			规则序号, 输入IP, 输入端口, 输出IP, 输出端口);

		if (AfxMessageBox(确认信息, MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			// 删除规则
			if (端口转发管理器.删除转发规则(规则序号))
			{
				// 保存配置
				保存端口转发配置();

				// 刷新显示
				刷新端口转发列表();

				CString 成功信息;
				成功信息.Format(_T("规则 %d 删除成功"), 规则序号);
				添加信息显示(成功信息);

				AfxMessageBox(_T("规则删除成功"), MB_OK | MB_ICONINFORMATION);
			}
			else
			{
				AfxMessageBox(_T("规则删除失败"), MB_OK | MB_ICONERROR);
			}
		}
	}
	else
	{
		AfxMessageBox(_T("请先选择要删除的规则"));
	}
}

// 初始化日志文件
BOOL NageDlqServerDlg::初始化日志文件()
{
	// 创建日志目录
	CString 日志目录 = _T(".\\logs\\");
	if (!PathFileExists(日志目录))
	{
		if (!CreateDirectory(日志目录, NULL))
		{
			TRACE(_T("创建日志目录失败\n"));
			return FALSE;
		}
	}

	// 检查并修复现有日志文件的编码
	CString 今日日志文件 = 生成日志文件名();
	if (PathFileExists(今日日志文件))
	{
		修复日志文件编码(今日日志文件);
	}

	return 创建日志文件();
}

// 创建日志文件  使用UTF-8编码
BOOL NageDlqServerDlg::创建日志文件()
{
	// 关闭已打开的文件
	if (日志文件已打开)
	{
		关闭日志文件();
	}

	// 生成新的日志文件名
	当前日志文件名 = 生成日志文件名();

	try
	{
		// 尝试打开日志文件
		CFileException 文件异常;
		if (日志文件.Open(当前日志文件名,
			CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareDenyNone,
			&文件异常))
		{
			// 移动到文件末尾
			日志文件.SeekToEnd();

			// 检查文件是否为空（新文件）
			if (日志文件.GetLength() == 0)
			{
				// 写入UTF-8 BOM头
				BYTE utf8BOM[] = { 0xEF, 0xBB, 0xBF };
				日志文件.Write(utf8BOM, sizeof(utf8BOM));

				CString 文件头;
				文件头.Format(_T("=== Nage服务器日志 ===\r\n")
					_T("启动时间: %s\r\n")
					_T("==========================================\r\n\r\n"),
					CTime::GetCurrentTime().Format(_T("%Y-%m-%d %H:%M:%S")));

				// 使用安全的UTF-8转换
				int 所需长度 = WideCharToMultiByte(CP_UTF8, 0, 文件头, -1, NULL, 0, NULL, NULL);
				if (所需长度 > 0)
				{
					CStringA utf8文件头;
					char* 缓冲区 = utf8文件头.GetBuffer(所需长度);
					WideCharToMultiByte(CP_UTF8, 0, 文件头, -1, 缓冲区, 所需长度, NULL, NULL);
					utf8文件头.ReleaseBuffer();

					日志文件.Write(utf8文件头, utf8文件头.GetLength());
					日志文件.Flush();
				}
			}

			日志文件已打开 = TRUE;
			return TRUE;
		}
		else
		{
			TRACE(_T("无法创建日志文件: %s, 错误代码: %d\n"),
				当前日志文件名, 文件异常.m_cause);
			return FALSE;
		}
	}
	catch (...)
	{
		TRACE(_T("创建日志文件时发生异常\n"));
		return FALSE;
	}
}

// 关闭日志文件
void NageDlqServerDlg::关闭日志文件()
{
	if (日志文件已打开 && 日志文件.m_hFile != CFile::hFileNull)
	{
		try
		{
			CString 结束信息 = _T("\r\n=== 会话结束 ===\r\n\r\n");
			CT2A utf8结束信息(结束信息, CP_UTF8);
			日志文件.Write(utf8结束信息, strlen(utf8结束信息));
			日志文件.Flush();
			日志文件.Close();
		}
		catch (...)
		{
			// 忽略关闭时的异常
			TRACE(_T("关闭日志文件时发生异常\n"));
		}
		日志文件已打开 = FALSE;
	}
}

// 生成日志文件名
CString NageDlqServerDlg::生成日志文件名()
{
	CString 日志文件名;
	日志文件名.Format(_T(".\\logs\\server_%s.log"),
		CTime::GetCurrentTime().Format(_T("%Y%m%d")));
	return 日志文件名;
}

// 写入日志文件 - 使用UTF-8编码
void NageDlqServerDlg::写入日志文件(const CString& 信息)
{
	// 检查是否需要创建新的日志文件（按日期）
	CString 新日志文件名 = 生成日志文件名();
	if (新日志文件名 != 当前日志文件名)
	{
		// 日期已变化，创建新的日志文件
		创建日志文件();
	}

	// 确保日志文件已打开且有效
	if (!日志文件已打开 || 日志文件.m_hFile == CFile::hFileNull)
	{
		// 尝试重新打开
		if (!创建日志文件())
		{
			TRACE(_T("无法打开日志文件进行写入: %s\n"), 信息);
			return;
		}
	}

	try
	{
		// 构建带时间戳的完整信息
		CString 时间戳信息 = CTime::GetCurrentTime().Format(_T("[%Y-%m-%d %H:%M:%S] ")) + 信息 + _T("\r\n");

		// 使用安全的UTF-8转换方法
		int 所需长度 = WideCharToMultiByte(CP_UTF8, 0, 时间戳信息, -1, NULL, 0, NULL, NULL);
		if (所需长度 > 0)
		{
			CStringA utf8信息;
			char* 缓冲区 = utf8信息.GetBuffer(所需长度);
			WideCharToMultiByte(CP_UTF8, 0, 时间戳信息, -1, 缓冲区, 所需长度, NULL, NULL);
			utf8信息.ReleaseBuffer();

			// 写入文件
			日志文件.Write(utf8信息, utf8信息.GetLength());
			日志文件.Flush();
		}
	}
	catch (CFileException* e)
	{
		TRACE(_T("日志写入失败，错误代码: %d\n"), e->m_cause);
		e->Delete();
		日志文件已打开 = FALSE;
	}
	catch (...)
	{
		TRACE(_T("日志写入发生未知异常\n"));
		日志文件已打开 = FALSE;
	}
}

// 检测文件编码并修复
BOOL NageDlqServerDlg::修复日志文件编码(const CString& 文件名)
{
	try
	{
		CFile 文件;
		if (文件.Open(文件名, CFile::modeRead))
		{
			BYTE 头信息[3];
			ULONGLONG 文件大小 = 文件.GetLength();

			if (文件大小 >= 3)
			{
				文件.Read(头信息, 3);

				// 检查是否是UTF-8 BOM
				if (头信息[0] == 0xEF && 头信息[1] == 0xBB && 头信息[2] == 0xBF)
				{
					文件.Close();
					return TRUE; // 已经是UTF-8
				}
			}
			文件.Close();

			// 重新创建文件并添加UTF-8 BOM
			CStdioFile 新文件;
			if (新文件.Open(文件名, CFile::modeCreate | CFile::modeWrite))
			{
				BYTE utf8BOM[] = { 0xEF, 0xBB, 0xBF };
				新文件.Write(utf8BOM, sizeof(utf8BOM));
				新文件.Close();
				return TRUE;
			}
		}
	}
	catch (...)
	{
		TRACE(_T("修复日志文件编码失败: %s\n"), 文件名);
	}

	return FALSE;
}

// 添加获取角色列表函数
void NageDlqServerDlg::获取用户角色列表(const CString& 用户名, CStringArray& 角色列表)
{
	角色列表.RemoveAll();
	SQLRETURN retcode;

	// 清理用户名：去除换行符、回车符和首尾空格
	CString 清理后的用户名 = 用户名;
	清理后的用户名.Remove(_T('\r'));
	清理后的用户名.Remove(_T('\n'));
	清理后的用户名.Trim();

	TRACE(_T("开始获取用户角色列表，原始用户名: [%s], 清理后: [%s]\n"), 用户名, 清理后的用户名);

	// 跨数据库查询：从nagelogin的CharName表获取角色名列表
	CString 查询语句;
	查询语句.Format(_T("SELECT charname FROM nagelogin.dbo.CharName WHERE id_loginid = '%s'"), 清理后的用户名);

	TRACE(_T("执行角色列表SQL: %s\n"), 查询语句);

	retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)查询语句.GetString(), SQL_NTS);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		int 角色数量 = 0;
		while (SQLFetch(SQL语句句柄) == SQL_SUCCESS)
		{
			SQLWCHAR 角色名[256];
			SQLLEN 角色名长度;

			SQLGetData(SQL语句句柄, 1, SQL_C_WCHAR, 角色名, sizeof(角色名), &角色名长度);
			if (角色名长度 != SQL_NULL_DATA)
			{
				CString 角色名字符串(角色名);
				// 清理角色名中的特殊字符
				角色名字符串.Remove(_T('\r'));
				角色名字符串.Remove(_T('\n'));
				角色名字符串.Trim();

				角色列表.Add(角色名字符串);
				角色数量++;
				TRACE(_T("找到角色: [%s]\n"), 角色名字符串);
			}
		}
		SQLCloseCursor(SQL语句句柄);

		TRACE(_T("共找到 %d 个角色\n"), 角色数量);

		if (角色数量 == 0)
		{
			TRACE(_T("警告：用户 [%s] 在CharName表中没有找到角色\n"), 清理后的用户名);

			// 调试：检查数据库中是否存在该用户
			CString 调试查询;
			调试查询.Format(_T("SELECT COUNT(*) FROM nagelogin.dbo.CharName WHERE id_loginid = '%s'"), 清理后的用户名);
			retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)调试查询.GetString(), SQL_NTS);
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				retcode = SQLFetch(SQL语句句柄);
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
				{
					SQLINTEGER 用户数量;
					SQLGetData(SQL语句句柄, 1, SQL_C_LONG, &用户数量, sizeof(用户数量), NULL);
					TRACE(_T("调试：CharName表中用户 %s 的记录数: %d\n"), 清理后的用户名, 用户数量);
				}
				SQLCloseCursor(SQL语句句柄);
			}
		}
	}
	else
	{
		TRACE(_T("查询角色列表失败\n"));
		// 获取错误信息
		SQLWCHAR sqlState[6], message[SQL_MAX_MESSAGE_LENGTH];
		SQLINTEGER nativeError;
		SQLSMALLINT msgLen;
		SQLGetDiagRecW(SQL_HANDLE_STMT, SQL语句句柄, 1, sqlState, &nativeError,
			message, SQL_MAX_MESSAGE_LENGTH, &msgLen);
		TRACE(_T("SQL错误: %s - %s\n"), CString(sqlState), CString(message));
	}
}

CString NageDlqServerDlg::获取排行榜数据(int 数量)
{
	CString 排行榜数据;
	SQLRETURN retcode;

	TRACE(_T("开始获取排行榜数据，数量: %d\n"), 数量);

	// 修改排序方式：从 DESC（降序）改为 ASC（升序）
	CString 查询语句;
	查询语句.Format(_T("SELECT TOP %d charName, baseskill, recount, Lv, relvC, (Lv + relvC) AS total_level ")
		_T("FROM nage.dbo.CharInfo ")
		_T("ORDER BY total_level DESC"), 数量);  // 保持 DESC 降序

	TRACE(_T("执行排行榜SQL: %s\n"), 查询语句);

	retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)查询语句.GetString(), SQL_NTS);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		int 记录数量 = 0;
		while (SQLFetch(SQL语句句柄) == SQL_SUCCESS)
		{
			SQLWCHAR 角色名[256];
			SQLINTEGER 职业代码, 转生次数, 等级, 转生等级, 累计等级;
			SQLLEN 角色名长度, 职业代码长度, 转生次数长度, 等级长度, 转生等级长度;

			SQLGetData(SQL语句句柄, 1, SQL_C_WCHAR, 角色名, sizeof(角色名), &角色名长度);
			SQLGetData(SQL语句句柄, 2, SQL_C_LONG, &职业代码, sizeof(职业代码), &职业代码长度);
			SQLGetData(SQL语句句柄, 3, SQL_C_LONG, &转生次数, sizeof(转生次数), &转生次数长度);
			SQLGetData(SQL语句句柄, 4, SQL_C_LONG, &等级, sizeof(等级), &等级长度);
			SQLGetData(SQL语句句柄, 5, SQL_C_LONG, &转生等级, sizeof(转生等级), &转生等级长度);
			SQLGetData(SQL语句句柄, 6, SQL_C_LONG, &累计等级, sizeof(累计等级), NULL);

			if (角色名长度 != SQL_NULL_DATA)
			{
				// 添加分隔符（第一个数据前不加）
				if (!排行榜数据.IsEmpty())
					排行榜数据 += _T("|");

				// 格式：角色名,职业代码,转生次数,累计等级
				CString 角色数据;
				角色数据.Format(_T("%s,%d,%d,%d"),
					CString(角色名), 职业代码, 转生次数, 累计等级);

				排行榜数据 += 角色数据;
				记录数量++;
			}
		}
		SQLCloseCursor(SQL语句句柄);
		TRACE(_T("排行榜查询成功，找到 %d 条记录\n"), 记录数量);
	}
	else
	{
		TRACE(_T("获取排行榜数据失败\n"));
		// 获取错误信息
		SQLWCHAR sqlState[6], message[SQL_MAX_MESSAGE_LENGTH];
		SQLINTEGER nativeError;
		SQLSMALLINT msgLen;
		SQLGetDiagRecW(SQL_HANDLE_STMT, SQL语句句柄, 1, sqlState, &nativeError,
			message, SQL_MAX_MESSAGE_LENGTH, &msgLen);
		TRACE(_T("SQL错误: %s - %s\n"), CString(sqlState), CString(message));
	}

	TRACE(_T("最终排行榜数据: %s\n"), 排行榜数据);
	return 排行榜数据;
}

void NageDlqServerDlg::停止所有后台操作()
{
	TRACE(_T("=== 停止所有后台操作开始 ===\n"));

	// 1. 停止服务器
	if (服务器运行状态)
	{
		TRACE(_T("停止服务器...\n"));
		停止服务器();
		服务器运行状态 = FALSE;
	}

	// 2. 停止端口转发
	TRACE(_T("停止端口转发...\n"));
	安全停止端口转发();

	// 3. 停止定时器
	if (端口转发刷新定时器 != 0)
	{
		TRACE(_T("停止定时器...\n"));
		KillTimer(端口转发刷新定时器);
		端口转发刷新定时器 = 0;
	}

	// 4. 关闭日志文件
	TRACE(_T("关闭日志文件...\n"));
	关闭日志文件();

	// 5. 等待所有线程结束（可选）
	Sleep(100); // 给线程一点时间清理

	TRACE(_T("=== 停止所有后台操作完成 ===\n"));
}

// 获取用户余额
int NageDlqServerDlg::获取用户余额(const CString& 用户名)
{
	SQLRETURN retcode;
	int 余额 = 0;

	try
	{
		// 假设有一个用户余额表 UserBalance
		CString 查询语句;
		查询语句.Format(_T("SELECT balance FROM nagelogin.dbo.UserBalance WHERE username = '%s'"), 用户名);

		retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)查询语句.GetString(), SQL_NTS);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			retcode = SQLFetch(SQL语句句柄);
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				SQLGetData(SQL语句句柄, 1, SQL_C_LONG, &余额, sizeof(余额), NULL);
				TRACE(_T("获取用户 %s 余额: %d\n"), 用户名, 余额);
			}
			else
			{
				// 如果没有记录，创建默认记录
				余额 = 1000; // 默认余额
				TRACE(_T("用户 %s 没有余额记录，使用默认值: %d\n"), 用户名, 余额);
			}
			SQLCloseCursor(SQL语句句柄);
		}
		else
		{
			TRACE(_T("查询用户余额失败\n"));
			余额 = 1000; // 默认余额
		}
	}
	catch (...)
	{
		TRACE(_T("获取用户余额时发生异常\n"));
		余额 = 1000; // 默认余额
	}

	return 余额;
}

// 处理网页购买
BOOL NageDlqServerDlg::处理网页购买(const CString& 用户名, const CString& 角色名,
	int 物品ID, const CString& 物品名称,
	int 价格, const CString& 客户端IP)
{
	SQLRETURN retcode;

	try
	{
		// 1. 检查用户余额是否足够
		int 当前余额 = 获取用户余额(用户名);
		if (当前余额 < 价格)
		{
			添加信息显示(_T("网页购买失败: 余额不足 - 用户: ") + 用户名);
			return FALSE;
		}

		// 2. 获取角色ID (recv_charid)
		int 角色ID = 0;
		CString 查询角色ID语句;
		查询角色ID语句.Format(_T("SELECT charpropid FROM nagelogin.dbo.CharName WHERE charname = '%s'"), 角色名);

		retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)查询角色ID语句.GetString(), SQL_NTS);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			retcode = SQLFetch(SQL语句句柄);
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				SQLGetData(SQL语句句柄, 1, SQL_C_LONG, &角色ID, sizeof(角色ID), NULL);
				TRACE(_T("获取角色ID成功: 角色 %s 的ID为 %d\n"), 角色名, 角色ID);
			}
			else
			{
				TRACE(_T("获取角色ID失败: 角色 %s 不存在\n"), 角色名);
				添加信息显示(_T("网页购买失败: 角色不存在 - ") + 角色名);
				SQLCloseCursor(SQL语句句柄);
				return FALSE;
			}
			SQLCloseCursor(SQL语句句柄);
		}
		else
		{
			TRACE(_T("查询角色ID失败\n"));
			添加信息显示(_T("网页购买失败: 查询角色信息错误"));
			return FALSE;
		}

		// 3. 扣除余额
		CString 更新余额语句;
		int 新余额 = 当前余额 - 价格;
		更新余额语句.Format(_T("UPDATE nagelogin.dbo.UserBalance SET balance = %d WHERE username = '%s'"),
			新余额, 用户名);

		retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)更新余额语句.GetString(), SQL_NTS);
		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			// 如果更新失败，尝试插入新记录
			CString 插入余额语句;
			插入余额语句.Format(_T("INSERT INTO nagelogin.dbo.UserBalance (username, balance) VALUES ('%s', %d)"),
				用户名, 新余额);
			SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)插入余额语句.GetString(), SQL_NTS);
		}

		// 4. 插入到Item_to_Game表（发送到伊甸园领取箱）
		CString 插入物品语句;
		插入物品语句.Format(_T("INSERT INTO nagelogin.dbo.Item_to_Game ")
			_T("(recv_charid, recv_name, recv_serv, recv_time, recv_loginid, itemid, period, is_status) ")
			_T("VALUES (%d, '%s', 11, GETDATE(), '%s', %d, 0, '0')"),
			角色ID, 角色名, 用户名, 物品ID);

		retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)插入物品语句.GetString(), SQL_NTS);
		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			// 获取错误信息
			SQLWCHAR sqlState[6], message[SQL_MAX_MESSAGE_LENGTH];
			SQLINTEGER nativeError;
			SQLSMALLINT msgLen;
			SQLGetDiagRecW(SQL_HANDLE_STMT, SQL语句句柄, 1, sqlState, &nativeError,
				message, SQL_MAX_MESSAGE_LENGTH, &msgLen);

			CString 错误信息;
			错误信息.Format(_T("插入Item_to_Game表失败: %s"), CString(message));
			添加信息显示(错误信息);

			// 回滚余额扣除
			CString 回滚余额语句;
			回滚余额语句.Format(_T("UPDATE nagelogin.dbo.UserBalance SET balance = %d WHERE username = '%s'"),
				当前余额, 用户名);
			SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)回滚余额语句.GetString(), SQL_NTS);

			SQLEndTran(SQL_HANDLE_DBC, SQL连接句柄, SQL_ROLLBACK);
			return FALSE;
		}

		// 5. 记录购买日志
		CString 插入日志语句;
		插入日志语句.Format(_T("INSERT INTO nagelogin.dbo.PurchaseLog ")
			_T("(username, charname, itemid, itemname, price, purchase_time, client_ip) ")
			_T("VALUES ('%s', '%s', %d, '%s', %d, GETDATE(), '%s')"),
			用户名, 角色名, 物品ID, 物品名称, 价格, 客户端IP);

		retcode = SQLExecDirectW(SQL语句句柄, (SQLWCHAR*)插入日志语句.GetString(), SQL_NTS);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			SQLEndTran(SQL_HANDLE_DBC, SQL连接句柄, SQL_COMMIT);

			CString 成功信息;
			成功信息.Format(_T("网页购买成功: 用户 %s 为角色 %s 购买 %s (价格: %d)，物品已发送到伊甸园领取箱"),
				用户名, 角色名, 物品名称, 价格);
			添加信息显示(成功信息);

			return TRUE;
		}
		else
		{
			SQLEndTran(SQL_HANDLE_DBC, SQL连接句柄, SQL_ROLLBACK);
			添加信息显示(_T("网页购买失败: 记录日志错误"));
			return FALSE;
		}
	}
	catch (...)
	{
		SQLEndTran(SQL_HANDLE_DBC, SQL连接句柄, SQL_ROLLBACK);
		添加信息显示(_T("网页购买失败: 发生未知错误"));
		return FALSE;
	}
}

// 获取物品游戏代码
CString NageDlqServerDlg::获取物品游戏代码(int 物品ID)
{
	// 这里应该根据物品ID从数据库获取游戏内物品代码
	// 这是一个示例映射，您需要根据实际情况调整

	std::map<int, CString> 物品映射;
	物品映射[1001] = _T("ITEM_HORSE_LEGENDARY");  // 传说级战马
	物品映射[1002] = _T("ITEM_SHIELD_DRAGON");    // 巨龙之盾
	物品映射[1003] = _T("ITEM_GEM_EXP");          // 经验加成宝石
	物品映射[1004] = _T("ITEM_SET_ASSASSIN");     // 幻影刺客套装
	物品映射[1005] = _T("ITEM_POTION_PACK");      // 生命恢复药水礼包
	物品映射[1006] = _T("ITEM_HOUSE_LUXURY");     // 豪华个人住宅

	auto it = 物品映射.find(物品ID);
	if (it != 物品映射.end())
	{
		return it->second;
	}

	return _T("");
}
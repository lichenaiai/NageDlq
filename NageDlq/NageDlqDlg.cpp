// NageDlqDlg.cpp
#include "pch.h"
#include "framework.h"
#include "NageDlq.h"
#include "NageDlqDlg.h"
#include "afxdialogex.h"
#include "注册页面类.h"
#include "网络通信类.h"
#include "登录页面类.h"
#include "转生页面类.h"
#include "加点页面类.h"
#include "排行榜页面类.h"
#include "注入页面类.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// NageDlqDlg 对话框

NageDlqDlg::NageDlqDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_NAGEDLQ_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}
NageDlqDlg::~NageDlqDlg()
{
}

void NageDlqDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_MAIN, 分页控件);
}

// 消息映射
BEGIN_MESSAGE_MAP(NageDlqDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_MAIN, &NageDlqDlg::OnTcnSelchangeTabMain)
	ON_MESSAGE(WM_USER + 100, &NageDlqDlg::OnNetworkMessage)
	ON_WM_TIMER()
END_MESSAGE_MAP()
IMPLEMENT_DYNAMIC(NageDlqDlg, CDialogEx)

// 初始化函数
BOOL NageDlqDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	// 设置窗口大小 850x600
	MoveWindow(0, 0, 850, 622);

	//初始化分页控件();
	// 初始化分页控件
	if (!初始化分页控件())
	{
		MessageBox(_T("初始化分页控件失败"), _T("错误"), MB_ICONERROR);
		EndDialog(-1);
		return FALSE;
	}

	/*
	// 初始化网络通信
	if (!初始化网络通信())
	{
		MessageBox(_T("网络初始化失败，部分功能可能无法使用"), _T("警告"), MB_ICONWARNING);
	}

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
	*/
	// 同步初始化网络（在主线程中）
	if (!初始化网络通信())
	{
		// 使用TRACE而不是弹窗，避免阻塞
		TRACE(_T("网络初始化失败，使用本地功能\n"));
	}
	
	// 延迟初始化网络，避免在对话框完全初始化前操作
	SetTimer(1, 100, NULL);
	return TRUE;
}

void NageDlqDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1)  // 延迟初始化定时器
	{
		KillTimer(1);
		TRACE(_T("=== 延迟初始化网络通信 ===\n"));
		if (!初始化网络通信())
		{
			TRACE(_T("网络初始化失败，使用本地功能\n"));
		}
	}
	else if (nIDEvent == 2)  // 重试发送连接请求定时器
	{
		// 检查是否已经连接成功
		if (网络通信.是否已连接())
		{
			TRACE(_T("已经连接成功，停止重试定时器\n"));
			KillTimer(2);
			return;
		}

		TRACE(_T("重试发送连接请求\n"));

		CString 连接请求;
		连接请求.Format(_T("CONNECT:%s:%s"), _T(CLIENT_VERSION), _T(SERVER_IP));

		if (网络通信.发送数据(连接请求))
		{
			TRACE(_T("连接请求发送成功\n"));
		}
		else
		{
			TRACE(_T("连接请求发送失败，继续重试\n"));
			// 继续重试，不停止定时器
		}
	}

	CDialogEx::OnTimer(nIDEvent);
}

BOOL NageDlqDlg::初始化分页控件()
{
	// 添加分页标签
	分页控件.InsertItem(0, _T("登录"));
	分页控件.InsertItem(1, _T("注册"));
	分页控件.InsertItem(2, _T("转生"));
	分页控件.InsertItem(3, _T("加点"));
	分页控件.InsertItem(4, _T("排行榜"));
	分页控件.InsertItem(5, _T("动态功能"));

	// 创建各个页面
	登录页面.Create(IDD_PAGE_LOGIN, &分页控件);
	注册页面.Create(IDD_PAGE_REGISTER, &分页控件);
	转生页面.Create(IDD_PAGE_REBIRTH, &分页控件);
	加点页面.Create(IDD_PAGE_STATS, &分页控件);
	排行榜页面.Create(IDD_PAGE_RANKING, &分页控件);
	注入页面.Create(IDD_PAGE_INJECTION, &分页控件);

	// 设置页面位置和大小
	CRect rect;
	分页控件.GetClientRect(&rect);
	rect.top += 25;
	rect.bottom -= 2;
	rect.left += 0;
	rect.right -= 2;

	登录页面.MoveWindow(&rect);
	注册页面.MoveWindow(&rect);
	转生页面.MoveWindow(&rect);
	加点页面.MoveWindow(&rect);
	排行榜页面.MoveWindow(&rect);
	注入页面.MoveWindow(&rect);

	// 显示登录页面
	登录页面.ShowWindow(SW_SHOW);
	分页控件.SetCurSel(0);
	return TRUE;
}

void NageDlqDlg::OnTcnSelchangeTabMain(NMHDR* pNMHDR, LRESULT* pResult)
{
	// 隐藏所有页面
	登录页面.ShowWindow(SW_HIDE);
	注册页面.ShowWindow(SW_HIDE);
	转生页面.ShowWindow(SW_HIDE);
	加点页面.ShowWindow(SW_HIDE);
	排行榜页面.ShowWindow(SW_HIDE);
	注入页面.ShowWindow(SW_HIDE);

	// 显示选中的页面
	int 当前选中页 = 分页控件.GetCurSel();
	switch (当前选中页)
	{
	case 0: 登录页面.ShowWindow(SW_SHOW); break;
	case 1: 注册页面.ShowWindow(SW_SHOW); break;
	case 2: 转生页面.ShowWindow(SW_SHOW); 转生页面.刷新角色列表(); break;	// 切换到转生页面时刷新角色列表
	case 3: 加点页面.ShowWindow(SW_SHOW); 加点页面.刷新角色列表(); break;	// 切换到加点页面时刷新角色列表
	case 4: 排行榜页面.ShowWindow(SW_SHOW); 排行榜页面.刷新排行榜数据(); break;	// 切换到排行榜页面时刷新数据
	case 5: 注入页面.ShowWindow(SW_SHOW); break;
	}

	*pResult = 0;
}

void NageDlqDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 当用户拖动最小化窗口时系统调用此函数取得光标显示。
HCURSOR NageDlqDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// 实现消息处理函数
LRESULT NageDlqDlg::OnNetworkMessage(WPARAM wParam, LPARAM lParam)
{
	TRACE(_T("=== OnNetworkMessage被调用 ===\n"));

	CString* pMsg = (CString*)lParam;
	if (pMsg)
	{
		TRACE(_T("收到网络消息: %s\n"), *pMsg);
		处理网络消息(*pMsg);
		delete pMsg;
	}
	else
	{
		TRACE(_T("收到空消息指针\n"));
	}

	return 0;
}

// 初始化网络通信
BOOL NageDlqDlg::初始化网络通信()
{
	// 检查是否已经连接
	if (网络通信.是否已连接())
	{
		TRACE(_T("网络通信已连接\n"));
		return TRUE;
	}

	TRACE(_T("开始初始化网络通信\n"));

	// 设置消息回调
	网络通信.设置消息回调函数(&NageDlqDlg::处理网络消息, this);

	// 更新状态为连接中
	登录页面.权限状态.SetWindowText(_T("状态：连接中..."));
	TRACE(_T("设置状态为连接中...\n"));

	// 尝试连接服务端
	if (网络通信.连接服务端(_T(SERVER_IP), SERVER_PORT))
	{
		TRACE(_T("连接服务端调用成功\n"));

		// 设置重试定时器（3秒后开始重试，间隔3秒）
		SetTimer(2, 3000, nullptr);
		return TRUE;
	}
	else
	{
		TRACE(_T("连接服务端失败\n"));
		登录页面.权限状态.SetWindowText(_T("状态：连接失败"));
		return FALSE;
	}
}

// 处理网络消息
void NageDlqDlg::处理网络消息(CString 消息)
{
	TRACE(_T("=== 处理网络消息开始 ===\n"));
	TRACE(_T("原始消息: %s\n"), 消息);

	if (消息.Find(_T("REGISTER_")) == 0)
	{
		TRACE(_T("检测到注册响应消息\n"));
		处理注册响应(消息);
	}
	else if (消息.Find(_T("CONNECT_SUCCESS:")) == 0)
	{
		TRACE(_T("检测到带密钥的连接成功消息\n"));
		// 连接成功，停止重试定时器
		KillTimer(2);

		// 解析连接成功响应 - 格式: CONNECT_SUCCESS:密钥:版本号
		CString 响应数据 = 消息.Mid(16); // 去掉"CONNECT_SUCCESS:"
		TRACE(_T("响应数据: %s\n"), 响应数据);

		int 分隔符位置 = 响应数据.Find(':');

		if (分隔符位置 != -1)
		{
			CString 密钥 = 响应数据.Left(分隔符位置);
			CString 服务端版本号 = 响应数据.Mid(分隔符位置 + 1);

			TRACE(_T("解析密钥: %s, 服务端版本: %s\n"), 密钥, 服务端版本号);

			// 检查版本更新
			检查版本更新(服务端版本号);

			// 更新状态标签为密钥信息
			CString 状态文本;
			if (密钥 == _T("chenge"))
			{
				状态文本 = _T("状态：权限全开");
			}
			else if (密钥 == _T("alucard"))
			{
				状态文本 = _T("状态：限制权限");
			}
			else if (密钥 == _T("feier"))
			{
				状态文本 = _T("状态：未授权");
			}
			else
			{
				状态文本.Format(_T("状态：%s"), 密钥);
			}

			TRACE(_T("设置状态文本: %s\n"), 状态文本);
			登录页面.权限状态.SetWindowText(状态文本);
		}
	}
	else if (消息 == _T("CONNECT_SUCCESS"))
	{
		TRACE(_T("检测到不带密钥的连接成功消息\n"));
		// 连接成功，停止重试定时器
		KillTimer(2);

		登录页面.权限状态.SetWindowText(_T("状态：已连接"));
	}
	else if (消息.Find(_T("LOGIN_")) == 0)
	{
		TRACE(_T("检测到登录响应消息\n"));
		// 处理登录响应
		登录页面.处理登录响应(消息);

		// 登录成功后刷新角色列表
		if (消息.Find(_T("LOGIN_SUCCESS")) == 0)
		{
			转生页面.刷新角色列表();
			加点页面.刷新角色列表();
		}
	}
	else if (消息 == _T("VERSION_OUTDATED"))
	{
		TRACE(_T("检测到版本过时消息\n"));
		AfxMessageBox(_T("客户端版本过时，请更新到最新版本！"), MB_ICONWARNING);
	}
	// 添加角色列表响应处理
	else if (消息.Find(_T("ROLES_LIST")) == 0)
	{
		TRACE(_T("检测到角色列表响应消息\n"));

		// 根据当前激活的页面分发消息
		int 当前选中页 = 分页控件.GetCurSel();
		switch (当前选中页)
		{
		case 2: // 转生页面
			转生页面.处理角色列表响应(消息);
			break;
		case 3: // 加点页面
			加点页面.处理角色列表响应(消息);
			break;
		default:
			// 如果两个页面都不活跃，暂时不处理
			break;
		}
	}
	// 修改为账号在线状态响应处理
	else if (消息.Find(_T("ACCOUNT_ONLINE")) == 0)
	{
		TRACE(_T("检测到账号在线状态响应消息\n"));

		// 根据当前激活的页面分发消息
		int 当前选中页 = 分页控件.GetCurSel();
		switch (当前选中页)
		{
		case 2: // 转生页面
			转生页面.处理账号在线状态响应(消息);
			break;
		case 3: // 加点页面
			加点页面.处理账号在线状态响应(消息);
			break;
		default:
			// 如果两个页面都不活跃，暂时不处理
			break;
		}
	}
	// 添加转生相关消息处理
	else if (消息.Find(_T("REBORN_")) == 0)
	{
		TRACE(_T("检测到转生响应消息\n"));
		转生页面.处理转生响应(消息);
	}
	// 添加加点相关消息处理
	else if (消息.Find(_T("ADD_POINTS_")) == 0)
	{
		TRACE(_T("检测到加点响应消息\n"));
		加点页面.处理加点响应(消息);
	}
	// 添加角色信息查询响应处理
	else if (消息.Find(_T("CHAR_INFO")) == 0)
	{
		TRACE(_T("检测到角色信息响应消息\n"));

		// 根据当前激活的页面分发消息
		int 当前选中页 = 分页控件.GetCurSel();
		switch (当前选中页)
		{
		case 2: // 转生页面
			转生页面.处理角色信息响应(消息);
			break;
		case 3: // 加点页面
			加点页面.处理角色信息响应(消息);
			break;
		default:
			// 如果两个页面都不活跃，暂时不处理
			break;
		}
	}
	// 添加排行榜数据响应处理
	else if (消息.Find(_T("RANKING_DATA")) == 0)
	{
		TRACE(_T("检测到排行榜数据响应消息\n"));
		排行榜页面.处理排行榜数据响应(消息);
		}
	else if (消息.Find(_T("RANKING_FAILED")) == 0)
	{
		TRACE(_T("检测到排行榜数据获取失败消息\n"));
		排行榜页面.处理排行榜数据响应(消息);
		}
	else
	{
		TRACE(_T("未知消息类型: %s\n"), 消息);
	}

	TRACE(_T("=== 处理网络消息结束 ===\n"));
}

// 发送请求到服务端
BOOL NageDlqDlg::发送请求到服务端(const CString& 请求数据)
{
	if (!网络通信.是否已连接())
	{
		if (!初始化网络通信())
		{
			return FALSE;
		}
	}

	return 网络通信.发送数据(请求数据);
}

// 处理服务端响应
void NageDlqDlg::处理服务端响应(const CString& 响应数据)
{
	// 根据响应类型分发处理
	if (响应数据.Find(_T("REGISTER_")) == 0)
	{
		处理注册响应(响应数据);
	}
	else if (响应数据.Find(_T("LOGIN_")) == 0)
	{
		// 处理登录响应
		登录页面.处理登录响应(响应数据);
	}
	else if (响应数据.Find(_T("CONNECT_")) == 0)
	{
		// 处理连接响应
		添加信息显示(响应数据);
	}
	// 其他响应类型...
	else
	{
		添加信息显示(CString(_T("收到未知响应: ")) + 响应数据);
	}
}

// 处理注册响应
void NageDlqDlg::处理注册响应(const CString& 响应数据)
{
	TRACE(_T("=== 处理注册响应开始 ===\n"));
	TRACE(_T("响应数据: %s\n"), 响应数据);

	if (响应数据.Find(_T("REGISTER_SUCCESS")) == 0)
	{
		TRACE(_T("注册成功\n"));
		// 注册成功
		注册页面.显示注册状态(_T("注册成功"), TRUE);
		注册页面.清空输入框();
	}
	else if (响应数据.Find(_T("REGISTER_FAILED")) == 0)
	{
		CString 错误信息 = 响应数据.Mid(16); // 去掉"REGISTER_FAILED:"
		TRACE(_T("注册失败: %s\n"), 错误信息);
		// 注册失败
		注册页面.显示注册状态(_T("注册失败: ") + 错误信息, FALSE);
	}
	else
	{
		TRACE(_T("未知注册响应格式\n"));
	}

	TRACE(_T("=== 处理注册响应结束 ===\n"));
}

// 添加信息显示
void NageDlqDlg::添加信息显示(const CString& 信息)
{
	// 这里可以添加信息到日志或状态栏
	TRACE(_T("信息: %s\n"), 信息);
}

//版本检查函数
void NageDlqDlg::检查版本更新(const CString& 服务端版本号)
{
	TRACE(_T("=== 检查版本更新开始 ===\n"));
	TRACE(_T("客户端版本: %s, 服务端版本: %s\n"), _T(CLIENT_VERSION), 服务端版本号);

	// 简单的版本号比较（按点分割比较）
	int 比较结果 = 比较版本号(_T(CLIENT_VERSION), 服务端版本号);

	if (比较结果 < 0)
	{
		TRACE(_T("检测到新版本，提示用户更新\n"));

		CString 提示信息;
		提示信息.Format(_T("发现新版本 %s，当前版本 %s。是否立即更新？"),
			服务端版本号, _T(CLIENT_VERSION));

		if (AfxMessageBox(提示信息, MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			TRACE(_T("用户选择更新，启动更新程序\n"));
			// 传递目标版本号给更新程序
			启动更新程序(服务端版本号);
		}
		else
		{
			TRACE(_T("用户取消更新\n"));
		}
	}
	else if (比较结果 > 0)
	{
		TRACE(_T("客户端版本比服务端新\n"));
		// 可选：提示用户客户端版本过新
	}
	else
	{
		TRACE(_T("客户端版本是最新的\n"));
	}

	TRACE(_T("=== 检查版本更新结束 ===\n"));
}

// 版本号比较函数
int NageDlqDlg::比较版本号(const CString& 版本1, const CString& 版本2)
{
	TRACE(_T("=== 比较版本号开始 ===\n"));
	TRACE(_T("版本1: %s, 版本2: %s\n"), 版本1, 版本2);

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

		TRACE(_T("比较部分 %d: %d vs %d\n"), i, 数字1, 数字2);

		if (数字1 < 数字2)
		{
			TRACE(_T("版本1 < 版本2\n"));
			return -1;
		}
		if (数字1 > 数字2)
		{
			TRACE(_T("版本1 > 版本2\n"));
			return 1;
		}
	}

	TRACE(_T("版本相同\n"));
	return 0; // 版本相同
}

// 启动更新程序
void NageDlqDlg::启动更新程序(const CString& 目标版本号)
{
	TRACE(_T("=== 启动更新程序开始 ===\n"));
	TRACE(_T("目标版本号: %s\n"), 目标版本号);

	// 构建更新程序路径
	TCHAR 当前路径[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, 当前路径);

	CString 更新程序路径;
	更新程序路径.Format(_T("%s\\NageUp.exe"), 当前路径);

	// 调试：显示实际命令行
	CString 命令行;
	命令行.Format(_T("\"%s\" --target-version=%s"), 更新程序路径, 目标版本号);

	CString 调试信息;
	调试信息.Format(_T("更新程序路径: %s\n目标版本号: %s\n完整命令行:\n%s"),
		更新程序路径, 目标版本号, 命令行);

	// 显示调试信息（确认后再继续）
	if (AfxMessageBox(调试信息 + _T("\n\n点击确定继续启动更新程序"), MB_OKCANCEL | MB_ICONINFORMATION) == IDCANCEL)
	{
		return; // 用户取消
	}

	// 检查更新程序是否存在
	if (GetFileAttributes(更新程序路径) == INVALID_FILE_ATTRIBUTES)
	{
		AfxMessageBox(_T("更新程序不存在，请联系管理员！"), MB_ICONERROR);
		return;
	}

	// 启动更新程序
	STARTUPINFO si = { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION pi;

	if (CreateProcess(NULL, 命令行.GetBuffer(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		AfxGetMainWnd()->PostMessage(WM_CLOSE);
	}
	else
	{
		AfxMessageBox(_T("启动更新程序失败！"), MB_ICONERROR);
	}

	命令行.ReleaseBuffer();
}

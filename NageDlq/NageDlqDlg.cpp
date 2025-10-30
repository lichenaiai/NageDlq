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
END_MESSAGE_MAP()
IMPLEMENT_DYNAMIC(NageDlqDlg, CDialogEx)

// 初始化函数
BOOL NageDlqDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	// 设置窗口大小 850x600
	MoveWindow(0, 0, 862, 622);

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
	if (nIDEvent == 1)
	{
		KillTimer(1);
		if (!初始化网络通信())
		{
			TRACE(_T("网络初始化失败，使用本地功能\n"));
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
	rect.left += 1;
	rect.right -= 3;

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
	case 2: 转生页面.ShowWindow(SW_SHOW); break;
	case 3: 加点页面.ShowWindow(SW_SHOW); break;
	case 4: 排行榜页面.ShowWindow(SW_SHOW); break;
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


/*
BOOL NageDlqDlg::初始化网络通信()
{
	// 初始化Socket库
	if (!AfxSocketInit())
	{
		MessageBox(_T("初始化网络失败"), _T("错误"), MB_ICONERROR);
		return FALSE;
	}

	// 设置消息回调
	网络通信.设置消息回调函数(static_cast<void (CWnd::*)(CString)>(&NageDlqDlg::处理网络消息), this);

	// 连接到服务端（这里使用默认地址和端口）
	CString 服务端地址 = _T("127.0.0.1");  // 默认本地地址
	UINT 服务端端口 = 9896;                 // 默认端口

	if (!网络通信.连接服务端(服务端地址, 服务端端口))
	{
		MessageBox(_T("连接服务端失败"), _T("错误"), MB_ICONERROR);
		return FALSE;
	}

	return TRUE;
}
*/

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
	// 检查是否已经初始化过
	if (网络通信.是否已连接())
	{
		TRACE(_T("网络通信已连接\n"));
		return TRUE;
	}

	TRACE(_T("开始初始化网络通信\n"));

	// 设置消息回调
	网络通信.设置消息回调函数(&NageDlqDlg::处理网络消息, this);

	// 先更新状态为连接中
	登录页面.权限状态.SetWindowText(_T("状态：连接中..."));
	TRACE(_T("设置状态为连接中...\n"));

	// 尝试连接服务端
	if (网络通信.连接服务端(_T("127.0.0.1"), 9896))
	{
		TRACE(_T("连接服务端成功"));

		// 发送连接请求
		CString 连接请求;
		连接请求.Format(_T("CONNECT:1.0.0:127.0.0.1\n"));

		TRACE(_T("准备发送连接请求: %s\n"), 连接请求);

		if (网络通信.发送数据(连接请求))
		{
			TRACE(_T("连接请求发送成功\n"));
			return TRUE;
		}
		else
		{
			TRACE(_T("发送连接请求失败\n"));
			登录页面.权限状态.SetWindowText(_T("状态：发送请求失败"));
			return FALSE;
		}
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

	// 检查消息是否包含注册响应
	if (消息.Find(_T("REGISTER_")) == 0)
	{
		TRACE(_T("检测到注册响应消息\n"));
		处理注册响应(消息);
	}
	else if (消息.Find(_T("CONNECT_SUCCESS:")) == 0)
	{
		TRACE(_T("检测到带密钥的连接成功消息\n"));
		// 解析连接成功响应 - 格式: CONNECT_SUCCESS:密钥:版本号
		CString 响应数据 = 消息.Mid(16); // 去掉"CONNECT_SUCCESS:"
		TRACE(_T("响应数据: %s\n"), 响应数据);

		int 分隔符位置 = 响应数据.Find(':');

		if (分隔符位置 != -1)
		{
			CString 密钥 = 响应数据.Left(分隔符位置);
			CString 版本号 = 响应数据.Mid(分隔符位置 + 1);

			TRACE(_T("解析密钥: %s, 版本: %s\n"), 密钥, 版本号);

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
		登录页面.权限状态.SetWindowText(_T("状态：已连接"));
	}
	else if (消息.Find(_T("LOGIN_")) == 0)
	{
		TRACE(_T("检测到登录响应消息\n"));
		// 处理登录响应
		登录页面.处理登录响应(消息);
	}
	else
	{
		TRACE(_T("未知消息类型\n"));
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
	TRACE(_T("处理注册响应: %s\n"), 响应数据);  // 添加调试

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
		TRACE(_T("未知注册响应: %s\n"), 响应数据);
	}
}

// 添加信息显示
void NageDlqDlg::添加信息显示(const CString& 信息)
{
	// 这里可以添加信息到日志或状态栏
	TRACE(_T("信息: %s\n"), 信息);
}
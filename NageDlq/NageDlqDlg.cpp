
// NageDlqDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "NageDlq.h"
#include "NageDlqDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CNageDlqDlg 对话框

CNageDlqDlg::CNageDlqDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_NAGEDLQ_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CNageDlqDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_MAIN, 分页控件);
}

//消息映射
BEGIN_MESSAGE_MAP(CNageDlqDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_MAIN, &CNageDlqDlg::OnTcnSelchangeTabMain)
END_MESSAGE_MAP()


// CNageDlqDlg 消息处理程序
// 初始化函数
BOOL CNageDlqDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// 设置窗口大小 850x600
	MoveWindow(0, 0, 862, 622);
	// 或者使用 SetWindowPos
	//SetWindowPos(NULL, 0, 0, 850, 600, SWP_NOMOVE | SWP_NOZORDER);

	初始化分页控件();
	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CNageDlqDlg::初始化分页控件()
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
}

void CNageDlqDlg::OnTcnSelchangeTabMain(NMHDR* pNMHDR, LRESULT* pResult)
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


// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CNageDlqDlg::OnPaint()
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

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CNageDlqDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


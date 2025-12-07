// 设置对话框类.cpp
#include "pch.h"
#include "framework.h"
#include "NageDlqServer.h"
#include "设置对话框类.h"
#include "afxdialogex.h"
#include "resource.h" 
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 设置对话框类 对话框
IMPLEMENT_DYNAMIC(设置对话框类, CDialogEx)

设置对话框类::设置对话框类(CWnd* pParent)
	: CDialogEx(IDD_SETTINGS_DIALOG, pParent)
	, 数据库用户名(_T("sa"))
	, 数据库密码(_T("password"))
	, 数据库名称(_T("nagelogin"))
	, 测试连接线程句柄(NULL)
	, 连接成功状态(FALSE)
	, 状态文本颜色(RGB(0, 0, 0)) // 默认黑色
{
}

设置对话框类::~设置对话框类()
{
	if (测试连接线程句柄)
	{
		WaitForSingleObject(测试连接线程句柄, 1000);
		CloseHandle(测试连接线程句柄);
		测试连接线程句柄 = NULL;
	}
}

void 设置对话框类::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_DB_USER, 数据库用户名);
	DDX_Text(pDX, IDC_EDIT_DB_PASSWORD, 数据库密码);
	DDX_Text(pDX, IDC_EDIT_DB_NAME, 数据库名称);
	DDX_Control(pDX, IDC_EDIT_DB_USER, 数据库用户编辑框);
	DDX_Control(pDX, IDC_EDIT_DB_PASSWORD, 数据库密码编辑框);
	DDX_Control(pDX, IDC_EDIT_DB_NAME, 数据库名称编辑框);
	DDX_Control(pDX, IDC_BUTTON_TEST_CONNECTION, 测试连接按钮);
	DDX_Control(pDX, IDC_STATIC_CONNECTION_STATUS, 连接状态标签);
}

BEGIN_MESSAGE_MAP(设置对话框类, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_TEST_CONNECTION, &设置对话框类::OnBnClickedButtonTestConnection)
	ON_MESSAGE(WM_USER + 100, &设置对话框类::OnUpdateStatus)
	ON_WM_ERASEBKGND()
	ON_WM_DRAWITEM()
	ON_BN_CLICKED(IDOK, &设置对话框类::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL 设置对话框类::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	数据库密码编辑框.SetPasswordChar('*');
	连接状态标签.SetWindowText(_T("连接状态:"));

	// 设置状态标签为自绘控件
	连接状态标签.ModifyStyle(0, SS_OWNERDRAW);

	// 更新控件显示（显示已设置的配置）
	UpdateData(FALSE);

	return TRUE;
}

// 自定义消息处理函数
LRESULT 设置对话框类::OnUpdateStatus(WPARAM wParam, LPARAM lParam)
{
	CString* 状态信息 = (CString*)lParam;
	if (状态信息)
	{
		// 判断连接状态并设置颜色
		if (状态信息->Find(_T("连接成功")) != -1)
		{
			连接成功状态 = TRUE;
			状态文本颜色 = RGB(0, 128, 0); // 绿色
		}
		else if (状态信息->Find(_T("连接失败")) != -1)
		{
			连接成功状态 = FALSE;
			状态文本颜色 = RGB(255, 0, 0); // 红色
		}
		else
		{
			连接成功状态 = FALSE;
			状态文本颜色 = RGB(0, 0, 0); // 黑色
		}

		连接状态标签.SetWindowText(*状态信息);
		连接状态标签.Invalidate(); // 重绘控件
		delete 状态信息;
	}
	return 0;
}

// 自绘控件
void 设置对话框类::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (nIDCtl == IDC_STATIC_CONNECTION_STATUS)
	{
		CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
		CRect rect = lpDrawItemStruct->rcItem;

		// 获取文本
		CString 文本;
		连接状态标签.GetWindowText(文本);

		// 设置背景模式为透明
		pDC->SetBkMode(TRANSPARENT);

		// 设置文本颜色
		pDC->SetTextColor(状态文本颜色);

		// 绘制文本
		pDC->DrawText(文本, rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		return;
	}

	CDialogEx::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

BOOL 设置对话框类::OnEraseBkgnd(CDC* pDC)
{
	return CDialogEx::OnEraseBkgnd(pDC);
}

// 测试连接线程函数
UINT 设置对话框类::测试连接线程函数(LPVOID pParam)
{
	线程参数* 参数 = (线程参数*)pParam;
	设置对话框类* 对话框指针 = 参数->对话框指针;
	CString 连接字符串 = 参数->连接字符串;

	SQLHENV 环境句柄 = NULL;
	SQLHDBC 连接句柄 = NULL;
	SQLRETURN 返回代码;

	// 分配环境句柄
	返回代码 = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &环境句柄);
	if (返回代码 != SQL_SUCCESS) {
		CString* 状态 = new CString(_T("连接状态: 分配环境句柄失败"));
		对话框指针->PostMessage(WM_USER + 100, 0, (LPARAM)状态);
		delete 参数;
		return 1;
	}

	// 设置ODBC版本
	SQLSetEnvAttr(环境句柄, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

	// 分配连接句柄
	返回代码 = SQLAllocHandle(SQL_HANDLE_DBC, 环境句柄, &连接句柄);
	if (返回代码 != SQL_SUCCESS) {
		SQLFreeHandle(SQL_HANDLE_ENV, 环境句柄);
		CString* 状态 = new CString(_T("连接状态: 分配连接句柄失败"));
		对话框指针->PostMessage(WM_USER + 100, 0, (LPARAM)状态);
		delete 参数;
		return 1;
	}

	SQLWCHAR* 宽字符连接字符串 = (SQLWCHAR*)连接字符串.GetBuffer();
	SQLSMALLINT 连接字符串输出长度;

	// 测试连接
	返回代码 = SQLDriverConnect(连接句柄, NULL, 宽字符连接字符串, SQL_NTS,
		NULL, 0, &连接字符串输出长度, SQL_DRIVER_NOPROMPT);

	连接字符串.ReleaseBuffer();

	CString* 状态 = new CString();
	if (返回代码 == SQL_SUCCESS || 返回代码 == SQL_SUCCESS_WITH_INFO) {
		*状态 = _T("连接状态: 连接成功");
		SQLDisconnect(连接句柄);
	}
	else {
		SQLWCHAR SQL状态[6], 错误消息[SQL_MAX_MESSAGE_LENGTH];
		SQLINTEGER 原生错误码;
		SQLSMALLINT 消息长度;

		SQLGetDiagRec(SQL_HANDLE_DBC, 连接句柄, 1, SQL状态, &原生错误码,
			错误消息, SQL_MAX_MESSAGE_LENGTH, &消息长度);

		状态->Format(_T("连接状态: 连接失败 - %s"), CString(错误消息));
	}

	if (连接句柄) SQLFreeHandle(SQL_HANDLE_DBC, 连接句柄);
	if (环境句柄) SQLFreeHandle(SQL_HANDLE_ENV, 环境句柄);

	对话框指针->PostMessage(WM_USER + 100, 0, (LPARAM)状态);
	delete 参数;
	return 0;
}

// 测试连接按钮点击事件
void 设置对话框类::OnBnClickedButtonTestConnection()
{
	UpdateData(TRUE);

	// 等待现有线程结束
	if (测试连接线程句柄)
	{
		WaitForSingleObject(测试连接线程句柄, 100);
		CloseHandle(测试连接线程句柄);
		测试连接线程句柄 = NULL;
	}

	// 创建连接字符串
	CString 连接字符串;
	连接字符串.Format(_T("DRIVER={SQL Server};SERVER=47.116.167.99;DATABASE=%s;UID=%s;PWD=%s;"),
		数据库名称, 数据库用户名, 数据库密码);

	// 创建线程参数
	线程参数* 参数 = new 线程参数;
	参数->对话框指针 = this;
	参数->连接字符串 = 连接字符串;

	// 更新状态为黑色
	状态文本颜色 = RGB(0, 0, 0);
	CString* 状态 = new CString(_T("连接状态: "));
	PostMessage(WM_USER + 100, 0, (LPARAM)状态);

	// 创建线程
	测试连接线程句柄 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)测试连接线程函数,
		(LPVOID)参数, 0, NULL);

	if (!测试连接线程句柄)
	{
		CString* 错误状态 = new CString(_T("连接状态: 创建线程失败"));
		PostMessage(WM_USER + 100, 0, (LPARAM)错误状态);
		delete 参数;
	}
}

// 保存按钮点击事件处理函数
void 设置对话框类::OnBnClickedOk()
{
	// 从控件更新变量
	//UpdateData(TRUE);

	if (数据库用户名.IsEmpty() || 数据库名称.IsEmpty())
	{
		AfxMessageBox(_T("用户名和数据库名不能为空"), MB_ICONWARNING);
		return;
	}

	CDialogEx::OnOK();
}
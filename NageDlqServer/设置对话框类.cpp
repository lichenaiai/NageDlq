#include "pch.h"
#include "framework.h"
#include "NageDlqServer.h"
#include "设置对话框类.h"
#include "afxdialogex.h"

// 添加ODBC头文件
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 设置对话框类 对话框
IMPLEMENT_DYNAMIC(设置对话框类, CDialogEx)

设置对话框类::设置对话框类(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SETTINGS_DIALOG, pParent)
	, 数据库用户名(_T("sa"))
	, 数据库密码(_T("password"))
	, 数据库名称(_T("nagelogin"))
{
}

设置对话框类::~设置对话框类()
{
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
}

BEGIN_MESSAGE_MAP(设置对话框类, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_TEST_CONNECTION, &设置对话框类::OnBnClickedButtonTestConnection)
END_MESSAGE_MAP()

BOOL 设置对话框类::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置密码编辑框为密码模式
	数据库密码编辑框.SetPasswordChar('*');

	return TRUE;
}

// 测试连接按钮点击事件
void 设置对话框类::OnBnClickedButtonTestConnection()
{
	UpdateData(TRUE); // 获取界面数据

	SQLHENV 环境句柄;
	SQLHDBC 连接句柄;
	SQLRETURN 返回代码;

	// 分配环境句柄
	返回代码 = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &环境句柄);
	if (返回代码 != SQL_SUCCESS) {
		MessageBox(_T("分配环境句柄失败"), _T("测试连接"), MB_ICONERROR);
		return;
	}

	// 设置ODBC版本
	SQLSetEnvAttr(环境句柄, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

	// 分配连接句柄
	返回代码 = SQLAllocHandle(SQL_HANDLE_DBC, 环境句柄, &连接句柄);
	if (返回代码 != SQL_SUCCESS) {
		SQLFreeHandle(SQL_HANDLE_ENV, 环境句柄);
		MessageBox(_T("分配连接句柄失败"), _T("测试连接"), MB_ICONERROR);
		return;
	}

	// 连接字符串
	CString 连接字符串;
	连接字符串.Format(_T("DRIVER={SQL Server};SERVER=127.0.0.1;DATABASE=%s;UID=%s;PWD=%s;"),
		数据库名称, 数据库用户名, 数据库密码);

	SQLWCHAR* 宽字符连接字符串 = (SQLWCHAR*)连接字符串.GetBuffer();
	SQLSMALLINT 连接字符串输出长度;

	// 测试连接
	返回代码 = SQLDriverConnect(连接句柄, NULL, 宽字符连接字符串, SQL_NTS,
		NULL, 0, &连接字符串输出长度, SQL_DRIVER_NOPROMPT);

	连接字符串.ReleaseBuffer();

	if (返回代码 == SQL_SUCCESS || 返回代码 == SQL_SUCCESS_WITH_INFO) {
		MessageBox(_T("数据库连接成功！"), _T("测试连接"), MB_ICONINFORMATION);
		SQLDisconnect(连接句柄);
	}
	else {
		SQLWCHAR SQL状态[6], 错误消息[SQL_MAX_MESSAGE_LENGTH];
		SQLINTEGER 原生错误码;
		SQLSMALLINT 消息长度;

		SQLGetDiagRec(SQL_HANDLE_DBC, 连接句柄, 1, SQL状态, &原生错误码,
			错误消息, SQL_MAX_MESSAGE_LENGTH, &消息长度);

		MessageBox(_T("数据库连接失败：") + CString(错误消息), _T("测试连接"), MB_ICONERROR);
	}

	SQLFreeHandle(SQL_HANDLE_DBC, 连接句柄);
	SQLFreeHandle(SQL_HANDLE_ENV, 环境句柄);
}

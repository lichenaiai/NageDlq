// 登录页面类.cpp: 实现文件
//

#include "pch.h"
#include "NageDlq.h"
#include "afxdialogex.h"
#include "登录页面类.h"


// 登录页面类 对话框

IMPLEMENT_DYNAMIC(登录页面类, CDialogEx)

登录页面类::登录页面类(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PAGE_LOGIN, pParent)
{

}

登录页面类::~登录页面类()
{
}

void 登录页面类::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_USERNAME, 用户名编辑框);
	DDX_Control(pDX, IDC_EDIT_PASSWORD, 密码编辑框);
	DDX_Control(pDX, IDC_BUTTON_LOGIN, 登录按钮);
	DDX_Control(pDX, IDC_BUTTON_START, 启动按钮);
	DDX_Control(pDX, IDC_CHECK_1280, 窗口1280);
}


BEGIN_MESSAGE_MAP(登录页面类, CDialogEx)
END_MESSAGE_MAP()


// 登录页面类 消息处理程序

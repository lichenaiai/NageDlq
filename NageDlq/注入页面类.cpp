// 注入页面类.cpp: 实现文件
//

#include "pch.h"
#include "NageDlq.h"
#include "afxdialogex.h"
#include "注入页面类.h"


// 注入页面类 对话框

IMPLEMENT_DYNAMIC(注入页面类, CDialogEx)

注入页面类::注入页面类(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PAGE_INJECTION, pParent)
{

}

注入页面类::~注入页面类()
{
}

void 注入页面类::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(注入页面类, CDialogEx)
END_MESSAGE_MAP()


// 注入页面类 消息处理程序

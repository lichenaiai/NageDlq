// 排行榜页面类.cpp: 实现文件
//

#include "pch.h"
#include "NageDlq.h"
#include "afxdialogex.h"
#include "排行榜页面类.h"


// 排行榜页面类 对话框

IMPLEMENT_DYNAMIC(排行榜页面类, CDialogEx)

排行榜页面类::排行榜页面类(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PAGE_RANKING, pParent)
{

}

排行榜页面类::~排行榜页面类()
{
}

void 排行榜页面类::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(排行榜页面类, CDialogEx)
END_MESSAGE_MAP()


// 排行榜页面类 消息处理程序

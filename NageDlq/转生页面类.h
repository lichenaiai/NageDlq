#pragma once
#include "afxdialogex.h"


// 转生页面类 对话框

class 转生页面类 : public CDialogEx
{
	DECLARE_DYNAMIC(转生页面类)

public:
	转生页面类(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~转生页面类();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PAGE_REBIRTH };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};

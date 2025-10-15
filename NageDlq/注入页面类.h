#pragma once
#include "afxdialogex.h"


// 注入页面类 对话框

class 注入页面类 : public CDialogEx
{
	DECLARE_DYNAMIC(注入页面类)

public:
	注入页面类(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~注入页面类();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PAGE_INJECTION };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};

#pragma once
#include "pch.h"
#include "afxdialogex.h"
#include "登录页面类.h"
#include "注册页面类.h"
#include "转生页面类.h"
#include "加点页面类.h"
#include "排行榜页面类.h"
#include "注入页面类.h"
#include "网络通信类.h"

// NageDlqDlg 对话框
class NageDlqDlg : public CDialogEx
{
    // 构造
public:
    NageDlqDlg(CWnd* pParent = nullptr);	// 标准构造函数

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_NAGEDLQ_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

    // 实现
protected:
    HICON m_hIcon;

    // 生成的消息映射函数
    virtual BOOL OnInitDialog();
    //afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()
public:
    // 控件变量
    CListCtrl 信息显示列表;
    CTabCtrl 分页控件;  // 分页控件变量

    // 网络通信相关
    BOOL 初始化网络通信();
    void 处理网络消息(CString 消息);
    BOOL 发送请求到服务端(const CString& 请求数据);
    void 处理服务端响应(const CString& 响应数据);

    // 注册功能
    void 处理注册响应(const CString& 响应数据);

    // 添加信息显示
    void 添加信息显示(const CString& 信息);

    // 获取网络通信对象引用
    网络通信类& 获取网络通信() { return 网络通信; }

private:
    // 各个页面对象
    登录页面类 登录页面;
    注册页面类 注册页面;
    转生页面类 转生页面;
    加点页面类 加点页面;
    排行榜页面类 排行榜页面;
    注入页面类 注入页面;

    网络通信类 网络通信;          // 网络通信对象

    // 页面初始化函数
    void 初始化分页控件();
    afx_msg void OnTcnSelchangeTabMain(NMHDR* pNMHDR, LRESULT* pResult);
};

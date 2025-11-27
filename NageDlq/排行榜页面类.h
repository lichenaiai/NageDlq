#pragma once
#include "pch.h"
#include "afxdialogex.h"

// 排行榜页面类
class 排行榜页面类 : public CDialogEx
{
    DECLARE_DYNAMIC(排行榜页面类)

public:
    排行榜页面类(CWnd* pParent = nullptr);   // 标准构造函数
    virtual ~排行榜页面类();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_PAGE_RANKING };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

public:
    // 控件变量
    CStatic 标题标签;
    CListCtrl 排行榜列表控件;

    // 消息处理函数
    afx_msg void OnBnClickedButtonRefresh();    // 刷新按钮点击

    // 功能函数
    void 初始化列表控件();
    void 刷新排行榜数据();
    void 从服务端获取排行榜数据();
    void 处理排行榜数据响应(const CString& 响应数据);
    void 清空列表数据();
    void 添加排行榜项(int 排名, const CString& 角色名字, const CString& 职业, int 转生次数, int 累计等级);
    CString 获取职业名称(int 职业代码);

private:
    // 数据库连接信息（从服务端获取）
    CString 服务器地址;
    CString 数据库名称;
    CString 用户名;
    CString 密码;
};
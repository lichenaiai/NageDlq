#include "登录页面类.h"
#include "注册页面类.h"
#include "转生页面类.h"
#include "加点页面类.h"
#include "排行榜页面类.h"
#include "注入页面类.h"

class CNageDlqDlg : public CDialogEx
{
    // 构造
public:
    CNageDlqDlg(CWnd* pParent = nullptr);	// 标准构造函数

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_NAGEDLQDLG_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

    // 实现
protected:
    HICON m_hIcon;

    // 生成的消息映射函数
    virtual BOOL OnInitDialog();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()

public:
    CTabCtrl 分页控件;  // 分页控件变量

private:
    // 各个页面对象
    登录页面类 登录页面;
    注册页面类 注册页面;
    转生页面类 转生页面;
    加点页面类 加点页面;
    排行榜页面类 排行榜页面;
    注入页面类 注入页面;

    // 页面初始化函数
    void 初始化分页控件();
    afx_msg void OnTcnSelchangeTabMain(NMHDR* pNMHDR, LRESULT* pResult);
};

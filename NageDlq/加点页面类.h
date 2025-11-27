#pragma once
#include "pch.h"
#include "afxdialogex.h"

// 加点页面类
class 加点页面类 : public CDialogEx
{
    DECLARE_DYNAMIC(加点页面类)

public:
    加点页面类(CWnd* pParent = nullptr);   // 标准构造函数
    virtual ~加点页面类();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_PAGE_STATS };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
    virtual BOOL OnInitDialog();
    virtual HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

    DECLARE_MESSAGE_MAP()

public:
    // 控件变量
    CStatic 角色标签;
    CStatic 提示标签;
    CComboBox 角色选择框;
    CStatic 剩余点数标签;
    CStatic 力量标签;
    CStatic 敏捷标签;
    CStatic 意念标签;
    CStatic 灵力标签;
    CEdit 力量编辑框;
    CEdit 敏捷编辑框;
    CEdit 意念编辑框;
    CEdit 灵力编辑框;
    CButton 加点按钮;

    // 消息处理函数
    afx_msg void OnCbnSelchangeStatsCombo();    // 角色选择改变
    afx_msg void OnBnClickedButtonAddPoints();  // 加点按钮点击
    afx_msg void OnEnChangeEditS_L();           // 力量编辑框改变
    afx_msg void OnEnChangeEditS_M();           // 敏捷编辑框改变
    afx_msg void OnEnChangeEditS_Y();           // 意念编辑框改变
    afx_msg void OnEnChangeEditS_LL();          // 灵力编辑框改变
    afx_msg void OnTimer(UINT_PTR nIDEvent);    // 定时器

    // 功能函数
    void 刷新角色列表();
    void 处理加点响应(const CString& 响应数据);
    void 处理角色信息响应(const CString& 响应数据);
    void 查询角色信息(const CString& 角色名);
    void 计算总点数();
    void 发送加点请求(const CString& 角色名, int 力量, int 敏捷, int 意念, int 灵力);
    void 更新属性显示(int 职业代码, int 剩余点数, int 力量, int 敏捷, int 意念, int 灵力);
    void 根据职业调整输入框(int 职业代码);
    BOOL 验证加点输入(int 力量, int 敏捷, int 意念, int 灵力, int 剩余点数, int 职业代码);
    void 处理角色列表响应(const CString& 响应数据);
    void 检查账号在线状态(const CString& 角色名, int 力量, int 敏捷, int 意念, int 灵力);
    void 处理账号在线状态响应(const CString& 响应数据);
    void 确认加点操作(const CString& 角色名, int 力量, int 敏捷, int 意念, int 灵力);

private:
    CString 当前用户名;
    int 当前剩余点数;
    int 当前职业代码;
    int 当前力量;
    int 当前敏捷;
    int 当前意念;
    int 当前灵力;
    CString 待处理角色名; // 用于保存待处理的角色名
    int 待处理力量;
    int 待处理敏捷;
    int 待处理意念;
    int 待处理灵力;

    COLORREF 提示文本颜色;
};
#pragma once
#include "pch.h"
#include "afxdialogex.h"

// 转生页面类
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
    virtual BOOL OnInitDialog();
    virtual HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

    DECLARE_MESSAGE_MAP()

public:
    // 控件变量
    CComboBox 角色选择框;
    CStatic 状态标签;
    CStatic 提示标签;
    CStatic 等级标签;
    CButton 转生按钮;

    // 消息处理函数
    afx_msg void OnCbnSelchangeRebirthCombo();  // 角色选择改变
    afx_msg void OnBnClickedRebirthR();         // 转生按钮点击
    afx_msg void OnTimer(UINT_PTR nIDEvent);    // 定时器

    // 功能函数
    void 刷新角色列表();
    void 处理转生响应(const CString& 响应数据);
    void 查询角色信息(const CString& 角色名);
    void 发送转生请求(const CString& 角色名);
    void 更新角色信息显示(int 职业代码, int 战斗等级, int 累计等级, int 剩余点数);
    void 处理角色信息响应(const CString& 响应数据);
    void 处理角色列表响应(const CString& 响应数据);
    void 检查账号在线状态(const CString& 角色名);
    void 处理账号在线状态响应(const CString& 响应数据);
    void 确认转生操作(const CString& 角色名);

private:
    CString 当前用户名;
    BOOL 转生冷却中;
    int 冷却剩余时间; // 秒
    CTime 最后转生时间;
    CString 待处理角色名; // 用于保存待处理的角色名
    COLORREF 提示文本颜色;
};
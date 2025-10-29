#include "pch.h"
#include "framework.h"
#include "NageDlq.h"
#include "注册页面类.h"
#include "afxdialogex.h"
#include "NageDlqDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 注册页面类 对话框
IMPLEMENT_DYNAMIC(注册页面类, CDialogEx)

注册页面类::注册页面类(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_PAGE_REGISTER, pParent)
{
    状态文本颜色 = RGB(0, 0, 0);  // 默认黑色
    需要设置颜色 = FALSE;
}

注册页面类::~注册页面类()
{
}

void 注册页面类::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_REG_USERNAME, 账号编辑框);
    DDX_Control(pDX, IDC_EDIT_REG_PASSWORD, 密码编辑框);
    DDX_Control(pDX, IDC_EDIT_CONFIRM_PASSWORD, 确认密码编辑框);
    DDX_Control(pDX, IDC_EDIT_EMAIL, 邮箱编辑框);
    DDX_Control(pDX, IDC_BUTTON_REGISTER_DLG, 注册按钮);
    DDX_Control(pDX, IDC_STATIC_REG_STATUS, 注册状态标签);
}

BEGIN_MESSAGE_MAP(注册页面类, CDialogEx)
    ON_BN_CLICKED(IDC_BUTTON_REGISTER_DLG, &注册页面类::OnBnClickedButtonRegConfirm)
    ON_WM_TIMER()   //定时器消息
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL 注册页面类::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 设置对话框标题
    SetWindowText(_T("账号注册"));

    // 设置密码框样式
    密码编辑框.SetPasswordChar('*');
    确认密码编辑框.SetPasswordChar('*');

    // 限制编辑框长度
    账号编辑框.SetLimitText(12);    // 账号最长12位
    密码编辑框.SetLimitText(12);    // 密码最长12位
    确认密码编辑框.SetLimitText(12); // 确认密码最长12位
    邮箱编辑框.SetLimitText(50);     // 邮箱最长50位
    注册状态标签.ShowWindow(SW_HIDE);

    return TRUE;
}

// 实现 OnCtlColor
HBRUSH 注册页面类::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    // 如果是状态标签并且需要设置颜色
    if (nCtlColor == CTLCOLOR_STATIC && pWnd == &注册状态标签 && 需要设置颜色)
    {
        pDC->SetTextColor(状态文本颜色);
        pDC->SetBkMode(TRANSPARENT);  // 透明背景
    }

    return hbr;
}

// 注册按钮点击事件
void 注册页面类::OnBnClickedButtonRegConfirm()
{
    CString 账号, 密码, 确认密码, 邮箱;

    // 获取输入内容
    账号编辑框.GetWindowText(账号);
    密码编辑框.GetWindowText(密码);
    确认密码编辑框.GetWindowText(确认密码);
    邮箱编辑框.GetWindowText(邮箱);

    // 验证输入
    if (!验证输入())
        return;

    // 检查密码一致性
    if (密码 != 确认密码)
    {
        MessageBox(_T("两次输入的密码不一致"), _T("错误"), MB_ICONERROR);
        return;
    }

    // 检查注册时间限制
    if (!检查注册时间限制())
        return;

    // 发送注册请求
    发送注册请求();
}

// 显示注册状态函数
void 注册页面类::显示注册状态(const CString& 状态信息, BOOL 成功)
{
    TRACE(_T("显示注册状态: %s, 成功: %d\n"), 状态信息, 成功);

    // 设置颜色
    if (成功)
    {
        状态文本颜色 = RGB(0, 128, 0); // 绿色
    }
    else
    {
        状态文本颜色 = RGB(255, 0, 0); // 红色
    }

    需要设置颜色 = TRUE;

    // 设置文字
    注册状态标签.SetWindowText(状态信息);

    // 设置字体
    CFont* p旧字体 = 注册状态标签.GetFont();
    CFont 新字体;

    if (p旧字体)
    {
        LOGFONT lf;
        p旧字体->GetLogFont(&lf);
        lf.lfHeight = -20; // 20号字体
        _tcscpy_s(lf.lfFaceName, _T("微软雅黑"));
        新字体.CreateFontIndirect(&lf);
    }
    else
    {
        新字体.CreatePointFont(200, _T("微软雅黑")); // 20号字体
    }

    注册状态标签.SetFont(&新字体);
    新字体.Detach();  // 分离字体，避免被销毁

    // 显示标签
    注册状态标签.ShowWindow(SW_SHOW);

    // 强制重绘
    注册状态标签.Invalidate();
    注册状态标签.UpdateWindow();

    // 5秒后自动隐藏
    SetTimer(1, 5000, nullptr);

    TRACE(_T("状态标签显示完成\n"));
}

//定时器
void 注册页面类::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1)
    {
        KillTimer(1);
        注册状态标签.ShowWindow(SW_HIDE);
        需要设置颜色 = FALSE;  // 重置颜色状态
    }
    CDialogEx::OnTimer(nIDEvent);
}

// 验证输入内容
BOOL 注册页面类::验证输入()
{
    CString 账号, 密码, 确认密码, 邮箱;

    账号编辑框.GetWindowText(账号);
    密码编辑框.GetWindowText(密码);
    确认密码编辑框.GetWindowText(确认密码);
    邮箱编辑框.GetWindowText(邮箱);

    // 检查必填项
    if (账号.IsEmpty())
    {
        MessageBox(_T("账号不能为空"), _T("错误"), MB_ICONERROR);
        账号编辑框.SetFocus();
        return FALSE;
    }

    if (密码.IsEmpty())
    {
        MessageBox(_T("密码不能为空"), _T("错误"), MB_ICONERROR);
        密码编辑框.SetFocus();
        return FALSE;
    }

    if (确认密码.IsEmpty())
    {
        MessageBox(_T("请确认密码"), _T("错误"), MB_ICONERROR);
        确认密码编辑框.SetFocus();
        return FALSE;
    }

    if (邮箱.IsEmpty())
    {
        MessageBox(_T("邮箱不能为空"), _T("错误"), MB_ICONERROR);
        邮箱编辑框.SetFocus();
        return FALSE;
    }

    // 验证账号规则
    if (!验证账号规则(账号))
    {
        账号编辑框.SetFocus();
        return FALSE;
    }

    // 验证密码规则
    if (!验证密码规则(密码))
    {
        密码编辑框.SetFocus();
        return FALSE;
    }

    // 验证邮箱规则
    if (!验证邮箱规则(邮箱))
    {
        邮箱编辑框.SetFocus();
        return FALSE;
    }

    return TRUE;
}

// 验证账号规则
BOOL 注册页面类::验证账号规则(const CString& 账号)
{
    // 检查长度
    if (账号.GetLength() > 12)
    {
        MessageBox(_T("账号长度不能超过12位"), _T("错误"), MB_ICONERROR);
        return FALSE;
    }

    // 检查字符范围（只能包含字母和数字）
    for (int i = 0; i < 账号.GetLength(); i++)
    {
        TCHAR c = 账号[i];
        if (!((c >= _T('a') && c <= _T('z')) ||
            (c >= _T('A') && c <= _T('Z')) ||
            (c >= _T('0') && c <= _T('9'))))
        {
            MessageBox(_T("账号只能包含字母和数字"), _T("错误"), MB_ICONERROR);
            return FALSE;
        }
    }

    return TRUE;
}

// 验证密码规则
BOOL 注册页面类::验证密码规则(const CString& 密码)
{
    // 检查长度
    if (密码.GetLength() > 12)
    {
        MessageBox(_T("密码长度不能超过12位"), _T("错误"), MB_ICONERROR);
        return FALSE;
    }

    return TRUE;
}

// 验证邮箱规则
BOOL 注册页面类::验证邮箱规则(const CString& 邮箱)
{
    // 简单的邮箱格式验证
    int at位置 = 邮箱.Find('@');
    int 点位置 = 邮箱.Find('.', at位置);

    if (at位置 == -1 || 点位置 == -1 || at位置 > 点位置)
    {
        MessageBox(_T("邮箱格式不正确"), _T("错误"), MB_ICONERROR);
        return FALSE;
    }

    return TRUE;
}

// 检查注册时间限制
BOOL 注册页面类::检查注册时间限制()
{
    CTime 当前时间 = CTime::GetCurrentTime();

    if (!最后注册时间.GetTime())  // 第一次注册
    {
        最后注册时间 = 当前时间;
        return TRUE;
    }

    CTimeSpan 时间差 = 当前时间 - 最后注册时间;
    if (时间差.GetTotalMinutes() < 1)  // 1分钟内只能注册一次
    {
        CString 提示信息;
        提示信息.Format(_T("1分钟内只能注册一次，请等待 %d 秒"), 60 - (int)时间差.GetTotalSeconds());
        MessageBox(提示信息, _T("提示"), MB_ICONINFORMATION);
        return FALSE;
    }

    最后注册时间 = 当前时间;
    return TRUE;
}

// 发送注册请求到服务端
void 注册页面类::发送注册请求()
{
    CString 账号, 密码, 邮箱;
    账号编辑框.GetWindowText(账号);
    密码编辑框.GetWindowText(密码);
    邮箱编辑框.GetWindowText(邮箱);

    // 确保数据格式正确，去除可能的空格
    账号.Trim();
    密码.Trim();
    邮箱.Trim();

    // 构建注册请求字符串
    CString 注册请求;
    注册请求.Format(_T("REGISTER:%s:%s:%s"), 账号, 密码, 邮箱);
    TRACE(_T("发送注册请求: %s\n"), 注册请求);

    // 获取主对话框并发送请求
    NageDlqDlg* 主对话框 = dynamic_cast<NageDlqDlg*>(AfxGetMainWnd());
    if (主对话框)
    {
        if (主对话框->发送请求到服务端(注册请求))
        {
            TRACE(_T("注册请求发送成功\n"));
        }
        else
        {
            TRACE(_T("注册请求发送失败\n"));
            MessageBox(_T("发送注册请求失败，请检查网络连接"), _T("错误"), MB_ICONERROR);
        }
    }
    else
    {
        TRACE(_T("获取主对话框失败\n"));
        MessageBox(_T("系统错误，无法发送请求"), _T("错误"), MB_ICONERROR);
    }
}

// 清空输入框
void 注册页面类::清空输入框()
{
    账号编辑框.SetWindowText(_T(""));
    密码编辑框.SetWindowText(_T(""));
    确认密码编辑框.SetWindowText(_T(""));
    邮箱编辑框.SetWindowText(_T(""));
    账号编辑框.SetFocus();
}
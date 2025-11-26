#include "pch.h"
#include "framework.h"
#include "NageDlq.h"
#include "转生页面类.h"
#include "afxdialogex.h"
#include "NageDlqDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 转生页面类 对话框
IMPLEMENT_DYNAMIC(转生页面类, CDialogEx)

转生页面类::转生页面类(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_PAGE_REBIRTH, pParent)
    , 转生冷却中(FALSE)
    , 冷却剩余时间(0)
{
}

转生页面类::~转生页面类()
{
}

void 转生页面类::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_REBIRTH_COMBO, 角色选择框);
    DDX_Control(pDX, IDC_STATIC_RI, 状态标签);
    DDX_Control(pDX, IDC_STATIC_RI_PS, 提示标签);
    DDX_Control(pDX, IDC_STATIC_RI_LV, 等级标签);
    DDX_Control(pDX, IDC_REBIRTH_R, 转生按钮);
}

BEGIN_MESSAGE_MAP(转生页面类, CDialogEx)
    ON_CBN_SELCHANGE(IDC_REBIRTH_COMBO, &转生页面类::OnCbnSelchangeRebirthCombo)
    ON_BN_CLICKED(IDC_REBIRTH_R, &转生页面类::OnBnClickedRebirthR)
    ON_WM_TIMER()
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

HBRUSH 转生页面类::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    // 如果是提示标签
    if (nCtlColor == CTLCOLOR_STATIC && pWnd == &提示标签)
    {
        pDC->SetTextColor(RGB(0, 128, 0));  // 绿色
        pDC->SetBkMode(TRANSPARENT);  // 透明背景
    }

    return hbr;
}

BOOL 转生页面类::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 设置状态标签字体
    CFont 大字体;
    大字体.CreatePointFont(180, _T("微软雅黑")); // 18号字体
    状态标签.SetFont(&大字体);
    大字体.Detach();

    // 设置提示标签文本（颜色在OnCtlColor中设置）
    提示标签.SetWindowText(_T("提示：角色等级达到130级方可转生，每次转生后需等待7天"));

    // 初始状态
    转生按钮.EnableWindow(FALSE);
    状态标签.SetWindowText(_T("请选择角色"));
    等级标签.SetWindowText(_T(""));

    return TRUE;
}

void 转生页面类::刷新角色列表()
{
    // 获取主对话框
    CWnd* 主窗口 = AfxGetMainWnd();
    if (!主窗口) return;

    NageDlqDlg* 主对话框 = dynamic_cast<NageDlqDlg*>(主窗口);
    if (!主对话框) return;

    // 获取登录页面中的用户名
    登录页面类* 登录页面 = &主对话框->登录页面;
    CString 用户名;
    登录页面->用户名编辑框.GetWindowText(用户名);

    if (用户名.IsEmpty())
    {
        当前用户名 = _T("");
        角色选择框.ResetContent();
        状态标签.SetWindowText(_T("请先登录"));
        return;
    }

    当前用户名 = 用户名;

    // 发送获取角色列表请求
    CString 获取角色请求;
    获取角色请求.Format(_T("GET_ROLES:%s"), 当前用户名);

    if (主对话框->发送请求到服务端(获取角色请求))
    {
        状态标签.SetWindowText(_T("获取角色列表中..."));
        角色选择框.ResetContent();
        角色选择框.EnableWindow(FALSE);
    }
    else
    {
        状态标签.SetWindowText(_T("获取角色列表失败"));
    }
}

void 转生页面类::OnCbnSelchangeRebirthCombo()
{
    int 选中索引 = 角色选择框.GetCurSel();
    if (选中索引 != CB_ERR)
    {
        CString 角色名;
        角色选择框.GetLBText(选中索引, 角色名);
        查询角色信息(角色名);
    }
}

void 转生页面类::OnBnClickedRebirthR()
{
    int 选中索引 = 角色选择框.GetCurSel();
    if (选中索引 == CB_ERR)
    {
        MessageBox(_T("请选择角色"), _T("提示"), MB_ICONWARNING);
        return;
    }

    CString 角色名;
    角色选择框.GetLBText(选中索引, 角色名);

    // 检查冷却时间
    if (转生冷却中)
    {
        CString 提示信息;
        提示信息.Format(_T("转生冷却中，请等待 %d 秒"), 冷却剩余时间);
        MessageBox(提示信息, _T("提示"), MB_ICONINFORMATION);
        return;
    }

    // 先检查角色是否在线
    检查角色在线状态(角色名);
}

void 转生页面类::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1)
    {
        冷却剩余时间--;

        if (冷却剩余时间 <= 0)
        {
            转生冷却中 = FALSE;
            KillTimer(1);
            转生按钮.EnableWindow(TRUE);
        }
    }
    else if (nIDEvent == 2)
    {
        // 检查角色在线状态超时
        KillTimer(2);
        MessageBox(_T("检查角色状态超时，请重试"), _T("提示"), MB_ICONWARNING);
        状态标签.SetWindowText(_T("状态检查超时"));
    }

    CDialogEx::OnTimer(nIDEvent);
}

void 转生页面类::查询角色信息(const CString& 角色名)
{
    // 通过主对话框发送查询请求
    CWnd* 主窗口 = AfxGetMainWnd();
    if (!主窗口) return;

    NageDlqDlg* 主对话框 = dynamic_cast<NageDlqDlg*>(主窗口);
    if (!主对话框) return;

    // 构建查询请求
    CString 查询请求;
    查询请求.Format(_T("GET_CHAR_INFO:%s:%s"), 当前用户名, 角色名);

    if (主对话框->发送请求到服务端(查询请求))
    {
        状态标签.SetWindowText(_T("查询角色信息中..."));
    }
    else
    {
        状态标签.SetWindowText(_T("查询失败"));
    }
}

void 转生页面类::发送转生请求(const CString& 角色名)
{
    // 通过主对话框发送转生请求
    CWnd* 主窗口 = AfxGetMainWnd();
    if (!主窗口) return;

    NageDlqDlg* 主对话框 = dynamic_cast<NageDlqDlg*>(主窗口);
    if (!主对话框) return;

    // 构建转生请求
    CString 转生请求;
    转生请求.Format(_T("REBORN:%s:%s"), 当前用户名, 角色名);

    if (主对话框->发送请求到服务端(转生请求))
    {
        状态标签.SetWindowText(_T("转生请求发送中..."));
    }
    else
    {
        状态标签.SetWindowText(_T("转生请求发送失败"));
    }
}

void 转生页面类::处理转生响应(const CString& 响应数据)
{
    TRACE(_T("=== 处理转生响应开始 ===\n"));
    TRACE(_T("响应数据: %s\n"), 响应数据);

    if (响应数据.Find(_T("REBORN_SUCCESS")) == 0)
    {
        TRACE(_T("转生成功\n"));
        状态标签.SetWindowText(_T("转生成功！"));
        MessageBox(_T("转生成功！"), _T("成功"), MB_ICONINFORMATION);

        // 刷新角色信息
        int 选中索引 = 角色选择框.GetCurSel();
        if (选中索引 != CB_ERR)
        {
            CString 角色名;
            角色选择框.GetLBText(选中索引, 角色名);
            查询角色信息(角色名);
        }
    }
    else if (响应数据.Find(_T("REBORN_FAILED")) == 0)
    {
        CString 错误信息 = 响应数据.Mid(14); // 去掉"REBORN_FAILED:"
        TRACE(_T("转生失败: %s\n"), 错误信息);
        状态标签.SetWindowText(_T("转生失败"));
        MessageBox(_T("转生失败: ") + 错误信息, _T("失败"), MB_ICONERROR);

        // 立即解除冷却，让用户可以重试
        转生冷却中 = FALSE;
        冷却剩余时间 = 0;
        KillTimer(1);
        转生按钮.EnableWindow(TRUE);
    }
    else
    {
        TRACE(_T("未知转生响应格式\n"));
    }

    TRACE(_T("=== 处理转生响应结束 ===\n"));
}

void 转生页面类::更新角色信息显示(int 职业代码, int 战斗等级, int 累计等级, int 剩余点数)
{
    CString 等级信息;
    等级信息.Format(_T("战斗等级: %d\n累计等级: %d\n剩余点数: %d"),
        战斗等级, 累计等级, 剩余点数);
    等级标签.SetWindowText(等级信息);

    // 检查转生条件
    if (战斗等级 >= 130)
    {
        状态标签.SetWindowText(_T("可以转生"));
        转生按钮.EnableWindow(TRUE);
    }
    else
    {
        状态标签.SetWindowText(_T("等级不足130级"));
        转生按钮.EnableWindow(FALSE);
    }
}

void 转生页面类::处理角色信息响应(const CString& 响应数据)
{
    TRACE(_T("=== 转生页面处理角色信息响应开始 ===\n"));
    TRACE(_T("响应数据: %s\n"), 响应数据);

    if (响应数据.Find(_T("CHAR_INFO:")) == 0)
    {
        CString 角色数据 = 响应数据.Mid(10); // 去掉"CHAR_INFO:"

        CStringArray 参数数组;
        int 起始位置 = 0;
        CString 参数 = 角色数据.Tokenize(_T(":"), 起始位置);

        while (!参数.IsEmpty())
        {
            参数数组.Add(参数);
            参数 = 角色数据.Tokenize(_T(":"), 起始位置);
        }

        if (参数数组.GetSize() >= 8)
        {
            int 职业代码 = _ttoi(参数数组[0]);
            int 战斗等级 = _ttoi(参数数组[1]);
            int 累计等级 = _ttoi(参数数组[2]);
            int 剩余点数 = _ttoi(参数数组[3]);
            int 力量 = _ttoi(参数数组[4]);
            int 敏捷 = _ttoi(参数数组[5]);
            int 意念 = _ttoi(参数数组[6]);
            int 灵力 = _ttoi(参数数组[7]);

            更新角色信息显示(职业代码, 战斗等级, 累计等级, 剩余点数);
        }
    }
    else if (响应数据.Find(_T("CHAR_INFO_FAILED")) == 0)
    {
        TRACE(_T("获取角色信息失败\n"));
        状态标签.SetWindowText(_T("获取角色信息失败"));
    }

    TRACE(_T("=== 转生页面处理角色信息响应结束 ===\n"));
}

void 转生页面类::处理角色列表响应(const CString& 响应数据)
{
    TRACE(_T("=== 处理角色列表响应开始 ===\n"));
    TRACE(_T("响应数据: %s\n"), 响应数据);

    if (响应数据.Find(_T("ROLES_LIST:")) == 0)
    {
        CString 角色列表数据 = 响应数据.Mid(11); // 去掉"ROLES_LIST:"

        角色选择框.ResetContent();

        if (角色列表数据.IsEmpty())
        {
            状态标签.SetWindowText(_T("该账号下没有角色"));
            角色选择框.EnableWindow(FALSE);
        }
        else
        {
            // 解析角色列表
            CStringArray 角色数组;
            int 起始位置 = 0;
            CString 角色名 = 角色列表数据.Tokenize(_T(";"), 起始位置);

            while (!角色名.IsEmpty())
            {
                角色数组.Add(角色名);
                角色名 = 角色列表数据.Tokenize(_T(";"), 起始位置);
            }

            // 添加到下拉框
            for (int i = 0; i < 角色数组.GetSize(); i++)
            {
                角色选择框.AddString(角色数组[i]);
            }

            角色选择框.EnableWindow(TRUE);

            CString 状态信息;
            状态信息.Format(_T("找到 %d 个角色，请选择"), 角色数组.GetSize());
            状态标签.SetWindowText(状态信息);

            // 默认选择第一个角色
            if (角色数组.GetSize() > 0)
            {
                角色选择框.SetCurSel(0);
                OnCbnSelchangeRebirthCombo(); // 触发选择改变事件
            }
        }
    }
    else
    {
        状态标签.SetWindowText(_T("获取角色列表失败"));
    }

    TRACE(_T("=== 处理角色列表响应结束 ===\n"));
}

void 转生页面类::检查角色在线状态(const CString& 角色名)
{
    // 通过主对话框发送检查请求
    CWnd* 主窗口 = AfxGetMainWnd();
    if (!主窗口) return;

    NageDlqDlg* 主对话框 = dynamic_cast<NageDlqDlg*>(主窗口);
    if (!主对话框) return;

    // 构建检查请求
    CString 检查请求;
    检查请求.Format(_T("CHECK_CHAR_ONLINE:%s"), 角色名);

    if (主对话框->发送请求到服务端(检查请求))
    {
        状态标签.SetWindowText(_T("检查角色在线状态..."));

        // 设置临时变量记录当前要处理的角色名
        待处理角色名 = 角色名;

        // 设置超时检查
        SetTimer(2, 5000, nullptr); // 5秒超时
    }
    else
    {
        MessageBox(_T("检查角色在线状态失败"), _T("错误"), MB_ICONERROR);
    }
}

// 添加处理角色在线状态响应函数
void 转生页面类::处理角色在线状态响应(const CString& 响应数据)
{
    TRACE(_T("=== 处理角色在线状态响应开始 ===\n"));
    TRACE(_T("响应数据: %s\n"), 响应数据);

    // 停止超时定时器
    KillTimer(2);

    if (响应数据.Find(_T("CHAR_ONLINE:")) == 0)
    {
        CString 在线状态数据 = 响应数据.Mid(12); // 去掉"CHAR_ONLINE:"
        int 在线状态 = _ttoi(在线状态数据);

        if (在线状态 == 1)
        {
            // 角色在线，不允许转生
            MessageBox(_T("该角色当前在线，无法进行转生操作"), _T("提示"), MB_ICONWARNING);
            状态标签.SetWindowText(_T("角色在线，无法转生"));
        }
        else
        {
            // 角色离线，继续转生流程
            确认转生操作(待处理角色名);
        }
    }
    else
    {
        // 未收到正确响应，默认认为角色在线
        MessageBox(_T("无法确定角色状态，请确保角色已离线"), _T("提示"), MB_ICONWARNING);
        状态标签.SetWindowText(_T("状态检查失败"));
    }

    TRACE(_T("=== 处理角色在线状态响应结束 ===\n"));
}

// 添加确认转生操作函数
void 转生页面类::确认转生操作(const CString& 角色名)
{
    // 确认转生
    CString 确认信息;
    确认信息.Format(_T("确定要对角色【%s】进行转生吗？\n\n转生后角色将重置为1级，并获得相应点数。"), 角色名);

    if (MessageBox(确认信息, _T("确认转生"), MB_YESNO | MB_ICONQUESTION) == IDYES)
    {
        发送转生请求(角色名);

        // 设置冷却时间（防止重复点击）
        转生冷却中 = TRUE;
        冷却剩余时间 = 5; // 5秒冷却
        SetTimer(1, 1000, nullptr); // 启动定时器
        转生按钮.EnableWindow(FALSE);
    }
}
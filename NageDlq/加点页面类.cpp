#include "pch.h"
#include "framework.h"
#include "NageDlq.h"
#include "加点页面类.h"
#include "afxdialogex.h"
#include "NageDlqDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 加点页面类 对话框
IMPLEMENT_DYNAMIC(加点页面类, CDialogEx)

加点页面类::加点页面类(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_PAGE_STATS, pParent)
    , 当前剩余点数(0)
    , 当前职业代码(0)
    , 当前力量(0)
    , 当前敏捷(0)
    , 当前意念(0)
    , 当前灵力(0)
{
}

加点页面类::~加点页面类()
{
}

void 加点页面类::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATIC_S_CHAR, 角色标签);
    DDX_Control(pDX, IDC_STATIC_S_PS, 提示标签);
    DDX_Control(pDX, IDC_STATS_COMBO, 角色选择框);
    DDX_Control(pDX, IDC_STATIC_S_PI, 剩余点数标签);
    DDX_Control(pDX, IDC_STATIC_S_L, 力量标签);
    DDX_Control(pDX, IDC_STATIC_S_M, 敏捷标签);
    DDX_Control(pDX, IDC_STATIC_S_Y, 意念标签);
    DDX_Control(pDX, IDC_STATIC_S_LL, 灵力标签);
    DDX_Control(pDX, IDC_EDIT_S_L, 力量编辑框);
    DDX_Control(pDX, IDC_EDIT_S_M, 敏捷编辑框);
    DDX_Control(pDX, IDC_EDIT_S_Y, 意念编辑框);
    DDX_Control(pDX, IDC_EDIT_S_LL, 灵力编辑框);
    DDX_Control(pDX, IDC_BUTTON_S_YES, 加点按钮);
}

BEGIN_MESSAGE_MAP(加点页面类, CDialogEx)
    ON_CBN_SELCHANGE(IDC_STATS_COMBO, &加点页面类::OnCbnSelchangeStatsCombo)
    ON_BN_CLICKED(IDC_BUTTON_S_YES, &加点页面类::OnBnClickedButtonAddPoints)
    ON_EN_CHANGE(IDC_EDIT_S_L, &加点页面类::OnEnChangeEditS_L)
    ON_EN_CHANGE(IDC_EDIT_S_M, &加点页面类::OnEnChangeEditS_M)
    ON_EN_CHANGE(IDC_EDIT_S_Y, &加点页面类::OnEnChangeEditS_Y)
    ON_EN_CHANGE(IDC_EDIT_S_LL, &加点页面类::OnEnChangeEditS_LL)
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

void 加点页面类::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1)
    {
        // 检查角色在线状态超时
        KillTimer(1);
        MessageBox(_T("检查角色状态超时，请重试"), _T("提示"), MB_ICONWARNING);
        角色标签.SetWindowText(_T("状态检查超时"));
    }

    CDialogEx::OnTimer(nIDEvent);
}

HBRUSH 加点页面类::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    // 如果是提示标签
    if (nCtlColor == CTLCOLOR_STATIC && pWnd == &提示标签)
    {
        pDC->SetTextColor(RGB(0, 128, 0));  // 绿色
        pDC->SetBkMode(TRANSPARENT);  // 透明背景
    }

    // 如果是剩余点数标签且超过限制
    if (nCtlColor == CTLCOLOR_STATIC && pWnd == &剩余点数标签)
    {
        // 这里可以根据需要设置剩余点数标签的颜色
        // 比如当点数不足时显示红色
    }

    return hbr;
}

BOOL 加点页面类::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 设置角色标签字体
    CFont 大字体;
    大字体.CreatePointFont(180, _T("微软雅黑")); // 18号字体
    角色标签.SetFont(&大字体);
    大字体.Detach();

    // 设置提示标签颜色为绿色
    提示标签.SetWindowText(_T("提示：根据职业不同，可加点的属性有所限制"));

    // 初始状态
    加点按钮.EnableWindow(FALSE);
    剩余点数标签.SetWindowText(_T("剩余点数: 0"));

    // 设置属性标签初始值
    力量标签.SetWindowText(_T("力量: 0"));
    敏捷标签.SetWindowText(_T("敏捷: 0"));
    意念标签.SetWindowText(_T("意念: 0"));
    灵力标签.SetWindowText(_T("灵力: 0"));

    // 清空编辑框
    力量编辑框.SetWindowText(_T(""));
    敏捷编辑框.SetWindowText(_T(""));
    意念编辑框.SetWindowText(_T(""));
    灵力编辑框.SetWindowText(_T(""));

    return TRUE;
}

void 加点页面类::刷新角色列表()
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
        角色标签.SetWindowText(_T("请先登录"));
        return;
    }

    当前用户名 = 用户名;

    // 发送获取角色列表请求
    CString 获取角色请求;
    获取角色请求.Format(_T("GET_ROLES:%s"), 当前用户名);

    if (主对话框->发送请求到服务端(获取角色请求))
    {
        角色标签.SetWindowText(_T("获取角色列表中..."));
        角色选择框.ResetContent();
        角色选择框.EnableWindow(FALSE);
    }
    else
    {
        角色标签.SetWindowText(_T("获取角色列表失败"));
    }
}

void 加点页面类::OnCbnSelchangeStatsCombo()
{
    int 选中索引 = 角色选择框.GetCurSel();
    if (选中索引 != CB_ERR)
    {
        CString 角色名;
        角色选择框.GetLBText(选中索引, 角色名);
        查询角色信息(角色名);
    }
}

void 加点页面类::OnBnClickedButtonAddPoints()
{
    int 选中索引 = 角色选择框.GetCurSel();
    if (选中索引 == CB_ERR)
    {
        MessageBox(_T("请选择角色"), _T("提示"), MB_ICONWARNING);
        return;
    }

    CString 角色名;
    角色选择框.GetLBText(选中索引, 角色名);

    // 获取输入的加点数值
    CString 力量文本, 敏捷文本, 意念文本, 灵力文本;
    力量编辑框.GetWindowText(力量文本);
    敏捷编辑框.GetWindowText(敏捷文本);
    意念编辑框.GetWindowText(意念文本);
    灵力编辑框.GetWindowText(灵力文本);

    int 力量 = _ttoi(力量文本);
    int 敏捷 = _ttoi(敏捷文本);
    int 意念 = _ttoi(意念文本);
    int 灵力 = _ttoi(灵力文本);

    // 验证输入
    if (!验证加点输入(力量, 敏捷, 意念, 灵力, 当前剩余点数, 当前职业代码))
    {
        return;
    }

    // 先检查角色是否在线
    检查角色在线状态(角色名, 力量, 敏捷, 意念, 灵力);
}

void 加点页面类::OnEnChangeEditS_L()
{
    // 实时计算总点数
    计算总点数();
}

void 加点页面类::OnEnChangeEditS_M()
{
    // 实时计算总点数
    计算总点数();
}

void 加点页面类::OnEnChangeEditS_Y()
{
    // 实时计算总点数
    计算总点数();
}

void 加点页面类::OnEnChangeEditS_LL()
{
    // 实时计算总点数
    计算总点数();
}

void 加点页面类::计算总点数()
{
    CString 力量文本, 敏捷文本, 意念文本, 灵力文本;
    力量编辑框.GetWindowText(力量文本);
    敏捷编辑框.GetWindowText(敏捷文本);
    意念编辑框.GetWindowText(意念文本);
    灵力编辑框.GetWindowText(灵力文本);

    int 力量 = 力量文本.IsEmpty() ? 0 : _ttoi(力量文本);
    int 敏捷 = 敏捷文本.IsEmpty() ? 0 : _ttoi(敏捷文本);
    int 意念 = 意念文本.IsEmpty() ? 0 : _ttoi(意念文本);
    int 灵力 = 灵力文本.IsEmpty() ? 0 : _ttoi(灵力文本);

    int 总点数 = 力量 + 敏捷 + 意念 + 灵力;

    // 更新剩余点数显示
    CString 剩余点数文本;
    剩余点数文本.Format(_T("剩余点数: %d (使用: %d)"), 当前剩余点数 - 总点数, 总点数);
    剩余点数标签.SetWindowText(剩余点数文本);

    // 检查是否可以加点
    if (总点数 > 0 && 总点数 <= 当前剩余点数)
    {
        加点按钮.EnableWindow(TRUE);
    }
    else
    {
        加点按钮.EnableWindow(FALSE);
    }

    剩余点数标签.Invalidate();
}

void 加点页面类::查询角色信息(const CString& 角色名)
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
        角色标签.SetWindowText(_T("查询角色信息中..."));
    }
    else
    {
        角色标签.SetWindowText(_T("查询失败"));
    }
}

void 加点页面类::发送加点请求(const CString& 角色名, int 力量, int 敏捷, int 意念, int 灵力)
{
    // 通过主对话框发送加点请求
    CWnd* 主窗口 = AfxGetMainWnd();
    if (!主窗口) return;

    NageDlqDlg* 主对话框 = dynamic_cast<NageDlqDlg*>(主窗口);
    if (!主对话框) return;

    // 构建加点请求
    CString 加点请求;
    加点请求.Format(_T("ADD_POINTS:%s:%s:%d:%d:%d:%d"),
        当前用户名, 角色名, 力量, 敏捷, 意念, 灵力);

    if (主对话框->发送请求到服务端(加点请求))
    {
        角色标签.SetWindowText(_T("加点请求发送中..."));
        加点按钮.EnableWindow(FALSE);
    }
    else
    {
        角色标签.SetWindowText(_T("加点请求发送失败"));
    }
}

void 加点页面类::处理加点响应(const CString& 响应数据)
{
    TRACE(_T("=== 处理加点响应开始 ===\n"));
    TRACE(_T("响应数据: %s\n"), 响应数据);

    if (响应数据.Find(_T("ADD_POINTS_SUCCESS")) == 0)
    {
        TRACE(_T("加点成功\n"));
        角色标签.SetWindowText(_T("加点成功！"));
        MessageBox(_T("加点成功！"), _T("成功"), MB_ICONINFORMATION);

        // 刷新角色信息
        int 选中索引 = 角色选择框.GetCurSel();
        if (选中索引 != CB_ERR)
        {
            CString 角色名;
            角色选择框.GetLBText(选中索引, 角色名);
            查询角色信息(角色名);
        }

        // 清空编辑框
        力量编辑框.SetWindowText(_T(""));
        敏捷编辑框.SetWindowText(_T(""));
        意念编辑框.SetWindowText(_T(""));
        灵力编辑框.SetWindowText(_T(""));
    }
    else if (响应数据.Find(_T("ADD_POINTS_FAILED")) == 0)
    {
        CString 错误信息 = 响应数据.Mid(18); // 去掉"ADD_POINTS_FAILED:"
        TRACE(_T("加点失败: %s\n"), 错误信息);
        角色标签.SetWindowText(_T("加点失败"));
        MessageBox(_T("加点失败: ") + 错误信息, _T("失败"), MB_ICONERROR);
        加点按钮.EnableWindow(TRUE);
    }
    else
    {
        TRACE(_T("未知加点响应格式\n"));
    }

    TRACE(_T("=== 处理加点响应结束 ===\n"));
}

void 加点页面类::处理角色信息响应(const CString& 响应数据)
{
    TRACE(_T("=== 处理角色信息响应开始 ===\n"));
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

            更新属性显示(职业代码, 剩余点数, 力量, 敏捷, 意念, 灵力);
        }
    }
    else if (响应数据.Find(_T("CHAR_INFO_FAILED")) == 0)
    {
        TRACE(_T("获取角色信息失败\n"));
        角色标签.SetWindowText(_T("获取角色信息失败"));
    }

    TRACE(_T("=== 处理角色信息响应结束 ===\n"));
}

void 加点页面类::更新属性显示(int 职业代码, int 剩余点数, int 力量, int 敏捷, int 意念, int 灵力)
{
    // 保存当前属性
    当前职业代码 = 职业代码;
    当前剩余点数 = 剩余点数;
    当前力量 = 力量;
    当前敏捷 = 敏捷;
    当前意念 = 意念;
    当前灵力 = 灵力;

    // 更新显示
    CString 剩余点数文本;
    剩余点数文本.Format(_T("剩余点数: %d"), 剩余点数);
    剩余点数标签.SetWindowText(剩余点数文本);

    CString 力量文本;
    力量文本.Format(_T("力量: %d"), 力量);
    力量标签.SetWindowText(力量文本);

    CString 敏捷文本;
    敏捷文本.Format(_T("敏捷: %d"), 敏捷);
    敏捷标签.SetWindowText(敏捷文本);

    CString 意念文本;
    意念文本.Format(_T("意念: %d"), 意念);
    意念标签.SetWindowText(意念文本);

    CString 灵力文本;
    灵力文本.Format(_T("灵力: %d"), 灵力);
    灵力标签.SetWindowText(灵力文本);

    // 根据职业调整输入框状态
    根据职业调整输入框(职业代码);

    // 清空编辑框
    力量编辑框.SetWindowText(_T(""));
    敏捷编辑框.SetWindowText(_T(""));
    意念编辑框.SetWindowText(_T(""));
    灵力编辑框.SetWindowText(_T(""));

    // 更新角色标签
    CString 职业名称;
    switch (职业代码)
    {
    case 0: 职业名称 = _T("格斗"); break;
    case 2: 职业名称 = _T("舞械"); break;
    case 6: 职业名称 = _T("超能"); break;
    case 7: 职业名称 = _T("枪手"); break;
    default: 职业名称 = _T("未知职业"); break;
    }

    CString 角色信息;
    角色信息.Format(_T("角色信息 - %s (剩余点数: %d)"), 职业名称, 剩余点数);
    角色标签.SetWindowText(角色信息);
}

void 加点页面类::根据职业调整输入框(int 职业代码)
{
    // 根据职业启用/禁用相应的输入框
    switch (职业代码)
    {
    case 0: // 格斗
    case 2: // 舞械
        力量编辑框.EnableWindow(TRUE);
        敏捷编辑框.EnableWindow(TRUE);
        意念编辑框.EnableWindow(FALSE);
        灵力编辑框.EnableWindow(FALSE);

        // 清空禁用的编辑框
        意念编辑框.SetWindowText(_T(""));
        灵力编辑框.SetWindowText(_T(""));
        break;

    case 6: // 超能
    case 7: // 枪手
        力量编辑框.EnableWindow(FALSE);
        敏捷编辑框.EnableWindow(TRUE);
        意念编辑框.EnableWindow(TRUE);
        灵力编辑框.EnableWindow(TRUE);

        // 清空禁用的编辑框
        力量编辑框.SetWindowText(_T(""));
        break;

    default:
        // 默认全部启用
        力量编辑框.EnableWindow(TRUE);
        敏捷编辑框.EnableWindow(TRUE);
        意念编辑框.EnableWindow(TRUE);
        灵力编辑框.EnableWindow(TRUE);
        break;
    }
}

BOOL 加点页面类::验证加点输入(int 力量, int 敏捷, int 意念, int 灵力, int 剩余点数, int 职业代码)
{
    // 检查总点数
    int 总点数 = 力量 + 敏捷 + 意念 + 灵力;

    if (总点数 <= 0)
    {
        MessageBox(_T("请至少输入一个属性的加点数值"), _T("提示"), MB_ICONWARNING);
        return FALSE;
    }

    if (总点数 > 剩余点数)
    {
        MessageBox(_T("加点总数超过剩余点数"), _T("错误"), MB_ICONERROR);
        return FALSE;
    }

    // 检查职业限制
    switch (职业代码)
    {
    case 0: // 格斗
    case 2: // 舞械
        if (意念 != 0 || 灵力 != 0)
        {
            MessageBox(_T("格斗和舞械职业不能加意念和灵力"), _T("错误"), MB_ICONERROR);
            return FALSE;
        }
        break;

    case 6: // 超能
    case 7: // 枪手
        if (力量 != 0)
        {
            MessageBox(_T("超能和枪手职业不能加力量"), _T("错误"), MB_ICONERROR);
            return FALSE;
        }
        break;
    }

    // 检查数值有效性
    if (力量 < 0 || 敏捷 < 0 || 意念 < 0 || 灵力 < 0)
    {
        MessageBox(_T("加点数值不能为负数"), _T("错误"), MB_ICONERROR);
        return FALSE;
    }

    return TRUE;
}

void 加点页面类::处理角色列表响应(const CString& 响应数据)
{
    TRACE(_T("=== 加点页面处理角色列表响应开始 ===\n"));
    TRACE(_T("响应数据: %s\n"), 响应数据);

    if (响应数据.Find(_T("ROLES_LIST:")) == 0)
    {
        CString 角色列表数据 = 响应数据.Mid(11); // 去掉"ROLES_LIST:"

        角色选择框.ResetContent();

        if (角色列表数据.IsEmpty())
        {
            角色标签.SetWindowText(_T("该账号下没有角色"));
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
            角色标签.SetWindowText(状态信息);

            // 默认选择第一个角色
            if (角色数组.GetSize() > 0)
            {
                角色选择框.SetCurSel(0);
                OnCbnSelchangeStatsCombo(); // 触发选择改变事件
            }
        }
    }
    else
    {
        角色标签.SetWindowText(_T("获取角色列表失败"));
    }

    TRACE(_T("=== 加点页面处理角色列表响应结束 ===\n"));
}

void 加点页面类::检查角色在线状态(const CString& 角色名, int 力量, int 敏捷, int 意念, int 灵力)
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
        角色标签.SetWindowText(_T("检查角色在线状态..."));

        // 保存加点参数
        待处理角色名 = 角色名;
        待处理力量 = 力量;
        待处理敏捷 = 敏捷;
        待处理意念 = 意念;
        待处理灵力 = 灵力;

        // 设置超时检查
        SetTimer(1, 5000, nullptr); // 5秒超时
    }
    else
    {
        MessageBox(_T("检查角色在线状态失败"), _T("错误"), MB_ICONERROR);
    }
}

void 加点页面类::处理角色在线状态响应(const CString& 响应数据)
{
    TRACE(_T("=== 加点页面处理角色在线状态响应开始 ===\n"));
    TRACE(_T("响应数据: %s\n"), 响应数据);

    // 停止超时定时器
    KillTimer(1);

    if (响应数据.Find(_T("CHAR_ONLINE:")) == 0)
    {
        CString 在线状态数据 = 响应数据.Mid(12); // 去掉"CHAR_ONLINE:"
        int 在线状态 = _ttoi(在线状态数据);

        if (在线状态 == 1)
        {
            // 角色在线，不允许加点
            MessageBox(_T("该角色当前在线，无法进行加点操作"), _T("提示"), MB_ICONWARNING);
            角色标签.SetWindowText(_T("角色在线，无法加点"));
        }
        else
        {
            // 角色离线，继续加点流程
            确认加点操作(待处理角色名, 待处理力量, 待处理敏捷, 待处理意念, 待处理灵力);
        }
    }
    else
    {
        // 未收到正确响应，默认认为角色在线
        MessageBox(_T("无法确定角色状态，请确保角色已离线"), _T("提示"), MB_ICONWARNING);
        角色标签.SetWindowText(_T("状态检查失败"));
    }

    TRACE(_T("=== 加点页面处理角色在线状态响应结束 ===\n"));
}

void 加点页面类::确认加点操作(const CString& 角色名, int 力量, int 敏捷, int 意念, int 灵力)
{
    // 确认加点
    CString 确认信息;
    确认信息.Format(_T("确定要对角色【%s】进行加点吗？\n\n")
        _T("力量: +%d\n敏捷: +%d\n意念: +%d\n灵力: +%d\n\n")
        _T("总计消耗点数: %d"),
        角色名, 力量, 敏捷, 意念, 灵力, 力量 + 敏捷 + 意念 + 灵力);

    if (MessageBox(确认信息, _T("确认加点"), MB_YESNO | MB_ICONQUESTION) == IDYES)
    {
        发送加点请求(角色名, 力量, 敏捷, 意念, 灵力);
    }
}


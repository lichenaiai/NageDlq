// 排行榜页面类.cpp
#include "pch.h"
#include "framework.h"
#include "NageDlq.h"
#include "排行榜页面类.h"
#include "afxdialogex.h"
#include "NageDlqDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 排行榜页面类 对话框
IMPLEMENT_DYNAMIC(排行榜页面类, CDialogEx)

排行榜页面类::排行榜页面类(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_PAGE_RANKING, pParent)
{
}

排行榜页面类::~排行榜页面类()
{
}

void 排行榜页面类::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_RANKING_TITLE, 标题标签);
    DDX_Control(pDX, IDC_RANKING_LIST, 排行榜列表控件);
}

BEGIN_MESSAGE_MAP(排行榜页面类, CDialogEx)
    //ON_BN_CLICKED(IDC_BUTTON_REFRESH, &排行榜页面类::OnBnClickedButtonRefresh)
END_MESSAGE_MAP()

BOOL 排行榜页面类::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 设置标题标签字体
    CFont 大字体;
    大字体.CreatePointFont(240, _T("微软雅黑")); // 24号字体
    标题标签.SetFont(&大字体);
    大字体.Detach();

    // 设置标题文本
    标题标签.SetWindowText(_T("角色排行榜"));

    // 初始化列表控件
    初始化列表控件();

    // 初始加载排行榜数据
    刷新排行榜数据();

    return TRUE;
}

// 初始化列表控件
void 排行榜页面类::初始化列表控件()
{
    // 确保控件窗口已创建
    if (!排行榜列表控件.GetSafeHwnd())
    {
        TRACE(_T("初始化列表控件：排行榜列表控件窗口尚未创建\n"));
        return;
    }

    // 确保列表为报表模式（有表头）
    if ((排行榜列表控件.GetStyle() & LVS_REPORT) == 0)
    {
        排行榜列表控件.ModifyStyle(0, LVS_REPORT);
    }

    // 设置列表控件样式
    排行榜列表控件.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    // 安全地获取表头并删除现有列
    CHeaderCtrl* pHeader = 排行榜列表控件.GetHeaderCtrl();
    int 列数 = (pHeader != nullptr) ? pHeader->GetItemCount() : 0;
    for (int i = 列数 - 1; i >= 0; --i)
    {
        排行榜列表控件.DeleteColumn(i);
    }

    // 添加列
    排行榜列表控件.InsertColumn(0, _T("排名"), LVCFMT_CENTER, 60);
    排行榜列表控件.InsertColumn(1, _T("角色名字"), LVCFMT_CENTER, 150);
    排行榜列表控件.InsertColumn(2, _T("职业"), LVCFMT_CENTER, 60);
    排行榜列表控件.InsertColumn(3, _T("转生次数"), LVCFMT_CENTER, 80);
    排行榜列表控件.InsertColumn(4, _T("累计等级"), LVCFMT_CENTER, 80);

    TRACE(_T("排行榜列表控件初始化完成\n"));
}

// 刷新排行榜数据
void 排行榜页面类::刷新排行榜数据()
{
    // 清空现有数据
    清空列表数据();

    // 显示加载中提示
    int 加载中项索引 = 排行榜列表控件.InsertItem(0, _T(""));
    排行榜列表控件.SetItemText(加载中项索引, 1, _T("加载中..."));
    排行榜列表控件.SetItemText(加载中项索引, 2, _T("请稍候"));

    // 从服务端获取数据
    从服务端获取排行榜数据();
}

// 从服务端获取排行榜数据
void 排行榜页面类::从服务端获取排行榜数据()
{
    // 通过主对话框发送获取排行榜请求
    CWnd* 主窗口 = AfxGetMainWnd();
    if (!主窗口)
    {
        TRACE(_T("获取主窗口失败\n"));
        return;
    }

    NageDlqDlg* 主对话框 = dynamic_cast<NageDlqDlg*>(主窗口);
    if (!主对话框)
    {
        TRACE(_T("获取主对话框失败\n"));
        return;
    }

    // 构建获取排行榜请求
    CString 排行榜请求 = _T("GET_RANKING:20"); // 获取前20名

    if (主对话框->发送请求到服务端(排行榜请求))
    {
        TRACE(_T("排行榜请求发送成功\n"));
    }
    else
    {
        TRACE(_T("排行榜请求发送失败\n"));
        // 显示错误信息
        清空列表数据();
        int 错误项索引 = 排行榜列表控件.InsertItem(0, _T(""));
        排行榜列表控件.SetItemText(错误项索引, 1, _T("获取数据失败"));
        排行榜列表控件.SetItemText(错误项索引, 2, _T("请检查网络连接"));
    }
}

// 处理排行榜数据响应
void 排行榜页面类::处理排行榜数据响应(const CString& 响应数据)
{
    TRACE(_T("=== 处理排行榜数据响应开始 ===\n"));
    TRACE(_T("响应数据: %s\n"), 响应数据);

    // 清空现有数据
    清空列表数据();

    if (响应数据.Find(_T("RANKING_DATA:")) == 0)
    {
        CString 排行榜数据 = 响应数据.Mid(13); // 去掉"RANKING_DATA:"

        TRACE(_T("排行榜数据: %s\n"), 排行榜数据);

        // 解析排行榜数据
        CStringArray 角色数据数组;
        int 起始位置 = 0;
        CString 角色数据 = 排行榜数据.Tokenize(_T("|"), 起始位置);

        // 先收集所有数据
        CStringArray 所有角色数据;
        while (!角色数据.IsEmpty())
        {
            所有角色数据.Add(角色数据);
            角色数据 = 排行榜数据.Tokenize(_T("|"), 起始位置);
        }

        // 按升序显示：从第1名开始（最高等级）
        int 总角色数 = 所有角色数据.GetSize();
        int 排名 = 1;

        for (int i = 0; i < 总角色数; i++)
        {
            CString 当前角色数据 = 所有角色数据[i];

            // 解析单个角色数据：角色名,职业代码,转生次数,累计等级
            CStringArray 角色字段数组;
            int 字段位置 = 0;
            CString 字段 = 当前角色数据.Tokenize(_T(","), 字段位置);

            CString 角色名, 职业代码文本, 转生次数文本, 累计等级文本;

            if (!字段.IsEmpty())
            {
                角色名 = 字段;
                字段 = 当前角色数据.Tokenize(_T(","), 字段位置);
            }
            if (!字段.IsEmpty())
            {
                职业代码文本 = 字段;
                字段 = 当前角色数据.Tokenize(_T(","), 字段位置);
            }
            if (!字段.IsEmpty())
            {
                转生次数文本 = 字段;
                字段 = 当前角色数据.Tokenize(_T(","), 字段位置);
            }
            if (!字段.IsEmpty())
            {
                累计等级文本 = 字段;
            }

            if (!角色名.IsEmpty() && !职业代码文本.IsEmpty() &&
                !转生次数文本.IsEmpty() && !累计等级文本.IsEmpty())
            {
                int 职业代码 = _ttoi(职业代码文本);
                int 转生次数 = _ttoi(转生次数文本);
                int 累计等级 = _ttoi(累计等级文本);

                CString 职业名称 = 获取职业名称(职业代码);

                // 添加到列表（排名从1开始递增）
                添加排行榜项(排名, 角色名, 职业名称, 转生次数, 累计等级);
                排名++;
            }
        }

        if (排名 == 1)
        {
            // 没有数据
            int 空项索引 = 排行榜列表控件.InsertItem(0, _T(""));
            排行榜列表控件.SetItemText(空项索引, 1, _T("暂无数据"));
        }
    }
    else if (响应数据.Find(_T("RANKING_FAILED")) == 0)
    {
        TRACE(_T("获取排行榜数据失败\n"));
        int 错误项索引 = 排行榜列表控件.InsertItem(0, _T(""));
        排行榜列表控件.SetItemText(错误项索引, 1, _T("获取数据失败"));
        排行榜列表控件.SetItemText(错误项索引, 2, _T("服务器错误"));
    }
    else
    {
        TRACE(_T("未知的排行榜响应格式\n"));
        int 错误项索引 = 排行榜列表控件.InsertItem(0, _T(""));
        排行榜列表控件.SetItemText(错误项索引, 1, _T("数据格式错误"));
        排行榜列表控件.SetItemText(错误项索引, 2, _T("请重试"));
    }

    TRACE(_T("=== 处理排行榜数据响应结束 ===\n"));
}

// 清空列表数据
void 排行榜页面类::清空列表数据()
{
    排行榜列表控件.DeleteAllItems();
}

// 添加排行榜项
void 排行榜页面类::添加排行榜项(int 排名, const CString& 角色名字, const CString& 职业, int 转生次数, int 累计等级)
{
    // 获取当前列表项数量，新项插入到末尾
    int 当前项数 = 排行榜列表控件.GetItemCount();
    int 项索引 = 排行榜列表控件.InsertItem(当前项数, _T(""));

    CString 排名文本;
    排名文本.Format(_T("%d"), 排名);
    排行榜列表控件.SetItemText(项索引, 0, 排名文本);
    排行榜列表控件.SetItemText(项索引, 1, 角色名字);
    排行榜列表控件.SetItemText(项索引, 2, 职业);

    CString 转生次数文本;
    转生次数文本.Format(_T("%d"), 转生次数);
    排行榜列表控件.SetItemText(项索引, 3, 转生次数文本);

    CString 累计等级文本;
    累计等级文本.Format(_T("%d"), 累计等级);
    排行榜列表控件.SetItemText(项索引, 4, 累计等级文本);

    TRACE(_T("添加排行榜项: 排名%d, 角色%s, 等级%d\n"), 排名, 角色名字, 累计等级);
}

// 获取职业名称
CString 排行榜页面类::获取职业名称(int 职业代码)
{
    switch (职业代码)
    {
    case 0:
        return _T("格斗");
    case 2:
        return _T("舞械");
    case 6:
        return _T("超能");
    case 7:
        return _T("枪手");
    default:
        return _T("未知");
    }
}

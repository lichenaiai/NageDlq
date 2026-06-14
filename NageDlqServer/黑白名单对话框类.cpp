// 黑白名单对话框类.cpp
#include "pch.h"
#include "framework.h"
#include "NageDlqServer.h"
#include "黑白名单对话框类.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 输入对话框类 实现
IMPLEMENT_DYNAMIC(输入对话框类, CDialogEx)

输入对话框类::输入对话框类(const CString& 提示文本, const CString& 初始值, CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_INPUT_DIALOG, pParent)
    , 提示文本(提示文本)
    , 输入文本(初始值)
{
}

输入对话框类::~输入对话框类()
{
}

void 输入对话框类::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_INPUT, 输入编辑框);
}

BEGIN_MESSAGE_MAP(输入对话框类, CDialogEx)
    ON_BN_CLICKED(IDOK, &输入对话框类::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL 输入对话框类::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    SetWindowText(_T("输入IP地址"));

    // 设置提示文本
    CWnd* 提示控件 = GetDlgItem(IDC_STATIC_PROMPT);
    if (提示控件)
    {
        提示控件->SetWindowText(提示文本);
    }

    // 设置初始值
    输入编辑框.SetWindowText(输入文本);
    输入编辑框.SetFocus();
    输入编辑框.SetSel(0, -1);

    return FALSE; // 返回TRUE除非将焦点设置到控件
}

void 输入对话框类::OnBnClickedOk()
{
    输入编辑框.GetWindowText(输入文本);
    CDialogEx::OnOK();
}

// 黑白名单对话框类 实现
IMPLEMENT_DYNAMIC(黑白名单对话框类, CDialogEx)

黑白名单对话框类::黑白名单对话框类(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_BWLIST_DIALOG, pParent)
    , 自动拉黑启用(FALSE)
    , 自动拉黑阈值(5)
    , 自动拉黑窗口秒(60)
{
}

黑白名单对话框类::~黑白名单对话框类()
{
}

void 黑白名单对话框类::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_BLIST_BWLIST, 黑名单列表框);
    DDX_Control(pDX, IDC_WLIST_BWLIST, 白名单列表框);
    DDX_Control(pDX, IDC_AUTO_BLACKLIST_ENABLE, 自动拉黑复选框);
    DDX_Control(pDX, IDC_AUTO_BLACKLIST_THRESHOLD, 阈值编辑框);
    DDX_Control(pDX, IDC_AUTO_BLACKLIST_WINDOW, 窗口编辑框);
}

BEGIN_MESSAGE_MAP(黑白名单对话框类, CDialogEx)
    ON_BN_CLICKED(ID_SAVE_BW, &黑白名单对话框类::OnBnClickedSaveBw)
    ON_BN_CLICKED(ID_CLOSE_BW, &黑白名单对话框类::OnBnClickedCloseBw)
    ON_LBN_DBLCLK(IDC_BLIST_BWLIST, &黑白名单对话框类::OnLbnDblclkBlistBwlist)
    ON_LBN_DBLCLK(IDC_WLIST_BWLIST, &黑白名单对话框类::OnLbnDblclkWlistBwlist)
    ON_BN_CLICKED(IDC_AUTO_BLACKLIST_ENABLE, &黑白名单对话框类::OnBnClickedAutoBlacklist)
END_MESSAGE_MAP()

// 黑白名单对话框类 消息处理程序
BOOL 黑白名单对话框类::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    SetWindowText(_T("IP黑白名单管理"));

    // 加载IP列表
    加载IP列表();

    // 加载自动拉黑配置
    加载自动拉黑配置();

    return TRUE;
}

void 黑白名单对话框类::加载IP列表()
{
    黑名单列表框.ResetContent();
    白名单列表框.ResetContent();

    HKEY 注册表键;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\NageServer\\IPLists"), 0, KEY_READ, &注册表键) == ERROR_SUCCESS)
    {
        DWORD 值类型, 值大小;
        TCHAR 值数据[4096];

        // 加载黑名单
        值大小 = sizeof(值数据);
        if (RegQueryValueEx(注册表键, _T("BlackList"), NULL, &值类型, (LPBYTE)值数据, &值大小) == ERROR_SUCCESS)
        {
            CString 黑名单字符串(值数据);
            int 位置 = 0;
            CString IP地址 = 黑名单字符串.Tokenize(_T(";"), 位置);
            int 序号 = 1;
            while (!IP地址.IsEmpty())
            {
                IP地址.Trim();
                if (!IP地址.IsEmpty())
                {
                    CString 显示文本;
                    显示文本.Format(_T("%d. %s"), 序号++, IP地址);
                    黑名单列表框.AddString(显示文本);
                }
                IP地址 = 黑名单字符串.Tokenize(_T(";"), 位置);
            }
        }

        // 加载白名单
        值大小 = sizeof(值数据);
        if (RegQueryValueEx(注册表键, _T("WhiteList"), NULL, &值类型, (LPBYTE)值数据, &值大小) == ERROR_SUCCESS)
        {
            CString 白名单字符串(值数据);
            int 位置 = 0;
            CString IP地址 = 白名单字符串.Tokenize(_T(";"), 位置);
            int 序号 = 1;
            while (!IP地址.IsEmpty())
            {
                IP地址.Trim();
                if (!IP地址.IsEmpty())
                {
                    CString 显示文本;
                    显示文本.Format(_T("%d. %s"), 序号++, IP地址);
                    白名单列表框.AddString(显示文本);
                }
                IP地址 = 白名单字符串.Tokenize(_T(";"), 位置);
            }
        }

        RegCloseKey(注册表键);
    }
}

void 黑白名单对话框类::保存IP列表()
{
    CString 黑名单字符串, 白名单字符串;

    // 收集黑名单字符串
    for (int i = 0; i < 黑名单列表框.GetCount(); i++)
    {
        CString 项文本;
        黑名单列表框.GetText(i, 项文本);

        // 获取IP地址（去掉序号）
        int 点位置 = 项文本.Find(_T(". "));
        if (点位置 != -1)
        {
            CString IP地址 = 项文本.Mid(点位置 + 2);
            IP地址.Trim();
            if (!IP地址.IsEmpty())
            {
                if (!黑名单字符串.IsEmpty())
                    黑名单字符串 += _T(";");
                黑名单字符串 += IP地址;
            }
        }
    }

    // 收集白名单字符串
    for (int i = 0; i < 白名单列表框.GetCount(); i++)
    {
        CString 项文本;
        白名单列表框.GetText(i, 项文本);

        // 获取IP地址（去掉序号）
        int 点位置 = 项文本.Find(_T(". "));
        if (点位置 != -1)
        {
            CString IP地址 = 项文本.Mid(点位置 + 2);
            IP地址.Trim();
            if (!IP地址.IsEmpty())
            {
                if (!白名单字符串.IsEmpty())
                    白名单字符串 += _T(";");
                白名单字符串 += IP地址;
            }
        }
    }

    // 保存到注册表
    HKEY 注册表键;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\NageServer\\IPLists"), 0, NULL, 0, KEY_WRITE, NULL, &注册表键, NULL) == ERROR_SUCCESS)
    {
        RegSetValueEx(注册表键, _T("BlackList"), 0, REG_SZ, (const BYTE*)(LPCTSTR)黑名单字符串, (黑名单字符串.GetLength() + 1) * sizeof(TCHAR));
        RegSetValueEx(注册表键, _T("WhiteList"), 0, REG_SZ, (const BYTE*)(LPCTSTR)白名单字符串, (白名单字符串.GetLength() + 1) * sizeof(TCHAR));
        RegCloseKey(注册表键);
    }

    AfxMessageBox(_T("黑白名单保存成功！"), MB_ICONINFORMATION);
}

void 黑白名单对话框类::OnBnClickedSaveBw()
{
    保存IP列表();
    保存自动拉黑配置();
}

void 黑白名单对话框类::OnBnClickedCloseBw()
{
    // 关闭时也保存自动拉黑配置
    保存自动拉黑配置();
    EndDialog(IDOK);
}

void 黑白名单对话框类::OnBnClickedAutoBlacklist()
{
    自动拉黑启用 = (自动拉黑复选框.GetCheck() == BST_CHECKED);
    // 启用/禁用阈值编辑框
    阈值编辑框.EnableWindow(自动拉黑启用);
    窗口编辑框.EnableWindow(自动拉黑启用);
}

void 黑白名单对话框类::OnLbnDblclkBlistBwlist()
{
    int 选择索引 = 黑名单列表框.GetCurSel();
    if (选择索引 != LB_ERR)
    {
        编辑列表项目(黑名单列表框, 选择索引);
    }
}

void 黑白名单对话框类::OnLbnDblclkWlistBwlist()
{
    int 选择索引 = 白名单列表框.GetCurSel();
    if (选择索引 != LB_ERR)
    {
        编辑列表项目(白名单列表框, 选择索引);
    }
}

BOOL 黑白名单对话框类::编辑列表项目(CListBox& 列表框, int 项目索引)
{
    CString 项文本;
    列表框.GetText(项目索引, 项文本);

    // 获取原始IP（去掉序号）
    CString 旧IP;
    int 点位置 = 项文本.Find(_T(". "));
    if (点位置 != -1)
    {
        旧IP = 项文本.Mid(点位置 + 2);
    }
    else
    {
        旧IP = 项文本;
    }

    CString 新IP = 旧IP;

    // 创建编辑对话框
    CString 提示信息;
    if (&列表框 == &黑名单列表框)
        提示信息 = _T("编辑黑名单IP地址:");
    else
        提示信息 = _T("编辑白名单IP地址:");

    输入对话框类 输入对话框(提示信息, 新IP);
    if (输入对话框.DoModal() == IDOK)
    {
        新IP = 输入对话框.获取输入文本();
        新IP.Trim();

        // 验证IP地址
        CString 验证后的IP = 验证IP地址(新IP);
        if (验证后的IP.IsEmpty())
        {
            AfxMessageBox(_T("IP地址格式无效！"), MB_ICONWARNING);
            return FALSE;
        }

        // 更新列表项
        CString 新文本;
        新文本.Format(_T("%d. %s"), 项目索引 + 1, 验证后的IP);
        列表框.DeleteString(项目索引);
        列表框.InsertString(项目索引, 新文本);
        列表框.SetCurSel(项目索引);

        return TRUE;
    }

    return FALSE;
}

CString 黑白名单对话框类::验证IP地址(const CString& IP地址)
{
    CString 验证后的IP = IP地址;
    验证后的IP.Trim();

    // 简单的IP地址验证
    if (验证后的IP.IsEmpty())
        return _T("");

    // 检查IP地址格式（简单验证）
    CStringArray 部分数组;
    int 位置 = 0;
    CString 部分 = 验证后的IP.Tokenize(_T("."), 位置);
    int 部分计数 = 0;

    while (!部分.IsEmpty())
    {
        部分数组.Add(部分);
        部分 = 验证后的IP.Tokenize(_T("."), 位置);
        部分计数++;
    }

    if (部分计数 != 4)
        return _T("");

    for (int i = 0; i < 部分数组.GetSize(); i++)
    {
        int 数字 = _ttoi(部分数组[i]);
        if (数字 < 0 || 数字 > 255)
            return _T("");
    }

    return 验证后的IP;
}

// ============================================================
// 自动拉黑配置：加载/保存
// ============================================================
void 黑白名单对话框类::加载自动拉黑配置()
{
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\NageServer\\IPLists"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD dwType, dwSize, dwValue;

        // 读取自动拉黑启用状态
        dwSize = sizeof(DWORD);
        if (RegQueryValueEx(hKey, _T("AutoBlacklistEnable"), NULL, &dwType, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
        {
            自动拉黑启用 = (dwValue != 0);
        }

        // 读取触发阈值
        dwSize = sizeof(DWORD);
        if (RegQueryValueEx(hKey, _T("AutoBlacklistThreshold"), NULL, &dwType, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
        {
            自动拉黑阈值 = (int)dwValue;
        }

        // 读取时间窗口
        dwSize = sizeof(DWORD);
        if (RegQueryValueEx(hKey, _T("AutoBlacklistWindow"), NULL, &dwType, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
        {
            自动拉黑窗口秒 = (int)dwValue;
        }

        RegCloseKey(hKey);
    }

    // 更新UI
    自动拉黑复选框.SetCheck(自动拉黑启用 ? BST_CHECKED : BST_UNCHECKED);

    CString 阈值文本;
    阈值文本.Format(_T("%d"), 自动拉黑阈值);
    阈值编辑框.SetWindowText(阈值文本);

    CString 窗口文本;
    窗口文本.Format(_T("%d"), 自动拉黑窗口秒);
    窗口编辑框.SetWindowText(窗口文本);

    // 根据启用状态设置编辑框可用性
    阈值编辑框.EnableWindow(自动拉黑启用);
    窗口编辑框.EnableWindow(自动拉黑启用);
}

void 黑白名单对话框类::保存自动拉黑配置()
{
    // 从编辑框读取当前值
    CString 阈值文本, 窗口文本;
    阈值编辑框.GetWindowText(阈值文本);
    窗口编辑框.GetWindowText(窗口文本);

    int 新阈值 = _ttoi(阈值文本);
    int 新窗口 = _ttoi(窗口文本);

    if (新阈值 > 0 && 新阈值 <= 100) 自动拉黑阈值 = 新阈值;
    if (新窗口 > 0 && 新窗口 <= 3600) 自动拉黑窗口秒 = 新窗口;

    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\NageServer\\IPLists"), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        DWORD dwEnable = 自动拉黑启用 ? 1 : 0;
        RegSetValueEx(hKey, _T("AutoBlacklistEnable"), 0, REG_DWORD, (const BYTE*)&dwEnable, sizeof(DWORD));

        DWORD dwThreshold = (DWORD)自动拉黑阈值;
        RegSetValueEx(hKey, _T("AutoBlacklistThreshold"), 0, REG_DWORD, (const BYTE*)&dwThreshold, sizeof(DWORD));

        DWORD dwWindow = (DWORD)自动拉黑窗口秒;
        RegSetValueEx(hKey, _T("AutoBlacklistWindow"), 0, REG_DWORD, (const BYTE*)&dwWindow, sizeof(DWORD));

        RegCloseKey(hKey);
    }
}
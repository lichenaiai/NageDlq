// 注入页面类.cpp
#include "pch.h"
#include "framework.h"
#include "NageDlq.h"
#include "注入页面类.h"
#include "afxdialogex.h"
#include "Resource.h"
#include "NageDlqDlg.h"

// 添加必要的Windows头文件
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>

// C++标准库
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <chrono>

#ifdef _DEBUG
#define new DEBUG_NEW
#define WM_UPDATE_TARGET_ID (WM_USER + 202)
#endif

// 静态回调函数声明
static BOOL CALLBACK 枚举进程窗口回调(HWND hwnd, LPARAM lParam);
static BOOL CALLBACK FindMainWindowCallback(HWND hwnd, LPARAM lParam);

namespace
{
    struct 自定义IP输入上下文
    {
        CString 输入IP;
    };

    INT_PTR CALLBACK 自定义IP输入对话框过程(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        自定义IP输入上下文* 上下文 = reinterpret_cast<自定义IP输入上下文*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        switch (message)
        {
        case WM_INITDIALOG:
        {
            上下文 = reinterpret_cast<自定义IP输入上下文*>(lParam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)上下文);
            SetWindowText(hwnd, _T("设置自定义Hook IP"));

            HFONT 默认字体 = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

            HWND 标签 = CreateWindowEx(0, _T("STATIC"), _T("请输入自定义 IP 地址："),
                WS_CHILD | WS_VISIBLE,
                12, 12, 160, 18,
                hwnd, NULL, AfxGetInstanceHandle(), NULL);

            HWND 编辑框 = CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), 上下文->输入IP,
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                12, 34, 176, 22,
                hwnd, (HMENU)10001, AfxGetInstanceHandle(), NULL);

            HWND 确定按钮 = CreateWindowEx(0, _T("BUTTON"), _T("确定"),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                48, 68, 60, 22,
                hwnd, (HMENU)IDOK, AfxGetInstanceHandle(), NULL);

            HWND 取消按钮 = CreateWindowEx(0, _T("BUTTON"), _T("取消"),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                120, 68, 60, 22,
                hwnd, (HMENU)IDCANCEL, AfxGetInstanceHandle(), NULL);

            SendMessage(标签, WM_SETFONT, (WPARAM)默认字体, TRUE);
            SendMessage(编辑框, WM_SETFONT, (WPARAM)默认字体, TRUE);
            SendMessage(确定按钮, WM_SETFONT, (WPARAM)默认字体, TRUE);
            SendMessage(取消按钮, WM_SETFONT, (WPARAM)默认字体, TRUE);

            SetFocus(编辑框);
            return FALSE;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK && 上下文 != nullptr)
            {
                TCHAR 输入缓冲区[64] = { 0 };
                GetDlgItemText(hwnd, 10001, 输入缓冲区, _countof(输入缓冲区));
                上下文->输入IP = 输入缓冲区;
                EndDialog(hwnd, IDOK);
                return TRUE;
            }
            if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hwnd, IDCANCEL);
                return TRUE;
            }
            break;
        }

        return FALSE;
    }

    BOOL 显示自定义IP输入对话框(CWnd* 父窗口, CString& 输入IP)
    {
        struct
        {
            DLGTEMPLATE 模板;
            WORD 菜单;
            WORD 窗口类;
            WORD 标题;
        } 对话框模板 = {};

        对话框模板.模板.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME;
        对话框模板.模板.dwExtendedStyle = 0;
        对话框模板.模板.cdit = 0;
        对话框模板.模板.x = 10;
        对话框模板.模板.y = 10;
        对话框模板.模板.cx = 205;
        对话框模板.模板.cy = 102;

        自定义IP输入上下文 上下文 = { 输入IP };
        INT_PTR 结果 = DialogBoxIndirectParam(AfxGetInstanceHandle(), &对话框模板.模板,
            父窗口 ? 父窗口->GetSafeHwnd() : NULL, 自定义IP输入对话框过程, (LPARAM)&上下文);
        if (结果 == IDOK)
        {
            输入IP = 上下文.输入IP;
            return TRUE;
        }

        return FALSE;
    }
}

// 注入页面类 对话框
IMPLEMENT_DYNAMIC(注入页面类, CDialogEx)

注入页面类::注入页面类(CWnd* p父窗口 /*=nullptr*/)
    : CDialogEx(IDD_PAGE_INJECTION, p父窗口)
    , 自动打怪运行中(false)
    , 游戏进程句柄(NULL)
    , 游戏进程ID(0)
    , 当前怪物地址索引(0)
    , 总攻击次数(0)
    , 自动打怪开始时间(0)
    , 游戏窗口句柄(NULL)
    , 当前目标ID(0xFFFFFFFF)
    , 线程停止标志(false)
    , 初始X坐标(0.0f)
    , 初始Y坐标(0.0f)
    , 上次检查坐标时间(0)
    , 记录X坐标(0.0f)        
    , 记录Y坐标(0.0f)          
    , 记录地图编号(0)         
    , 有记录坐标(FALSE)      
    , 隐藏入口点击次数(0)
{
}

注入页面类::~注入页面类()
{
    // 确保线程停止
    停止自动打怪();
}

void 注入页面类::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_INJE_ATTKMOB, 自动打怪按钮);
    DDX_Control(pDX, IDC_AUTOAM_ID, 状态标签);
    DDX_Control(pDX, IDC_DT_MOVE_SAVE, 记录坐标按钮);  
    DDX_Control(pDX, IDC_DT_MOVE_MOVE, 传送按钮);      
    DDX_Control(pDX, IDC_DT_MOVE_STATIC, 坐标状态标签);
    DDX_Control(pDX, IDC_DT_BUFF, 加BUFF按钮);
}

BEGIN_MESSAGE_MAP(注入页面类, CDialogEx)
    ON_BN_CLICKED(IDC_INJE_ATTKMOB, &注入页面类::点击自动打怪按钮)
    ON_BN_CLICKED(IDC_DT_MOVE_SAVE, &注入页面类::点击记录坐标)    
    ON_BN_CLICKED(IDC_DT_MOVE_MOVE, &注入页面类::点击传送)  
    ON_BN_CLICKED(IDC_DT_BUFF, &注入页面类::点击加枪手BUFF)
    ON_BN_CLICKED(IDC_DT_OPENBOX, &注入页面类::点击隐藏IP入口)
    ON_WM_TIMER()
    ON_WM_DESTROY()
    ON_MESSAGE(WM_USER + 200, &注入页面类::游戏进程退出消息处理)
    ON_MESSAGE(WM_USER + 201, &注入页面类::自动打怪停止消息处理)
    ON_MESSAGE(WM_UPDATE_TARGET_ID, &注入页面类::更新目标ID消息处理)
END_MESSAGE_MAP()

// 初始化对话框
BOOL 注入页面类::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 设置按钮初始文本
    自动打怪按钮.SetWindowText(_T("开始自动打怪"));

    // 初始化状态标签显示
    更新目标ID显示(0xFFFFFFFF);

    // 初始化坐标状态标签
    坐标状态标签.SetWindowText(_T("坐标：0"));

    return TRUE;
}

BOOL 注入页面类::验证IPv4格式(const CString& IP地址) const
{
    unsigned int IP片段1 = 0, IP片段2 = 0, IP片段3 = 0, IP片段4 = 0;
    TCHAR 额外字符 = 0;
    if (_stscanf_s(IP地址, _T("%u.%u.%u.%u%c"),
        &IP片段1, &IP片段2, &IP片段3, &IP片段4, &额外字符, 1) != 4)
    {
        return FALSE;
    }

    return IP片段1 <= 255 && IP片段2 <= 255 && IP片段3 <= 255 && IP片段4 <= 255;
}

void 注入页面类::处理自定义HookIP设置()
{
    CString 输入IP;
    while (显示自定义IP输入对话框(this, 输入IP))
    {
        输入IP.Trim();
        if (!验证IPv4格式(输入IP))
        {
            MessageBox(_T("请输入正确的 IPv4 地址格式，例如 124.220.82.87。"), _T("提示"), MB_ICONWARNING);
            continue;
        }

        CWnd* 主窗口 = AfxGetMainWnd();
        NageDlqDlg* 主对话框 = dynamic_cast<NageDlqDlg*>(主窗口);
        if (主对话框 == nullptr)
        {
            MessageBox(_T("无法获取主窗口，设置失败。"), _T("错误"), MB_ICONERROR);
            return;
        }

        主对话框->登录页面.设置自定义HookIP(输入IP);
        MessageBox(_T("自定义 Hook IP 已保存。返回登录页面后，点击启动游戏即会使用该 Hook IP。"), _T("提示"), MB_ICONINFORMATION);
        return;
    }
}

void 注入页面类::点击隐藏IP入口()
{
    隐藏入口点击次数++;
    if (隐藏入口点击次数 < 10)
    {
        return;
    }

    隐藏入口点击次数 = 0;
    处理自定义HookIP设置();
}

// 窗口销毁时调用
void 注入页面类::OnDestroy()
{
    // 确保线程停止
    停止自动打怪();

    CDialogEx::OnDestroy();
}

// 获取游戏进程ID函数
DWORD 注入页面类::获取游戏进程ID()
{
    return 游戏进程ID;  // 直接返回已获取的进程ID
}

// 通过进程ID查找窗口
BOOL 注入页面类::通过进程ID查找窗口()
{
    if (游戏进程ID == 0)
    {
        TRACE(_T("进程ID为0，无法查找窗口\n"));
        return FALSE;
    }

    游戏窗口句柄 = NULL;

    // 方法1：使用FindMainWindow查找主窗口
    游戏窗口句柄 = FindMainWindow(游戏进程ID);

    if (游戏窗口句柄 != NULL && IsWindow(游戏窗口句柄))
    {
        TCHAR 窗口标题[256] = { 0 };
        ::GetWindowText(游戏窗口句柄, 窗口标题, 255);
        TRACE(_T("通过FindMainWindow找到窗口: '%s' (0x%08X)\n"), 窗口标题, 游戏窗口句柄);
        return TRUE;
    }

    // 方法2：使用深度查找
    TRACE(_T("开始深度查找进程 %d 的窗口\n"), 游戏进程ID);
    游戏窗口句柄 = 深度查找进程窗口(游戏进程ID);

    if (游戏窗口句柄 != NULL && IsWindow(游戏窗口句柄))
    {
        TCHAR 窗口标题[256] = { 0 };
        ::GetWindowText(游戏窗口句柄, 窗口标题, 255);
        TRACE(_T("通过深度查找找到窗口: '%s' (0x%08X)\n"), 窗口标题, 游戏窗口句柄);
        return TRUE;
    }

    // 方法3：使用枚举窗口回调
    TRACE(_T("开始枚举窗口查找进程ID: %d\n"), 游戏进程ID);
    EnumWindows(枚举进程窗口回调, reinterpret_cast<LPARAM>(this));

    if (游戏窗口句柄 != NULL && IsWindow(游戏窗口句柄))
    {
        TCHAR 窗口标题[256] = { 0 };
        ::GetWindowText(游戏窗口句柄, 窗口标题, 255);
        TRACE(_T("通过枚举窗口找到窗口: '%s' (0x%08X)\n"), 窗口标题, 游戏窗口句柄);
        return TRUE;
    }

    TRACE(_T("未找到进程 %d 的任何窗口\n"), 游戏进程ID);
    return FALSE;
}

// 枚举窗口的回调函数（静态）
static BOOL CALLBACK 枚举进程窗口回调(HWND hwnd, LPARAM lParam)
{
    注入页面类* p注入页面 = reinterpret_cast<注入页面类*>(lParam);
    if (p注入页面 == nullptr) return TRUE;

    DWORD 窗口进程ID = 0;
    ::GetWindowThreadProcessId(hwnd, &窗口进程ID);

    if (窗口进程ID == p注入页面->游戏进程ID)
    {
        // 检查窗口是否可见且不是子窗口
        if (::IsWindowVisible(hwnd) && ::GetParent(hwnd) == NULL)
        {
            // 获取窗口标题用于调试
            TCHAR 窗口标题[256] = { 0 };
            ::GetWindowText(hwnd, 窗口标题, 255);

            TRACE(_T("找到游戏窗口: 句柄=0x%08X, 标题='%s', 进程ID=%d\n"),
                hwnd, 窗口标题, 窗口进程ID);

            p注入页面->游戏窗口句柄 = hwnd;
            return FALSE; // 找到窗口，停止枚举
        }
    }
    return TRUE; // 继续枚举
}

// 查找进程的主窗口
HWND 注入页面类::查找进程主窗口(DWORD 目标进程ID)
{
    return FindMainWindow(目标进程ID);
}

// 通用的查找进程主窗口函数（静态）
static BOOL CALLBACK FindMainWindowCallback(HWND hwnd, LPARAM lParam)
{
    struct WindowInfo
    {
        DWORD 进程ID;
        HWND 窗口句柄;
    };

    WindowInfo* pInfo = reinterpret_cast<WindowInfo*>(lParam);
    DWORD 进程ID = 0;

    ::GetWindowThreadProcessId(hwnd, &进程ID);
    if (进程ID == pInfo->进程ID)
    {
        // 检查窗口是否可见、没有父窗口、不是工具窗口
        if (::IsWindowVisible(hwnd) &&
            ::GetParent(hwnd) == NULL &&
            (::GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) == 0)
        {
            pInfo->窗口句柄 = hwnd;
            return FALSE; // 停止枚举
        }
    }
    return TRUE; // 继续枚举
}

// 通用的查找进程主窗口函数
HWND 注入页面类::FindMainWindow(DWORD dwPID)
{
    struct WindowInfo
    {
        DWORD 进程ID;
        HWND 窗口句柄;
    };

    WindowInfo 信息 = { dwPID, NULL };

    EnumWindows(FindMainWindowCallback, reinterpret_cast<LPARAM>(&信息));

    return 信息.窗口句柄;
}

// 深度查找窗口（包含子窗口）
HWND 注入页面类::深度查找进程窗口(DWORD 目标进程ID)
{
    return 深度查找窗口递归(::GetDesktopWindow(), 目标进程ID);
}

// 递归查找窗口的辅助函数
HWND 注入页面类::深度查找窗口递归(HWND 父窗口, DWORD 目标进程ID)
{
    HWND 子窗口 = ::GetWindow(父窗口, GW_CHILD);

    while (子窗口 != NULL)
    {
        DWORD 窗口进程ID = 0;
        ::GetWindowThreadProcessId(子窗口, &窗口进程ID);

        if (窗口进程ID == 目标进程ID && ::IsWindowVisible(子窗口))
        {
            return 子窗口;
        }

        // 递归查找子窗口的子窗口
        HWND 找到的窗口 = 深度查找窗口递归(子窗口, 目标进程ID);
        if (找到的窗口 != NULL)
        {
            return 找到的窗口;
        }

        子窗口 = ::GetWindow(子窗口, GW_HWNDNEXT);
    }

    return NULL;
}

// 获取进程的所有窗口
std::vector<HWND> 注入页面类::获取进程所有窗口(DWORD 目标进程ID)
{
    std::vector<HWND> 窗口列表;

    // 创建一个左值变量，而不是临时对象
    std::pair<DWORD, std::vector<HWND>*> 数据(目标进程ID, &窗口列表);

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL
        {
            auto* p数据 = reinterpret_cast<std::pair<DWORD, std::vector<HWND>*>*>(lParam);
            DWORD 窗口进程ID = 0;
            ::GetWindowThreadProcessId(hwnd, &窗口进程ID);

            if (窗口进程ID == p数据->first)
            {
                p数据->second->push_back(hwnd);
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&数据));

    return 窗口列表;
}

// 记录初始坐标
void 注入页面类::记录初始坐标()
{
    if (游戏进程句柄 == NULL) return;

    SIZE_T 读取字节数;
    ReadProcessMemory(游戏进程句柄, (LPCVOID)0x319B8A8,
        &初始X坐标, sizeof(float), &读取字节数);
    ReadProcessMemory(游戏进程句柄, (LPCVOID)0x319B8B0,
        &初始Y坐标, sizeof(float), &读取字节数);

    TRACE(_T("记录初始坐标: X=%.2f, Y=%.2f\n"), 初始X坐标, 初始Y坐标);
}

// 检查并限制坐标范围
BOOL 注入页面类::检查坐标范围()
{
    if (游戏进程句柄 == NULL) return TRUE;

    static DWORD 上次检查时间 = 0;
    DWORD 当前时间 = GetTickCount();

    // 每秒检查一次
    if (当前时间 - 上次检查时间 < 1000)
        return TRUE;

    上次检查坐标时间 = 当前时间;

    SIZE_T 读取字节数;
    float 当前X = 0.0f, 当前Y = 0.0f;

    // 读取当前坐标
    ReadProcessMemory(游戏进程句柄, (LPCVOID)0x319B8A8,
        &当前X, sizeof(float), &读取字节数);
    ReadProcessMemory(游戏进程句柄, (LPCVOID)0x319B8B0,
        &当前Y, sizeof(float), &读取字节数);

    // 计算距离
    float 距离X = fabs(当前X - 初始X坐标);
    float 距离Y = fabs(当前Y - 初始Y坐标);

    // 如果X或Y任意一个超出20，就返回起始点
    if (距离X > 20.0f || 距离Y > 20.0f)
    {
        // 停止攻击
        DWORD 攻击标志 = 0;
        DWORD 无目标 = 0x00000000;
        WriteProcessMemory(游戏进程句柄, (LPVOID)攻击标志地址,
            &攻击标志, sizeof(DWORD), &读取字节数);
        WriteProcessMemory(游戏进程句柄, (LPVOID)目标怪物地址,
            &无目标, sizeof(DWORD), &读取字节数);

        // 返回初始位置
        WriteProcessMemory(游戏进程句柄, (LPVOID)0x319B8A8,
            &初始X坐标, sizeof(float), &读取字节数);
        WriteProcessMemory(游戏进程句柄, (LPVOID)0x319B8B0,
            &初始Y坐标, sizeof(float), &读取字节数);

        // 更新显示为无目标
        ::PostMessage(GetSafeHwnd(), WM_UPDATE_TARGET_ID, 0x00000000, 0);

        return FALSE;
    }

    return TRUE;
}

// 自动打怪按钮点击事件
void 注入页面类::点击自动打怪按钮()
{
    if (!自动打怪运行中)
    {
        // 检查游戏进程
        if (!检查游戏进程())
        {
            MessageBox(_T("找不到游戏进程，请先启动游戏！"), _T("错误"), MB_ICONERROR);
            return;
        }

        // 通过进程ID查找游戏窗口
        if (!通过进程ID查找窗口())
        {
            CString 错误信息;
            错误信息.Format(_T("找不到游戏窗口！\n进程ID: %d\n\n请确保游戏窗口未被最小化。"), 游戏进程ID);
            MessageBox(错误信息, _T("错误"), MB_ICONERROR);
            return;
        }

        // 获取窗口标题用于显示
        TCHAR 窗口标题[256] = { 0 };
        ::GetWindowText(游戏窗口句柄, 窗口标题, 255);
        TRACE(_T("成功获取游戏窗口: '%s' (0x%08X)\n"), 窗口标题, 游戏窗口句柄);

        // 注入自动打怪功能
        if (!注入自动打怪功能())
        {
            MessageBox(_T("注入自动打怪功能失败！"), _T("错误"), MB_ICONERROR);
            return;
        }

        // 记录初始坐标
        记录初始坐标();

        // 重置停止标志
        线程停止标志 = false;

        // 启动自动打怪线程
        启动自动打怪线程();

        // 更新按钮文本
        自动打怪按钮.SetWindowText(_T("停止打怪"));

        // 记录开始时间
        自动打怪开始时间 = GetTickCount();
        总攻击次数 = 0;
        上次检查坐标时间 = GetTickCount();

        // 初始化显示
        更新目标ID显示(0x00000000);

        TRACE(_T("自动打怪已启动\n"));
    }
    else
    {
        // 停止自动打怪
        停止自动打怪();

        // 更新按钮文本
        自动打怪按钮.SetWindowText(_T("开始自动打怪"));

        TRACE(_T("自动打怪已停止\n"));
    }
}

// 定时器处理函数
void 注入页面类::OnTimer(UINT_PTR nIDEvent)
{
    // 这里可以添加定时检查游戏进程的功能
    CDialogEx::OnTimer(nIDEvent);
}

// 检查游戏进程
BOOL 注入页面类::检查游戏进程()
{
    std::lock_guard<std::mutex> 锁(游戏进程互斥锁);

    // 清理旧的进程句柄
    if (游戏进程句柄 != NULL)
    {
        CloseHandle(游戏进程句柄);
        游戏进程句柄 = NULL;
    }

    游戏进程ID = 0;

    // 可能的进程名列表
    const TCHAR* 进程名列表[] = {
        _T("nage.bin"),
        _T("Nage.bin"),
        _T("nage.exe"),
        _T("Nage.exe"),
        _T("NageClient.exe"),
        _T("Nage Client.exe"),
        NULL
    };

    // 获取进程ID
    HANDLE 进程快照句柄 = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (进程快照句柄 == INVALID_HANDLE_VALUE)
    {
        TRACE(_T("创建进程快照失败\n"));
        return FALSE;
    }

    PROCESSENTRY32 进程信息;
    进程信息.dwSize = sizeof(PROCESSENTRY32);

    BOOL 找到进程 = FALSE;
    if (Process32First(进程快照句柄, &进程信息))
    {
        do
        {
            // 检查所有可能的进程名
            for (int i = 0; 进程名列表[i] != NULL; i++)
            {
                if (_wcsicmp(进程信息.szExeFile, 进程名列表[i]) == 0)
                {
                    游戏进程ID = 进程信息.th32ProcessID;
                    找到进程 = TRUE;
                    TRACE(_T("找到游戏进程: %s (PID: %d)\n"), 进程信息.szExeFile, 游戏进程ID);
                    break;
                }
            }

            if (找到进程) break;

        } while (Process32Next(进程快照句柄, &进程信息));
    }

    CloseHandle(进程快照句柄);

    if (!找到进程)
    {
        TRACE(_T("未找到游戏进程\n"));
        return FALSE;
    }

    // 打开进程
    游戏进程句柄 = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, 游戏进程ID);
    if (游戏进程句柄 == NULL)
    {
        TRACE(_T("打开游戏进程失败，进程ID: %d\n"), 游戏进程ID);
        游戏进程ID = 0;
        return FALSE;
    }

    TRACE(_T("成功打开游戏进程，句柄: 0x%08X, 进程ID: %d\n"), 游戏进程句柄, 游戏进程ID);
    return TRUE;
}

// 注入自动打怪功能
BOOL 注入页面类::注入自动打怪功能()
{
    std::lock_guard<std::mutex> 锁(游戏进程互斥锁);

    if (游戏进程句柄 == NULL || 游戏进程ID == 0)
    {
        TRACE(_T("游戏进程未打开，无法注入\n"));
        return FALSE;
    }

    // 检查内存地址是否可读写
    SIZE_T 读取字节数;
    DWORD 测试值;

    // 测试读取攻击标志地址
    if (!ReadProcessMemory(游戏进程句柄, (LPCVOID)攻击标志地址,
        &测试值, sizeof(DWORD), &读取字节数))
    {
        DWORD 错误代码 = GetLastError();
        TRACE(_T("无法读取攻击标志地址 0x%08X，错误代码: %d\n"), 攻击标志地址, 错误代码);

        CString 错误信息;
        错误信息.Format(_T("无法访问游戏内存地址 0x%08X\n请确认游戏版本是否正确。"), 攻击标志地址);
        MessageBox(错误信息, _T("内存访问错误"), MB_ICONERROR);
        return FALSE;
    }

    TRACE(_T("攻击标志地址 0x%08X 当前值: 0x%08X\n"), 攻击标志地址, 测试值);

    // 测试读取目标怪物地址
    if (!ReadProcessMemory(游戏进程句柄, (LPCVOID)目标怪物地址,
        &测试值, sizeof(DWORD), &读取字节数))
    {
        TRACE(_T("无法读取目标怪物地址 0x%08X\n"), 目标怪物地址);
        return FALSE;
    }

    TRACE(_T("目标怪物地址 0x%08X 当前值: 0x%08X\n"), 目标怪物地址, 测试值);

    // 测试读取三个怪物ID地址
    for (int i = 0; i < 3; i++)
    {
        DWORD 地址 = 0;
        switch (i)
        {
        case 0: 地址 = 怪物ID地址1; break;
        case 1: 地址 = 怪物ID地址2; break;
        case 2: 地址 = 怪物ID地址3; break;
        }

        if (!ReadProcessMemory(游戏进程句柄, (LPCVOID)地址,
            &测试值, sizeof(DWORD), &读取字节数))
        {
            TRACE(_T("无法读取怪物ID地址%d: 0x%08X\n"), i, 地址);
        }
        else
        {
            TRACE(_T("怪物ID地址%d: 0x%08X 当前值: 0x%08X\n"), i, 地址, 测试值);
        }
    }

    // 尝试写入测试值
    DWORD 安全值 = 0;
    SIZE_T 写入字节数;

    if (!WriteProcessMemory(游戏进程句柄, (LPVOID)攻击标志地址,
        &安全值, sizeof(DWORD), &写入字节数))
    {
        TRACE(_T("警告：无法写入攻击标志地址，可能权限不足\n"));
        // 不返回FALSE，因为可能只需要读取权限
    }
    else
    {
        TRACE(_T("成功写入攻击标志地址\n"));
    }

    // 检查游戏进程的模块信息
    HANDLE 模块快照句柄 = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, 游戏进程ID);
    if (模块快照句柄 != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 模块信息;
        模块信息.dwSize = sizeof(MODULEENTRY32);

        if (Module32First(模块快照句柄, &模块信息))
        {
            do
            {
                TRACE(_T("游戏模块: %s (基址: 0x%08X, 大小: %d)\n"),
                    模块信息.szModule, (DWORD)模块信息.modBaseAddr, 模块信息.modBaseSize);
            } while (Module32Next(模块快照句柄, &模块信息));
        }
        CloseHandle(模块快照句柄);
    }

    TRACE(_T("自动打怪功能注入成功\n"));

    return TRUE; // 移除了消息框，改为只记录日志
}

BOOL 注入页面类::激活并聚焦游戏窗口()
{
    if (!游戏窗口句柄 || !IsWindow(游戏窗口句柄))
    {
        // 如果窗口句柄为空或无效，重新通过进程ID查找
        if (!通过进程ID查找窗口())
        {
            TRACE(_T("重新查找游戏窗口失败\n"));
            return FALSE;
        }
    }

    // 只恢复窗口，不激活到前台
    if (::IsIconic(游戏窗口句柄))
    {
        ::ShowWindow(游戏窗口句柄, SW_RESTORE);
        Sleep(100);
    }

    // 只保证窗口可见，不抢焦点
    ::BringWindowToTop(游戏窗口句柄);

    return TRUE;
}

void 注入页面类::后台模拟鼠标移动()
{
    if (!游戏窗口句柄 || !IsWindow(游戏窗口句柄))
    {
        // 重新查找窗口
        if (!通过进程ID查找窗口())
        {
            return;
        }
    }

    // 确保窗口在前台才模拟
    if (::GetForegroundWindow() == 游戏窗口句柄)
    {
        // 只在窗口内轻微移动
        RECT 窗口矩形;
        ::GetWindowRect(游戏窗口句柄, &窗口矩形);

        // 生成窗口内的随机位置
        int 宽度 = 窗口矩形.right - 窗口矩形.left;
        int 高度 = 窗口矩形.bottom - 窗口矩形.top;

        if (宽度 > 100 && 高度 > 100)
        {
            // 在窗口中心区域移动（避开边缘）
            int 中心X = 宽度 / 2;
            int 中心Y = 高度 / 2;
            int 随机X = 中心X + (rand() % 100 - 50); // ±50像素
            int 随机Y = 中心Y + (rand() % 100 - 50);

            // 使用SendInput模拟（只在游戏窗口激活时）
            INPUT 输入 = { 0 };
            输入.type = INPUT_MOUSE;
            输入.mi.dx = 随机X * (65535 / 宽度);
            输入.mi.dy = 随机Y * (65535 / 高度);
            输入.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
            SendInput(1, &输入, sizeof(INPUT));

            TRACE(_T("后台模拟鼠标移动到(%d, %d)\n"), 随机X, 随机Y);
        }
    }
}

// 启动自动打怪线程
void 注入页面类::启动自动打怪线程()
{
    if (自动打怪运行中)
    {
        TRACE(_T("自动打怪线程已经在运行\n"));
        return;
    }

    自动打怪运行中 = true;
    线程停止标志 = false;
    当前怪物地址索引 = 0;

    // 创建自动打怪线程
    自动打怪线程 = std::thread(&注入页面类::自动打怪线程函数, this);

    TRACE(_T("自动打怪线程已启动\n"));
}

// 停止自动打怪
void 注入页面类::停止自动打怪()
{
    if (!自动打怪运行中)
    {
        return;
    }

    TRACE(_T("正在停止自动打怪...\n"));

    线程停止标志 = true;
    自动打怪运行中 = false;

    // 等待线程结束（最多等待3秒）
    if (自动打怪线程.joinable())
    {
        // 使用timeout等待，避免死锁
        auto 等待超时 = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        if (自动打怪线程.joinable())
        {
            try
            {
                // 尝试分离线程以避免阻塞
                自动打怪线程.detach();
            }
            catch (...)
            {
                TRACE(_T("线程分离失败\n"));
            }
        }
    }

    // 关闭进程句柄
    {
        std::lock_guard<std::mutex> 锁(游戏进程互斥锁);
        if (游戏进程句柄 != NULL)
        {
            CloseHandle(游戏进程句柄);
            游戏进程句柄 = NULL;
        }

        游戏进程ID = 0;
    }

    // 清理显示
    更新目标ID显示(0xFFFFFFFF);
    TRACE(_T("自动打怪已停止\n"));
}

// 自动打怪线程函数
void 注入页面类::自动打怪线程函数()
{
    TRACE(_T("=== 自动打怪线程开始 ===\n"));

    自动打怪开始时间 = GetTickCount();
    总攻击次数 = 0;

    // 记录初始坐标
    记录初始坐标();

    // 主循环
    while (自动打怪运行中 && !线程停止标志)
    {
        try
        {
            // 检查进程状态
            DWORD 退出代码;
            if (!GetExitCodeProcess(游戏进程句柄, &退出代码) || 退出代码 != STILL_ACTIVE)
            {
                TRACE(_T("游戏进程已退出\n"));
                ::PostMessage(GetSafeHwnd(), WM_USER + 200, 0, 0);
                break;
            }

            // 执行攻击逻辑
            执行智能攻击();

            // 短暂延迟
            Sleep(50);
        }
        catch (...)
        {
            TRACE(_T("线程异常\n"));
            Sleep(1000);
        }
    }

    TRACE(_T("=== 自动打怪线程结束 ===\n"));
    ::PostMessage(GetSafeHwnd(), WM_USER + 201, 0, 0);
}

// 获取有效目标怪物
DWORD 注入页面类::获取有效目标怪物()
{
    if (游戏进程句柄 == NULL) return 0x00000000;

    DWORD 怪物ID列表[3] = { 0 };
    SIZE_T 读取字节数;

    // 读取三个怪物ID地址
    ReadProcessMemory(游戏进程句柄, (LPCVOID)怪物ID地址1,
        &怪物ID列表[0], sizeof(DWORD), &读取字节数);
    ReadProcessMemory(游戏进程句柄, (LPCVOID)怪物ID地址2,
        &怪物ID列表[1], sizeof(DWORD), &读取字节数);
    ReadProcessMemory(游戏进程句柄, (LPCVOID)怪物ID地址3,
        &怪物ID列表[2], sizeof(DWORD), &读取字节数);

    // 轮询选择怪物（避免总是用同一个地址）
    static int 轮询索引 = 0;
    轮询索引 = (轮询索引 + 1) % 3;

    // 从轮询索引开始查找有效怪物
    for (int 尝试 = 0; 尝试 < 3; 尝试++)
    {
        int 当前索引 = (轮询索引 + 尝试) % 3;
        DWORD 怪物ID = 怪物ID列表[当前索引];

        // 有效的怪物ID：非0且非FFFFFFFF
        if (怪物ID != 0x00000000 && 怪物ID != 0xFFFFFFFF)
        {
            TRACE(_T("从地址%d选择怪物: 0x%08X\n"), 当前索引 + 1, 怪物ID);
            return 怪物ID;
        }
    }

    // 如果所有地址都是无效的，尝试获取0值的怪物
    for (int i = 0; i < 3; i++)
    {
        if (怪物ID列表[i] == 0x00000000)
        {
            TRACE(_T("所有怪物ID都是0xFFFFFFFF或0，等待新怪物\n"));
        }
    }

    return 0x00000000; // 无有效怪物
}

// 执行智能攻击（60秒超时，0.5秒等待）
void 注入页面类::执行智能攻击()
{
    if (线程停止标志 || !自动打怪运行中 || 游戏进程句柄 == NULL)
    {
        return;
    }

    // 每分钟检查一次坐标范围
    if (!检查坐标范围())
    {
        // 返回初始位置后等待2秒
        Sleep(2000);
        return;
    }

    // 读取当前目标状态
    SIZE_T 读取字节数;
    DWORD 当前目标 = 0x00000000;
    ReadProcessMemory(游戏进程句柄, (LPCVOID)目标怪物地址,
        &当前目标, sizeof(DWORD), &读取字节数);

    // 更新显示
    ::PostMessage(GetSafeHwnd(), WM_UPDATE_TARGET_ID, 当前目标, 0);

    // 处理不同状态
    if (当前目标 == 0x00000000) // 无目标
    {
        TRACE(_T("状态: 无目标，寻找新怪物...\n"));

        // 从3个地址中寻找有效怪物
        DWORD 新怪物ID = 获取有效目标怪物();

        if (新怪物ID != 0x00000000 && 新怪物ID != 0xFFFFFFFF)
        {
            TRACE(_T("找到新怪物: 0x%08X\n"), 新怪物ID);

            // 写入目标怪物地址
            WriteProcessMemory(游戏进程句柄, (LPVOID)目标怪物地址,
                &新怪物ID, sizeof(DWORD), &读取字节数);

            // 设置攻击标志
            DWORD 攻击标志 = 1;
            WriteProcessMemory(游戏进程句柄, (LPVOID)攻击标志地址,
                &攻击标志, sizeof(DWORD), &读取字节数);

            TRACE(_T("开始攻击: 0x%08X\n"), 新怪物ID);
        }
        else
        {
            // 没有找到怪物，等待后重试
            Sleep(500);
        }
    }
    else if (当前目标 == 0xFFFFFFFF) // 怪物死亡（尸体还在）
    {
        TRACE(_T("状态: 怪物死亡(0xFFFFFFFF)，立即切换到下一个目标\n"));

        // 清除攻击标志
        DWORD 攻击标志 = 0;
        WriteProcessMemory(游戏进程句柄, (LPVOID)攻击标志地址,
            &攻击标志, sizeof(DWORD), &读取字节数);

        // 立即设置为无目标，以便寻找下一个
        DWORD 无目标 = 0x00000000;
        WriteProcessMemory(游戏进程句柄, (LPVOID)目标怪物地址,
            &无目标, sizeof(DWORD), &读取字节数);

        // 短暂延迟后继续
        Sleep(100);
    }
    else // 有目标ID
    {
        // TRACE(_T("状态: 正在攻击: 0x%08X\n"), 当前目标);

        // 确保攻击标志为1
        DWORD 攻击标志 = 1;
        WriteProcessMemory(游戏进程句柄, (LPVOID)攻击标志地址,
            &攻击标志, sizeof(DWORD), &读取字节数);

        // 等待并检查怪物状态
        Sleep(500);

        // 检查怪物是否死亡
        DWORD 目标状态 = 0x00000000;
        ReadProcessMemory(游戏进程句柄, (LPCVOID)目标怪物地址,
            &目标状态, sizeof(DWORD), &读取字节数);

        if (目标状态 == 0xFFFFFFFF)
        {
            TRACE(_T("怪物死亡！目标ID: 0x%08X\n"), 当前目标);
            总攻击次数++;

            // 每10次显示一次统计
            if (总攻击次数 % 10 == 0)
            {
                CString 统计信息;
                DWORD 运行秒数 = (GetTickCount() - 自动打怪开始时间) / 1000;
                统计信息.Format(_T("已攻击 %d 次，运行 %d 秒"), 总攻击次数, 运行秒数);
                TRACE(_T("%s\n"), 统计信息);
            }
        }
    }
}

// 游戏进程退出消息处理
LRESULT 注入页面类::游戏进程退出消息处理(WPARAM w参数, LPARAM l参数)
{
    TRACE(_T("收到游戏进程退出消息\n"));

    停止自动打怪();
    自动打怪按钮.SetWindowText(_T("开始自动打怪"));

    MessageBox(_T("游戏进程已退出，自动打怪已停止"), _T("提示"), MB_ICONINFORMATION);

    return 0;
}

// 自动打怪停止消息处理
LRESULT 注入页面类::自动打怪停止消息处理(WPARAM w参数, LPARAM l参数)
{
    TRACE(_T("收到自动打怪停止消息\n"));

    // 确保按钮状态正确
    if (!自动打怪运行中)
    {
        自动打怪按钮.SetWindowText(_T("开始自动打怪"));
    }

    return 0;
}

// 更新目标ID显示
void 注入页面类::更新目标ID显示(DWORD 目标ID)
{
    当前目标ID = 目标ID;

    CString 显示文本;

    if (目标ID == 0x00000000)
    {
        显示文本 = _T("当前目标：无目标");
    }
    else if (目标ID == 0xFFFFFFFF)
    {
        显示文本 = _T("当前目标：怪物死亡");
    }
    else
    {
        CString 十六进制文本;
        十六进制文本.Format(_T("%08X"), 目标ID);
        显示文本.Format(_T("当前目标：%s"), 十六进制文本);
    }

    // 更新状态标签
    状态标签.SetWindowText(显示文本);

    // 强制重绘
    状态标签.Invalidate();
    状态标签.UpdateWindow();
}

// 更新目标ID消息处理
LRESULT 注入页面类::更新目标ID消息处理(WPARAM w参数, LPARAM l参数)
{
    DWORD 目标ID = (DWORD)w参数;
    更新目标ID显示(目标ID);
    return 0;
}


//传送
//地图名称
CString 注入页面类::获取地图名称(int 地图编号)
{
    switch (地图编号)
    {
    case 1: return _T("波本");
    case 2: return _T("荒废");
    case 3: return _T("自由");
    case 4: return _T("大熊");
    case 5: return _T("遗忘1");
    case 6: return _T("遗忘2");
    case 7: return _T("监狱");
    case 8: return _T("乐透");
    case 9: return _T("遗忘森林");
    case 10: return _T("水晶1");
    case 11: return _T("水晶2");
    case 12: return _T("罪恶1");
    case 13: return _T("罪恶2");
    case 14: return _T("罪恶3");
    case 15: return _T("大漠");
    case 16: return _T("要塞1");
    case 17: return _T("要塞2");
    case 18: return _T("PVP地图");
    case 19: return _T("要塞3");
    case 20: return _T("终结者屋");
    case 21: return _T("研究所1");
    case 22: return _T("研究所2");
    case 23: return _T("研究所3");
    case 24: return _T("迷雾1");
    case 25: return _T("迷雾2");
    case 26: return _T("迷雾3");
    case 27: return _T("迷雾4");
    case 28: return _T("迷雾5");
    case 29: return _T("中央市");
    case 30: return _T("中央公园");
    case 31: return _T("中央广场");
    case 32: return _T("中央营地A");
    case 33: return _T("中央营地B");
    case 34: return _T("中央营地C");
    case 35: return _T("中央管制塔");
    case 36: return _T("加勒比海");
    default: return _T("未知地图");
    }
}

BOOL 注入页面类::是否允许记录坐标(int 地图编号)
{
    // 不允许记录坐标的地图列表
    switch (地图编号)
    {
    case 11:  // 水晶2
    case 14:  // 罪恶3
	case 20:  // 终结者屋
    case 35:  // 中央管制塔
    case 24:  // 迷雾1
    case 25:  // 迷雾2
    case 26:  // 迷雾3
    case 27:  // 迷雾4
    case 28:  // 迷雾5
        return FALSE;
    default:
        return TRUE;
    }
}

void 注入页面类::点击记录坐标()
{
    // 检查游戏进程
    if (!检查游戏进程())
    {
        MessageBox(_T("找不到游戏进程，请先启动游戏！"), _T("错误"), MB_ICONERROR);
        return;
    }

    SIZE_T 读取字节数;
    float 当前X, 当前Y;
    DWORD 当前地图;

    // 读取当前坐标
    if (!ReadProcessMemory(游戏进程句柄, (LPCVOID)坐标X地址, &当前X, sizeof(float), &读取字节数))
    {
        MessageBox(_T("读取X坐标失败！"), _T("错误"), MB_ICONERROR);
        return;
    }

    if (!ReadProcessMemory(游戏进程句柄, (LPCVOID)坐标Y地址, &当前Y, sizeof(float), &读取字节数))
    {
        MessageBox(_T("读取Y坐标失败！"), _T("错误"), MB_ICONERROR);
        return;
    }

    if (!ReadProcessMemory(游戏进程句柄, (LPCVOID)地图编号地址, &当前地图, sizeof(DWORD), &读取字节数))
    {
        MessageBox(_T("读取地图编号失败！"), _T("错误"), MB_ICONERROR);
        return;
    }

    // 游戏中的地图编号可能是从0开始的，加1后查表
    int 显示地图编号 = (int)当前地图 + 1;

    // 检查是否允许记录坐标（使用原始编号）
    if (!是否允许记录坐标((int)当前地图 + 1))
    {
        CString 地图名称 = 获取地图名称(显示地图编号);
        CString 提示信息;
        提示信息.Format(_T("当前地图[%s]不允许记录坐标！"), 地图名称);
        MessageBox(提示信息, _T("提示"), MB_ICONWARNING);
        return;
    }

    // 保存坐标
    记录X坐标 = 当前X;
    记录Y坐标 = 当前Y;
    记录地图编号 = (int)当前地图;
    有记录坐标 = TRUE;

    // 更新显示
    CString 地图名称 = 获取地图名称(显示地图编号);
    CString 显示文本;
    显示文本.Format(_T("%s-%.2f:%.2f"), 地图名称, 当前X, 当前Y);
    坐标状态标签.SetWindowText(显示文本);

    TRACE(_T("记录坐标: 地图=%s(原始:%d,显示:%d), X=%.2f, Y=%.2f\n"),
        地图名称, 当前地图, 显示地图编号, 当前X, 当前Y);
}

void 注入页面类::执行传送(int 目标地图原始编号, float 目标X, float 目标Y)
{
    // 检查游戏进程
    if (!检查游戏进程())
    {
        MessageBox(_T("找不到游戏进程，请先启动游戏！"), _T("错误"), MB_ICONERROR);
        return;
    }

    // 读取当前地图ID（原始编号）
    DWORD 当前地图原始编号;
    SIZE_T 读取字节数;
    if (!ReadProcessMemory(游戏进程句柄, (LPCVOID)地图编号地址, &当前地图原始编号, sizeof(DWORD), &读取字节数))
    {
        MessageBox(_T("读取当前地图失败！"), _T("错误"), MB_ICONERROR);
        return;
    }

    // 判断是否需要切换地图（使用原始编号比较）
    bool needMapChange = ((int)当前地图原始编号 != 目标地图原始编号);

    if (needMapChange)
    {
        TRACE(_T("需要切换地图，当前地图原始编号=%d，目标地图原始编号=%d\n"),
            当前地图原始编号, 目标地图原始编号);

        // 构建地图切换代码 - 使用显示编号（原始编号+1）
        int 目标地图显示编号 = 目标地图原始编号 + 1;

        std::vector<BYTE> 切换代码;

        // pushad
        切换代码.push_back(0x60);
        // pushfd
        切换代码.push_back(0x9C);
        // push 地图编号（使用显示编号）
        切换代码.push_back(0x6A);
        切换代码.push_back((BYTE)目标地图显示编号);

        // mov eax, 0x006ADF6E
        切换代码.push_back(0xB8);
        DWORD funcAddr = 0x006ADF6E;
        切换代码.push_back(funcAddr & 0xFF);
        切换代码.push_back((funcAddr >> 8) & 0xFF);
        切换代码.push_back((funcAddr >> 16) & 0xFF);
        切换代码.push_back((funcAddr >> 24) & 0xFF);
        // call eax
        切换代码.push_back(0xFF);
        切换代码.push_back(0xD0);
        // add esp, 4
        切换代码.push_back(0x83);
        切换代码.push_back(0xC4);
        切换代码.push_back(0x04);
        // popfd
        切换代码.push_back(0x9D);
        // popad
        切换代码.push_back(0x61);
        // ret
        切换代码.push_back(0xC3);

        // 分配远程内存
        BYTE* 远程内存 = (BYTE*)VirtualAllocEx(游戏进程句柄, NULL, 切换代码.size(),
            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (远程内存 == NULL)
        {
            MessageBox(_T("分配远程内存失败！"), _T("错误"), MB_ICONERROR);
            return;
        }

        // 写入代码
        SIZE_T 写入字节数;
        if (!WriteProcessMemory(游戏进程句柄, 远程内存, 切换代码.data(), 切换代码.size(), &写入字节数))
        {
            VirtualFreeEx(游戏进程句柄, 远程内存, 0, MEM_RELEASE);
            MessageBox(_T("写入代码失败！"), _T("错误"), MB_ICONERROR);
            return;
        }

        // 创建远程线程执行
        HANDLE 远程线程 = CreateRemoteThread(游戏进程句柄, NULL, 0,
            (LPTHREAD_START_ROUTINE)远程内存, NULL, 0, NULL);
        if (远程线程 == NULL)
        {
            VirtualFreeEx(游戏进程句柄, 远程内存, 0, MEM_RELEASE);
            MessageBox(_T("创建远程线程失败！"), _T("错误"), MB_ICONERROR);
            return;
        }

        // 等待线程完成
        WaitForSingleObject(远程线程, 5000);
        CloseHandle(远程线程);

        // 释放内存
        VirtualFreeEx(游戏进程句柄, 远程内存, 0, MEM_RELEASE);

        // ===== 新增：等待地图切换完成 =====
        BOOL 地图切换成功 = FALSE;
        for (int 尝试次数 = 0; 尝试次数 < 20; 尝试次数++) // 最多尝试10次，每次500ms，共5秒
        {
            Sleep(500); // 等待半秒

            DWORD 当前地图;
            if (ReadProcessMemory(游戏进程句柄, (LPCVOID)地图编号地址, &当前地图, sizeof(DWORD), &读取字节数))
            {
                if ((int)当前地图 == 目标地图原始编号)
                {
                    地图切换成功 = TRUE;
                    TRACE(_T("地图切换成功！当前地图编号=%d\n"), 当前地图);
                    break;
                }
                else
                {
                    TRACE(_T("等待地图切换，当前地图=%d，目标地图=%d\n"), 当前地图, 目标地图原始编号);
                }
            }
        }

        if (!地图切换成功)
        {
            MessageBox(_T("地图切换超时，传送失败！"), _T("错误"), MB_ICONERROR);
            return;
        }
    }
    else
    {
        TRACE(_T("已在目标地图，无需切换\n"));
    }

    Sleep(1000);

    // 写入坐标
    TRACE(_T("写入坐标: X=%.2f, Y=%.2f\n"), 目标X, 目标Y);

    // 尝试多次写入确保成功
    for (int i = 0; i < 3; i++)
    {
        SIZE_T 写入字节数;

        // 写入X坐标
        WriteProcessMemory(游戏进程句柄, (LPVOID)坐标X地址, &目标X, sizeof(float), &写入字节数);

        // 写入Y坐标
        WriteProcessMemory(游戏进程句柄, (LPVOID)坐标Y地址, &目标Y, sizeof(float), &写入字节数);

        Sleep(500);
    }

}

void 注入页面类::点击传送()
{
    if (!有记录坐标)
    {
        MessageBox(_T("请先记录坐标！"), _T("提示"), MB_ICONINFORMATION);
        return;
    }

    // 检查目标地图是否允许传送（同记录限制）
    if (!是否允许记录坐标(记录地图编号 + 1))
    {
        CString 地图名称 = 获取地图名称(记录地图编号 + 1);
        CString 提示信息;
        提示信息.Format(_T("目标地图[%s]不允许传送！"), 地图名称);
        MessageBox(提示信息, _T("提示"), MB_ICONWARNING);
        return;
    }

    // 执行传送，传入记录的地图编号和坐标
    执行传送(记录地图编号, 记录X坐标, 记录Y坐标);

}

//黑商BUFF
void 注入页面类::执行加BUFF()
{
    // 检查游戏进程
    if (!检查游戏进程())
    {
        MessageBox(_T("找不到游戏进程，请先启动游戏！"), _T("错误"), MB_ICONERROR);
        return;
    }

    TRACE(_T("开始执行加枪手BUFF...\n"));

    // 构建加BUFF代码
    std::vector<BYTE> 加BUFF代码;

    // pushad
    加BUFF代码.push_back(0x60);
    // pushfd
    加BUFF代码.push_back(0x9C);

    // push [319B850] - 使用间接寻址
    // mov eax, [0x0319B850]; push eax
    加BUFF代码.push_back(0xA1);  // mov eax, [imm32]
    DWORD 参数地址 = 0x0319B850;
    加BUFF代码.push_back(参数地址 & 0xFF);
    加BUFF代码.push_back((参数地址 >> 8) & 0xFF);
    加BUFF代码.push_back((参数地址 >> 16) & 0xFF);
    加BUFF代码.push_back((参数地址 >> 24) & 0xFF);
    加BUFF代码.push_back(0x50);  // push eax

    // mov eax, 0x006B43D8
    加BUFF代码.push_back(0xB8);
    DWORD funcAddr = 0x006B43D8;
    加BUFF代码.push_back(funcAddr & 0xFF);
    加BUFF代码.push_back((funcAddr >> 8) & 0xFF);
    加BUFF代码.push_back((funcAddr >> 16) & 0xFF);
    加BUFF代码.push_back((funcAddr >> 24) & 0xFF);
    // call eax
    加BUFF代码.push_back(0xFF);
    加BUFF代码.push_back(0xD0);
    // add esp, 4
    加BUFF代码.push_back(0x83);
    加BUFF代码.push_back(0xC4);
    加BUFF代码.push_back(0x04);
    // popfd
    加BUFF代码.push_back(0x9D);
    // popad
    加BUFF代码.push_back(0x61);
    // ret
    加BUFF代码.push_back(0xC3);

    // 分配远程内存
    BYTE* 远程内存 = (BYTE*)VirtualAllocEx(游戏进程句柄, NULL, 加BUFF代码.size(),
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (远程内存 == NULL)
    {
        MessageBox(_T("分配远程内存失败！"), _T("错误"), MB_ICONERROR);
        return;
    }

    // 写入代码
    SIZE_T 写入字节数;
    if (!WriteProcessMemory(游戏进程句柄, 远程内存, 加BUFF代码.data(), 加BUFF代码.size(), &写入字节数))
    {
        VirtualFreeEx(游戏进程句柄, 远程内存, 0, MEM_RELEASE);
        MessageBox(_T("写入代码失败！"), _T("错误"), MB_ICONERROR);
        return;
    }

    TRACE(_T("加BUFF代码已写入远程内存: 0x%08X, 大小: %d 字节\n"),
        (DWORD)远程内存, 加BUFF代码.size());

    // 创建远程线程执行
    HANDLE 远程线程 = CreateRemoteThread(游戏进程句柄, NULL, 0,
        (LPTHREAD_START_ROUTINE)远程内存, NULL, 0, NULL);
    if (远程线程 == NULL)
    {
        VirtualFreeEx(游戏进程句柄, 远程内存, 0, MEM_RELEASE);
        MessageBox(_T("创建远程线程失败！"), _T("错误"), MB_ICONERROR);
        return;
    }

    // 等待线程执行完成
    WaitForSingleObject(远程线程, 5000);
    CloseHandle(远程线程);

    // 释放内存
    VirtualFreeEx(游戏进程句柄, 远程内存, 0, MEM_RELEASE);

}

void 注入页面类::点击加枪手BUFF()
{
    // 检查游戏进程
    if (!检查游戏进程())
    {
        MessageBox(_T("找不到游戏进程，请先启动游戏！"), _T("错误"), MB_ICONERROR);
        return;
    }

    // 执行加BUFF
    执行加BUFF();
}
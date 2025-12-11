// NageDlq.cpp: 定义应用程序的类行为。
//

#include "pch.h"
#include "framework.h"
#include "NageDlq.h"
#include "NageDlqDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 全局互斥体句柄
HANDLE g_hSingleInstanceMutex = NULL;

// 单例检测函数实现
BOOL IsAlreadyRunning()
{
    // 创建互斥体，使用唯一的名称
    g_hSingleInstanceMutex = CreateMutex(NULL, TRUE, _T("NageDlq_SingleInstance_Mutex_By_Client"));

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // 已经有一个实例在运行
        if (g_hSingleInstanceMutex != NULL)
        {
            CloseHandle(g_hSingleInstanceMutex);
            g_hSingleInstanceMutex = NULL;
        }

        // 查找并激活已存在的窗口
        // 尝试不同的窗口标题
        HWND hWnd = FindWindow(NULL, _T("震撼美丽登录器"));
        if (hWnd == NULL)
        {
            hWnd = FindWindow(NULL, _T("NageDlq"));
        }

        if (hWnd != NULL)
        {
            // 如果窗口最小化，恢复它
            if (IsIconic(hWnd))
            {
                ShowWindow(hWnd, SW_RESTORE);
            }
            // 激活窗口并置于前台
            SetForegroundWindow(hWnd);
            BringWindowToTop(hWnd);
        }

        return TRUE;
    }

    // 互斥体创建成功，这是第一个实例
    return FALSE;
}

// 释放互斥体
void ReleaseSingletonMutex()
{
    if (g_hSingleInstanceMutex != NULL)
    {
        ReleaseMutex(g_hSingleInstanceMutex);
        CloseHandle(g_hSingleInstanceMutex);
        g_hSingleInstanceMutex = NULL;
    }
}

// CNageDlqApp

BEGIN_MESSAGE_MAP(CNageDlqApp, CWinApp)
    ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CNageDlqApp 构造

CNageDlqApp::CNageDlqApp()
{
    // TODO: 在此处添加构造代码，
    // 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的 CNageDlqApp 对象

CNageDlqApp theApp;


// CNageDlqApp 初始化

BOOL CNageDlqApp::InitInstance()
{
    // 单例检测 - 确保只运行一个实例
    if (IsAlreadyRunning())
    {
        AfxMessageBox(_T("登录器已经在运行中！"), MB_OK | MB_ICONINFORMATION);
        return FALSE; // 退出当前实例
    }

    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    // 将它设置为包括所有要在应用程序中使用的
    // 公共控件类。
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();

    if (!AfxSocketInit())
    {
        AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
        ReleaseSingletonMutex(); // 释放互斥体
        return FALSE;
    }


    AfxEnableControlContainer();

    // 任何 shell 树视图控件或 shell 列表视图控件。
    CShellManager* pShellManager = new CShellManager;

    // 激活"Windows Native"视觉管理器，以便在 MFC 控件中启用主题
    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));


    SetRegistryKey(_T("应用程序向导生成的本地应用程序"));

    NageDlqDlg dlg;
    m_pMainWnd = &dlg;

    INT_PTR nResponse = dlg.DoModal();
    if (nResponse == IDOK)
    {
        // TODO: 在此放置处理何时用
        //  "确定"来关闭对话框的代码
    }
    else if (nResponse == IDCANCEL)
    {
        // TODO: 在此放置处理何时用
        //  "取消"来关闭对话框的代码
    }
    else if (nResponse == -1)
    {
        TRACE(traceAppMsg, 0, "警告: 对话框创建失败，应用程序将意外终止。\n");
        TRACE(traceAppMsg, 0, "警告: 如果您在对话框上使用 MFC 控件，则无法 #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS。\n");
    }

    if (pShellManager != nullptr)
    {
        delete pShellManager;
    }

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
    ControlBarCleanUp();
#endif

    // 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
    //  而不是启动应用程序的消息泵。
    return FALSE;
}

// 退出实例时释放资源
int CNageDlqApp::ExitInstance()
{
    // 释放互斥体
    ReleaseSingletonMutex();

    return CWinApp::ExitInstance();
}
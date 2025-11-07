#include <windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxdisp.h>
#include <afxdtctl.h>
#include <afxcmn.h>
#include "pch.h"
#include "NageUP.h"
#include "NageUPDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CNageUPApp, CWinApp)
END_MESSAGE_MAP()

CNageUPApp::CNageUPApp()
{
}

CNageUPApp theApp;

BOOL CNageUPApp::InitInstance()
{
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();

    CString 命令行 = GetCommandLine();
    CString 当前版本号 = _T("1.0.0");

    int 参数位置 = 命令行.Find(_T("--current-version="));
    if (参数位置 != -1)
    {
        当前版本号 = 命令行.Mid(参数位置 + 18);
        当前版本号.Trim(_T("\" "));
    }

    NageUPDlg dlg;
    dlg.设置当前版本(当前版本号);
    m_pMainWnd = &dlg;
    dlg.DoModal();

    return FALSE;
}
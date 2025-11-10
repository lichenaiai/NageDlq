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
    // 调试：显示完整命令行
    CString 命令行 = GetCommandLine();

    CString 调试信息;
    调试信息.Format(_T("完整命令行:\n%s"), 命令行);

    // 解析目标版本号
    CString 目标版本号 = _T("");

    // 查找参数位置
    int 参数位置 = 命令行.Find(_T("--target-version="));

    if (参数位置 != -1)
    {
        // 提取参数值
        目标版本号 = 命令行.Mid(参数位置 + 17); // "--target-version=" 长度是17

        调试信息.Format(_T("提取的原始参数: %s"), 目标版本号);

        // 清理参数 - 去掉可能的引号和后续参数
        目标版本号.Trim();
        if (目标版本号.Find(_T(' ')) != -1)
        {
            目标版本号 = 目标版本号.Left(目标版本号.Find(_T(' ')));
        }
        目标版本号.Trim(_T("\""));

        调试信息.Format(_T("清理后目标版本号: %s"), 目标版本号);
    }

    // 创建并显示主对话框
    NageUPDlg dlg;
    dlg.设置目标版本号(目标版本号);
    m_pMainWnd = &dlg;
    dlg.DoModal();

    return FALSE;
}
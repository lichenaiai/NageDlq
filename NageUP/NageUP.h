#pragma once

// 手动包含所有必要的MFC头文件
#include <afxwin.h>         // MFC核心
#include <afxext.h>         // MFC扩展  
#include <afxdisp.h>        // MFC自动化
#include <afxdtctl.h>       // MFC支持
#include <afxcmn.h>         // MFC公共控件
#include "resource.h"

class CNageUPApp : public CWinApp
{
public:
    CNageUPApp();
    virtual BOOL InitInstance();

    DECLARE_MESSAGE_MAP()
};
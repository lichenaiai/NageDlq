// NageDlq.h: PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
#error "在包含此文件之前包含 'pch.h' 以生成 PCH"
#endif

#include "resource.h"		// 主符号

// 单例检测函数声明
BOOL IsAlreadyRunning();
void ReleaseSingletonMutex();

// CNageDlqApp:
// 有关此类的实现，请参阅 NageDlq.cpp
//

class CNageDlqApp : public CWinApp
{
public:
	CNageDlqApp();

	// 重写
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();  // 添加退出实例函数

	// 实现
	DECLARE_MESSAGE_MAP()
};

extern CNageDlqApp theApp;
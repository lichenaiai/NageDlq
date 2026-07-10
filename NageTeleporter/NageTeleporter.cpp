#include "pch.h"
#include "NageTeleporter.h"
#include "NageTeleporterDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CNageTeleporterApp, CWinApp)
END_MESSAGE_MAP()

CNageTeleporterApp::CNageTeleporterApp()
{
}

CNageTeleporterApp theApp;

BOOL CNageTeleporterApp::InitInstance()
{
	// 创建并显示主对话框
	NageTeleporterDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();

	return FALSE;
}

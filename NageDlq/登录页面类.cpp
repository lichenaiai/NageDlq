// 登录页面类.cpp
#include "pch.h"
#include "framework.h"
#include "NageDlq.h"
#include "登录页面类.h"
#include "afxdialogex.h"
#include "NageDlqDlg.h"
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 登录页面类 对话框
IMPLEMENT_DYNAMIC(登录页面类, CDialogEx)

登录页面类::登录页面类(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PAGE_LOGIN, pParent)
	, 已登录(false)
	, 窗口1280选中状态(true)  // 默认选中
	, 游戏进程句柄(NULL)
	, 游戏进程ID(0)
	, 游戏运行中(FALSE)
	, 客户端重新连接标志(FALSE)     // 初始化客户端重新连接标志
	, 账号重新连接标志(FALSE)    // 初始化账号重新连接标志
{
}

登录页面类::~登录页面类()
{
}

// 数据交换
void 登录页面类::DoDataExchange(CDataExchange* pDX)	
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_USERNAME, 用户名编辑框);
	DDX_Control(pDX, IDC_EDIT_PASSWORD, 密码编辑框);
	DDX_Control(pDX, IDC_BUTTON_LOGIN, 登录按钮);
	DDX_Control(pDX, IDC_CHECK_1280, 窗口1280复选框);
	DDX_Control(pDX, IDC_BUTTON_START, 启动按钮);
	DDX_Control(pDX, IDC_STATIC_BG, 背景图片);
	DDX_Control(pDX, IDC_STATIC_AU, 权限状态);
}

// 登录页面类 消息处理程序
BEGIN_MESSAGE_MAP(登录页面类, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_LOGIN, &登录页面类::OnBnClickedButtonLogin)	//::点击登录按钮
	ON_BN_CLICKED(IDC_BUTTON_START, &登录页面类::OnBnClickedButtonStart)	//::点击启动按钮
	ON_BN_CLICKED(IDC_RE_LOGIN, &登录页面类::OnBnClickedButtonRelogin)	//重新连接按钮
	ON_WM_TIMER()  // 定时器消息处理
END_MESSAGE_MAP()

// 定时器处理函数
void 登录页面类::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 100)  // 登录按钮冷却定时器
	{
		KillTimer(100);
		登录按钮.EnableWindow(TRUE);
	}
	else if (nIDEvent == 103)  // 重新连接超时定时器
	{
		KillTimer(103);

		// 检查连接是否真的成功了
		CWnd* 主窗口 = AfxGetMainWnd();
		if (主窗口)
		{
			NageDlqDlg* 主对话框 = dynamic_cast<NageDlqDlg*>(主窗口);
			if (主对话框 && 主对话框->网络通信.是否已连接())
			{
				TRACE(_T("客户端重新连接成功\n"));
				权限状态.SetWindowText(_T("状态：客户端重新连接成功"));

				// 连接成功，按钮保持禁用状态
				启用重新连接按钮(FALSE);
			}
			else
			{
				TRACE(_T("客户端连接超时\n"));
				权限状态.SetWindowText(_T("状态：客户端连接超时，请重试"));

				// 连接超时，重新启用按钮
				启用重新连接按钮(TRUE);
			}
		}
		else
		{
			// 主窗口不存在，重新启用按钮
			启用重新连接按钮(TRUE);
		}

		客户端重新连接标志 = FALSE;
	}
	else if (nIDEvent == 104)  // 心跳检测定时器（每10秒检查一次连接状态）
	{
		CWnd* 主窗口 = AfxGetMainWnd();
		if (主窗口)
		{
			NageDlqDlg* 主对话框 = dynamic_cast<NageDlqDlg*>(主窗口);
			if (主对话框)
			{
				if (!主对话框->网络通信.是否已连接())
				{
					// 连接断开，启用重新连接按钮
					启用重新连接按钮(TRUE);
					权限状态.SetWindowText(_T("状态：连接已断开"));
				}
			}
		}
	}

	CDialogEx::OnTimer(nIDEvent);
}

// 启用或禁用重新连接按钮
void 登录页面类::启用重新连接按钮(BOOL 启用)
{
	CButton* 重新连接按钮 = (CButton*)GetDlgItem(IDC_RE_LOGIN);
	if (重新连接按钮)
	{
		重新连接按钮->EnableWindow(启用);
		if (启用)
		{
			重新连接按钮->SetWindowText(_T("重新连接"));
		}
		else
		{
			重新连接按钮->SetWindowText(_T("已连接"));
		}
	}
}

// 初始化对话框
BOOL 登录页面类::OnInitDialog()		
{
	CDialogEx::OnInitDialog();

	// 设置密码编辑框为密码模式
	密码编辑框.SetPasswordChar('*');

	// 默认选中窗口1280复选框
	窗口1280复选框.SetCheck(BST_CHECKED);
	窗口1280选中状态 = true;

	// 加载背景图片
	HBITMAP 背景位图 = (HBITMAP)LoadImage(AfxGetInstanceHandle(),
		MAKEINTRESOURCE(IDB_LOGIN_MAP),
		IMAGE_BITMAP, 825, 490, LR_DEFAULTCOLOR);

	if (背景位图 != NULL)
	{
		// 设置背景图片
		背景图片.SetBitmap(背景位图);

		// 获取图片尺寸并调整控件大小
		BITMAP bmpInfo;
		GetObject(背景位图, sizeof(BITMAP), &bmpInfo);

		// 调整背景图片控件大小以适应图片
		背景图片.SetWindowPos(NULL, 800, 600, bmpInfo.bmWidth, bmpInfo.bmHeight,
			SWP_NOZORDER | SWP_NOMOVE);
	}
	else
	{
		TRACE(_T("加载背景图片失败\n"));
	}
	
	// 初始化重新连接按钮状态
	启用重新连接按钮(FALSE);  // 初始时禁用，因为没有连接

	// 启动心跳检测
	SetTimer(104, 10000, nullptr);  // 每10秒检测一次连接状态

	return TRUE;
}

// 点击登录按钮
void 登录页面类::OnBnClickedButtonLogin()		
{
	// 获取用户名和密码
	CString 用户名;
	CString 密码;
	用户名编辑框.GetWindowText(用户名);
	密码编辑框.GetWindowText(密码);

	// 简单的非空检查
	if (用户名.IsEmpty() || 密码.IsEmpty())
	{
		MessageBox(_T("请输入用户名和密码"), _T("提示"), MB_ICONWARNING);
		return;
	}

	// 防止重复点击
	登录按钮.EnableWindow(FALSE);

	// 首先确保客户端连接正常
	CWnd* 主窗口 = AfxGetMainWnd();
	if (主窗口)
	{
		NageDlqDlg* 主对话框 = dynamic_cast<NageDlqDlg*>(主窗口);
		if (主对话框)
		{
			// 检查客户端连接状态
			if (!主对话框->网络通信.是否已连接())
			{
				TRACE(_T("客户端未连接，先建立连接\n"));
				权限状态.SetWindowText(_T("状态：正在连接服务器..."));

				// 尝试建立客户端连接
				if (主对话框->初始化网络通信())
				{
					// 等待连接建立（最多等待3秒）
					int 等待次数 = 0;
					while (!主对话框->网络通信.是否已连接() && 等待次数 < 30)
					{
						Sleep(100);
						等待次数++;
					}

					if (!主对话框->网络通信.是否已连接())
					{
						TRACE(_T("客户端连接超时\n"));
						MessageBox(_T("连接服务器超时，请重试"), _T("错误"), MB_ICONERROR);
						登录按钮.EnableWindow(TRUE);
						return;
					}
					// 连接成功，禁用重新连接按钮
					启用重新连接按钮(FALSE);
				}
				else
				{
					TRACE(_T("客户端连接失败\n"));
					MessageBox(_T("连接服务器失败，请检查网络"), _T("错误"), MB_ICONERROR);
					登录按钮.EnableWindow(TRUE);
					return;
				}
			}

			// 客户端连接正常，发送登录请求
			TRACE(_T("客户端连接正常，发送登录请求\n"));
			执行重新连接账号();  // 使用账号重新连接函数
		}
	}

	// 3秒后重新启用按钮，防止重复发送
	SetTimer(100, 3000, nullptr);
}

// 重新连接按钮点击事件处理
void 登录页面类::OnBnClickedButtonRelogin()
{
	TRACE(_T("=== 点击重新连接按钮 ===\n"));

	// 防止重复点击
	if (客户端重新连接标志)
	{
		TRACE(_T("客户端重新连接中，请稍候...\n"));
		return;
	}

	// 开始客户端重新连接流程
	执行重新连接客户端();
}

// 执行客户端重新连接（只重新连接客户端到服务端，不影响账号登录状态）
void 登录页面类::执行重新连接客户端()
{
	TRACE(_T("=== 开始执行客户端重新连接 ===\n"));

	客户端重新连接标志 = TRUE;

	// 禁用重新连接按钮防止重复操作
	启用重新连接按钮(FALSE);

	// 更新状态显示
	CString 状态文本 = _T("状态：客户端重新连接中...");
	权限状态.SetWindowText(状态文本);
	TRACE(_T("设置状态为客户端重新连接中...\n"));

	// 通过主对话框重新初始化网络
	CWnd* 主窗口 = AfxGetMainWnd();
	if (主窗口)
	{
		NageDlqDlg* 主对话框 = dynamic_cast<NageDlqDlg*>(主窗口);
		if (主对话框)
		{
			// 1. 关闭现有连接
			if (主对话框->网络通信.是否已连接())
			{
				TRACE(_T("关闭现有网络连接\n"));
				主对话框->网络通信.关闭连接();
			}

			// 2. 重新初始化网络连接（重点：只重新建立客户端连接）
			TRACE(_T("开始重新初始化网络连接\n"));

			// 直接调用主对话框的初始化网络通信
			if (主对话框->初始化网络通信())
			{
				TRACE(_T("客户端重新连接网络成功\n"));
				权限状态.SetWindowText(_T("状态：客户端重新连接成功，等待服务器响应..."));

				// 设置一个定时器检查连接状态（3秒后）
				SetTimer(103, 3000, nullptr);
			}
			else
			{
				TRACE(_T("客户端重新连接网络失败\n"));
				权限状态.SetWindowText(_T("状态：客户端重新连接失败"));

				// 启用重新连接按钮
				启用重新连接按钮(TRUE);

				客户端重新连接标志 = FALSE;
			}
		}
	}
}

// 执行账号重新连接（用于登录按钮中的重新连接逻辑）
void 登录页面类::执行重新连接账号()
{
	TRACE(_T("=== 开始执行账号重新连接 ===\n"));

	账号重新连接标志 = TRUE;

	// 获取用户名和密码
	CString 用户名;
	CString 密码;
	用户名编辑框.GetWindowText(用户名);
	密码编辑框.GetWindowText(密码);

	// 检查是否已有账号信息
	if (用户名.IsEmpty() || 密码.IsEmpty())
	{
		TRACE(_T("账号或密码为空，无法重新连接\n"));
		账号重新连接标志 = FALSE;
		return;
	}

	// 构建登录请求
	CString 登录请求;
	登录请求.Format(_T("LOGIN:%s:%s"), 用户名, 密码);

	TRACE(_T("发送账号重新登录请求: %s\n"), 登录请求);

	// 通过主对话框发送请求
	CWnd* 主窗口 = AfxGetMainWnd();
	if (主窗口)
	{
		NageDlqDlg* 主对话框 = dynamic_cast<NageDlqDlg*>(主窗口);
		if (主对话框 && 主对话框->发送请求到服务端(登录请求))
		{
			TRACE(_T("账号重新登录请求发送成功\n"));
			权限状态.SetWindowText(_T("状态：账号重新登录中..."));
		}
		else
		{
			TRACE(_T("发送账号重新登录请求失败\n"));
			权限状态.SetWindowText(_T("状态：账号重新登录失败"));
			账号重新连接标志 = FALSE;
		}
	}
}

// 退出登录状态方法
void 登录页面类::退出登录状态()
{
	TRACE(_T("=== 退出登录状态 ===\n"));

	if (已登录)
	{
		// 清空用户名和密码
		用户名编辑框.SetWindowText(_T(""));
		密码编辑框.SetWindowText(_T(""));

		// 重置登录状态
		已登录 = FALSE;

		// 更新界面状态
		登录按钮.EnableWindow(TRUE);
		登录按钮.SetWindowText(_T("登录"));

		TRACE(_T("已退出登录状态\n"));
	}
}

// 点击启动按钮
void 登录页面类::OnBnClickedButtonStart()		
{
	// 立即禁用按钮，防止重复点击
	CButton* p启动按钮 = (CButton*)GetDlgItem(IDC_BUTTON_START);
	if (p启动按钮)
	{
		p启动按钮->EnableWindow(FALSE);
		p启动按钮->SetWindowText(_T("游戏启动中..."));
	}

	// 更新成员变量状态
	窗口1280选中状态 = (窗口1280复选框.GetCheck() == BST_CHECKED);

	if (!窗口1280选中状态)
	{
		std::thread 窗口大小线程(&登录页面类::注入窗口大小修改代码, this);
		窗口大小线程.detach();
	}

	// 始终注入IP修改（无论窗口大小如何）
	std::thread IP注入线程(&登录页面类::注入IP修改代码, this);
	IP注入线程.detach();

	// 启动游戏
	启动游戏进程();

	// 设置游戏运行标志
	游戏运行中 = TRUE;
}

// 关闭游戏进程的方法
BOOL 登录页面类::关闭游戏进程()
{
	游戏运行中 = FALSE;

	// 直接终止游戏进程，不做检测
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	BOOL bFound = FALSE;
	if (Process32First(hProcessSnap, &pe32))
	{
		do
		{
			if (_wcsicmp(pe32.szExeFile, L"nage.bin") == 0)
			{
				HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
				if (hProcess != NULL)
				{
					TerminateProcess(hProcess, 0);
					CloseHandle(hProcess);
					bFound = TRUE;
				}
				break;
			}
		} while (Process32Next(hProcessSnap, &pe32));
	}

	CloseHandle(hProcessSnap);
	return bFound;
}

// 处理登录响应
void 登录页面类::处理登录响应(const CString& 响应数据)
{
	if (响应数据.Find(_T("LOGIN_SUCCESS")) == 0)
	{
		MessageBox(_T("登录成功"), _T("提示"), MB_ICONINFORMATION);
		已登录 = true;
		
		// 更新登录按钮状态
		登录按钮.EnableWindow(FALSE);
		登录按钮.SetWindowText(_T("已登录"));

		// 更新状态显示
		CString 状态文本;
		状态文本.Format(_T("状态：已登录（用户: %s）"), 获取当前用户名());
		权限状态.SetWindowText(状态文本);

		// 重置重新连接标志
		账号重新连接标志 = FALSE;
	}
	else if (响应数据.Find(_T("LOGIN_FAILED")) == 0)
	{
		CString 错误信息 = 响应数据.Mid(12);
		MessageBox(错误信息, _T("登录失败"), MB_ICONERROR);
		已登录 = false;

		// 重置登录按钮状态
		登录按钮.EnableWindow(TRUE);
		登录按钮.SetWindowText(_T("登录"));

		// 更新状态显示
		权限状态.SetWindowText(_T("状态：登录失败"));

		// 重置重新连接标志
		账号重新连接标志 = FALSE;
	}
}

// 获取当前输入的用户名
CString 登录页面类::获取当前用户名()
{
	CString 用户名;
	用户名编辑框.GetWindowText(用户名);
	return 用户名;
}

// 注入IP修改代码
void 登录页面类::注入IP修改代码()
{
	// 等待并安装IP钩子
	等待并安装IP钩子(L"nage.bin");
}

// 注入窗口大小修改代码（只有在复选框未选中时才调用）
void 登录页面类::注入窗口大小修改代码()
{
	// 等待并安装窗口大小钩子
	等待并安装窗口大小钩子(L"nage.bin");
}

// 启动游戏进程
void 登录页面类::启动游戏进程()
{
	// 获取当前目录
	TCHAR 当前路径[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, 当前路径);

	// 构建游戏路径
	CString 游戏路径;
	游戏路径.Format(_T("%s\\nage.bin"), 当前路径);

	// 检查文件是否存在
	if (GetFileAttributes(游戏路径) == INVALID_FILE_ATTRIBUTES)
	{
		MessageBox(_T("找不到游戏文件 nage.bin"), _T("错误"), MB_ICONERROR);
		return;
	}

	// 构建命令行
	CString 命令行;
	命令行.Format(_T("\"%s\" 1 0 0"), 游戏路径);

	// 启动游戏进程
	STARTUPINFO si = { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION pi;
	if (CreateProcess(NULL, 命令行.GetBuffer(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		// 关闭句柄
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		命令行.ReleaseBuffer();
	}
	else
	{
		DWORD 错误码 = GetLastError();
		CString 错误信息;
		错误信息.Format(_T("启动游戏失败，错误码：%d"), 错误码);
		MessageBox(错误信息, _T("错误"), MB_ICONERROR);
		命令行.ReleaseBuffer();
	}
}

// 获取进程ID
DWORD 登录页面类::获取进程ID(const wchar_t* 进程名)
{
	DWORD pid = 0;
	HANDLE 快照句柄 = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (快照句柄 == INVALID_HANDLE_VALUE) return 0;

	PROCESSENTRY32 进程信息;
	进程信息.dwSize = sizeof(进程信息);

	BOOL 有进程 = Process32First(快照句柄, &进程信息);
	while (有进程)
	{
		if (_wcsicmp(进程信息.szExeFile, 进程名) == 0)
		{
			pid = 进程信息.th32ProcessID;
			break;
		}
		有进程 = Process32Next(快照句柄, &进程信息);
	}
	CloseHandle(快照句柄);
	return pid;
}

// 等待并安装窗口大小钩子（只有在复选框未选中时才调用）
void 登录页面类::等待并安装窗口大小钩子(const wchar_t* 监控进程名)
{
	DWORD pid = 0;
	HANDLE 目标进程句柄 = NULL;

	// 等待进程启动
	while (true)
	{
		pid = 获取进程ID(监控进程名);
		if (pid)
		{
			// 打开目标进程
			目标进程句柄 = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
			if (目标进程句柄 == NULL)
			{
				Sleep(1000);
				continue;
			}

			// 修改窗口大小,地址：0x5AAB1E 和 0x5AAB23
			BYTE 窗口高度代码[] = { 0x68, 0x28, 0x03, 0x00, 0x00 }; // push 328
			BYTE 窗口宽度代码[] = { 0x68, 0x73, 0x02, 0x00, 0x00 }; // push 273

			// 写入内存
			SIZE_T 写入字节数;
			WriteProcessMemory(目标进程句柄, (LPVOID)0x5AAB1E, 窗口宽度代码, sizeof(窗口宽度代码), &写入字节数);
			WriteProcessMemory(目标进程句柄, (LPVOID)0x5AAB23, 窗口高度代码, sizeof(窗口高度代码), &写入字节数);

			CloseHandle(目标进程句柄);
			break;
		}
		Sleep(1000);
	}
}

// 等待并安装IP钩子
/*
void 登录页面类::等待并安装IP钩子(const wchar_t* 监控进程名)
{
	DWORD pid = 0;
	HANDLE 目标进程句柄 = NULL;

	// 等待进程启动
	while (true)
	{
		pid = 获取进程ID(监控进程名);
		if (pid)
		{
			// 打开目标进程
			目标进程句柄 = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
			if (目标进程句柄 == NULL)
			{
				Sleep(1000);
				continue;
			}

			BYTE* 远程内存 = (BYTE*)VirtualAllocEx(目标进程句柄, NULL, 1024, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			// 分配远程内存用于钩子代码
			if (远程内存 == NULL)
			{
				CloseHandle(目标进程句柄);
				Sleep(1000);
				continue;
			}

			SIZE_T 写入字节数;

			// 1. 首先写入钩子代码到远程内存
			BYTE IP钩子代码[] = {
				0xB8, 0x7C, 0xDC, 0x52, 0x57,	// mov eax,63A7742F  5752DC7C
				0x89, 0x45, 0xE8,				// mov dword ptr ss:[ebp-18],eax
				0x8B, 0x4D, 0xE8,				// mov ecx,dword ptr ss:[ebp-18]
				0xE9, 0xD2, 0xBB, 0x50, 0x00,	// jmp 6ABBE2
				0x90							// nop
			};

			if (!WriteProcessMemory(目标进程句柄, 远程内存, IP钩子代码, sizeof(IP钩子代码), &写入字节数))
			{
				VirtualFreeEx(目标进程句柄, 远程内存, 0, MEM_RELEASE);
				CloseHandle(目标进程句柄);
				Sleep(1000);
				continue;
			}

			// 2. 计算跳转偏移：远程内存 - (0x006ABBDC + 5)
			DWORD 跳转偏移 = (DWORD)(远程内存 - (0x006ABBDC + 5));

			// 3. 修改 006ABBDC 地址的跳转指令
			BYTE 修改代码1[] = { 0xE9, 0x00, 0x00, 0x00, 0x00 }; // jmp 远程内存
			*(DWORD*)(修改代码1 + 1) = 跳转偏移;

			// 4. 修改 006ABBE1 地址为nop
			BYTE 修改代码2[] = { 0x90 }; // nop

			// 5. 写入修改到目标地址
			if (!WriteProcessMemory(目标进程句柄, (LPVOID)0x006ABBDC, 修改代码1, sizeof(修改代码1), &写入字节数) ||
				!WriteProcessMemory(目标进程句柄, (LPVOID)0x006ABBE1, 修改代码2, sizeof(修改代码2), &写入字节数))
			{
				VirtualFreeEx(目标进程句柄, 远程内存, 0, MEM_RELEASE);
				CloseHandle(目标进程句柄);
				Sleep(1000);
				continue;
			}

			CloseHandle(目标进程句柄);
			break;
		}
		Sleep(1000);
	}
}
*/
void 登录页面类::等待并安装IP钩子(const wchar_t* 监控进程名)
{
	DWORD pid = 0;
	HANDLE 目标进程句柄 = NULL;

	// 等待进程启动
	while (true)
	{
		pid = 获取进程ID(监控进程名);
		if (pid)
		{
			// 打开目标进程
			目标进程句柄 = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
			if (目标进程句柄 == NULL)
			{
				Sleep(1000);
				continue;
			}

			// 分配远程内存用于钩子代码
			BYTE* 远程内存 = (BYTE*)VirtualAllocEx(目标进程句柄, NULL, 1024, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (远程内存 == NULL)
			{
				CloseHandle(目标进程句柄);
				Sleep(1000);
				continue;
			}

			SIZE_T 写入字节数;

			// 1. 动态计算跳转地址
			// 目标跳回地址：0x006ABBE2
			DWORD 目标跳回地址 = 0x006ABBE2;

			// 计算跳转偏移：目标跳回地址 - (远程内存 + IP钩子代码长度 - 5 + 1)
			// IP钩子代码长度是 sizeof(IP钩子代码)，但我们需要的是包含jmp指令之前的位置

			// 钩子代码结构：
			// mov eax,5752DC7C (5字节)
			// mov dword ptr ss:[ebp-18],eax (3字节)
			// mov ecx,dword ptr ss:[ebp-18] (3字节)
			// jmp XXXXXXXX (5字节) <-- 这里需要动态计算
			// nop (1字节)

			// 创建动态的钩子代码
			std::vector<BYTE> IP钩子代码动态;

			// 前13字节固定：mov eax + mov [ebp-18],eax + mov ecx,[ebp-18]
			BYTE 固定代码[] = {
				0xB8, 0x7C, 0xDC, 0x52, 0x57,	// mov eax, 5752DC7C
				0x89, 0x45, 0xE8,				// mov dword ptr ss:[ebp-18],eax
				0x8B, 0x4D, 0xE8				// mov ecx,dword ptr ss:[ebp-18]
			};

			// 复制固定代码
			IP钩子代码动态.insert(IP钩子代码动态.end(), 固定代码, 固定代码 + sizeof(固定代码));

			// 计算jmp指令的偏移
			// jmp指令的位置：远程内存 + IP钩子代码动态.size()
			DWORD jmp指令位置 = (DWORD)远程内存 + IP钩子代码动态.size();

			// 计算跳转偏移：目标跳回地址 - (jmp指令位置 + 5)
			DWORD 跳转偏移 = 目标跳回地址 - (jmp指令位置 + 5);

			// 添加jmp指令
			IP钩子代码动态.push_back(0xE9); // jmp操作码

			// 添加跳转偏移（4字节，小端序）
			IP钩子代码动态.push_back(跳转偏移 & 0xFF);
			IP钩子代码动态.push_back((跳转偏移 >> 8) & 0xFF);
			IP钩子代码动态.push_back((跳转偏移 >> 16) & 0xFF);
			IP钩子代码动态.push_back((跳转偏移 >> 24) & 0xFF);

			// 添加nop
			IP钩子代码动态.push_back(0x90);

			// 2. 写入动态生成的钩子代码到远程内存
			if (!WriteProcessMemory(目标进程句柄, 远程内存, IP钩子代码动态.data(), IP钩子代码动态.size(), &写入字节数))
			{
				VirtualFreeEx(目标进程句柄, 远程内存, 0, MEM_RELEASE);
				CloseHandle(目标进程句柄);
				Sleep(1000);
				continue;
			}

			// 3. 计算从目标地址到远程内存的跳转偏移
			// 目标地址：0x006ABBDC
			DWORD 目标地址 = 0x006ABBDC;

			// 计算跳转偏移：远程内存 - (目标地址 + 5)
			DWORD 跳转偏移到远程内存 = (DWORD)远程内存 - (目标地址 + 5);

			// 4. 修改 006ABBDC 地址的跳转指令
			BYTE 修改代码1[] = {
				0xE9, 							// jmp
				0x00, 0x00, 0x00, 0x00 		// 跳转偏移
			};

			// 设置跳转偏移
			*(DWORD*)(修改代码1 + 1) = 跳转偏移到远程内存;

			// 5. 修改 006ABBE1 地址为nop
			BYTE 修改代码2[] = { 0x90 }; // nop

			// 6. 写入修改到目标地址
			if (!WriteProcessMemory(目标进程句柄, (LPVOID)目标地址, 修改代码1, sizeof(修改代码1), &写入字节数) ||
				!WriteProcessMemory(目标进程句柄, (LPVOID)0x006ABBE1, 修改代码2, sizeof(修改代码2), &写入字节数))
			{
				VirtualFreeEx(目标进程句柄, 远程内存, 0, MEM_RELEASE);
				CloseHandle(目标进程句柄);
				Sleep(1000);
				continue;
			}

			TRACE(_T("IP钩子安装成功\n"));
			TRACE(_T("远程内存地址: 0x%08X\n"), 远程内存);
			TRACE(_T("钩子代码长度: %d 字节\n"), IP钩子代码动态.size());
			TRACE(_T("跳转到远程内存偏移: 0x%08X\n"), 跳转偏移到远程内存);
			TRACE(_T("跳回目标地址偏移: 0x%08X\n"), 跳转偏移);

			CloseHandle(目标进程句柄);
			break;
		}
		Sleep(1000);
	}
}

// 等待并安装UI钩子
void 登录页面类::等待并安装UI钩子(const wchar_t* 监控进程名)
{
	// 希望HOOK的目标函数地址
	void* 目标地址 = (void*)0x6ABC1A;

	// 钩子机器码
	BYTE 钩子机器码[] = {
		0x60, 0x9C, 0xB8, 0xC4, 0x04, 0x00, 0x00, 0xB9, 0x5C, 0xAF, 0xEF, 0x02, 0x89, 0x01, 0xB9, 0x30, 0xB3, 0xEF
	};

	const int 钩子代码长度 = sizeof(钩子机器码);
	BYTE 原始字节[7];
	DWORD pid = 0;
	HANDLE 目标进程句柄 = NULL;

	// 等待进程启动
	while (true)
	{
		pid = 获取进程ID(监控进程名);
		if (pid)
		{
			// 打开目标进程
			目标进程句柄 = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
			if (目标进程句柄 == NULL)
			{
				Sleep(1000);
				continue;
			}

			// 分配远程可执行内存
			BYTE* 远程钩子区 = (BYTE*)VirtualAllocEx(目标进程句柄, NULL, 2048, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (远程钩子区 == NULL)
			{
				CloseHandle(目标进程句柄);
				Sleep(1000);
				continue;
			}

			SIZE_T 写入字节数 = 0;
			SIZE_T 读取字节数;

			// 1. 读取目标进程原始代码
			if (!ReadProcessMemory(目标进程句柄, 目标地址, 原始字节, 7, &读取字节数))
			{
				VirtualFreeEx(目标进程句柄, 远程钩子区, 0, MEM_RELEASE);
				CloseHandle(目标进程句柄);
				Sleep(1000);
				continue;
			}

			// 2. 原始指令到跳板开头
			if (!WriteProcessMemory(目标进程句柄, 远程钩子区, 原始字节, 7, &写入字节数))
			{
				VirtualFreeEx(目标进程句柄, 远程钩子区, 0, MEM_RELEASE);
				CloseHandle(目标进程句柄);
				Sleep(1000);
				continue;
			}

			// 3. 写入钩子逻辑
			if (!WriteProcessMemory(目标进程句柄, 远程钩子区 + 7, 钩子机器码, 钩子代码长度, &写入字节数))
			{
				VirtualFreeEx(目标进程句柄, 远程钩子区, 0, MEM_RELEASE);
				CloseHandle(目标进程句柄);
				Sleep(1000);
				continue;
			}

			// 4. 跳回原函数
			BYTE 跳转指令[5];
			跳转指令[0] = 0xE9;
			*(DWORD*)(跳转指令 + 1) = (DWORD)((BYTE*)目标地址 + 7 - (远程钩子区 + 7 + 钩子代码长度 + 5));

			if (!WriteProcessMemory(目标进程句柄, 远程钩子区 + 7 + 钩子代码长度, 跳转指令, 5, &写入字节数))
			{
				VirtualFreeEx(目标进程句柄, 远程钩子区, 0, MEM_RELEASE);
				CloseHandle(目标进程句柄);
				Sleep(1000);
				continue;
			}

			// 5. 安装钩子：在目标地址插入jmp，跳到钩子代码区
			BYTE 跳转[5] = { 0xE9, 0, 0, 0, 0 };
			*((DWORD*)(跳转 + 1)) = (DWORD)(远程钩子区 - (BYTE*)目标地址 - 5);

			if (!WriteProcessMemory(目标进程句柄, 目标地址, 跳转, 5, &写入字节数))
			{
				VirtualFreeEx(目标进程句柄, 远程钩子区, 0, MEM_RELEASE);
				CloseHandle(目标进程句柄);
				Sleep(1000);
				continue;
			}

			CloseHandle(目标进程句柄);
			break;
		}
		Sleep(1000); // 每秒检测一次
	}
}
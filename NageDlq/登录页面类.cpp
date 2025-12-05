// 登录页面类.cpp
#include "pch.h"
#include "framework.h"
#include "NageDlq.h"
#include "登录页面类.h"
#include "afxdialogex.h"
#include "NageDlqDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 登录页面类 对话框
IMPLEMENT_DYNAMIC(登录页面类, CDialogEx)

登录页面类::登录页面类(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PAGE_LOGIN, pParent)
	, 已登录(false)
	, 窗口1280选中状态(true)  // 默认选中
	, m_hGameProcess(NULL)
	, m_dwGameProcessId(0)
	, m_bGameRunning(FALSE)
	, m_bIsReconnecting(FALSE)
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
END_MESSAGE_MAP()

// 修改定时器处理函数（添加重新连接相关处理）
void 登录页面类::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 100)  // 登录按钮冷却定时器
	{
		KillTimer(100);
		登录按钮.EnableWindow(TRUE);
	}
	else if (nIDEvent == 101)  // 重新连接定时器
	{
		KillTimer(101);

		TRACE(_T("=== 延迟重新连接网络 ===\n"));

		// 通过主对话框重新初始化网络
		CWnd* 主窗口 = AfxGetMainWnd();
		if (主窗口)
		{
			NageDlqDlg* 主对话框 = dynamic_cast<NageDlqDlg*>(主窗口);
			if (主对话框)
			{
				// 重新初始化网络连接
				if (主对话框->初始化网络通信())
				{
					TRACE(_T("重新连接网络成功\n"));
					权限状态.SetWindowText(_T("状态：重新连接成功"));
				}
				else
				{
					TRACE(_T("重新连接网络失败\n"));
					权限状态.SetWindowText(_T("状态：重新连接失败"));
				}
			}
		}

		结束重新连接();
	}

	CDialogEx::OnTimer(nIDEvent);
}

// 初始化对话框
BOOL 登录页面类::OnInitDialog()		
{
	CDialogEx::OnInitDialog();

	// 检查游戏是否已运行
	检查游戏是否运行();
	更新启动按钮状态();

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

	// 构建登录请求
	CString 登录请求;
	登录请求.Format(_T("LOGIN:%s:%s"), 用户名, 密码);

	TRACE(_T("发送登录请求: %s\n"), 登录请求);

	/// 通过主对话框发送请求
	CWnd* 主窗口 = AfxGetMainWnd();
	if (主窗口)
	{
		NageDlqDlg* 主对话框 = dynamic_cast<NageDlqDlg*>(主窗口);
		if (主对话框 && 主对话框->发送请求到服务端(登录请求))
		{
			TRACE(_T("登录请求发送成功\n"));
			// 3秒后重新启用按钮，防止重复发送
			SetTimer(100, 3000, nullptr);
		}
		else
		{
			TRACE(_T("发送登录请求失败\n"));
			MessageBox(_T("发送登录请求失败"), _T("错误"), MB_ICONERROR);
			登录按钮.EnableWindow(TRUE);
		}
	}
	else
	{
		登录按钮.EnableWindow(TRUE);
	}
}

// 重新连接按钮点击事件处理
void 登录页面类::OnBnClickedButtonRelogin()
{
	TRACE(_T("=== 点击重新连接按钮 ===\n"));

	// 防止重复点击
	if (m_bIsReconnecting)
	{
		TRACE(_T("正在重新连接中，请稍候...\n"));
		return;
	}

	// 退出当前登录状态
	if (已登录)
	{
		退出登录状态();
	}

	// 开始重新连接流程
	开始重新连接();
}

// 执行重新连接方法
void 登录页面类::执行重新连接()
{
	TRACE(_T("=== 执行重新连接 ===\n"));

	OnBnClickedButtonRelogin();
}

// 开始重新连接方法
void 登录页面类::开始重新连接()
{
	m_bIsReconnecting = TRUE;

	// 禁用相关按钮防止重复操作
	CWnd* 重新连接按钮 = GetDlgItem(IDC_RE_LOGIN);
	if (重新连接按钮)
		重新连接按钮->EnableWindow(FALSE);

	// 更新状态显示
	CString 状态文本 = _T("状态：重新连接中...");
	权限状态.SetWindowText(状态文本);
	TRACE(_T("设置状态为重新连接中...\n"));

	// 通过主对话框重新初始化网络连接
	CWnd* 主窗口 = AfxGetMainWnd();
	if (主窗口)
	{
		NageDlqDlg* 主对话框 = dynamic_cast<NageDlqDlg*>(主窗口);
		if (主对话框)
		{
			// 关闭现有连接
			if (主对话框->网络通信.是否已连接())
			{
				TRACE(_T("关闭现有网络连接\n"));
				主对话框->网络通信.关闭连接();
			}

			// 延迟执行重新连接（避免阻塞UI）
			SetTimer(101, 500, nullptr);
		}
	}
	else
	{
		结束重新连接();
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

// 结束重新连接方法
void 登录页面类::结束重新连接()
{
	m_bIsReconnecting = FALSE;

	// 启用重新连接按钮
	CWnd* 重新连接按钮 = GetDlgItem(IDC_RE_LOGIN);
	if (重新连接按钮)
		重新连接按钮->EnableWindow(TRUE);

	TRACE(_T("重新连接流程结束\n"));
}

// 点击启动按钮
void 登录页面类::OnBnClickedButtonStart()		
{
	// 检查游戏是否已经运行
	if (检查游戏是否运行())
	{
		MessageBox(_T("游戏已经在运行中，请勿重复启动！"), _T("提示"), MB_ICONWARNING);
		return;
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

	// 更新按钮状态
	更新启动按钮状态();
}

// 检查游戏是否运行的方法
BOOL 登录页面类::检查游戏是否运行()
{
	m_bGameRunning = FALSE;
	m_dwGameProcessId = 0;
	m_hGameProcess = NULL;

	// 获取进程ID
	DWORD pid = 获取进程ID(L"nage.bin");

	if (pid > 0)
	{
		// 打开进程
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, pid);
		if (hProcess != NULL)
		{
			DWORD exitCode;
			if (GetExitCodeProcess(hProcess, &exitCode) && exitCode == STILL_ACTIVE)
			{
				m_bGameRunning = TRUE;
				m_dwGameProcessId = pid;
				m_hGameProcess = hProcess;
				TRACE(_T("检测到游戏进程正在运行，PID: %d\n"), pid);
			}
			else
			{
				CloseHandle(hProcess);
			}
		}
	}

	return m_bGameRunning;
}

// 关闭游戏进程的方法
BOOL 登录页面类::关闭游戏进程()
{
	if (!m_bGameRunning || m_dwGameProcessId == 0)
		return TRUE;

	TRACE(_T("开始关闭游戏进程，PID: %d\n"), m_dwGameProcessId);

	// 方法1: 优雅关闭 - 发送关闭消息
	HWND hGameWnd = NULL;
	do {
		hGameWnd = ::FindWindow(NULL, L"美丽世界");  // 游戏窗口标题
		if (hGameWnd)
		{
			::PostMessage(hGameWnd, WM_CLOSE, 0, 0);
			TRACE(_T("发送关闭消息给游戏窗口\n"));
			Sleep(1000);  // 等待1秒
		}
	} while (hGameWnd && 检查游戏是否运行());

	// 如果游戏还在运行，使用强制终止
	if (m_bGameRunning)
	{
		TRACE(_T("优雅关闭失败，强制终止进程\n"));
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, m_dwGameProcessId);
		if (hProcess)
		{
			BOOL bResult = TerminateProcess(hProcess, 0);
			CloseHandle(hProcess);

			if (bResult)
			{
				TRACE(_T("进程终止成功\n"));
				m_bGameRunning = FALSE;
				m_dwGameProcessId = 0;
				return TRUE;
			}
			else
			{
				TRACE(_T("进程终止失败\n"));
				return FALSE;
			}
		}
	}

	TRACE(_T("游戏进程已关闭\n"));
	m_bGameRunning = FALSE;
	m_dwGameProcessId = 0;
	return TRUE;
}

// 更新启动按钮状态的方法
void 登录页面类::更新启动按钮状态()
{
	if (检查游戏是否运行())
	{
		启动按钮.EnableWindow(FALSE);
		CString 提示文本;
		提示文本.Format(_T("游戏正在运行(PID:%d)"), m_dwGameProcessId);
		启动按钮.SetWindowText(提示文本);
	}
	else
	{
		启动按钮.EnableWindow(TRUE);
		启动按钮.SetWindowText(_T("启动游戏"));
	}
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
	}
	else if (响应数据.Find(_T("LOGIN_FAILED")) == 0)
	{
		CString 错误信息 = 响应数据.Mid(12);
		MessageBox(错误信息, _T("登录失败"), MB_ICONERROR);
		已登录 = false;

		// 重置登录按钮状态
		登录按钮.EnableWindow(TRUE);
		登录按钮.SetWindowText(_T("登录"));
	}
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
	// 再次检查游戏是否已经运行（防止重复点击）
	if (检查游戏是否运行())
	{
		MessageBox(_T("游戏已经在运行中，请勿重复启动！"), _T("提示"), MB_ICONWARNING);
		return;
	}

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

	// 创建进程后记录进程信息
	if (CreateProcess(NULL, 命令行.GetBuffer(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		// 记录进程信息
		m_dwGameProcessId = pi.dwProcessId;
		m_hGameProcess = pi.hProcess;
		m_bGameRunning = TRUE;

		// 关闭线程句柄
		CloseHandle(pi.hThread);
		// 等待进程完全启动
		Sleep(2000);
		// 更新按钮状态
		更新启动按钮状态();

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

			// 修改窗口大小
			// 地址：0x5AAB1E 和 0x5AAB23
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

// 等待并安装UI钩子
void 登录页面类::等待并安装UI钩子(const wchar_t* 监控进程名)
{
	// 这里写你希望HOOK的目标函数地址
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

			// 2. 先写入原始指令到跳板开头
			if (!WriteProcessMemory(目标进程句柄, 远程钩子区, 原始字节, 7, &写入字节数))
			{
				VirtualFreeEx(目标进程句柄, 远程钩子区, 0, MEM_RELEASE);
				CloseHandle(目标进程句柄);
				Sleep(1000);
				continue;
			}

			// 3. 接着写入钩子逻辑
			if (!WriteProcessMemory(目标进程句柄, 远程钩子区 + 7, 钩子机器码, 钩子代码长度, &写入字节数))
			{
				VirtualFreeEx(目标进程句柄, 远程钩子区, 0, MEM_RELEASE);
				CloseHandle(目标进程句柄);
				Sleep(1000);
				continue;
			}

			// 4. 最后补上跳回原函数的指令
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

			// 5. 安装钩子：在目标地址插入jmp，跳到你的钩子代码区
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
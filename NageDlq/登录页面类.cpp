#include "pch.h"
#include "framework.h"
#include "NageDlq.h"
#include "登录页面类.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 登录页面类 对话框

IMPLEMENT_DYNAMIC(登录页面类, CDialogEx)

登录页面类::登录页面类(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PAGE_LOGIN, pParent)
	, 已登录(false)
{
}

登录页面类::~登录页面类()
{
}

void 登录页面类::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_USERNAME, 用户名编辑框);
	DDX_Control(pDX, IDC_EDIT_PASSWORD, 密码编辑框);
	DDX_Control(pDX, IDC_BUTTON_LOGIN, 登录按钮);
	DDX_Control(pDX, IDC_CHECK_1280, 窗口1280);
	DDX_Control(pDX, IDC_BUTTON_START, 启动按钮);
	DDX_Control(pDX, IDC_STATIC_BG, 背景图片);
}

BEGIN_MESSAGE_MAP(登录页面类, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_LOGIN, &登录页面类::OnBnClickedButtonLogin)
	ON_BN_CLICKED(IDC_BUTTON_START, &登录页面类::OnBnClickedButtonStart)
END_MESSAGE_MAP()

// 登录页面类 消息处理程序

BOOL 登录页面类::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置控件字体
	CFont* 字体 = GetFont();
	if (字体 != nullptr)
	{
		用户名编辑框.SetFont(字体);
		密码编辑框.SetFont(字体);
		登录按钮.SetFont(字体);
		窗口1280.SetFont(字体);
		启动按钮.SetFont(字体);
	}

	// 设置密码编辑框为密码模式
	密码编辑框.SetPasswordChar('*');

	// 默认选中窗口1280复选框
	窗口1280.SetCheck(BST_CHECKED);

	// 加载背景图片
	// 假设图片资源ID为IDB_LOGIN_BG
	HBITMAP 背景位图 = (HBITMAP)LoadImage(AfxGetInstanceHandle(),
		MAKEINTRESOURCE(IDB_LOGIN_MAP),
		IMAGE_BITMAP, 818, 490, LR_DEFAULTCOLOR);

	if (背景位图 != NULL)
	{
		背景图片.SetBitmap(背景位图);
	}

	return TRUE;
}

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

	// TODO: 这里添加实际的登录验证逻辑
	// 用于转生、加点、排行榜等功能权限验证

	// 模拟登录成功
	MessageBox(_T("登录成功，功能权限已激活"), _T("提示"), MB_ICONINFORMATION);
	已登录 = true;
}

void 登录页面类::OnBnClickedButtonStart()
{
	// 启动游戏不需要登录验证
	// 检查窗口1280复选框是否选中
	bool 需要修改窗口大小 = (窗口1280.GetCheck() == BST_CHECKED);

	// 启动注入线程
	if (需要修改窗口大小)
	{
		// 创建线程注入窗口大小修改代码
		std::thread 窗口大小线程(&登录页面类::注入窗口大小修改代码, this);
		窗口大小线程.detach(); // 分离线程，让它在后台运行
	}

	// 启动游戏
	启动游戏进程();

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

// 注入窗口大小修改代码
void 登录页面类::注入窗口大小修改代码()
{
	// 等待并安装窗口大小钩子
	等待并安装窗口大小钩子(L"nage.bin");
}

// 注入游戏UI修改代码
void 登录页面类::注入游戏UI修改代码()
{
	// 等待并安装UI钩子
	等待并安装UI钩子(L"nage.bin");
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

// 等待并安装窗口大小钩子
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
			BYTE 窗口宽度代码[] = { 0x68, 0x00, 0x05, 0x00, 0x00 }; // push 1280
			BYTE 窗口高度代码[] = { 0x68, 0x00, 0x03, 0x00, 0x00 }; // push 768

			// 如果窗口1280复选框未选中，则使用默认大小
			if (窗口1280.GetCheck() != BST_CHECKED)
			{
				窗口宽度代码[1] = 0x11;
				窗口宽度代码[2] = 0x01; // push 273
				窗口高度代码[1] = 0x48;
				窗口高度代码[2] = 0x01; // push 328
			}

			// 写入内存
			SIZE_T 写入字节数;
			WriteProcessMemory(目标进程句柄, (LPVOID)0x5AAB1E, 窗口宽度代码, sizeof(窗口宽度代码), &写入字节数);
			WriteProcessMemory(目标进程句柄, (LPVOID)0x5AAB23, 窗口高度代码, sizeof(窗口高度代码), &写入字节数);

			CloseHandle(目标进程句柄);
			break;
		}
		Sleep(1000); // 每秒检测一次
	}
}

// 等待并安装UI钩子
void 登录页面类::等待并安装UI钩子(const wchar_t* 监控进程名)
{
	// 这里写你希望HOOK的目标函数地址
	void* 目标地址 = (void*)0x6ABC1A;

	// 钩子机器码
	BYTE 钩子机器码[] = {
		0x60, 0x9C, 0xB8, 0xC4, 0x04, 0x00, 0x00, 0xB9, 0x5C,0xAF, 0xEF, 0x02, 0x89, 0x01, 0xB9, 0x30, 0xB3, 0xEF
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

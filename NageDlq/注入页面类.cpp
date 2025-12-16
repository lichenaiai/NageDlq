// 注入页面类.cpp
#include "pch.h"
#include "framework.h"
#include "NageDlq.h"
#include "注入页面类.h"
#include "afxdialogex.h"

// 添加必要的Windows头文件
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>

// C++标准库
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 静态回调函数声明
static BOOL CALLBACK 枚举进程窗口回调(HWND hwnd, LPARAM lParam);
static BOOL CALLBACK FindMainWindowCallback(HWND hwnd, LPARAM lParam);

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
}

BEGIN_MESSAGE_MAP(注入页面类, CDialogEx)
	ON_BN_CLICKED(IDC_INJE_ATTKMOB, &注入页面类::点击自动打怪按钮)
	ON_WM_TIMER()
	ON_MESSAGE(WM_USER + 200, &注入页面类::游戏进程退出消息处理)
	ON_MESSAGE(WM_USER + 201, &注入页面类::自动打怪停止消息处理)
END_MESSAGE_MAP()

// 初始化对话框
BOOL 注入页面类::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置按钮初始文本
	自动打怪按钮.SetWindowText(_T("开始自动打怪"));

	return TRUE;
}

// 获取游戏进程ID函数
DWORD 注入页面类::获取游戏进程ID()
{
	return 游戏进程ID;  // 直接返回已获取的进程ID
}

// ============================================================================
// 新增：通过进程ID查找窗口的函数实现
// ============================================================================

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
		GetWindowText(游戏窗口句柄, 窗口标题, 255);
		TRACE(_T("通过FindMainWindow找到窗口: '%s' (0x%08X)\n"), 窗口标题, 游戏窗口句柄);
		return TRUE;
	}

	// 方法2：使用深度查找
	TRACE(_T("开始深度查找进程 %d 的窗口\n"), 游戏进程ID);
	游戏窗口句柄 = 深度查找进程窗口(游戏进程ID);

	if (游戏窗口句柄 != NULL && IsWindow(游戏窗口句柄))
	{
		TCHAR 窗口标题[256] = { 0 };
		GetWindowText(游戏窗口句柄, 窗口标题, 255);
		TRACE(_T("通过深度查找找到窗口: '%s' (0x%08X)\n"), 窗口标题, 游戏窗口句柄);
		return TRUE;
	}

	// 方法3：使用枚举窗口回调
	TRACE(_T("开始枚举窗口查找进程ID: %d\n"), 游戏进程ID);
	EnumWindows(枚举进程窗口回调, reinterpret_cast<LPARAM>(this));

	if (游戏窗口句柄 != NULL && IsWindow(游戏窗口句柄))
	{
		TCHAR 窗口标题[256] = { 0 };
		GetWindowText(游戏窗口句柄, 窗口标题, 255);
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

	return p注入页面->枚举窗口回调函数(hwnd);
}

// 实例回调函数
BOOL 注入页面类::枚举窗口回调函数(HWND hwnd)
{
	DWORD 窗口进程ID = 0;
	GetWindowThreadProcessId(hwnd, &窗口进程ID);

	if (窗口进程ID == 游戏进程ID)
	{
		// 检查窗口是否可见且不是子窗口
		if (IsWindowVisible(hwnd) && GetParent(hwnd) == NULL)
		{
			// 获取窗口标题用于调试
			TCHAR 窗口标题[256] = { 0 };
			GetWindowText(hwnd, 窗口标题, 255);

			TRACE(_T("找到游戏窗口: 句柄=0x%08X, 标题='%s', 进程ID=%d\n"),
				hwnd, 窗口标题, 窗口进程ID);

			游戏窗口句柄 = hwnd;
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

// 通用的查找进程主窗口函数（静态版本）
static BOOL CALLBACK FindMainWindowCallback(HWND hwnd, LPARAM lParam)
{
	struct WindowInfo
	{
		DWORD 进程ID;
		HWND 窗口句柄;
	};

	WindowInfo* pInfo = reinterpret_cast<WindowInfo*>(lParam);
	DWORD 进程ID = 0;

	GetWindowThreadProcessId(hwnd, &进程ID);
	if (进程ID == pInfo->进程ID)
	{
		// 检查窗口是否可见、没有父窗口、不是工具窗口
		if (IsWindowVisible(hwnd) &&
			GetParent(hwnd) == NULL &&
			(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) == 0)
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
	return 深度查找窗口递归(GetDesktopWindow(), 目标进程ID);
}

// 递归查找窗口的辅助函数
HWND 注入页面类::深度查找窗口递归(HWND 父窗口, DWORD 目标进程ID)
{
	HWND 子窗口 = GetWindow(父窗口, GW_CHILD);

	while (子窗口 != NULL)
	{
		DWORD 窗口进程ID = 0;
		GetWindowThreadProcessId(子窗口, &窗口进程ID);

		if (窗口进程ID == 目标进程ID && IsWindowVisible(子窗口))
		{
			return 子窗口;
		}

		// 递归查找子窗口的子窗口
		HWND 找到的窗口 = 深度查找窗口递归(子窗口, 目标进程ID);
		if (找到的窗口 != NULL)
		{
			return 找到的窗口;
		}

		子窗口 = GetWindow(子窗口, GW_HWNDNEXT);
	}

	return NULL;
}

// 获取进程的所有窗口
std::vector<HWND> 注入页面类::获取进程所有窗口(DWORD 进程ID)
{
	std::vector<HWND> 窗口列表;

	EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL
		{
			std::vector<HWND>* p列表 = reinterpret_cast<std::vector<HWND>*>(lParam);
			DWORD 窗口进程ID = 0;

			GetWindowThreadProcessId(hwnd, &窗口进程ID);
			注入页面类* pThis = reinterpret_cast<注入页面类*>(lParam);
			if (pThis != nullptr && 窗口进程ID == pThis->游戏进程ID)
			{
				p列表->push_back(hwnd);
			}
			return TRUE;
		}, reinterpret_cast<LPARAM>(this));

	return 窗口列表;
}

// ============================================================================
// 自动打怪按钮点击事件（修改版）
// ============================================================================
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
		GetWindowText(游戏窗口句柄, 窗口标题, 255);
		TRACE(_T("成功获取游戏窗口: '%s' (0x%08X)\n"), 窗口标题, 游戏窗口句柄);

		// 注入自动打怪功能
		if (!注入自动打怪功能())
		{
			MessageBox(_T("注入自动打怪功能失败！"), _T("错误"), MB_ICONERROR);
			return;
		}

		// 启动自动打怪线程
		启动自动打怪线程();

		// 更新按钮文本
		自动打怪按钮.SetWindowText(_T("停止打怪"));

		// 记录开始时间
		自动打怪开始时间 = GetTickCount();
		总攻击次数 = 0;

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

// ============================================================================
// 原有函数保持不变，只修改了相关的窗口查找调用
// ============================================================================

// 定时器处理函数
void 注入页面类::OnTimer(UINT_PTR nIDEvent)
{
	// 这里可以添加定时检查游戏进程的功能
	CDialogEx::OnTimer(nIDEvent);
}

// 检查游戏进程（优化版）
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
	游戏进程句柄 = OpenProcess(PROCESS_ALL_ACCESS, FALSE, 游戏进程ID);
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

	// 第一步：检查内存地址是否可读写
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

	// 第二步：尝试写入测试值
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

	// 第三步：检查游戏进程的模块信息
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

	// 显示成功消息
	CString 成功信息;
	成功信息.Format(_T("自动打怪功能已注入游戏进程 (进程ID: %d)\n\n")
		_T("攻击标志地址: 0x%08X\n")
		_T("目标怪物地址: 0x%08X\n")
		_T("怪物ID地址1: 0x%08X\n")
		_T("怪物ID地址2: 0x%08X\n")
		_T("怪物ID地址3: 0x%08X\n"),
		游戏进程ID,
		攻击标志地址,
		目标怪物地址,
		怪物ID地址1,
		怪物ID地址2,
		怪物ID地址3);

	MessageBox(成功信息, _T("注入成功"), MB_ICONINFORMATION);

	return TRUE;
}

// ============================================================================
// 激活并聚焦游戏窗口（修改版）
// ============================================================================
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

	// 恢复窗口（如果最小化）
	if (::IsIconic(游戏窗口句柄))
	{
		::ShowWindow(游戏窗口句柄, SW_RESTORE);
		Sleep(100);
	}

	// 激活窗口到前台
	::SetForegroundWindow(游戏窗口句柄);
	::BringWindowToTop(游戏窗口句柄);
	::SetActiveWindow(游戏窗口句柄);
	::SetFocus(游戏窗口句柄);

	// 短暂延迟确保窗口激活完成
	Sleep(200);

	// 验证窗口是否在前台
	if (::GetForegroundWindow() == 游戏窗口句柄)
	{
		TRACE(_T("游戏窗口已成功激活\n"));
		return TRUE;
	}
	else
	{
		TRACE(_T("警告：游戏窗口未能激活到前台\n"));
		return FALSE;
	}
}

// ============================================================================
// 后台模拟鼠标移动（修改版）
// ============================================================================
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

// ============================================================================
// 以下函数保持不变
// ============================================================================

// 启动自动打怪线程
void 注入页面类::启动自动打怪线程()
{
	if (自动打怪运行中)
	{
		TRACE(_T("自动打怪线程已经在运行\n"));
		return;
	}

	自动打怪运行中 = true;
	当前怪物地址索引 = 0;

	// 创建自动打怪线程
	自动打怪线程 = std::thread(&注入页面类::自动打怪线程函数, this);
	自动打怪线程.detach();  // 分离线程

	TRACE(_T("自动打怪线程已启动\n"));
}

// 停止自动打怪
void 注入页面类::停止自动打怪()
{
	if (!自动打怪运行中)
	{
		return;
	}

	自动打怪运行中 = false;

	// 等待线程结束
	if (自动打怪线程.joinable())
	{
		自动打怪线程.join();
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

	TRACE(_T("自动打怪已停止\n"));
}

// 自动打怪线程函数
void 注入页面类::自动打怪线程函数()
{
	TRACE(_T("=== 自动打怪线程开始 ===\n"));

	自动打怪开始时间 = GetTickCount();
	总攻击次数 = 0;

	// 线程开始时激活窗口
	激活并聚焦游戏窗口();

	while (自动打怪运行中)
	{
		try
		{
			// 每秒检查一次进程状态
			if (GetTickCount() % 1000 == 0)
			{
				DWORD 退出代码;
				if (!GetExitCodeProcess(游戏进程句柄, &退出代码) || 退出代码 != STILL_ACTIVE)
				{
					TRACE(_T("游戏进程已退出，停止自动打怪\n"));

					// 在主线程中更新UI
					::PostMessage(GetSafeHwnd(), WM_USER + 200, 0, 0);

					自动打怪运行中 = false;
					break;
				}
			}

			// 执行智能攻击（包含窗口激活、鼠标模拟、60秒超时、0.5秒等待）
			执行智能攻击();

			// 短暂延迟，避免CPU占用过高
			Sleep(100);
		}
		catch (...)
		{
			TRACE(_T("自动打怪线程发生异常\n"));
			Sleep(1000);
		}
	}

	// 显示最终统计信息
	DWORD 运行时间 = (GetTickCount() - 自动打怪开始时间) / 1000;
	TRACE(_T("=== 自动打怪线程结束 ===\n"));
	TRACE(_T("总攻击次数: %d\n"), 总攻击次数);
	TRACE(_T("运行时间: %d 秒\n"), 运行时间);

	if (运行时间 > 0)
	{
		CString 效率信息;
		效率信息.Format(_T("平均每分钟攻击: %.1f 次"),
			(float)总攻击次数 / 运行时间 * 60.0f);
		TRACE(_T("%s\n"), 效率信息);
	}

	// 通知主线程自动打怪已停止
	::PostMessage(GetSafeHwnd(), WM_USER + 201, 0, 0);
}

// 获取有效目标怪物（改进版）
DWORD 注入页面类::获取有效目标怪物()
{
	if (游戏进程句柄 == NULL)
	{
		return 0xFFFFFFFF;
	}

	// 尝试多次读取
	for (int 尝试次数 = 0; 尝试次数 < 10; 尝试次数++)
	{
		// 随机选择一个怪物地址开始，避免总是从同一个开始
		int 起始索引 = GetTickCount() % 3;

		for (int i = 0; i < 3; i++)
		{
			int 当前索引 = (起始索引 + i) % 3;
			DWORD 怪物地址 = 0;

			switch (当前索引)
			{
			case 0: 怪物地址 = 怪物ID地址1; break;
			case 1: 怪物地址 = 怪物ID地址2; break;
			case 2: 怪物地址 = 怪物ID地址3; break;
			}

			DWORD 怪物ID = 0xFFFFFFFF;
			SIZE_T 读取字节数;

			if (ReadProcessMemory(游戏进程句柄, (LPCVOID)怪物地址,
				&怪物ID, sizeof(DWORD), &读取字节数))
			{
				// 只检查是否为有效ID（不是特殊值）
				if (怪物ID != 0xFFFFFFFF && 怪物ID != 0)
				{
					TRACE(_T("从地址%d找到怪物: 0x%08X\n"), 当前索引, 怪物ID);
					return 怪物ID;
				}
			}
		}

		// 短暂等待后重试
		if (尝试次数 < 9) // 最后一次不等待
		{
			Sleep(100);
		}
	}

	return 0xFFFFFFFF;
}

// 执行智能攻击（60秒超时，0.5秒等待）
void 注入页面类::执行智能攻击()
{
	// 1. 确保游戏窗口激活
	激活并聚焦游戏窗口();

	// 2. 防止挂机检测（后台模拟，不影响用户）
	if (GetTickCount() % 30000 == 0) // 每30秒执行一次
	{
		后台模拟鼠标移动();
	}

	// 3. 检查当前是否有锁定目标
	DWORD 当前目标 = 0xFFFFFFFF;
	SIZE_T 读取字节数;

	if (!ReadProcessMemory(游戏进程句柄, (LPCVOID)目标怪物地址,
		&当前目标, sizeof(DWORD), &读取字节数))
	{
		TRACE(_T("读取目标怪物地址失败\n"));
		Sleep(500);
		return;
	}

	// 4. 如果没有锁定目标，寻找新怪物
	if (当前目标 == 0xFFFFFFFF)
	{
		当前目标 = 获取有效目标怪物();
		if (当前目标 == 0xFFFFFFFF)
		{
			TRACE(_T("没有找到有效怪物，等待...\n"));
			Sleep(500);
			return;
		}

		// 写入新的目标怪物
		if (!WriteProcessMemory(游戏进程句柄, (LPVOID)目标怪物地址,
			&当前目标, sizeof(DWORD), &读取字节数))
		{
			TRACE(_T("写入新目标怪物失败\n"));
			return;
		}

		// 等待游戏反应
		Sleep(50);
	}

	TRACE(_T("开始攻击怪物ID: 0x%08X\n"), 当前目标);

	// 5. 设置攻击标志为1（开始攻击）
	DWORD 攻击标志 = 1;
	WriteProcessMemory(游戏进程句柄, (LPVOID)攻击标志地址,
		&攻击标志, sizeof(DWORD), &读取字节数);

	// 6. 监控攻击状态（最长60秒）
	DWORD 攻击开始时间 = GetTickCount();
	BOOL 怪物死亡 = FALSE;

	while (自动打怪运行中 && (GetTickCount() - 攻击开始时间 < 60000)) // +++ 修改：60秒超时 +++
	{
		// 读取当前目标状态
		DWORD 目标状态 = 0xFFFFFFFF;
		ReadProcessMemory(游戏进程句柄, (LPCVOID)目标怪物地址,
			&目标状态, sizeof(DWORD), &读取字节数);

		// 检查怪物是否死亡（变成FFFFFFFF）
		if (目标状态 == 0xFFFFFFFF)
		{
			TRACE(_T("怪物已死亡！\n"));
			怪物死亡 = TRUE;
			break;
		}

		// 定期保持攻击标志（防止被其他操作清除）
		if (GetTickCount() % 1000 == 0) // 每秒刷新一次攻击标志
		{
			WriteProcessMemory(游戏进程句柄, (LPVOID)攻击标志地址,
				&攻击标志, sizeof(DWORD), NULL);
		}

		// 每5秒显示一次状态
		if (GetTickCount() % 5000 == 0)
		{
			TRACE(_T("正在攻击怪物ID: 0x%08X，已攻击%.1f秒\n"),
				目标状态, (GetTickCount() - 攻击开始时间) / 1000.0f);
		}

		Sleep(100); // 避免CPU占用过高
	}

	// 7. 处理攻击结果
	if (怪物死亡)
	{
		TRACE(_T("攻击完成，怪物死亡\n"));
	}
	else
	{
		TRACE(_T("攻击超时（60秒），放弃当前目标\n"));
		// 攻击标志由游戏自动清除
	}

	// 8. 等待0.5秒（模拟技能冷却） 
	Sleep(500);

	// 9. 记录攻击次数
	总攻击次数++;

	// 每10次显示统计
	if (总攻击次数 % 10 == 0)
	{
		CString 统计信息;
		DWORD 运行秒数 = (GetTickCount() - 自动打怪开始时间) / 1000;
		统计信息.Format(_T("已攻击 %d 次，运行 %d 秒"),
			总攻击次数, 运行秒数);
		TRACE(_T("%s\n"), 统计信息);
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
	if (自动打怪运行中 == false)
	{
		自动打怪按钮.SetWindowText(_T("开始自动打怪"));
	}

	return 0;
}
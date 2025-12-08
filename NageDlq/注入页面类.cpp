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

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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

// 自动打怪按钮点击事件
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

// 定时器处理函数
void 注入页面类::OnTimer(UINT_PTR nIDEvent)
{
	// 这里可以添加定时检查游戏进程的功能
	CDialogEx::OnTimer(nIDEvent);
}

// 检查游戏进程
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
			if (_wcsicmp(进程信息.szExeFile, L"nage.bin") == 0)
			{
				游戏进程ID = 进程信息.th32ProcessID;
				找到进程 = TRUE;
				break;
			}
		} while (Process32Next(进程快照句柄, &进程信息));
	}

	CloseHandle(进程快照句柄);

	if (!找到进程)
	{
		TRACE(_T("未找到游戏进程 nage.bin\n"));
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

	TRACE(_T("找到游戏进程，进程ID: %d\n"), 游戏进程ID);
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

			// 获取下一个怪物ID
			DWORD 怪物ID = 获取下一个怪物ID();

			// 检查怪物ID是否有效（不是0xFFFFFFFF或0）
			if (怪物ID == 0xFFFFFFFF || 怪物ID == 0)
			{
				// 无效怪物ID，等待后继续
				Sleep(300);
				continue;
			}

			TRACE(_T("攻击怪物ID: 0x%08X\n"), 怪物ID);

			// 第一步：写入目标怪物ID
			SIZE_T 写入字节数;
			if (!WriteProcessMemory(游戏进程句柄, (LPVOID)目标怪物地址,
				&怪物ID, sizeof(DWORD), &写入字节数))
			{
				DWORD 错误代码 = GetLastError();
				TRACE(_T("写入怪物ID失败，错误代码: %d\n"), 错误代码);

				if (错误代码 == ERROR_INVALID_HANDLE)
				{
					// 进程句柄无效，停止自动打怪
					自动打怪运行中 = false;
					break;
				}

				Sleep(100);
				continue;
			}

			// 短暂延迟确保怪物ID已写入
			Sleep(50);

			// 第二步：设置攻击标志为1
			DWORD 攻击标志 = 1;
			if (!WriteProcessMemory(游戏进程句柄, (LPVOID)攻击标志地址,
				&攻击标志, sizeof(DWORD), &写入字节数))
			{
				TRACE(_T("设置攻击标志失败\n"));
				Sleep(100);
				continue;
			}

			// 攻击计数
			总攻击次数++;

			// 每10次攻击显示一次统计
			if (总攻击次数 % 10 == 0)
			{
				CString 信息;
				信息.Format(_T("已自动攻击 %d 次，当前使用地址索引: %d"),
					总攻击次数, 当前怪物地址索引);
				TRACE(_T("%s\n"), 信息);
			}

			// 第三步：等待攻击完成（根据游戏节奏调整）
			// 这里可以根据需要调整等待时间
			DWORD 攻击开始时间 = GetTickCount();
			BOOL 怪物死亡 = FALSE;

			while (自动打怪运行中 && (GetTickCount() - 攻击开始时间 < 5000))  // 最多等待5秒
			{
				// 每隔500毫秒检查一次怪物是否死亡
				if (GetTickCount() % 500 == 0)
				{
					DWORD 当前目标怪物 = 0;
					SIZE_T 读取字节数;

					if (ReadProcessMemory(游戏进程句柄, (LPCVOID)目标怪物地址,
						&当前目标怪物, sizeof(DWORD), &读取字节数))
					{
						if (当前目标怪物 == 0xFFFFFFFF)
						{
							// 怪物已死亡！
							TRACE(_T("怪物已死亡，立即寻找下一个\n"));
							怪物死亡 = TRUE;
							break;
						}
					}
				}

				// 保持攻击标志为1（持续攻击）
				WriteProcessMemory(游戏进程句柄, (LPVOID)攻击标志地址,
					&攻击标志, sizeof(DWORD), NULL);

				Sleep(100);
			}

			// 第四步：清除攻击标志
			DWORD 停止攻击 = 0;
			WriteProcessMemory(游戏进程句柄, (LPVOID)攻击标志地址,
				&停止攻击, sizeof(DWORD), NULL);

			// 第五步：根据怪物死亡情况决定等待时间
			if (怪物死亡)
			{
				// 怪物已死亡，短暂延迟后立即寻找下一个
				Sleep(200);
			}
			else
			{
				// 怪物未死亡（可能是打不动或者miss了），等待稍长时间
				TRACE(_T("攻击超时，可能怪物未死亡\n"));
				Sleep(1000);
			}

		}
		catch (...)
		{
			TRACE(_T("自动打怪线程发生异常\n"));
			Sleep(1000);
		}
	}

	// 线程结束时确保攻击标志为0
	if (游戏进程句柄 != NULL)
	{
		DWORD 停止攻击 = 0;
		WriteProcessMemory(游戏进程句柄, (LPVOID)攻击标志地址,
			&停止攻击, sizeof(DWORD), NULL);
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

// 获取下一个怪物ID - 简单轮流使用三个地址
DWORD 注入页面类::获取下一个怪物ID()
{
	if (游戏进程句柄 == NULL)
	{
		return 0xFFFFFFFF;
	}

	DWORD 怪物ID = 0xFFFFFFFF;
	SIZE_T 读取字节数;

	// 根据当前索引选择要读取的地址
	DWORD 怪物地址 = 0;
	switch (当前怪物地址索引)
	{
	case 0:
		怪物地址 = 怪物ID地址1;
		break;
	case 1:
		怪物地址 = 怪物ID地址2;
		break;
	case 2:
		怪物地址 = 怪物ID地址3;
		break;
	default:
		怪物地址 = 怪物ID地址1;
		当前怪物地址索引 = 0;
		break;
	}

	// 读取怪物ID
	if (!ReadProcessMemory(游戏进程句柄, (LPCVOID)怪物地址,
		&怪物ID, sizeof(DWORD), &读取字节数))
	{
		TRACE(_T("读取怪物ID地址 %d (0x%08X) 失败\n"), 当前怪物地址索引, 怪物地址);
		return 0xFFFFFFFF;
	}

	TRACE(_T("从地址 %d (0x%08X) 读取到怪物ID: 0x%08X\n"),
		当前怪物地址索引, 怪物地址, 怪物ID);

	// 轮询到下一个地址（无论是否读取成功）
	当前怪物地址索引 = (当前怪物地址索引 + 1) % 3;

	return 怪物ID;
}

// 更新当前怪物ID
void 注入页面类::更新当前怪物ID(DWORD 怪物ID)
{
	if (游戏进程句柄 == NULL)
	{
		return;
	}

	// 写入目标怪物ID
	if (!WriteProcessMemory(游戏进程句柄, (LPVOID)目标怪物地址,
		&怪物ID, sizeof(DWORD), NULL))
	{
		TRACE(_T("更新怪物ID失败\n"));
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
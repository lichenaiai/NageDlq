#include "pch.h"
#include "framework.h"
#include "NageTeleporter.h"
#include "NageTeleporterDlg.h"
#include "afxdialogex.h"
#include <windows.h>
#include <tlhelp32.h>
#include <stdexcept>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 地图名称对照表
const 地图信息 NageTeleporterDlg::地图列表[] = {
	{1,  _T("波本")},
	{2,  _T("荒废")},
	{3,  _T("自由")},
	{4,  _T("大熊")},
	{5,  _T("遗忘1")},
	{6,  _T("遗忘2")},
	{7,  _T("监狱")},
	{8,  _T("乐透")},
	{9,  _T("遗忘森林")},
	{10, _T("水晶1")},
	{11, _T("水晶2")},
	{12, _T("罪恶1")},
	{13, _T("罪恶2")},
	{14, _T("罪恶3")},
	{15, _T("大漠")},
	{16, _T("要塞1")},
	{17, _T("要塞2")},
	{18, _T("PVP地图")},
	{19, _T("要塞3")},
	{20, _T("终结者屋")},
	{21, _T("研究所1")},
	{22, _T("研究所2")},
	{23, _T("研究所3")},
	{24, _T("迷雾1")},
	{25, _T("迷雾2")},
	{26, _T("迷雾3")},
	{27, _T("迷雾4")},
	{28, _T("迷雾5")},
	{29, _T("中央市")},
	{30, _T("中央公园")},
	{31, _T("中央广场")},
	{32, _T("中央营地A")},
	{33, _T("中央营地B")},
	{34, _T("中央营地C")},
	{35, _T("中央管制塔")},
	{36, _T("加勒比海")},
};

const int NageTeleporterDlg::地图数量 = sizeof(地图列表) / sizeof(地图列表[0]);

IMPLEMENT_DYNAMIC(NageTeleporterDlg, CDialogEx)

NageTeleporterDlg::NageTeleporterDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TELEPORTER_DIALOG, pParent)
	, 游戏进程句柄(NULL)
	, 游戏进程ID(0)
	, 进程已附加(FALSE)
	, 已保存计数(0)
{
	// 初始化保存坐标数组
	for (int i = 0; i < 最大保存数量; i++)
	{
		已保存坐标[i].有效 = FALSE;
		已保存坐标[i].地图编号 = 0;
		已保存坐标[i].X = 0.0f;
		已保存坐标[i].Y = 0.0f;
	}
}

NageTeleporterDlg::~NageTeleporterDlg()
{
	if (游戏进程句柄 != NULL)
	{
		CloseHandle(游戏进程句柄);
		游戏进程句柄 = NULL;
	}
}

void NageTeleporterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROCESS_COMBO, 进程选择组合框);
	DDX_Control(pDX, IDC_BTN_REFRESH_PROC, 刷新进程按钮);
	DDX_Control(pDX, IDC_BTN_ATTACH_PROC, 附加进程按钮);
	DDX_Control(pDX, IDC_BTN_READ_CURRENT, 读取当前坐标按钮);
	DDX_Control(pDX, IDC_EDIT_MAP_ID, 地图编号编辑框);
	DDX_Control(pDX, IDC_EDIT_COORD_X, X坐标编辑框);
	DDX_Control(pDX, IDC_EDIT_COORD_Y, Y坐标编辑框);
	DDX_Control(pDX, IDC_STATIC_MAP_NAME, 地图名称标签);
	DDX_Control(pDX, IDC_STATIC_CURRENT_INFO, 当前信息标签);
	DDX_Control(pDX, IDC_STATIC_PROC_STATUS, 进程状态标签);
	DDX_Control(pDX, IDC_BTN_TELEPORT, 传送按钮);
	DDX_Control(pDX, IDC_EDIT_LOG, 日志编辑框);
	DDX_Control(pDX, IDC_EDIT_SAVE_LABEL, 保存标签编辑框);
	DDX_Control(pDX, IDC_BTN_SAVE_COORD, 保存坐标按钮);
	DDX_Control(pDX, IDC_LIST_SAVED, 已保存列表);
	DDX_Control(pDX, IDC_BTN_USE_SAVED, 使用已保存按钮);
	DDX_Control(pDX, IDC_BTN_DELETE_SAVED, 删除已保存按钮);
}

BEGIN_MESSAGE_MAP(NageTeleporterDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_REFRESH_PROC, &NageTeleporterDlg::点击刷新进程)
	ON_BN_CLICKED(IDC_BTN_ATTACH_PROC, &NageTeleporterDlg::点击附加进程)
	ON_BN_CLICKED(IDC_BTN_READ_CURRENT, &NageTeleporterDlg::点击读取当前坐标)
	ON_BN_CLICKED(IDC_BTN_TELEPORT, &NageTeleporterDlg::点击传送)
	ON_BN_CLICKED(IDC_BTN_SAVE_COORD, &NageTeleporterDlg::点击保存坐标)
	ON_BN_CLICKED(IDC_BTN_USE_SAVED, &NageTeleporterDlg::点击使用已保存)
	ON_BN_CLICKED(IDC_BTN_DELETE_SAVED, &NageTeleporterDlg::点击删除已保存)
	ON_LBN_SELCHANGE(IDC_LIST_SAVED, &NageTeleporterDlg::已保存列表选中改变)
	ON_LBN_DBLCLK(IDC_LIST_SAVED, &NageTeleporterDlg::已保存列表双击)
	ON_EN_CHANGE(IDC_EDIT_MAP_ID, &NageTeleporterDlg::地图编号改变)
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL NageTeleporterDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置窗口标题
	SetWindowText(_T("NAGE传送器"));

	// 设置控件初始状态
	附加进程按钮.EnableWindow(FALSE);
	读取当前坐标按钮.EnableWindow(FALSE);
	传送按钮.EnableWindow(FALSE);

	// 设置中文界面文本（所有静态文本和分组框）
	SetDlgItemText(IDC_GROUP_PROC, _T("进程管理"));
	SetDlgItemText(IDC_LBL_PROC, _T("游戏进程:"));
	SetDlgItemText(IDC_STATIC_PROC_STATUS, _T("状态: 未连接"));
	SetDlgItemText(IDC_GROUP_CURR, _T("当前坐标（读取游戏内存）"));
	SetDlgItemText(IDC_STATIC_CURRENT_INFO, _T("当前信息: 尚未读取"));
	SetDlgItemText(IDC_GROUP_TARGET, _T("目标坐标设置"));
	SetDlgItemText(IDC_LBL_MAP_ID, _T("地图编号:"));
	SetDlgItemText(IDC_LBL_MAP_NAME, _T("地图名称:"));
	SetDlgItemText(IDC_LBL_X, _T("X 坐标:"));
	SetDlgItemText(IDC_LBL_Y, _T("Y 坐标:"));
	SetDlgItemText(IDC_LBL_TIP, _T("提示: 地图编号范围 1-36"));
	SetDlgItemText(IDC_GROUP_SAVED, _T("保存坐标"));
	SetDlgItemText(IDC_LBL_SAVE_LABEL, _T("标签:"));
	SetDlgItemText(IDC_LBL_SAVED_LIST, _T("已保存:"));
	SetDlgItemText(IDC_GROUP_LOG, _T("传送日志"));
	SetDlgItemText(IDC_BTN_REFRESH_PROC, _T("刷新"));
	SetDlgItemText(IDC_BTN_ATTACH_PROC, _T("附加进程"));
	SetDlgItemText(IDC_BTN_READ_CURRENT, _T("读取当前坐标"));
	SetDlgItemText(IDC_BTN_TELEPORT, _T("开始传送"));
	SetDlgItemText(IDC_BTN_SAVE_COORD, _T("保存"));
	SetDlgItemText(IDC_BTN_USE_SAVED, _T("使用"));
	SetDlgItemText(IDC_BTN_DELETE_SAVED, _T("删除"));

	// 初始化地图名称显示
	地图名称标签.SetWindowText(_T("未知地图"));

	// 设置日志编辑框为只读
	日志编辑框.SetReadOnly(TRUE);

	// 初始化保存坐标区域控件状态
	保存坐标按钮.EnableWindow(FALSE);
	使用已保存按钮.EnableWindow(FALSE);
	删除已保存按钮.EnableWindow(FALSE);

	// 从文件加载已保存的坐标
	加载坐标从文件();
	刷新已保存列表();
	if (已保存计数 > 0)
	{
		添加日志(_T("已加载上次保存的坐标数据"));
	}
	else
	{
		CString 路径提示;
		路径提示.Format(_T("数据文件: %s (保存坐标后自动创建)"), 获取数据文件路径());
		添加日志(路径提示);
	}

	// 自动扫描游戏进程
	枚举游戏进程();

	// 设置定时器，每3秒刷新进程列表
	SetTimer(1, 3000, NULL);

	// 添加启动日志
	添加日志(_T("记忆传送器已启动"));
	添加日志(_T("请选择游戏进程后点击\"附加进程\""));

	return TRUE;
}

void NageTeleporterDlg::OnDestroy()
{
	if (游戏进程句柄 != NULL)
	{
		CloseHandle(游戏进程句柄);
		游戏进程句柄 = NULL;
	}
	KillTimer(1);
	CDialogEx::OnDestroy();
}

void NageTeleporterDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1 && !进程已附加)
	{
		枚举游戏进程();
	}
	CDialogEx::OnTimer(nIDEvent);
}

// ==================== 进程管理 ====================

void NageTeleporterDlg::枚举游戏进程()
{
	// 保存当前选中项
	CString 当前选中;
	int 当前索引 = 进程选择组合框.GetCurSel();
	if (当前索引 != CB_ERR)
	{
		进程选择组合框.GetLBText(当前索引, 当前选中);
	}

	// 清空组合框
	进程选择组合框.ResetContent();

	// 可能的游戏进程名列表
	const TCHAR* 进程名列表[] = {
		_T("nage.bin"),
		_T("Nage.bin"),
		_T("nage.exe"),
		_T("Nage.exe"),
		_T("NageClient.exe"),
		_T("Nage Client.exe"),
		NULL
	};

	// 创建进程快照
	HANDLE 进程快照句柄 = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (进程快照句柄 == INVALID_HANDLE_VALUE)
	{
		return;
	}

	PROCESSENTRY32 进程信息;
	进程信息.dwSize = sizeof(PROCESSENTRY32);

	int 找到计数 = 0;

	if (Process32First(进程快照句柄, &进程信息))
	{
		do
		{
			for (int i = 0; 进程名列表[i] != NULL; i++)
			{
				if (_wcsicmp(进程信息.szExeFile, 进程名列表[i]) == 0)
				{
					CString 显示文本;
					显示文本.Format(_T("%s (PID: %d)"), 进程信息.szExeFile, 进程信息.th32ProcessID);
					int 添加索引 = 进程选择组合框.AddString(显示文本);
					进程选择组合框.SetItemData(添加索引, 进程信息.th32ProcessID);
					找到计数++;
					break;
				}
			}
		} while (Process32Next(进程快照句柄, &进程信息));
	}

	CloseHandle(进程快照句柄);

	// 更新进程状态显示
	CString 状态文本;
	if (进程已附加)
	{
		状态文本.Format(_T("已附加 - PID: %d"), 游戏进程ID);
	}
	else if (找到计数 > 0)
	{
		状态文本.Format(_T("找到 %d 个游戏进程，请选择一个后附加"), 找到计数);
	}
	else
	{
		状态文本 = _T("未找到游戏进程");
	}
	进程状态标签.SetWindowText(状态文本);

	// 尝试恢复之前选中的项
	if (!当前选中.IsEmpty())
	{
		for (int i = 0; i < 进程选择组合框.GetCount(); i++)
		{
			CString 文本;
			进程选择组合框.GetLBText(i, 文本);
			if (文本 == 当前选中)
			{
				进程选择组合框.SetCurSel(i);
				break;
			}
		}
	}
	else if (进程选择组合框.GetCount() > 0)
	{
		进程选择组合框.SetCurSel(0);
	}

	// 如果有进程且未附加，启用附加按钮
	附加进程按钮.EnableWindow(进程选择组合框.GetCount() > 0 && !进程已附加);
}

void NageTeleporterDlg::点击刷新进程()
{
	枚举游戏进程();
}

void NageTeleporterDlg::点击附加进程()
{
	int 当前索引 = 进程选择组合框.GetCurSel();
	if (当前索引 == CB_ERR)
	{
		MessageBox(_T("请先选择一个游戏进程！"), _T("提示"), MB_ICONWARNING);
		return;
	}

	DWORD 选中进程ID = (DWORD)进程选择组合框.GetItemData(当前索引);
	if (选中进程ID == 0)
	{
		MessageBox(_T("无效的进程ID！"), _T("错误"), MB_ICONERROR);
		return;
	}

	// 打开进程
	HANDLE 新句柄 = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
		FALSE, 选中进程ID);
	if (新句柄 == NULL)
	{
		CString 错误信息;
		错误信息.Format(_T("无法打开进程 (PID: %d)\n请以管理员身份运行此程序！"), 选中进程ID);
		MessageBox(错误信息, _T("错误"), MB_ICONERROR);
		return;
	}

	// 关闭旧句柄，保存新句柄
	if (游戏进程句柄 != NULL)
	{
		CloseHandle(游戏进程句柄);
	}
	游戏进程句柄 = 新句柄;
	游戏进程ID = 选中进程ID;
	进程已附加 = TRUE;

	// 更新界面状态
	附加进程按钮.EnableWindow(FALSE);
	读取当前坐标按钮.EnableWindow(TRUE);
	传送按钮.EnableWindow(TRUE);
	保存坐标按钮.EnableWindow(TRUE);

	CString 状态文本;
	状态文本.Format(_T("已附加 - PID: %d"), 游戏进程ID);
	进程状态标签.SetWindowText(状态文本);

	添加日志(_T("成功附加到游戏进程"));
	添加日志(_T("现在可以读取当前坐标或直接输入目标坐标进行传送"));
}

// ==================== 坐标操作 ====================

BOOL NageTeleporterDlg::检查游戏进程()
{
	if (游戏进程句柄 == NULL || 游戏进程ID == 0)
	{
		return FALSE;
	}

	DWORD 退出代码;
	if (!GetExitCodeProcess(游戏进程句柄, &退出代码) || 退出代码 != STILL_ACTIVE)
	{
		添加日志(_T("游戏进程已退出"));
		进程已附加 = FALSE;
		游戏进程ID = 0;
		if (游戏进程句柄 != NULL)
		{
			CloseHandle(游戏进程句柄);
			游戏进程句柄 = NULL;
		}
		附加进程按钮.EnableWindow(进程选择组合框.GetCount() > 0);
		读取当前坐标按钮.EnableWindow(FALSE);
		传送按钮.EnableWindow(FALSE);
		进程状态标签.SetWindowText(_T("进程已退出"));
		return FALSE;
	}

	return TRUE;
}

void NageTeleporterDlg::读取当前坐标(float& X, float& Y, DWORD& 地图编号)
{
	SIZE_T 读取字节数;

	if (!ReadProcessMemory(游戏进程句柄, (LPCVOID)坐标X地址, &X, sizeof(float), &读取字节数))
	{
		throw std::runtime_error("读取X坐标失败");
	}

	if (!ReadProcessMemory(游戏进程句柄, (LPCVOID)坐标Y地址, &Y, sizeof(float), &读取字节数))
	{
		throw std::runtime_error("读取Y坐标失败");
	}

	if (!ReadProcessMemory(游戏进程句柄, (LPCVOID)地图编号地址, &地图编号, sizeof(DWORD), &读取字节数))
	{
		throw std::runtime_error("读取地图编号失败");
	}
}

void NageTeleporterDlg::点击读取当前坐标()
{
	if (!检查游戏进程())
	{
		MessageBox(_T("游戏进程无效，请重新附加进程！"), _T("错误"), MB_ICONERROR);
		return;
	}

	try
	{
		float 当前X = 0, 当前Y = 0;
		DWORD 当前地图编号 = 0;

		读取当前坐标(当前X, 当前Y, 当前地图编号);

		int 显示地图编号 = (int)当前地图编号 + 1;
		CString 地图名称 = 获取地图名称(显示地图编号);

		// 更新输入框
		CString X文本, Y文本, 地图文本;
		地图文本.Format(_T("%d"), 显示地图编号);
		X文本.Format(_T("%.2f"), 当前X);
		Y文本.Format(_T("%.2f"), 当前Y);

		地图编号编辑框.SetWindowText(地图文本);
		X坐标编辑框.SetWindowText(X文本);
		Y坐标编辑框.SetWindowText(Y文本);

		// 更新显示
		CString 信息文本;
		信息文本.Format(_T("当前位置 - 地图: %s(%d)  X: %.2f  Y: %.2f"),
			地图名称, 显示地图编号, 当前X, 当前Y);
		当前信息标签.SetWindowText(信息文本);
		地图名称标签.SetWindowText(地图名称);

		添加日志(_T("已读取当前坐标"));
		CString 日志消息;
		日志消息.Format(_T("  地图: %s(%d), X=%.2f, Y=%.2f"), 地图名称, 显示地图编号, 当前X, 当前Y);
		添加日志(日志消息);
	}
	catch (const std::exception& e)
	{
		CString 错误信息 = CString(e.what());
		MessageBox(错误信息, _T("读取失败"), MB_ICONERROR);
		添加日志(_T("读取坐标失败: ") + 错误信息);
	}
}

// ==================== 地图相关 ====================

CString NageTeleporterDlg::获取地图名称(int 地图编号)
{
	for (int i = 0; i < 地图数量; i++)
	{
		if (地图列表[i].编号 == 地图编号)
		{
			return 地图列表[i].名称;
		}
	}
	return _T("未知地图");
}

BOOL NageTeleporterDlg::是否允许传送(int 地图编号)
{
	// 不允许传送的地图列表（与注入页面类保持一致）
	switch (地图编号)
	{
	case 11:  // 水晶2
	case 14:  // 罪恶3
	case 20:  // 终结者屋
	case 35:  // 中央管制塔
	case 24:  // 迷雾1
	case 25:  // 迷雾2
	case 26:  // 迷雾3
	case 27:  // 迷雾4
	case 28:  // 迷雾5
		return FALSE;
	default:
		return TRUE;
	}
}

void NageTeleporterDlg::地图编号改变()
{
	CString 文本;
	地图编号编辑框.GetWindowText(文本);
	if (文本.IsEmpty()) return;

	int 地图编号 = _ttoi(文本);
	if (地图编号 >= 1 && 地图编号 <= 地图数量)
	{
		CString 地图名称 = 获取地图名称(地图编号);
		地图名称标签.SetWindowText(地图名称);
	}
	else
	{
		地图名称标签.SetWindowText(_T("未知地图"));
	}
}

// ==================== 传送核心功能 ====================

BOOL NageTeleporterDlg::执行地图切换(int 目标地图原始编号)
{
	// 构建地图切换代码（使用显示编号 = 原始编号 + 1）
	int 目标地图显示编号 = 目标地图原始编号 + 1;

	std::vector<BYTE> 切换代码;

	// pushad
	切换代码.push_back(0x60);
	// pushfd
	切换代码.push_back(0x9C);
	// push 地图编号
	切换代码.push_back(0x6A);
	切换代码.push_back((BYTE)目标地图显示编号);

	// mov eax, 地图切换函数地址
	切换代码.push_back(0xB8);
	DWORD_PTR funcAddr = 地图切换函数地址;
	切换代码.push_back(funcAddr & 0xFF);
	切换代码.push_back((funcAddr >> 8) & 0xFF);
	切换代码.push_back((funcAddr >> 16) & 0xFF);
	切换代码.push_back((funcAddr >> 24) & 0xFF);
	// call eax
	切换代码.push_back(0xFF);
	切换代码.push_back(0xD0);
	// add esp, 4
	切换代码.push_back(0x83);
	切换代码.push_back(0xC4);
	切换代码.push_back(0x04);
	// popfd
	切换代码.push_back(0x9D);
	// popad
	切换代码.push_back(0x61);
	// ret
	切换代码.push_back(0xC3);

	// 分配远程内存
	BYTE* 远程内存 = (BYTE*)VirtualAllocEx(游戏进程句柄, NULL, 切换代码.size(),
		MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (远程内存 == NULL)
	{
		添加日志(_T("分配远程内存失败"));
		return FALSE;
	}

	// 写入代码
	SIZE_T 写入字节数;
	if (!WriteProcessMemory(游戏进程句柄, 远程内存, 切换代码.data(), 切换代码.size(), &写入字节数))
	{
		VirtualFreeEx(游戏进程句柄, 远程内存, 0, MEM_RELEASE);
		添加日志(_T("写入远程代码失败"));
		return FALSE;
	}

	// 创建远程线程执行
	HANDLE 远程线程 = CreateRemoteThread(游戏进程句柄, NULL, 0,
		(LPTHREAD_START_ROUTINE)远程内存, NULL, 0, NULL);
	if (远程线程 == NULL)
	{
		VirtualFreeEx(游戏进程句柄, 远程内存, 0, MEM_RELEASE);
		添加日志(_T("创建远程线程失败"));
		return FALSE;
	}

	// 等待线程完成
	WaitForSingleObject(远程线程, 5000);
	CloseHandle(远程线程);

	// 释放内存
	VirtualFreeEx(游戏进程句柄, 远程内存, 0, MEM_RELEASE);

	// 等待地图切换完成
	BOOL 地图切换成功 = FALSE;
	for (int 尝试次数 = 0; 尝试次数 < 20; 尝试次数++)
	{
		Sleep(500);

		DWORD 当前地图;
		SIZE_T 读取字节数;
		if (ReadProcessMemory(游戏进程句柄, (LPCVOID)地图编号地址, &当前地图, sizeof(DWORD), &读取字节数))
		{
			if ((int)当前地图 == 目标地图原始编号)
			{
				地图切换成功 = TRUE;
				CString 日志消息;
				日志消息.Format(_T("地图切换成功！当前地图编号=%d"), 当前地图);
				添加日志(日志消息);
				break;
			}
		}
	}

	return 地图切换成功;
}

BOOL NageTeleporterDlg::写入坐标(float X, float Y)
{
	// 尝试多次写入确保成功
	for (int i = 0; i < 3; i++)
	{
		SIZE_T 写入字节数;

		if (!WriteProcessMemory(游戏进程句柄, (LPVOID)坐标X地址, &X, sizeof(float), &写入字节数))
		{
			添加日志(_T("写入X坐标失败"));
			return FALSE;
		}

		if (!WriteProcessMemory(游戏进程句柄, (LPVOID)坐标Y地址, &Y, sizeof(float), &写入字节数))
		{
			添加日志(_T("写入Y坐标失败"));
			return FALSE;
		}

		Sleep(500);
	}

	return TRUE;
}

void NageTeleporterDlg::点击传送()
{
	if (!检查游戏进程())
	{
		MessageBox(_T("游戏进程无效，请重新附加进程！"), _T("错误"), MB_ICONERROR);
		return;
	}

	// 获取目标地图编号
	CString 地图文本;
	地图编号编辑框.GetWindowText(地图文本);
	if (地图文本.IsEmpty())
	{
		MessageBox(_T("请输入目标地图编号！"), _T("提示"), MB_ICONWARNING);
		return;
	}
	int 目标地图显示编号 = _ttoi(地图文本);

	// 获取目标X坐标
	CString X文本;
	X坐标编辑框.GetWindowText(X文本);
	if (X文本.IsEmpty())
	{
		MessageBox(_T("请输入目标X坐标！"), _T("提示"), MB_ICONWARNING);
		return;
	}
	float 目标X = (float)_tstof(X文本);

	// 获取目标Y坐标
	CString Y文本;
	Y坐标编辑框.GetWindowText(Y文本);
	if (Y文本.IsEmpty())
	{
		MessageBox(_T("请输入目标Y坐标！"), _T("提示"), MB_ICONWARNING);
		return;
	}
	float 目标Y = (float)_tstof(Y文本);

	// 检查地图编号是否有效
	if (目标地图显示编号 < 1 || 目标地图显示编号 > 地图数量)
	{
		CString 提示;
		提示.Format(_T("地图编号范围: 1 - %d\n当前输入: %d"), 地图数量, 目标地图显示编号);
		MessageBox(提示, _T("无效的地图编号"), MB_ICONWARNING);
		return;
	}

	// 检查是否允许传送
	if (!是否允许传送(目标地图显示编号))
	{
		CString 地图名称 = 获取地图名称(目标地图显示编号);
		CString 提示信息;
		提示信息.Format(_T("目标地图[%s]不允许传送！"), 地图名称);
		MessageBox(提示信息, _T("提示"), MB_ICONWARNING);
		return;
	}

	CString 地图名称 = 获取地图名称(目标地图显示编号);
	int 目标地图原始编号 = 目标地图显示编号 - 1; // 游戏内从0开始

	添加日志(_T("===== 开始传送 ====="));
	CString 日志消息;
	日志消息.Format(_T("目标: %s(编号%d), X=%.2f, Y=%.2f"),
		地图名称, 目标地图显示编号, 目标X, 目标Y);
	添加日志(日志消息);

	// 读取当前地图编号
	DWORD 当前地图原始编号;
	SIZE_T 读取字节数;
	if (!ReadProcessMemory(游戏进程句柄, (LPCVOID)地图编号地址, &当前地图原始编号, sizeof(DWORD), &读取字节数))
	{
		MessageBox(_T("读取当前地图失败！"), _T("错误"), MB_ICONERROR);
		添加日志(_T("读取当前地图失败"));
		return;
	}

	// 判断是否需要切换地图
	bool needMapChange = ((int)当前地图原始编号 != 目标地图原始编号);

	if (needMapChange)
	{
		添加日志(_T("需要切换地图，正在执行地图切换..."));

		if (!执行地图切换(目标地图原始编号))
		{
			MessageBox(_T("地图切换失败或超时！"), _T("错误"), MB_ICONERROR);
			添加日志(_T("地图切换失败"));
			return;
		}

		Sleep(1000);
	}
	else
	{
		添加日志(_T("已在目标地图，无需切换"));
	}

	// 写入坐标
	添加日志(_T("正在写入坐标..."));
	if (写入坐标(目标X, 目标Y))
	{
		添加日志(_T("坐标写入成功！"));
	}
	else
	{
		添加日志(_T("坐标写入失败"));
	}

	添加日志(_T("===== 传送结束 ====="));
}

// ==================== 执行传送（带参数，供保存坐标使用） ====================

void NageTeleporterDlg::执行传送参数(int 目标地图显示编号, float 目标X, float 目标Y)
{
	if (!检查游戏进程())
	{
		MessageBox(_T("游戏进程无效，请重新附加进程！"), _T("错误"), MB_ICONERROR);
		return;
	}

	int 目标地图原始编号 = 目标地图显示编号 - 1;

	CString 地图名称 = 获取地图名称(目标地图显示编号);

	添加日志(_T("===== 开始传送 ====="));
	CString 日志消息;
	日志消息.Format(_T("目标: %s(编号%d), X=%.2f, Y=%.2f"),
		地图名称, 目标地图显示编号, 目标X, 目标Y);
	添加日志(日志消息);

	// 读取当前地图编号
	DWORD 当前地图原始编号;
	SIZE_T 读取字节数;
	if (!ReadProcessMemory(游戏进程句柄, (LPCVOID)地图编号地址, &当前地图原始编号, sizeof(DWORD), &读取字节数))
	{
		MessageBox(_T("读取当前地图失败！"), _T("错误"), MB_ICONERROR);
		添加日志(_T("读取当前地图失败"));
		return;
	}

	bool needMapChange = ((int)当前地图原始编号 != 目标地图原始编号);

	if (needMapChange)
	{
		添加日志(_T("需要切换地图，正在执行地图切换..."));
		if (!执行地图切换(目标地图原始编号))
		{
			MessageBox(_T("地图切换失败或超时！"), _T("错误"), MB_ICONERROR);
			添加日志(_T("地图切换失败"));
			return;
		}
		Sleep(1000);
	}
	else
	{
		添加日志(_T("已在目标地图，无需切换"));
	}

	添加日志(_T("正在写入坐标..."));
	if (写入坐标(目标X, 目标Y))
	{
		添加日志(_T("坐标写入成功！"));
	}
	else
	{
		添加日志(_T("坐标写入失败"));
	}

	添加日志(_T("===== 传送结束 ====="));
}

// ==================== 保存坐标功能 ====================

int NageTeleporterDlg::查找下一个空槽()
{
	for (int i = 0; i < 最大保存数量; i++)
	{
		if (!已保存坐标[i].有效)
			return i;
	}
	return -1; // 已满
}

void NageTeleporterDlg::刷新已保存列表()
{
	已保存列表.ResetContent();
	int 有效计数 = 0;
	for (int i = 0; i < 最大保存数量; i++)
	{
		if (已保存坐标[i].有效)
		{
			CString 文本;
			文本.Format(_T("%d. %s"), i + 1, 已保存坐标[i].标签);
			已保存列表.AddString(文本);
			有效计数++;
		}
	}
	已保存计数 = 有效计数;

	// 更新按钮状态
	使用已保存按钮.EnableWindow(已保存列表.GetCurSel() != LB_ERR && 进程已附加);
	删除已保存按钮.EnableWindow(已保存列表.GetCurSel() != LB_ERR);
}

void NageTeleporterDlg::点击保存坐标()
{
	if (!检查游戏进程())
	{
		MessageBox(_T("游戏进程无效，请重新附加进程！"), _T("错误"), MB_ICONERROR);
		return;
	}

	// 查找空槽
	int 空槽 = 查找下一个空槽();
	if (空槽 == -1)
	{
		CString 满提示;
		满提示.Format(_T("已保存 %d 个坐标，已达上限！\n请先删除不需要的保存项。"), 最大保存数量);
		MessageBox(满提示, _T("提示"), MB_ICONWARNING);
		return;
	}

	// 获取当前输入框中的值
	CString 地图文本;
	地图编号编辑框.GetWindowText(地图文本);
	if (地图文本.IsEmpty())
	{
		MessageBox(_T("请先输入或读取坐标！"), _T("提示"), MB_ICONWARNING);
		return;
	}

	CString X文本, Y文本;
	X坐标编辑框.GetWindowText(X文本);
	Y坐标编辑框.GetWindowText(Y文本);
	if (X文本.IsEmpty() || Y文本.IsEmpty())
	{
		MessageBox(_T("请先输入或读取坐标！"), _T("提示"), MB_ICONWARNING);
		return;
	}

	// 获取标签
	CString 标签;
	保存标签编辑框.GetWindowText(标签);
	if (标签.IsEmpty())
	{
		标签.Format(_T("坐标%d"), 已保存计数 + 1);
	}

	// 保存数据
	已保存坐标[空槽].标签 = 标签;
	已保存坐标[空槽].地图编号 = _ttoi(地图文本);
	已保存坐标[空槽].X = (float)_tstof(X文本);
	已保存坐标[空槽].Y = (float)_tstof(Y文本);
	已保存坐标[空槽].地图名称 = 获取地图名称(已保存坐标[空槽].地图编号);
	已保存坐标[空槽].有效 = TRUE;

	// 刷新列表
	刷新已保存列表();

	// 清空标签输入框
	保存标签编辑框.SetWindowText(_T(""));

	CString 日志消息;
	日志消息.Format(_T("已保存坐标 [%s]: %s(%d), X=%.2f, Y=%.2f"),
		标签, 已保存坐标[空槽].地图名称, 已保存坐标[空槽].地图编号,
		已保存坐标[空槽].X, 已保存坐标[空槽].Y);
	添加日志(日志消息);

	// 保存到文件
	保存坐标到文件();
}

void NageTeleporterDlg::点击使用已保存()
{
	int 选中索引 = 已保存列表.GetCurSel();
	if (选中索引 == LB_ERR)
	{
		MessageBox(_T("请先从列表中选择一个已保存的坐标！"), _T("提示"), MB_ICONWARNING);
		return;
	}

	// 找到选中的有效项
	int 真实索引 = -1;
	int 有效计数 = 0;
	for (int i = 0; i < 最大保存数量; i++)
	{
		if (已保存坐标[i].有效)
		{
			if (有效计数 == 选中索引)
			{
				真实索引 = i;
				break;
			}
			有效计数++;
		}
	}

	if (真实索引 == -1) return;

	// 填充到输入框
	CString 地图文本, X文本, Y文本;
	地图文本.Format(_T("%d"), 已保存坐标[真实索引].地图编号);
	X文本.Format(_T("%.2f"), 已保存坐标[真实索引].X);
	Y文本.Format(_T("%.2f"), 已保存坐标[真实索引].Y);

	地图编号编辑框.SetWindowText(地图文本);
	X坐标编辑框.SetWindowText(X文本);
	Y坐标编辑框.SetWindowText(Y文本);

	CString 日志消息;
	日志消息.Format(_T("已加载保存的坐标 [%s]: %s(%d), X=%.2f, Y=%.2f"),
		已保存坐标[真实索引].标签, 已保存坐标[真实索引].地图名称,
		已保存坐标[真实索引].地图编号, 已保存坐标[真实索引].X, 已保存坐标[真实索引].Y);
	添加日志(日志消息);
}

void NageTeleporterDlg::点击删除已保存()
{
	int 选中索引 = 已保存列表.GetCurSel();
	if (选中索引 == LB_ERR)
	{
		MessageBox(_T("请先从列表中选择一个要删除的坐标！"), _T("提示"), MB_ICONWARNING);
		return;
	}

	// 找到选中的有效项
	int 真实索引 = -1;
	int 有效计数 = 0;
	for (int i = 0; i < 最大保存数量; i++)
	{
		if (已保存坐标[i].有效)
		{
			if (有效计数 == 选中索引)
			{
				真实索引 = i;
				break;
			}
			有效计数++;
		}
	}

	if (真实索引 == -1) return;

	// 删除
	CString 删除标签 = 已保存坐标[真实索引].标签;
	已保存坐标[真实索引].有效 = FALSE;
	已保存坐标[真实索引].标签.Empty();
	已保存坐标[真实索引].地图编号 = 0;
	已保存坐标[真实索引].X = 0.0f;
	已保存坐标[真实索引].Y = 0.0f;

	刷新已保存列表();

	CString 日志消息;
	日志消息.Format(_T("已删除保存的坐标 [%s]"), 删除标签);
	添加日志(日志消息);

	// 保存到文件
	保存坐标到文件();
}

// ==================== 文件持久化 ====================

CString NageTeleporterDlg::获取数据文件路径()
{
	TCHAR 模块路径[MAX_PATH] = { 0 };
	GetModuleFileName(AfxGetInstanceHandle(), 模块路径, MAX_PATH);
	CString 路径(模块路径);
	int 位置 = 路径.ReverseFind(_T('\\'));
	if (位置 != -1)
	{
		路径 = 路径.Left(位置 + 1);
	}
	路径 += _T("NageTeleporter.dat");
	return 路径;
}

void NageTeleporterDlg::保存坐标到文件()
{
	CString 路径 = 获取数据文件路径();
	添加日志(_T("正在保存坐标到: ") + 路径);

	try
	{
		CFile 文件;
		CFileException 异常;
		if (!文件.Open(路径, CFile::modeCreate | CFile::modeWrite, &异常))
		{
			CString 错误信息;
			错误信息.Format(_T("无法创建坐标文件 (错误码: %d)"), 异常.m_lOsError);
			添加日志(错误信息);
			return;
		}

		// 写入文件头：签名 + 版本
		DWORD 签名 = 0x4E544C52; // "NTLR"
		DWORD 版本 = 1;
		文件.Write(&签名, sizeof(DWORD));
		文件.Write(&版本, sizeof(DWORD));

		// 写入有效计数
		文件.Write(&已保存计数, sizeof(int));

		// 写入每个有效坐标（固定格式，每项大小固定）
		for (int i = 0; i < 最大保存数量; i++)
		{
			BOOL 有效 = 已保存坐标[i].有效;
			文件.Write(&有效, sizeof(BOOL));
			if (有效)
			{
				// 标签：固定最大32字节ANSI
				char 标签缓冲区[64] = { 0 };
				CStringA 标签A(已保存坐标[i].标签);
				strncpy_s(标签缓冲区, 标签A.GetString(), _TRUNCATE);
				文件.Write(标签缓冲区, sizeof(标签缓冲区));

				// 地图编号、X、Y
				文件.Write(&已保存坐标[i].地图编号, sizeof(int));
				文件.Write(&已保存坐标[i].X, sizeof(float));
				文件.Write(&已保存坐标[i].Y, sizeof(float));

				// 地图名称：固定最大32字节ANSI
				char 名称缓冲区[64] = { 0 };
				CStringA 名称A(已保存坐标[i].地图名称);
				strncpy_s(名称缓冲区, 名称A.GetString(), _TRUNCATE);
				文件.Write(名称缓冲区, sizeof(名称缓冲区));
			}
		}

		文件.Close();
		添加日志(_T("坐标数据保存完成"));
	}
	catch (CException* e)
	{
		CString 错误信息;
		e->GetErrorMessage(错误信息.GetBuffer(256), 256);
		错误信息.ReleaseBuffer();
		添加日志(_T("保存坐标文件异常: ") + 错误信息);
		e->Delete();
	}
}

void NageTeleporterDlg::加载坐标从文件()
{
	CString 路径 = 获取数据文件路径();

	try
	{
		CFile 文件;
		CFileException 异常;
		if (!文件.Open(路径, CFile::modeRead, &异常))
		{
			return; // 文件不存在，首次运行
		}

		// 读取文件头
		DWORD 签名 = 0, 版本 = 0;
		文件.Read(&签名, sizeof(DWORD));
		文件.Read(&版本, sizeof(DWORD));

		if (签名 != 0x4E544C52 || 版本 != 1)
		{
			文件.Close();
			添加日志(_T("坐标数据文件格式不兼容，已忽略"));
			return;
		}

		// 读取计数
		int 读取计数 = 0;
		文件.Read(&读取计数, sizeof(int));

		// 重置数组
		for (int i = 0; i < 最大保存数量; i++)
		{
			已保存坐标[i].有效 = FALSE;
			已保存坐标[i].标签.Empty();
			已保存坐标[i].地图名称.Empty();
		}
		已保存计数 = 0;

		// 读取每个坐标
		for (int i = 0; i < 最大保存数量; i++)
		{
			BOOL 有效 = FALSE;
			if (文件.Read(&有效, sizeof(BOOL)) != sizeof(BOOL))
				break;

			if (有效)
			{
				// 标签：固定64字节
				char 标签缓冲区[64] = { 0 };
				文件.Read(标签缓冲区, sizeof(标签缓冲区));

				已保存坐标[i].有效 = TRUE;
				已保存坐标[i].标签 = CString(CStringA(标签缓冲区));

				// 地图编号、X、Y
				文件.Read(&已保存坐标[i].地图编号, sizeof(int));
				文件.Read(&已保存坐标[i].X, sizeof(float));
				文件.Read(&已保存坐标[i].Y, sizeof(float));

				// 地图名称：固定64字节
				char 名称缓冲区[64] = { 0 };
				文件.Read(名称缓冲区, sizeof(名称缓冲区));
				已保存坐标[i].地图名称 = CString(CStringA(名称缓冲区));

				已保存计数++;
			}
		}

		文件.Close();

		CString 加载日志;
		加载日志.Format(_T("已从文件加载 %d 个坐标"), 已保存计数);
		添加日志(加载日志);
	}
	catch (CException* e)
	{
		CString 错误信息;
		e->GetErrorMessage(错误信息.GetBuffer(256), 256);
		错误信息.ReleaseBuffer();
		添加日志(_T("加载坐标文件失败: ") + 错误信息);
		e->Delete();
	}
}

void NageTeleporterDlg::已保存列表选中改变()
{
	BOOL 有选中 = (已保存列表.GetCurSel() != LB_ERR);
	使用已保存按钮.EnableWindow(有选中 && 进程已附加);
	删除已保存按钮.EnableWindow(有选中);
}

void NageTeleporterDlg::已保存列表双击()
{
	// 双击直接加载坐标到输入框
	点击使用已保存();
}

// ==================== 日志工具 ====================

void NageTeleporterDlg::添加日志(const CString& 消息)
{
	// 获取当前时间
	SYSTEMTIME 系统时间;
	GetLocalTime(&系统时间);

	CString 时间戳;
	时间戳.Format(_T("[%02d:%02d:%02d] "), 系统时间.wHour, 系统时间.wMinute, 系统时间.wSecond);

	CString 完整消息 = 时间戳 + 消息 + _T("\r\n");

	// 追加到日志编辑框
	int 文本长度 = 日志编辑框.GetWindowTextLength();
	日志编辑框.SetSel(文本长度, 文本长度);
	日志编辑框.ReplaceSel(完整消息);

	// 自动滚动到底部
	日志编辑框.LineScroll(日志编辑框.GetLineCount());
}

void NageTeleporterDlg::清空日志()
{
	日志编辑框.SetWindowText(_T(""));
}

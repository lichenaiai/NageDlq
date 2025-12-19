// 注入页面类.h
#pragma once

#include "afxdialogex.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

// 注入页面类 对话框
class 注入页面类 : public CDialogEx
{
	DECLARE_DYNAMIC(注入页面类)

public:
	注入页面类(CWnd* p父窗口 = nullptr);   // 标准构造函数
	virtual ~注入页面类();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PAGE_INJECTION };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual void OnDestroy();  // 窗口销毁处理

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();  // 使用MFC标准函数名
	afx_msg void 点击自动打怪按钮();  // 自动打怪按钮点击事件
	afx_msg void OnTimer(UINT_PTR nIDEvent);  // 使用MFC标准函数名

	// 消息处理函数声明
	afx_msg LRESULT 游戏进程退出消息处理(WPARAM w参数, LPARAM l参数);
	afx_msg LRESULT 自动打怪停止消息处理(WPARAM w参数, LPARAM l参数);
	afx_msg LRESULT 更新目标ID消息处理(WPARAM w参数, LPARAM l参数);

private:
	// 进程窗口查找相关函数
	BOOL 通过进程ID查找窗口();
	HWND 查找进程主窗口(DWORD 目标进程ID);
	HWND FindMainWindow(DWORD dwPID);
	HWND 深度查找进程窗口(DWORD 目标进程ID);
	BOOL 枚举窗口回调函数(HWND hwnd);
	std::vector<HWND> 获取进程所有窗口(DWORD 进程ID);
	HWND 深度查找窗口递归(HWND 父窗口, DWORD 目标进程ID);

	// 自动打怪相关函数
	DWORD 获取游戏进程ID();
	void 启动自动打怪线程();
	void 停止自动打怪();
	void 自动打怪线程函数();

	// 怪物ID地址轮询
	DWORD 获取有效目标怪物();
	void 执行智能攻击();

	// 状态更新函数
	void 更新目标ID显示(DWORD 目标ID);

	// 窗口和鼠标操作
	BOOL 激活并聚焦游戏窗口();
	void 后台模拟鼠标移动();

	// 注入功能
	BOOL 注入自动打怪功能();

	// 进程操作
	BOOL 检查游戏进程();

private:
	std::atomic<bool> 自动打怪运行中;            // 自动打怪是否运行
	std::atomic<bool> 线程停止标志;             // 线程停止标志
	std::thread 自动打怪线程;                   // 自动打怪线程
	std::mutex 游戏进程互斥锁;                  // 进程操作互斥锁

	// 自动打怪相关地址
	const DWORD 攻击标志地址 = 0x31A2DCC;       // 攻击标志地址
	const DWORD 目标怪物地址 = 0x31A2DDC;       // 目标怪物ID地址
	const DWORD 怪物ID地址1 = 0x87FD50;         // 怪物ID地址1
	const DWORD 怪物ID地址2 = 0x96D5F0;         // 怪物ID地址2
	const DWORD 怪物ID地址3 = 0x86C098;         // 怪物ID地址3

	int 当前怪物地址索引;                       // 当前使用的怪物地址索引
	int 总攻击次数;                            // 攻击统计
	DWORD 自动打怪开始时间;                     // 开始时间记录
	DWORD 当前目标ID;                          // 当前目标怪物ID

public:
	CButton 自动打怪按钮;
	HANDLE 游戏进程句柄;                        // 游戏进程句柄
	DWORD 游戏进程ID;                          // 游戏进程ID
	HWND 游戏窗口句柄;
	CStatic 状态标签;
	float 初始X坐标;
	float 初始Y坐标;
	DWORD 上次检查坐标时间;
	BOOL 检查坐标范围();
	void 记录初始坐标();
};
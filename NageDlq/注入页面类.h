// 注入页面类.h
#pragma once

#include "afxdialogex.h"
#include <thread>
#include <atomic>
#include <mutex>

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

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();  // 使用MFC标准函数名
	afx_msg void 点击自动打怪按钮();  // 自动打怪按钮点击事件
	afx_msg void OnTimer(UINT_PTR nIDEvent);  // 使用MFC标准函数名
	afx_msg LRESULT 游戏进程退出消息处理(WPARAM w参数, LPARAM l参数);
	afx_msg LRESULT 自动打怪停止消息处理(WPARAM w参数, LPARAM l参数);

private:
	// 自动打怪相关函数
	DWORD 获取游戏进程ID();
	void 启动自动打怪线程();
	void 停止自动打怪();
	void 自动打怪线程函数();

	// 怪物ID地址轮询
	DWORD 获取下一个怪物ID();
	void 更新当前怪物ID(DWORD 怪物ID);

	// 注入功能
	BOOL 注入自动打怪功能();

	// 进程操作
	BOOL 检查游戏进程();

private:
	std::atomic<bool> 自动打怪运行中;            // 自动打怪是否运行
	std::thread 自动打怪线程;                   // 自动打怪线程
	HANDLE 游戏进程句柄;                        // 游戏进程句柄
	DWORD 游戏进程ID;                          // 游戏进程ID
	std::mutex 游戏进程互斥锁;                  // 进程操作互斥锁

	// 自动打怪相关地址（根据您的描述）
	const DWORD 攻击标志地址 = 0x31A2DCC;       // 攻击标志地址
	const DWORD 目标怪物地址 = 0x31A2DDC;       // 目标怪物ID地址
	const DWORD 怪物ID地址1 = 0x87FD50;         // 怪物ID地址1
	const DWORD 怪物ID地址2 = 0x96D5F0;         // 怪物ID地址2
	const DWORD 怪物ID地址3 = 0x86C098;         // 怪物ID地址3

	int 当前怪物地址索引;                       // 当前使用的怪物地址索引
	int 总攻击次数;                            // 攻击统计
	DWORD 自动打怪开始时间;                     // 开始时间记录

public:
	CButton 自动打怪按钮;
};
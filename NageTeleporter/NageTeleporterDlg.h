#pragma once

#include "afxdialogex.h"
#include <vector>
#include <string>
#include <atomic>

// 内存地址常量（与注入页面类保持一致）
constexpr DWORD_PTR 坐标X地址 = 0x319B8A8;      // X坐标
constexpr DWORD_PTR 坐标Y地址 = 0x319B8B0;      // Y坐标
constexpr DWORD_PTR 地图编号地址 = 0x31A2808;    // 当前地图编号
constexpr DWORD_PTR 地图切换函数地址 = 0x006ADF6E; // 游戏内地图切换函数

// 地图名称对照表
struct 地图信息
{
	int 编号;
	CString 名称;
};

// 保存的坐标数据
struct 保存坐标数据
{
	CString 标签;
	int 地图编号;      // 显示编号 (1-based)
	float X;
	float Y;
	CString 地图名称;
	BOOL 有效;
};

class NageTeleporterDlg : public CDialogEx
{
	DECLARE_DYNAMIC(NageTeleporterDlg)

public:
	NageTeleporterDlg(CWnd* pParent = nullptr);
	virtual ~NageTeleporterDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TELEPORTER_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnDestroy();

	DECLARE_MESSAGE_MAP()

public:
	// 控件
	CComboBox 进程选择组合框;
	CButton 刷新进程按钮;
	CButton 附加进程按钮;
	CButton 读取当前坐标按钮;
	CEdit 地图编号编辑框;
	CEdit X坐标编辑框;
	CEdit Y坐标编辑框;
	CStatic 地图名称标签;
	CStatic 当前信息标签;
	CStatic 进程状态标签;
	CButton 传送按钮;
	CEdit 日志编辑框;

	// 保存坐标控件
	CEdit 保存标签编辑框;
	CButton 保存坐标按钮;
	CListBox 已保存列表;
	CButton 使用已保存按钮;
	CButton 删除已保存按钮;

	// 消息处理
	afx_msg void 点击刷新进程();
	afx_msg void 点击附加进程();
	afx_msg void 点击读取当前坐标();
	afx_msg void 点击传送();
	afx_msg void 地图编号改变();
	afx_msg void 点击保存坐标();
	afx_msg void 点击使用已保存();
	afx_msg void 点击删除已保存();
	afx_msg void 已保存列表选中改变();
	afx_msg void 已保存列表双击();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

private:
	// 进程相关
	HANDLE 游戏进程句柄;
	DWORD 游戏进程ID;
	BOOL 进程已附加;

	// 地图数据
	static const 地图信息 地图列表[];
	static const int 地图数量;

	// 保存的坐标
	static const int 最大保存数量 = 10;
	保存坐标数据 已保存坐标[10];
	int 已保存计数;

	// 函数声明
	void 枚举游戏进程();
	CString 获取地图名称(int 地图编号);
	BOOL 是否允许传送(int 地图编号);
	void 添加日志(const CString& 消息);
	void 清空日志();
	void 读取当前坐标(float& X, float& Y, DWORD& 地图编号);
	BOOL 写入坐标(float X, float Y);
	BOOL 执行地图切换(int 目标地图原始编号);
	BOOL 检查游戏进程();

	// 保存坐标相关
	void 刷新已保存列表();
	int 查找下一个空槽();
	void 执行传送参数(int 目标地图显示编号, float 目标X, float 目标Y);
	void 保存坐标到文件();
	void 加载坐标从文件();
	CString 获取数据文件路径();
};

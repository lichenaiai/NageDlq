#pragma once
#include "afxdialogex.h"
#include <urlmon.h>
#include <wininet.h>
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "wininet.lib")

// NageUPDlg 对话框
class NageUPDlg : public CDialogEx
{
    DECLARE_DYNAMIC(NageUPDlg)

public:
    NageUPDlg(CWnd* pParent = nullptr);
    virtual ~NageUPDlg();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_UPDATER_DIALOG };
#endif

    void 设置当前版本(const CString& 版本号) { 当前版本号 = 版本号; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

public:
    // 控件变量
    CProgressCtrl 进度条控件;
    CStatic 当前状态标签;
    CStatic 文件名标签;
    CStatic 下载速度标签;
    CStatic 文件大小标签;
    CButton 取消按钮;

    afx_msg void OnBnClickedButtonCancel();
    afx_msg void OnTimer(UINT_PTR nIDEvent);

private:
    CString 当前版本号;
    BOOL 正在更新;
    BOOL 用户取消;
    CString 更新服务器地址;

    // 更新流程
    void 开始更新流程();
    BOOL 获取服务器更新信息(CString& 最新版本号, CString& 更新文件名);
    BOOL 下载更新文件(const CString& 文件名);
    BOOL 验证文件完整性(const CString& 文件路径);
    BOOL 应用更新(const CString& 文件路径);
    void 完成更新();

    // 辅助函数
    void 更新进度(int 进度, const CString& 状态 = _T(""));
    void 更新下载信息(const CString& 文件名, const CString& 速度, const CString& 大小);
    CString 构建文件URL(const CString& 文件名);
    ULONG 获取文件大小(const CString& 文件路径);
    BOOL 解压ZIP文件(const CString& 压缩文件路径, const CString& 解压目录);
    BOOL 解压RAR文件(const CString& 压缩文件路径, const CString& 解压目录);
    BOOL 解压压缩文件(const CString& 压缩文件路径, const CString& 解压目录);
    BOOL 替换程序文件(const CString& 源目录, const CString& 目标目录);
    CString 从文件名提取版本号(const CString& 文件名);
    BOOL 通过文件名扫描获取更新信息(CString& 最新版本号, CString& 更新文件名);

    // 自定义消息处理
    afx_msg LRESULT OnUpdateProgress(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnUpdateDownloadInfo(WPARAM wParam, LPARAM lParam);
};
#pragma once

#include "afxdialogex.h"
#include "Resource.h"
#include <wrl/client.h>
#include <WebView2.h>

using namespace Microsoft::WRL;

// 网页对话框类
class 网页对话框类 : public CDialogEx
{
    DECLARE_DYNAMIC(网页对话框类)

public:
    网页对话框类(CWnd* pParent = nullptr);
    virtual ~网页对话框类();

    enum { IDD = IDD_DIALOG_HTML };

    void 设置网页地址(const CString& 地址);

    // WebView2 相关成员（改为 public，以便回调类访问）
    ComPtr<ICoreWebView2Controller> webViewController;
    ComPtr<ICoreWebView2> webView;
    CString 网页地址;

    void 调整WebView2大小();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK() {}
    virtual void OnCancel() {}

    DECLARE_MESSAGE_MAP()

private:
    typedef HRESULT(WINAPI* CreateCoreWebView2EnvironmentWithOptionsFunc)(
        PCWSTR browserExecutableFolder,
        PCWSTR userDataFolder,
        ICoreWebView2EnvironmentOptions* options,
        ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* handler);

    HMODULE webView2Loader;
    CreateCoreWebView2EnvironmentWithOptionsFunc pfnCreateEnvironment;

    BOOL 初始化WebView2();
    BOOL 动态加载WebView2();

    // 悬浮关闭按钮
    CButton m_btnClose;

public:
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnDestroy();
    afx_msg void OnClose();  // 添加关闭消息处理
    afx_msg void OnBnClickedClose();  // 关闭按钮点击
    afx_msg LRESULT OnNcHitTest(CPoint point);  // 处理窗口拖动
};
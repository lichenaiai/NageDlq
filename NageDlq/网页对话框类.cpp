#include "pch.h"
#include "framework.h"
#include "网页对话框类.h"
#include "afxdialogex.h"
#include <Shlwapi.h>
#include "NageDlqDlg.h"
#pragma comment(lib, "shlwapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 定义回调类的结构
class CCreateEnvironmentHandler : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
{
private:
    网页对话框类* m_pDlg;
    LONG m_cRef;

public:
    CCreateEnvironmentHandler(网页对话框类* pDlg) : m_pDlg(pDlg), m_cRef(1) {}

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObject) override
    {
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler))
        {
            *ppvObject = this;
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&m_cRef); }
    STDMETHODIMP_(ULONG) Release() override
    {
        ULONG cRef = InterlockedDecrement(&m_cRef);
        if (cRef == 0) delete this;
        return cRef;
    }

    STDMETHODIMP Invoke(HRESULT result, ICoreWebView2Environment* env) override;
};

class CCreateControllerHandler : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
{
private:
    网页对话框类* m_pDlg;
    LONG m_cRef;

public:
    CCreateControllerHandler(网页对话框类* pDlg) : m_pDlg(pDlg), m_cRef(1) {}

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObject) override
    {
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler))
        {
            *ppvObject = this;
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&m_cRef); }
    STDMETHODIMP_(ULONG) Release() override
    {
        ULONG cRef = InterlockedDecrement(&m_cRef);
        if (cRef == 0) delete this;
        return cRef;
    }

    STDMETHODIMP Invoke(HRESULT result, ICoreWebView2Controller* controller) override;
};

class CContextMenuHandler : public ICoreWebView2ContextMenuRequestedEventHandler
{
private:
    LONG m_cRef;

public:
    CContextMenuHandler() : m_cRef(1) {}

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObject) override
    {
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2ContextMenuRequestedEventHandler))
        {
            *ppvObject = this;
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&m_cRef); }
    STDMETHODIMP_(ULONG) Release() override
    {
        ULONG cRef = InterlockedDecrement(&m_cRef);
        if (cRef == 0) delete this;
        return cRef;
    }

    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2ContextMenuRequestedEventArgs* args) override
    {
        return args->put_Handled(TRUE);
    }
};

// 实现回调方法
STDMETHODIMP CCreateEnvironmentHandler::Invoke(HRESULT result, ICoreWebView2Environment* env)
{
    if (result != S_OK || !env) return result;

    CCreateControllerHandler* pHandler = new CCreateControllerHandler(m_pDlg);
    return env->CreateCoreWebView2Controller(m_pDlg->GetSafeHwnd(), pHandler);
}

STDMETHODIMP CCreateControllerHandler::Invoke(HRESULT result, ICoreWebView2Controller* controller)
{
    if (result != S_OK || !controller) return result;

    m_pDlg->webViewController = controller;
    controller->get_CoreWebView2(&m_pDlg->webView);
    controller->put_IsVisible(TRUE);

    // 禁用右键菜单（使用正确的事件接口）
    if (m_pDlg->webView)
    {
        // 注意：某些版本的 SDK 中方法名可能不同，如果编译失败可以注释掉
        //m_pDlg->webView->add_ContextMenuRequested(new CContextMenuHandler(), nullptr);
    }

    m_pDlg->调整WebView2大小();
    m_pDlg->webView->Navigate(m_pDlg->网页地址.AllocSysString());

    return S_OK;
}

// 网页对话框类实现
IMPLEMENT_DYNAMIC(网页对话框类, CDialogEx)

网页对话框类::网页对话框类(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_DIALOG_HTML, pParent)
    , webView2Loader(NULL)
    , pfnCreateEnvironment(NULL)
{
    网页地址 = _T("http://124.220.82.87:8080");
}

网页对话框类::~网页对话框类()
{
    if (webView2Loader)
    {
        FreeLibrary(webView2Loader);
        webView2Loader = NULL;
    }
}

void 网页对话框类::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_BTN_CLOSE, m_btnClose);  // 关联关闭按钮
}

BEGIN_MESSAGE_MAP(网页对话框类, CDialogEx)
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_BTN_CLOSE, &网页对话框类::OnBnClickedClose)
    ON_WM_NCHITTEST()
END_MESSAGE_MAP()

BOOL 网页对话框类::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 去掉标题栏（无边框）
    LONG style = GetWindowLong(m_hWnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU);
    SetWindowLong(m_hWnd, GWL_STYLE, style);

    // 设置窗口大小和位置（居中）
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int dlgWidth = 1120;
    int dlgHeight = 900;
    SetWindowPos(NULL, (screenWidth - dlgWidth) / 2, (screenHeight - dlgHeight) / 2,
        dlgWidth, dlgHeight, SWP_NOZORDER);

    SetWindowText(_T("兑换商城"));

    // 设置关闭按钮文本和样式
    m_btnClose.SetWindowText(_T("✕"));
    m_btnClose.SetFont(GetFont());

    if (!初始化WebView2())
    {
        CString 提示信息;
        提示信息 = _T("无法加载网页组件，您的系统可能未安装 WebView2 运行时。\n\n")
            _T("是否现在下载并安装？\n\n")
            _T("（约2MB，仅需安装一次，安装后重启登录器即可）");

        if (AfxMessageBox(提示信息, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1) == IDYES)
        {
            ShellExecute(NULL, _T("open"),
                _T("https://go.microsoft.com/fwlink/p/?LinkId=2124703"),
                NULL, NULL, SW_SHOWNORMAL);
        }
        EndDialog(IDCANCEL);
        return FALSE;
    }

    return TRUE;
}

void 网页对话框类::OnClose()
{
    // 处理关闭事件
    DestroyWindow();
}

void 网页对话框类::OnBnClickedClose()
{
    // 关闭按钮点击
    DestroyWindow();
}

void 网页对话框类::OnDestroy()
{
    // 通知登录页面对话框已销毁
    CWnd* pParent = GetParent();
    if (pParent)
    {
        // 通过主窗口找到登录页面
        NageDlqDlg* pMain = (NageDlqDlg*)AfxGetMainWnd();
        if (pMain)
        {
            pMain->登录页面.m_pHtmlDialog = nullptr;
        }
    }
    CDialogEx::OnDestroy();
}

LRESULT 网页对话框类::OnNcHitTest(CPoint point)
{
    // 允许拖动窗口
    CRect rc;
    GetWindowRect(&rc);

    // 如果点击在标题区域（顶部30像素），允许拖动
    if (point.y >= rc.top && point.y <= rc.top + 10)
    {
        return HTCAPTION;
    }
    return CDialogEx::OnNcHitTest(point);
}


void 网页对话框类::设置网页地址(const CString& 地址)
{
    网页地址 = 地址;
    if (webView)
    {
        webView->Navigate(网页地址.AllocSysString());
    }
}

BOOL 网页对话框类::动态加载WebView2()
{
    TCHAR dllPath[MAX_PATH];
    GetModuleFileName(NULL, dllPath, MAX_PATH);
    PathRemoveFileSpec(dllPath);
    PathAppend(dllPath, _T("WebView2Loader.dll"));

    webView2Loader = LoadLibrary(dllPath);
    if (!webView2Loader)
    {
        webView2Loader = LoadLibrary(_T("WebView2Loader.dll"));
    }

    if (!webView2Loader)
    {
        TRACE(_T("无法加载 WebView2Loader.dll\n"));
        return FALSE;
    }

    pfnCreateEnvironment = (CreateCoreWebView2EnvironmentWithOptionsFunc)
        GetProcAddress(webView2Loader, "CreateCoreWebView2EnvironmentWithOptions");

    return (pfnCreateEnvironment != NULL);
}

BOOL 网页对话框类::初始化WebView2()
{
    if (!动态加载WebView2())
    {
        return FALSE;
    }

    CCreateEnvironmentHandler* pHandler = new CCreateEnvironmentHandler(this);
    HRESULT hr = pfnCreateEnvironment(nullptr, nullptr, nullptr, pHandler);
    return SUCCEEDED(hr);
}

void 网页对话框类::调整WebView2大小()
{
    if (webViewController)
    {
        RECT rc;
        GetClientRect(&rc);
        rc.top += 5;
        webViewController->put_Bounds(rc);
    }
}

void 网页对话框类::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);

    // 调整关闭按钮位置（右上角）
    if (m_btnClose.GetSafeHwnd())
    {
        m_btnClose.MoveWindow(cx - 55, 15, 30, 25);     //cx = 窗口宽度
    }

    调整WebView2大小();
}

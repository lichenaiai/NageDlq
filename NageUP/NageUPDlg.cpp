#include "pch.h"
#include "framework.h"
#include "NageUP.h"
#include "NageUPDlg.h"
#include "afxdialogex.h"
#include <thread>
#include <vector>
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// NageUPDlg 对话框
IMPLEMENT_DYNAMIC(NageUPDlg, CDialogEx)

NageUPDlg::NageUPDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_UPDATER_DIALOG, pParent)
    , 正在更新(FALSE)
    , 用户取消(FALSE)
    , 更新服务器地址(UPDATE_SERVER)
{
}

NageUPDlg::~NageUPDlg()
{
}

void NageUPDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PROGRESS_UPDATE, 进度条控件);
    DDX_Control(pDX, IDC_STATIC_NOW, 当前状态标签);
    DDX_Control(pDX, IDC_STATIC_FILE, 文件名标签);
    DDX_Control(pDX, IDC_STATIC_SPEED, 下载速度标签);
    DDX_Control(pDX, IDC_STATIC_SIZE, 文件大小标签);
    DDX_Control(pDX, IDC_BUTTON_CANCEL, 取消按钮);
}

BEGIN_MESSAGE_MAP(NageUPDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BUTTON_CANCEL, &NageUPDlg::OnBnClickedButtonCancel)
    ON_WM_TIMER()
    ON_MESSAGE(WM_USER + 100, &NageUPDlg::OnUpdateProgress)
    ON_MESSAGE(WM_USER + 101, &NageUPDlg::OnUpdateDownloadInfo)
END_MESSAGE_MAP()

// 自定义消息处理函数
LRESULT NageUPDlg::OnUpdateProgress(WPARAM wParam, LPARAM lParam)
{
    int 进度 = (int)wParam;
    CString* 状态 = (CString*)lParam;

    进度条控件.SetPos(进度);

    if (状态 && !状态->IsEmpty())
    {
        当前状态标签.SetWindowText(*状态);
        delete 状态;
    }

    return 0;
}

LRESULT NageUPDlg::OnUpdateDownloadInfo(WPARAM wParam, LPARAM lParam)
{
    struct 下载信息
    {
        CString 文件名;
        CString 速度;
        CString 大小;
    };

    下载信息* 信息 = (下载信息*)wParam;
    if (信息)
    {
        文件名标签.SetWindowText(CString(_T("文件: ")) + 信息->文件名);
        下载速度标签.SetWindowText(信息->速度);
        文件大小标签.SetWindowText(信息->大小);
        delete 信息;
    }
    return 0;
}

// NageUPDlg 消息处理程序
BOOL NageUPDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    CString 窗口标题;
    窗口标题.Format(_T("NageDLQ 更新程序 - 当前版本: %s"), 当前版本号);
    SetWindowText(窗口标题);

    进度条控件.SetRange(0, 100);
    进度条控件.SetPos(0);

    当前状态标签.SetWindowText(_T("正在初始化..."));
    文件名标签.SetWindowText(_T("文件: 等待中..."));
    下载速度标签.SetWindowText(_T("速度: 0 KB/s"));
    文件大小标签.SetWindowText(_T("已下载: 0 MB / 0 MB"));

    SetTimer(1, 500, NULL);

    return TRUE;
}

void NageUPDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1)
    {
        KillTimer(1);
        开始更新流程();
    }
    else if (nIDEvent == 2)
    {
        KillTimer(2);
        CDialogEx::OnOK();
    }

    CDialogEx::OnTimer(nIDEvent);
}

void NageUPDlg::开始更新流程()
{
    if (正在更新) return;

    正在更新 = TRUE;
    用户取消 = FALSE;

    std::thread 更新线程([this]() {
        TRACE(_T("=== 更新流程开始 ===\n"));

        try
        {
            // 步骤1: 获取服务器更新信息
            if (用户取消) return;
            更新进度(10, _T("正在连接更新服务器..."));

            CString 最新版本号, 更新文件名;
            if (!获取服务器更新信息(最新版本号, 更新文件名))
            {
                AfxMessageBox(_T("无法获取服务器更新信息"), MB_ICONERROR);
                return;
            }

            // 更新窗口标题显示目标版本
            CString 窗口标题;
            窗口标题.Format(_T("NageDLQ 更新程序 - %s -> %s"), 当前版本号, 最新版本号);
            PostMessage(WM_SETTEXT, 0, (LPARAM)窗口标题.GetString());

            // 步骤2: 下载更新文件
            if (用户取消) return;
            更新进度(20, _T("正在下载更新文件..."));

            if (!下载更新文件(更新文件名))
            {
                AfxMessageBox(_T("下载更新文件失败"), MB_ICONERROR);
                return;
            }

            // 步骤3: 验证文件完整性
            if (用户取消) return;
            更新进度(80, _T("正在验证文件完整性..."));

            CString 本地文件路径;
            本地文件路径.Format(_T(".\\%s"), 更新文件名);

            if (!验证文件完整性(本地文件路径))
            {
                AfxMessageBox(_T("文件验证失败，可能文件已损坏"), MB_ICONERROR);
                DeleteFile(本地文件路径);
                return;
            }

            // 步骤4: 应用更新
            if (用户取消) return;
            更新进度(90, _T("正在应用更新..."));

            if (!应用更新(本地文件路径))
            {
                AfxMessageBox(_T("应用更新失败"), MB_ICONERROR);
                return;
            }

            // 步骤5: 完成更新
            if (用户取消) return;
            更新进度(100, _T("更新完成！"));

            完成更新();
        }
        catch (const std::exception& e)
        {
            CString 错误信息;
            错误信息.Format(_T("更新过程中发生错误: %hs"), e.what());
            更新进度(0, 错误信息);
            AfxMessageBox(错误信息, MB_ICONERROR);
        }
        catch (...)
        {
            CString 错误信息 = _T("更新过程中发生未知错误");
            更新进度(0, 错误信息);
            AfxMessageBox(错误信息, MB_ICONERROR);
        }

        TRACE(_T("=== 更新流程结束 ===\n"));
        正在更新 = FALSE;
        });

    更新线程.detach();
}

BOOL NageUPDlg::获取服务器更新信息(CString& 最新版本号, CString& 更新文件名)
{
    TRACE(_T("获取服务器更新信息\n"));

    // 只需要获取更新文件名，不需要比较版本
    // 尝试几个固定的更新文件名

    std::vector<CString> 可能文件名 = {
        _T("nageup.zip"),           // 默认文件名
        _T("update.zip"),           // 备用文件名
        _T("client_update.zip")     // 另一个备用名
    };

    // 检查文件是否存在
    for (const auto& 文件名 : 可能文件名)
    {
        if (用户取消) break;

        CString 文件URL = 构建文件URL(文件名);
        TRACE(_T("检查更新文件: %s\n"), 文件URL);

        // 检查远程文件是否存在
        HINTERNET hInternet = InternetOpen(_T("NageUpdater"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (hInternet)
        {
            HINTERNET hUrl = InternetOpenUrl(hInternet, 文件URL, NULL, 0, INTERNET_FLAG_RELOAD, 0);
            if (hUrl)
            {
                // 文件存在
                InternetCloseHandle(hUrl);
                InternetCloseHandle(hInternet);

                更新文件名 = 文件名;
                最新版本号 = 从文件名提取版本号(文件名);

                TRACE(_T("找到更新文件: %s, 显示版本: %s\n"), 更新文件名, 最新版本号);
                return TRUE;
            }
            InternetCloseHandle(hInternet);
        }
    }

    // 如果固定文件名都不存在，尝试从文件名模式查找
    return 通过文件名模式查找(最新版本号, 更新文件名);
}

BOOL NageUPDlg::通过文件名模式查找(CString& 最新版本号, CString& 更新文件名)
{
    TRACE(_T("通过文件名模式查找更新文件\n"));

    // 按照 nageup{版本号}.zip 模式查找
    // 生成一些可能的版本号组合进行尝试

    std::vector<CString> 可能版本号 = {
        _T("1.3.1"), _T("1.3.0"), _T("1.2.9"), _T("1.2.8"),
        _T("1.4.0"), _T("1.3.2"), _T("2.0.0"), _T("1.5.0")
    };

    for (const auto& 版本号 : 可能版本号)
    {
        if (用户取消) break;

        // 尝试ZIP格式
        CString 文件名;
        文件名.Format(_T("nageup%s.zip"), 版本号);
        CString 文件URL = 构建文件URL(文件名);

        TRACE(_T("尝试文件: %s\n"), 文件URL);

        HINTERNET hInternet = InternetOpen(_T("NageUpdater"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (hInternet)
        {
            HINTERNET hUrl = InternetOpenUrl(hInternet, 文件URL, NULL, 0, INTERNET_FLAG_RELOAD, 0);
            if (hUrl)
            {
                InternetCloseHandle(hUrl);
                InternetCloseHandle(hInternet);

                更新文件名 = 文件名;
                最新版本号 = 版本号;

                TRACE(_T("通过模式找到更新文件: %s\n"), 更新文件名);
                return TRUE;
            }
            InternetCloseHandle(hInternet);
        }

        // 尝试RAR格式
        文件名.Format(_T("nageup%s.rar"), 版本号);
        文件URL = 构建文件URL(文件名);

        TRACE(_T("尝试文件: %s\n"), 文件URL);

        hInternet = InternetOpen(_T("NageUpdater"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (hInternet)
        {
            HINTERNET hUrl2 = InternetOpenUrl(hInternet, 文件URL, NULL, 0, INTERNET_FLAG_RELOAD, 0);
            if (hUrl2)
            {
                InternetCloseHandle(hUrl2);
                InternetCloseHandle(hInternet);

                更新文件名 = 文件名;
                最新版本号 = 版本号;

                TRACE(_T("通过模式找到更新文件: %s\n"), 更新文件名);
                return TRUE;
            }
            InternetCloseHandle(hInternet);
        }
    }

    TRACE(_T("未找到任何更新文件\n"));
    return FALSE;
}

CString NageUPDlg::从文件名提取版本号(const CString& 文件名)
{
    TRACE(_T("从文件名提取版本号: %s\n"), 文件名);

    CString 纯文件名 = 文件名;

    // 移除扩展名
    int 点位置 = 纯文件名.ReverseFind(_T('.'));
    if (点位置 != -1)
    {
        纯文件名 = 纯文件名.Left(点位置);
    }

    // 移除 "nageup" 前缀
    if (纯文件名.Find(_T("nageup")) == 0)
    {
        纯文件名 = 纯文件名.Mid(6); // 移除 "nageup"
    }
    else if (纯文件名.Find(_T("update")) == 0)
    {
        纯文件名 = 纯文件名.Mid(6); // 移除 "update"
    }

    // 如果提取后为空，使用默认版本号
    if (纯文件名.IsEmpty())
    {
        return _T("最新版本");
    }

    TRACE(_T("提取的版本号: %s\n"), 纯文件名);
    return 纯文件名;
}

BOOL NageUPDlg::下载更新文件(const CString& 文件名)
{
    TRACE(_T("开始下载更新文件: %s\n"), 文件名);

    CString 文件URL = 构建文件URL(文件名);
    CString 本地路径 = _T(".\\") + 文件名;

    TRACE(_T("下载URL: %s\n"), 文件URL);
    TRACE(_T("本地路径: %s\n"), 本地路径);

    // 删除已存在的文件
    DeleteFile(本地路径);

    // 方法1：使用WinINet API下载（更可靠）
    更新进度(25, _T("正在连接下载服务器..."));

    BOOL 下载结果 = 使用WinINet下载文件(文件URL, 本地路径);

    if (!下载结果)
    {
        TRACE(_T("WinINet下载失败，尝试备用方法\n"));

        // 方法2：备用方案 - 使用URLDownloadToFile但修复URL
        下载结果 = 使用URLDownloadToFile下载(文件URL, 本地路径);
    }

    if (下载结果)
    {
        // 验证下载的文件
        ULONG 文件大小 = 获取文件大小(本地路径);
        if (文件大小 > 0)
        {
            CString 大小信息;
            大小信息.Format(_T("已下载: %.1f MB"), 文件大小 / 1024.0 / 1024.0);
            更新下载信息(文件名, _T("下载完成"), 大小信息);

            TRACE(_T("文件下载完成，大小: %lu 字节\n"), 文件大小);
            return TRUE;
        }
        else
        {
            TRACE(_T("下载的文件大小为0\n"));
            DeleteFile(本地路径);
            return FALSE;
        }
    }

    TRACE(_T("所有下载方法都失败\n"));
    return FALSE;
}

// 使用WinINet API下载
BOOL NageUPDlg::使用WinINet下载文件(const CString& 文件URL, const CString& 本地路径)
{
    TRACE(_T("使用WinINet下载文件\n"));

    HINTERNET hInternet = NULL;
    HINTERNET hUrl = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    BOOL 结果 = FALSE;

    try
    {
        // 初始化WinINet
        hInternet = InternetOpen(_T("NageUpdater"),
            INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

        if (!hInternet)
        {
            TRACE(_T("InternetOpen失败，错误: %d\n"), GetLastError());
            return FALSE;
        }

        // 设置超时 - 使用局部变量
        DWORD 连接超时 = 30000;  // 30秒
        DWORD 接收超时 = 30000;  // 30秒

        InternetSetOption(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT,
            &连接超时, sizeof(连接超时));
        InternetSetOption(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT,
            &接收超时, sizeof(接收超时));

        // 打开URL
        hUrl = InternetOpenUrl(hInternet, 文件URL, NULL, 0,
            INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_KEEP_CONNECTION, 0);

        if (!hUrl)
        {
            DWORD 错误 = GetLastError();
            TRACE(_T("InternetOpenUrl失败，错误: %d, URL: %s\n"), 错误, 文件URL);
            return FALSE;
        }

        // 创建本地文件
        hFile = CreateFile(本地路径, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            TRACE(_T("创建文件失败: %s\n"), 本地路径);
            return FALSE;
        }

        // 读取数据并写入文件
        DWORD 已读取 = 0;
        DWORD 已写入 = 0;
        BYTE 缓冲区[8192];
        ULONGLONG 总大小 = 0;

        while (InternetReadFile(hUrl, 缓冲区, sizeof(缓冲区), &已读取) && 已读取 > 0)
        {
            if (用户取消)
            {
                TRACE(_T("用户取消下载\n"));
                break;
            }

            WriteFile(hFile, 缓冲区, 已读取, &已写入, NULL);
            总大小 += 已读取;

            // 更新进度
            if (总大小 % (1024 * 100) == 0) // 每100KB更新一次
            {
                CString 状态;
                状态.Format(_T("正在下载... (%.1f MB)"), 总大小 / 1024.0 / 1024.0);
                更新进度(25 + (int)(总大小 / (1024 * 1024) * 2), 状态); // 添加显式类型转换
            }
        }

        if (总大小 > 0)
        {
            结果 = TRUE;
            TRACE(_T("WinINet下载成功，文件大小: %llu 字节\n"), 总大小);
        }
        else
        {
            TRACE(_T("WinINet下载失败，未读取到数据\n"));
        }
    }
    catch (...)
    {
        TRACE(_T("WinINet下载过程中发生异常\n"));
        结果 = FALSE;
    }

    // 清理资源
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (hUrl)
        InternetCloseHandle(hUrl);
    if (hInternet)
        InternetCloseHandle(hInternet);

    return 结果;
}

// 使用URLDownloadToFile下载
BOOL NageUPDlg::使用URLDownloadToFile下载(const CString& 文件URL, const CString& 本地路径)
{
    TRACE(_T("使用URLDownloadToFile下载\n"));

    // 确保URL格式正确
    CString 修复的URL = 文件URL;

    // 如果URL没有协议头，添加http://
    if (修复的URL.Find(_T("://")) == -1)
    {
        修复的URL = _T("http://") + 修复的URL;
        TRACE(_T("修复后的URL: %s\n"), 修复的URL);
    }

    // 创建下载回调（可选，用于显示进度）
    // 这里使用NULL，如果需要进度显示可以实现IBindStatusCallback接口

    HRESULT hr = URLDownloadToFile(NULL, 修复的URL, 本地路径, 0, NULL);

    TRACE(_T("URLDownloadToFile返回: 0x%X\n"), hr);

    return (hr == S_OK);
}

BOOL NageUPDlg::验证文件完整性(const CString& 文件路径)
{
    TRACE(_T("验证文件完整性: %s\n"), 文件路径);

    // 验证1: 检查文件大小
    ULONG 文件大小 = 获取文件大小(文件路径);
    if (文件大小 == 0)
    {
        TRACE(_T("文件大小为0，验证失败\n"));
        return FALSE;
    }

    // 验证2: 检查文件扩展名并验证文件头
    CString 扩展名 = 文件路径.Right(4);
    扩展名.MakeLower();

    CFile 文件;
    if (文件.Open(文件路径, CFile::modeRead))
    {
        BYTE 文件头[4];
        if (文件.Read(文件头, 4) == 4)
        {
            if (扩展名 == _T(".zip"))
            {
                // ZIP文件头: PK.. (0x50 0x4B 0x03 0x04)
                if (!(文件头[0] == 0x50 && 文件头[1] == 0x4B && 文件头[2] == 0x03 && 文件头[3] == 0x04))
                {
                    TRACE(_T("ZIP文件头验证失败\n"));
                    文件.Close();
                    return FALSE;
                }
            }
            else if (扩展名 == _T(".rar"))
            {
                // RAR文件头: Rar! (0x52 0x61 0x72 0x21)
                if (!(文件头[0] == 0x52 && 文件头[1] == 0x61 && 文件头[2] == 0x72 && 文件头[3] == 0x21))
                {
                    TRACE(_T("RAR文件头验证失败\n"));
                    文件.Close();
                    return FALSE;
                }
            }
        }
        文件.Close();
    }

    TRACE(_T("文件验证通过，大小: %lu 字节\n"), 文件大小);
    return TRUE;
}

BOOL NageUPDlg::应用更新(const CString& 文件路径)
{
    TRACE(_T("应用更新: %s\n"), 文件路径);

    // 步骤1: 解压文件
    更新进度(91, _T("正在解压更新包..."));
    if (!解压压缩文件(文件路径, _T(".\\")))
    {
        TRACE(_T("解压文件失败\n"));
        return FALSE;
    }

    // 步骤2: 删除压缩包
    更新进度(98, _T("正在清理临时文件..."));
    DeleteFile(文件路径);

    TRACE(_T("更新应用完成\n"));
    return TRUE;
}

BOOL NageUPDlg::解压压缩文件(const CString& 压缩文件路径, const CString& 解压目录)
{
    TRACE(_T("解压压缩文件: %s -> %s\n"), 压缩文件路径, 解压目录);

    CString 扩展名 = 压缩文件路径.Right(4);
    扩展名.MakeLower();

    if (扩展名 == _T(".zip"))
    {
        return 解压ZIP文件(压缩文件路径, 解压目录);
    }
    else if (扩展名 == _T(".rar"))
    {
        return 解压RAR文件(压缩文件路径, 解压目录);
    }

    return FALSE;
}

BOOL NageUPDlg::解压ZIP文件(const CString& 压缩文件路径, const CString& 解压目录)
{
    TRACE(_T("解压ZIP文件: %s -> %s\n"), 压缩文件路径, 解压目录);

    // 方法1: 使用Windows内置的压缩功能（更简单的方式）
    CoInitialize(NULL);

    BOOL 解压结果 = FALSE;
    IShellDispatch* pShell = NULL;

    VARIANT 压缩文件变量;
    VARIANT 目标文件夹变量;
    VariantInit(&压缩文件变量);
    VariantInit(&目标文件夹变量);

    HRESULT hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void**)&pShell);

    if (SUCCEEDED(hr) && pShell)
    {
        // 创建压缩文件对象
        VARIANT 压缩文件变量;
        VariantInit(&压缩文件变量);
        压缩文件变量.vt = VT_BSTR;
        压缩文件变量.bstrVal = ::SysAllocString(压缩文件路径);

        Folder* pZipFolder = NULL;
        hr = pShell->NameSpace(压缩文件变量, &pZipFolder);

        if (SUCCEEDED(hr) && pZipFolder)
        {
            // 创建目标文件夹对象
            VARIANT 目标文件夹变量;
            VariantInit(&目标文件夹变量);
            目标文件夹变量.vt = VT_BSTR;
            目标文件夹变量.bstrVal = ::SysAllocString(解压目录);

            Folder* pDestFolder = NULL;
            hr = pShell->NameSpace(目标文件夹变量, &pDestFolder);

            if (SUCCEEDED(hr) && pDestFolder)
            {
                // 获取压缩包中的所有项目
                FolderItems* pItems = NULL;
                hr = pZipFolder->Items(&pItems);

                if (SUCCEEDED(hr) && pItems)
                {
                    // 正确调用CopyHere方法
                    VARIANT 选项变量;
                    VariantInit(&选项变量);
                    选项变量.vt = VT_I4;
                    选项变量.lVal = 4 | 16 | 1024; // 不显示UI，覆盖现有文件

                    VARIANT 项目变量;
                    VariantInit(&项目变量);
                    项目变量.vt = VT_DISPATCH;
                    项目变量.pdispVal = pItems;

                    hr = pDestFolder->CopyHere(项目变量, 选项变量);

                    if (SUCCEEDED(hr))
                    {
                        解压结果 = TRUE;
                        TRACE(_T("ZIP解压成功\n"));
                    }
                    else
                    {
                        TRACE(_T("ZIP解压失败，错误码: 0x%X\n"), hr);
                    }

                    pItems->Release();
                }
                pDestFolder->Release();
            }
            pZipFolder->Release();
        }
        pShell->Release();
    }
    if (pShell)
        pShell->Release();

    CoUninitialize();

    // 如果方法1失败，尝试方法2：使用命令行
    if (!解压结果)
    {
        TRACE(_T("尝试使用命令行解压ZIP文件\n"));
        解压结果 = 使用命令行解压ZIP文件(压缩文件路径, 解压目录);
    }

    return 解压结果;
}

BOOL NageUPDlg::使用命令行解压ZIP文件(const CString& 压缩文件路径, const CString& 解压目录)
{
    // 使用PowerShell解压ZIP文件
    CString 解压命令;
    解压命令.Format(_T("powershell -command \"& {Add-Type -AssemblyName 'System.IO.Compression.FileSystem'; [System.IO.Compression.ZipFile]::ExtractToDirectory('%s', '%s');}\""),
        压缩文件路径, 解压目录);

    TRACE(_T("执行解压命令: %s\n"), 解压命令);

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // 隐藏窗口

    BOOL 创建成功 = CreateProcess(NULL, 解压命令.GetBuffer(), NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    解压命令.ReleaseBuffer();

    if (创建成功)
    {
        WaitForSingleObject(pi.hProcess, 60000); // 等待60秒
        DWORD 退出码;
        GetExitCodeProcess(pi.hProcess, &退出码);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        TRACE(_T("命令行解压退出码: %d\n"), 退出码);
        return (退出码 == 0);
    }

    return FALSE;
}

BOOL NageUPDlg::解压RAR文件(const CString& 压缩文件路径, const CString& 解压目录)
{
    TRACE(_T("解压RAR文件: %s -> %s\n"), 压缩文件路径, 解压目录);

    // 查找WinRAR安装路径
    CString WinRAR路径;
    HKEY hKey;

    // 先尝试64位WinRAR
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\WinRAR"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        TCHAR szPath[MAX_PATH];
        DWORD dwSize = sizeof(szPath);

        if (RegQueryValueEx(hKey, _T("exe64"), NULL, NULL, (LPBYTE)szPath, &dwSize) == ERROR_SUCCESS)
        {
            WinRAR路径 = szPath;
        }
        else if (RegQueryValueEx(hKey, _T("exe32"), NULL, NULL, (LPBYTE)szPath, &dwSize) == ERROR_SUCCESS)
        {
            WinRAR路径 = szPath;
        }
        RegCloseKey(hKey);
    }

    // 如果没找到，尝试当前目录下的WinRAR
    if (WinRAR路径.IsEmpty())
    {
        WinRAR路径 = _T(".\\WinRAR\\WinRAR.exe");
        if (GetFileAttributes(WinRAR路径) == INVALID_FILE_ATTRIBUTES)
        {
            WinRAR路径 = _T("WinRAR.exe");
        }
    }

    CString 解压命令;
    if (GetFileAttributes(WinRAR路径) != INVALID_FILE_ATTRIBUTES)
    {
        // 使用WinRAR解压
        解压命令.Format(_T("\"%s\" x -y -ibck \"%s\" \"%s\\\""), WinRAR路径, 压缩文件路径, 解压目录);
    }
    else
    {
        // 如果没有WinRAR，提示用户
        AfxMessageBox(_T("未找到WinRAR，无法解压RAR文件。请安装WinRAR或使用ZIP格式的更新包。"), MB_ICONERROR);
        return FALSE;
    }

    TRACE(_T("执行RAR解压命令: %s\n"), 解压命令);

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    BOOL 创建成功 = CreateProcess(NULL, 解压命令.GetBuffer(), NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    解压命令.ReleaseBuffer();

    if (创建成功)
    {
        WaitForSingleObject(pi.hProcess, 60000); // 等待60秒
        DWORD 退出码;
        GetExitCodeProcess(pi.hProcess, &退出码);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        TRACE(_T("RAR解压退出码: %d\n"), 退出码);
        return (退出码 == 0);
    }

    return FALSE;
}

void NageUPDlg::完成更新()
{
    TRACE(_T("完成更新\n"));

    当前状态标签.SetWindowText(_T("更新完成！"));
    文件名标签.SetWindowText(_T("文件: 所有文件更新完成"));
    下载速度标签.SetWindowText(_T("速度: -"));
    文件大小标签.SetWindowText(_T("更新完成"));

    取消按钮.EnableWindow(FALSE);
    取消按钮.SetWindowText(_T("完成"));

    CString 完成信息;
    完成信息.Format(_T("更新完成！\n程序将在3秒后重新启动。"));
    AfxMessageBox(完成信息, MB_ICONINFORMATION);

    SetTimer(2, 3000, NULL);
}

void NageUPDlg::OnBnClickedButtonCancel()
{
    if (正在更新)
    {
        if (AfxMessageBox(_T("确定要取消更新吗？"), MB_ICONQUESTION | MB_YESNO) == IDYES)
        {
            用户取消 = TRUE;
            当前状态标签.SetWindowText(_T("正在取消更新..."));
            取消按钮.EnableWindow(FALSE);
        }
    }
    else
    {
        CDialogEx::OnCancel();
    }
}

// 辅助函数
CString NageUPDlg::构建文件URL(const CString& 文件名)
{
    return 更新服务器地址 + 文件名;
}

ULONG NageUPDlg::获取文件大小(const CString& 文件路径)
{
    WIN32_FILE_ATTRIBUTE_DATA 文件属性;
    if (GetFileAttributesEx(文件路径, GetFileExInfoStandard, &文件属性))
    {
        return 文件属性.nFileSizeLow;
    }
    return 0;
}

void NageUPDlg::更新进度(int 进度, const CString& 状态)
{
    PostMessage(WM_USER + 100, (WPARAM)进度, (LPARAM)new CString(状态));
}

void NageUPDlg::更新下载信息(const CString& 文件名, const CString& 速度, const CString& 大小)
{
    struct 下载信息
    {
        CString 文件名;
        CString 速度;
        CString 大小;
    };

    下载信息* 信息 = new 下载信息{ 文件名, 速度, 大小 };
    PostMessage(WM_USER + 101, (WPARAM)信息, 0);
}
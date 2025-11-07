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

    // 从服务器下载版本信息文件
    CString 版本信息URL = 更新服务器地址 + _T("version.txt");
    CString 本地版本文件 = _T(".\\version_temp.txt");

    // 下载版本信息文件
    HRESULT 下载结果 = URLDownloadToFile(NULL, 版本信息URL, 本地版本文件, 0, NULL);
    if (下载结果 == S_OK)
    {
        // 读取版本信息文件
        CStdioFile 版本文件;
        if (版本文件.Open(本地版本文件, CFile::modeRead))
        {
            CString 行内容;
            if (版本文件.ReadString(行内容))
            {
                行内容.Trim();
                int 分隔符位置 = 行内容.Find(_T('|'));
                if (分隔符位置 != -1)
                {
                    最新版本号 = 行内容.Left(分隔符位置);
                    更新文件名 = 行内容.Mid(分隔符位置 + 1);

                    TRACE(_T("从版本文件获取: 版本=%s, 文件=%s\n"), 最新版本号, 更新文件名);

                    版本文件.Close();
                    DeleteFile(本地版本文件);
                    return TRUE;
                }
            }
            版本文件.Close();
        }
        DeleteFile(本地版本文件);
    }

    // 如果版本信息文件不存在，通过文件名扫描获取
    return 通过文件名扫描获取更新信息(最新版本号, 更新文件名);
}

BOOL NageUPDlg::通过文件名扫描获取更新信息(CString& 最新版本号, CString& 更新文件名)
{
    TRACE(_T("通过文件名扫描获取更新信息\n"));

    // 根据文件名规则扫描可能的更新文件
    std::vector<CString> 可能文件名;

    // 生成可能的文件名组合
    for (int 主版本 = 1; 主版本 <= 10; 主版本++)
    {
        for (int 次版本 = 0; 次版本 <= 20; 次版本++)
        {
            for (int 修订版本 = 0; 修订版本 <= 99; 修订版本++)
            {
                CString 文件名;
                文件名.Format(_T("nageup%d.%d.%d.zip"), 主版本, 次版本, 修订版本);
                可能文件名.push_back(文件名);

                文件名.Format(_T("nageup%d.%d.%d.rar"), 主版本, 次版本, 修订版本);
                可能文件名.push_back(文件名);
            }

            CString 文件名;
            文件名.Format(_T("nageup%d.%d.zip"), 主版本, 次版本);
            可能文件名.push_back(文件名);

            文件名.Format(_T("nageup%d.%d.rar"), 主版本, 次版本);
            可能文件名.push_back(文件名);
        }
    }

    // 检查文件是否存在
    for (const auto& 文件名 : 可能文件名)
    {
        CString 文件URL = 构建文件URL(文件名);

        // 尝试HEAD请求检查文件是否存在
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

                TRACE(_T("找到更新文件: %s, 版本: %s\n"), 更新文件名, 最新版本号);
                return TRUE;
            }
            InternetCloseHandle(hInternet);
        }

        if (用户取消) break;
    }

    TRACE(_T("未找到更新文件\n"));
    return FALSE;
}

CString NageUPDlg::从文件名提取版本号(const CString& 文件名)
{
    // 从文件名中提取版本号
    // 支持格式: nageup1.3.zip, nageup1.3.0.zip, nageup1.3.0.rar 等

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

    // 验证版本号格式
    for (int i = 0; i < 纯文件名.GetLength(); i++)
    {
        TCHAR c = 纯文件名[i];
        if (!((c >= _T('0') && c <= _T('9')) || c == _T('.')))
        {
            纯文件名 = 纯文件名.Left(i);
            break;
        }
    }

    if (纯文件名.IsEmpty())
    {
        return _T("未知版本");
    }

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

    // 下载文件
    更新进度(25, _T("正在连接下载服务器..."));

    // 使用 URLDownloadToFile 下载
    HRESULT 下载结果 = URLDownloadToFile(NULL, 文件URL, 本地路径, 0, NULL);

    if (下载结果 != S_OK)
    {
        TRACE(_T("文件下载失败，错误码: %d\n"), 下载结果);
        return FALSE;
    }

    // 获取实际文件大小
    ULONG 文件大小 = 获取文件大小(本地路径);
    if (文件大小 == 0)
    {
        TRACE(_T("下载的文件大小为0\n"));
        return FALSE;
    }

    // 更新下载信息
    CString 大小信息;
    大小信息.Format(_T("已下载: %.1f MB"), 文件大小 / 1024.0 / 1024.0);
    更新下载信息(文件名, _T("下载完成"), 大小信息);

    TRACE(_T("文件下载完成，大小: %lu 字节\n"), 文件大小);
    return TRUE;
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
    // 使用Windows内置的压缩功能
    // 需要 Windows 8 或更高版本，或者安装 .NET Framework

    CoInitialize(NULL);

    // 创建Shell对象
    IShellDispatch* pShell = NULL;
    HRESULT hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void**)&pShell);

    if (SUCCEEDED(hr) && pShell)
    {
        // 创建压缩文件对象
        VARIANT 压缩文件变量;
        VariantInit(&压缩文件变量);
        压缩文件变量.vt = VT_BSTR;
        压缩文件变量.bstrVal = 压缩文件路径.AllocSysString();

        Folder* pZipFolder = NULL;
        hr = pShell->NameSpace(压缩文件变量, &pZipFolder);

        if (SUCCEEDED(hr) && pZipFolder)
        {
            // 创建目标文件夹对象
            VARIANT 目标文件夹变量;
            VariantInit(&目标文件夹变量);
            目标文件夹变量.vt = VT_BSTR;
            目标文件夹变量.bstrVal = 解压目录.AllocSysString();

            Folder* pDestFolder = NULL;
            hr = pShell->NameSpace(目标文件夹变量, &pDestFolder);

            if (SUCCEEDED(hr) && pDestFolder)
            {
                // 复制所有项目到目标文件夹
                FolderItems* pItems = NULL;
                hr = pZipFolder->Items(&pItems);

                if (SUCCEEDED(hr) && pItems)
                {
                    VARIANT 选项变量;
                    VariantInit(&选项变量);
                    选项变量.vt = VT_I4;
                    选项变量.lVal = 0; // 不显示进度对话框

                    hr = pDestFolder->CopyHere(pItems, 选项变量);
                    pItems->Release();
                }

                pDestFolder->Release();
            }

            pZipFolder->Release();
        }

        VariantClear(&压缩文件变量);
        pShell->Release();
    }

    CoUninitialize();

    return SUCCEEDED(hr);
}

BOOL NageUPDlg::解压RAR文件(const CString& 压缩文件路径, const CString& 解压目录)
{
    // 对于RAR文件，使用命令行工具
    CString 解压命令;

    // 检查是否安装了WinRAR
    CString WinRAR路径;
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\WinRAR"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        TCHAR szPath[MAX_PATH];
        DWORD dwSize = sizeof(szPath);
        if (RegQueryValueEx(hKey, _T("exe64"), NULL, NULL, (LPBYTE)szPath, &dwSize) == ERROR_SUCCESS ||
            RegQueryValueEx(hKey, _T("exe32"), NULL, NULL, (LPBYTE)szPath, &dwSize) == ERROR_SUCCESS)
        {
            WinRAR路径 = szPath;
        }
        RegCloseKey(hKey);
    }

    if (!WinRAR路径.IsEmpty())
    {
        // 使用WinRAR解压
        解压命令.Format(_T("\"%s\" x -y \"%s\" \"%s\\\""), WinRAR路径, 压缩文件路径, 解压目录);
    }
    else
    {
        // 使用系统内置支持（如果有）或7-zip
        解压命令.Format(_T("powershell -command \"& {Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::ExtractToDirectory('%s', '%s');}\""),
            压缩文件路径, 解压目录);
    }

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    if (CreateProcess(NULL, 解压命令.GetBuffer(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        WaitForSingleObject(pi.hProcess, 30000); // 等待30秒
        DWORD 退出码;
        GetExitCodeProcess(pi.hProcess, &退出码);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        解压命令.ReleaseBuffer();

        return (退出码 == 0);
    }

    解压命令.ReleaseBuffer();
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
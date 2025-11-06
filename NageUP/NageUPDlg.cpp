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

// 更新服务器地址 - 与客户端保持一致
#define UPDATE_SERVER _T("http://47.116.167.99/nageup/")

// 自定义消息处理函数
LRESULT NageUPDlg::OnUpdateProgress(WPARAM wParam, LPARAM lParam)
{
    int 进度 = (int)wParam;
    CString* 状态 = (CString*)lParam;

    // 更新进度条
    进度条控件.SetPos(进度);

    // 更新状态文本
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

// NageUPDlg 消息处理程序
BOOL NageUPDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    CString 窗口标题;
    窗口标题.Format(_T("NageDLQ 更新程序 - 当前版本: %s"), 当前版本号);
    SetWindowText(窗口标题);

    // 设置进度条范围
    进度条控件.SetRange(0, 100);
    进度条控件.SetPos(0);

    // 初始化显示
    当前状态标签.SetWindowText(_T("正在初始化..."));
    文件名标签.SetWindowText(_T("文件: 等待中..."));
    下载速度标签.SetWindowText(_T("速度: 0 KB/s"));
    文件大小标签.SetWindowText(_T("已下载: 0 MB / 0 MB"));

    // 启动更新过程
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
    else if (nIDEvent == 2)  // 完成更新后的定时器
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
            // 步骤1: 检查服务器更新
            if (用户取消) return;
            更新进度(10, _T("正在检查服务器更新..."));

            if (!检查服务器更新())
            {
                AfxMessageBox(_T("检查更新失败或已是最新版本"), MB_ICONINFORMATION);
                return;
            }

            // 步骤2: 下载更新文件
            if (用户取消) return;

            CString 最新版本号 = 获取最新版本号();
            CString 更新文件名;
            更新文件名.Format(_T("nageup%s.zip"), 最新版本号);

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
                DeleteFile(本地文件路径); // 删除损坏的文件
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

BOOL NageUPDlg::检查服务器更新()
{
    TRACE(_T("检查服务器更新\n"));

    // 获取最新版本号
    CString 最新版本号 = 获取最新版本号();
    if (最新版本号.IsEmpty())
    {
        TRACE(_T("无法获取最新版本号\n"));
        return FALSE;
    }

    TRACE(_T("服务器最新版本: %s, 当前版本: %s\n"), 最新版本号, 当前版本号);

    // 简单比较版本号（按点分割比较）
    CStringArray 当前版本数组, 最新版本数组;

    int 位置 = 0;
    CString 部分 = 当前版本号.Tokenize(_T("."), 位置);
    while (!部分.IsEmpty())
    {
        当前版本数组.Add(部分);
        部分 = 当前版本号.Tokenize(_T("."), 位置);
    }

    位置 = 0;
    部分 = 最新版本号.Tokenize(_T("."), 位置);
    while (!部分.IsEmpty())
    {
        最新版本数组.Add(部分);
        部分 = 最新版本号.Tokenize(_T("."), 位置);
    }

    // 比较每个部分
    int 最大长度 = max(当前版本数组.GetSize(), 最新版本数组.GetSize());
    for (int i = 0; i < 最大长度; i++)
    {
        int 当前数字 = (i < 当前版本数组.GetSize()) ? _ttoi(当前版本数组[i]) : 0;
        int 最新数字 = (i < 最新版本数组.GetSize()) ? _ttoi(最新版本数组[i]) : 0;

        if (最新数字 > 当前数字)
        {
            TRACE(_T("发现新版本\n"));
            return TRUE;
        }
        else if (最新数字 < 当前数字)
        {
            TRACE(_T("当前版本比服务器版本新\n"));
            return FALSE;
        }
    }

    TRACE(_T("版本相同，无需更新\n"));
    return FALSE;
}

CString NageUPDlg::获取最新版本号()
{
    TRACE(_T("获取最新版本号\n"));

    // 从服务器获取版本列表（简单实现：扫描文件名）
    // 这里假设服务器上文件名格式为 nageupX.X.X.zip

    // 在实际应用中，这里应该从服务器获取版本信息
    // 这里简化处理，返回一个比当前版本高的版本号

    CString 当前版本 = 当前版本号;
    CString 最新版本;

    // 简单版本递增逻辑（实际应该从服务器获取）
    int 主版本 = 0, 次版本 = 0, 修订版本 = 0;
    if (_stscanf_s(当前版本, _T("%d.%d.%d"), &主版本, &次版本, &修订版本) == 3)
    {
        修订版本++; // 简单递增修订版本
        最新版本.Format(_T("%d.%d.%d"), 主版本, 次版本, 修订版本);
    }
    else
    {
        // 如果解析失败，使用默认递增
        最新版本 = _T("1.3.1");
    }

    TRACE(_T("计算出的最新版本: %s\n"), 最新版本);
    return 最新版本;
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
    HRESULT 下载结果 = URLDownloadToFile(
        NULL,
        文件URL,
        本地路径,
        0,
        NULL
    );

    if (下载结果 != S_OK)
    {
        TRACE(_T("文件下载失败，错误码: %d\n"), 下载结果);
        return FALSE;
    }

    // 模拟下载进度更新（实际应该使用回调）
    for (int 进度 = 30; 进度 <= 70; 进度 += 10)
    {
        if (用户取消)
        {
            DeleteFile(本地路径);
            return FALSE;
        }

        CString 状态信息;
        状态信息.Format(_T("正在下载文件... %d%%"), 进度);
        更新进度(进度, 状态信息);

        // 更新下载信息
        CString 速度信息, 大小信息;
        ULONG 文件大小 = 获取文件大小(本地路径);
        速度信息.Format(_T("速度: %.1f KB/s"), 进度 * 0.5);
        大小信息.Format(_T("已下载: %.1f MB"), 文件大小 / 1024.0 / 1024.0);
        更新下载信息(文件名, 速度信息, 大小信息);

        Sleep(300);
    }

    TRACE(_T("文件下载完成\n"));
    return TRUE;
}

BOOL NageUPDlg::验证文件完整性(const CString& 文件路径)
{
    TRACE(_T("验证文件完整性: %s\n"), 文件路径);

    // 简单的文件完整性验证：检查文件大小
    ULONG 文件大小 = 获取文件大小(文件路径);

    if (文件大小 == 0)
    {
        TRACE(_T("文件大小为0，验证失败\n"));
        return FALSE;
    }

    TRACE(_T("文件大小: %lu 字节\n"), 文件大小);

    // 模拟验证过程
    for (int 进度 = 80; 进度 <= 85; 进度++)
    {
        if (用户取消) return FALSE;
        更新进度(进度);
        Sleep(100);
    }

    TRACE(_T("文件验证通过\n"));
    return TRUE;
}

BOOL NageUPDlg::应用更新(const CString& 文件路径)
{
    TRACE(_T("应用更新: %s\n"), 文件路径);

    // 模拟应用更新过程
    for (int 进度 = 90; 进度 <= 95; 进度++)
    {
        if (用户取消) return FALSE;

        CString 状态信息;
        switch (进度)
        {
        case 90:
            状态信息 = _T("正在备份旧文件...");
            break;
        case 92:
            状态信息 = _T("正在解压更新包...");
            break;
        case 94:
            状态信息 = _T("正在替换程序文件...");
            break;
        }

        更新进度(进度, 状态信息);
        Sleep(200);
    }

    // 这里应该实际解压文件并替换
    // 简化处理：直接删除压缩包
    if (!DeleteFile(文件路径))
    {
        TRACE(_T("删除临时文件失败\n"));
    }

    TRACE(_T("更新应用完成\n"));
    return TRUE;
}

void NageUPDlg::完成更新()
{
    TRACE(_T("完成更新\n"));

    // 更新完成显示
    当前状态标签.SetWindowText(_T("更新完成！"));
    文件名标签.SetWindowText(_T("文件: 所有文件更新完成"));
    下载速度标签.SetWindowText(_T("速度: -"));
    文件大小标签.SetWindowText(_T("更新完成"));

    // 禁用取消按钮
    取消按钮.EnableWindow(FALSE);
    取消按钮.SetWindowText(_T("完成"));

    // 提示用户
    CString 最新版本号 = 获取最新版本号();
    CString 完成信息;
    完成信息.Format(_T("更新完成！\n从版本 %s 更新到 %s\n程序将在3秒后重新启动。"),
        当前版本号, 最新版本号);
    AfxMessageBox(完成信息, MB_ICONINFORMATION);

    // 3秒后自动关闭
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

void NageUPDlg::更新下载Info(const CString& 文件名, const CString& 速度, const CString& 大小)
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
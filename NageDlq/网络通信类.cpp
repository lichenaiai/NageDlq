#include "pch.h"
#include "网络通信类.h"
#include "NageDlqDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(网络通信类, CAsyncSocket)

// 网络通信类 构造函数
网络通信类::网络通信类()
    : 服务端端口(0)
    , 连接状态(FALSE)
    , 消息回调函数(nullptr)
    , 回调窗口指针(nullptr)
{
}

// 网络通信类 析构函数
网络通信类::~网络通信类()
{
    关闭连接();
}

// 连接服务端
BOOL 网络通信类::连接服务端(const CString& 地址, UINT 端口)
{
    // 如果已连接，先关闭
    if (是否已连接())
    {
        关闭连接();
    }

    服务端地址 = 地址;
    服务端端口 = 端口;

    TRACE(_T("开始创建Socket\n"));

    // 创建socket
    if (!Create())
    {
        TRACE(_T("创建Socket失败\n"));
        return FALSE;
    }

    TRACE(_T("Socket创建成功，开始连接: %s:%d\n"), 地址, 端口);

    // 连接到服务端
    if (!Connect(地址, 端口))
    {
        int 错误码 = GetLastError();
        if (错误码 != WSAEWOULDBLOCK)
        {
            TRACE(_T("连接服务端失败，错误码: %d\n"), 错误码);
            return FALSE;
        }
        else
        {
            TRACE(_T("连接操作正在进行(非阻塞)\n"));
            return TRUE; // 非阻塞连接，返回TRUE等待OnConnect回调
        }
    }

    TRACE(_T("连接立即成功\n"));
    return TRUE;
}

// 发送数据到服务端
BOOL 网络通信类::发送数据(const CString& 数据)
{
    // 等待连接建立
    int 等待次数 = 0;
    while (!是否已连接() && 等待次数 < 50) // 最多等待5秒
    {
        Sleep(100);
        等待次数++;
    }

    if (!是否已连接())
    {
        TRACE(_T("未连接到服务端，无法发送数据\n"));
        return FALSE;
    }

    // 添加换行符作为结束标记
    CString 发送数据 = 数据 + _T("\n");

    TRACE(_T("准备发送数据: %s\n"), 发送数据);

    // 转换为UTF-8
    int 字节长度 = WideCharToMultiByte(CP_UTF8, 0, 发送数据, -1, NULL, 0, NULL, NULL);
    if (字节长度 <= 0)
    {
        TRACE(_T("转换数据到UTF-8失败\n"));
        return FALSE;
    }

    char* 字节缓冲区 = new char[字节长度];
    WideCharToMultiByte(CP_UTF8, 0, 发送数据, -1, 字节缓冲区, 字节长度, NULL, NULL);

    // 发送数据
    int 发送结果 = Send(字节缓冲区, 字节长度 - 1);  // -1 去掉null终止符

    delete[] 字节缓冲区;

    if (发送结果 == SOCKET_ERROR)
    {
        TRACE(_T("发送数据失败\n"));
        return FALSE;
    }

    TRACE(_T("发送数据成功，发送字节数: %d\n"), 发送结果);
    return TRUE;
}

// 关闭连接
void 网络通信类::关闭连接()
{
    if (是否已连接())
    {
        ShutDown(2);  // 停止发送和接收
        Close();
    }

    连接状态 = FALSE;
}

// 检查连接状态
BOOL 网络通信类::是否已连接() const
{
    return 连接状态 && (m_hSocket != INVALID_SOCKET);
}

// 设置消息回调函数
/*
void 网络通信类::设置消息回调函数(消息回调函数类型 回调函数, CWnd* 窗口指针)
{
    消息回调函数 = 回调函数;
    回调窗口指针 = 窗口指针;
}
*/
void 网络通信类::设置消息回调函数(消息回调函数类型 回调函数, NageDlqDlg* 窗口指针)
{
    消息回调函数 = 回调函数;
    回调窗口指针 = 窗口指针;
}

// Socket连接事件
void 网络通信类::OnConnect(int 错误代码)
{
    if (错误代码 == 0)
    {
        连接状态 = TRUE;
        TRACE(_T("成功连接到服务端\n"));

        // 连接成功后立即发送连接请求
        CString 连接请求;
        连接请求.Format(_T("CONNECT:1.0.0:127.0.0.1\n"));

        TRACE(_T("连接成功，发送连接请求: %s\n"), 连接请求);

        if (发送数据(连接请求))
        {
            TRACE(_T("连接请求发送成功\n"));
        }
        else
        {
            TRACE(_T("连接请求发送失败\n"));
        }

        // 通知主窗口连接成功
        if (回调窗口指针 && 消息回调函数)
        {
            CString* p消息 = new CString(_T("CONNECT_SUCCESS"));
            ::PostMessage(回调窗口指针->GetSafeHwnd(), WM_USER + 100, 0, (LPARAM)p消息);
        }
    }
    else
    {
        TRACE(_T("连接失败，错误代码: %d\n"), 错误代码);
        连接状态 = FALSE;

        if (回调窗口指针 && 消息回调函数)
        {
            CString* p消息 = new CString(_T("CONNECT_FAILED"));
            ::PostMessage(回调窗口指针->GetSafeHwnd(), WM_USER + 100, 0, (LPARAM)p消息);
        }
    }

    CAsyncSocket::OnConnect(错误代码);
}

// Socket接收数据事件
void 网络通信类::OnReceive(int 错误代码)
{
    if (错误代码 != 0)
    {
        TRACE(_T("OnReceive错误代码: %d\n"), 错误代码);
        CAsyncSocket::OnReceive(错误代码);
        return;
    }

    const int 缓冲区大小 = 1024;
    char 字节缓冲区[缓冲区大小];
    int 接收长度;

    do
    {
        接收长度 = Receive(字节缓冲区, 缓冲区大小 - 1);
        if (接收长度 == SOCKET_ERROR)
        {
            int 错误 = GetLastError();
            if (错误 != WSAEWOULDBLOCK)
            {
                TRACE(_T("接收数据错误: %d\n"), 错误);
            }
            break;
        }
        else if (接收长度 > 0)
        {
            字节缓冲区[接收长度] = '\0';
            TRACE(_T("接收到原始字节数据，长度: %d\n"), 接收长度);

            // 调试输出字节内容
            TRACE(_T("字节内容: "));
            for (int i = 0; i < 接收长度; i++) {
                TRACE(_T("%02x "), (unsigned char)字节缓冲区[i]);
            }
            TRACE(_T("\n"));

            // 尝试多种编码转换
            CString 解析结果;

            // 先尝试UTF-8
            int 宽字符长度 = MultiByteToWideChar(CP_UTF8, 0, 字节缓冲区, 接收长度, NULL, 0);
            if (宽字符长度 > 0)
            {
                wchar_t* 宽字符缓冲区 = new wchar_t[宽字符长度 + 1];
                MultiByteToWideChar(CP_UTF8, 0, 字节缓冲区, 接收长度, 宽字符缓冲区, 宽字符长度);
                宽字符缓冲区[宽字符长度] = L'\0';
                解析结果 = CString(宽字符缓冲区);
                delete[] 宽字符缓冲区;
                TRACE(_T("UTF-8解析结果: %s\n"), 解析结果);
            }
            else
            {
                // 尝试ANSI
                解析结果 = CString(字节缓冲区);
                TRACE(_T("ANSI解析结果: %s\n"), 解析结果);
            }

            if (!解析结果.IsEmpty())
            {
                接收缓冲区 += 解析结果;
                TRACE(_T("添加到接收缓冲区: %s\n"), 解析结果);
                解析接收数据(接收缓冲区);
            }
        }
    } while (接收长度 == 缓冲区大小 - 1);

    CAsyncSocket::OnReceive(错误代码);
}

// 解析接收到的数据
void 网络通信类::解析接收数据(const CString& 数据)
{
    TRACE(_T("开始解析接收数据: %s\n"), 数据);

    CString 临时数据 = 数据;
    int 换行位置;

    while ((换行位置 = 临时数据.Find(_T('\n'))) != -1)
    {
        CString 单条数据 = 临时数据.Left(换行位置);
        临时数据 = 临时数据.Mid(换行位置 + 1);

        单条数据.TrimRight(_T("\r"));   //去除可能的回车

        TRACE(_T("解析到单条数据: %s\n"), 单条数据);

        if (!单条数据.IsEmpty())
        {
            // 回调到主窗口处理
            if (回调窗口指针 && 消息回调函数)
            {
                TRACE(_T("准备发送消息到主窗口: %s\n"), 单条数据);
                CString* p消息数据 = new CString(单条数据);
                ::PostMessage(回调窗口指针->GetSafeHwnd(), WM_USER + 100, 0, (LPARAM)p消息数据);
            }
        }
    }

    // 如果没有换行符，直接处理整个数据
    if (临时数据 == 数据 && !临时数据.IsEmpty())
    {
        TRACE(_T("没有换行符，直接处理: %s\n"), 临时数据);
        if (回调窗口指针 && 消息回调函数)
        {
            CString* p消息数据 = new CString(临时数据);
            ::PostMessage(回调窗口指针->GetSafeHwnd(), WM_USER + 100, 0, (LPARAM)p消息数据);
        }
        临时数据.Empty();
    }

    // 保存剩余数据
    接收缓冲区 = 临时数据;
    TRACE(_T("解析完成，剩余缓冲区: %s\n"), 接收缓冲区);
}

// Socket关闭事件
void 网络通信类::OnClose(int 错误代码)
{
    TRACE(_T("连接已关闭\n"));
    连接状态 = FALSE;

    if (回调窗口指针 && 消息回调函数)
    {
        (回调窗口指针->*消息回调函数)(_T("CONNECTION_CLOSED")); //去除/T前的CString
    }

    CAsyncSocket::OnClose(错误代码);
}

// Socket发送事件
void 网络通信类::OnSend(int 错误代码)
{
    // 可以在这里处理发送完成后的逻辑
    CAsyncSocket::OnSend(错误代码);
}
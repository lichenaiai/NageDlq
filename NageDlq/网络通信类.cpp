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

    // 创建socket
    if (!Create())
    {
        TRACE(_T("创建Socket失败\n"));
        return FALSE;
    }

    // 连接到服务端
    if (!Connect(地址, 端口))
    {
        TRACE(_T("连接服务端失败\n"));
        return FALSE;
    }

    return TRUE;
}

// 发送数据到服务端
BOOL 网络通信类::发送数据(const CString& 数据)
{
    if (!是否已连接())
    {
        TRACE(_T("未连接到服务端，无法发送数据\n"));
        return FALSE;
    }

    // 添加换行符作为结束标记
    CString 发送数据 = 数据 + _T("\n");

    // 发送数据
    int 发送结果 = Send((LPCTSTR)发送数据, 发送数据.GetLength() * sizeof(TCHAR));

    if (发送结果 == SOCKET_ERROR)
    {
        TRACE(_T("发送数据失败\n"));
        return FALSE;
    }

    TRACE(_T("发送数据: %s\n"), 发送数据);
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
void 网络通信类::设置消息回调函数(消息回调函数类型 回调函数, CWnd* 窗口指针)
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

        // 通知主窗口连接成功
        if (回调窗口指针 && 消息回调函数)
        {
            (回调窗口指针->*消息回调函数)(CString(_T("CONNECT_SUCCESS")));
        }
    }
    else
    {
        TRACE(_T("连接失败，错误代码: %d\n"), 错误代码);

        if (回调窗口指针 && 消息回调函数)
        {
            (回调窗口指针->*消息回调函数)(CString(_T("CONNECT_FAILED")));
        }
    }

    CAsyncSocket::OnConnect(错误代码);
}

// Socket接收数据事件
void 网络通信类::OnReceive(int 错误代码)
{
    if (错误代码 != 0)
    {
        TRACE(_T("接收数据错误，错误代码: %d\n"), 错误代码);
        CAsyncSocket::OnReceive(错误代码);
        return;
    }

    // 接收数据
    const int 缓冲区大小 = 1024;
    TCHAR 缓冲区[缓冲区大小];
    int 接收长度;

    do
    {
        接收长度 = Receive(缓冲区, 缓冲区大小 - 1);
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
            缓冲区[接收长度] = _T('\0');
            接收缓冲区 += 缓冲区;

            // 解析接收到的数据
            解析接收数据(接收缓冲区);
        }
    } while (接收长度 == 缓冲区大小 - 1);

    CAsyncSocket::OnReceive(错误代码);
}

// 解析接收到的数据
void 网络通信类::解析接收数据(const CString& 数据)
{
    int 起始位置 = 0;
    CString 临时数据 = 数据;

    // 按换行符分割数据
    int 换行位置;
    while ((换行位置 = 临时数据.Find(_T('\n'))) != -1)
    {
        CString 单条数据 = 临时数据.Left(换行位置);
        临时数据 = 临时数据.Mid(换行位置 + 1);

        // 处理单条数据
        if (!单条数据.IsEmpty())
        {
            TRACE(_T("接收到数据: %s\n"), 单条数据);

            // 回调到主窗口处理
            if (回调窗口指针 && 消息回调函数)
            {
                (回调窗口指针->*消息回调函数)(单条数据);
            }
        }
    }

    // 更新接收缓冲区
    接收缓冲区 = 临时数据;
}

// Socket关闭事件
void 网络通信类::OnClose(int 错误代码)
{
    TRACE(_T("连接已关闭\n"));
    连接状态 = FALSE;

    if (回调窗口指针 && 消息回调函数)
    {
        (回调窗口指针->*消息回调函数)(CString(_T("CONNECTION_CLOSED")));
    }

    CAsyncSocket::OnClose(错误代码);
}

// Socket发送事件
void 网络通信类::OnSend(int 错误代码)
{
    // 可以在这里处理发送完成后的逻辑
    CAsyncSocket::OnSend(错误代码);
}
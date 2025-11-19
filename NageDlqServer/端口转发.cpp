//端口转发.cpp
#include "pch.h"
#include "端口转发.h"
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

端口转发管理类::端口转发管理类()
{
}

端口转发管理类::~端口转发管理类()
{
    停止所有转发();

    for (auto* 规则 : 转发规则列表) {
        delete 规则;
    }
    转发规则列表.clear();
}

// 启动所有转发规则
BOOL 端口转发管理类::启动所有转发()
{
    std::vector<端口转发规则*> 临时规则列表;

    {
        std::lock_guard<std::mutex> 锁(规则列表锁);
        临时规则列表 = 转发规则列表; // 获取副本
    }

    BOOL 全部成功 = TRUE;
    for (auto* 规则 : 临时规则列表)
    {
        if (!规则->运行中)
        {
            if (!启动转发规则(规则->序号))
            {
                全部成功 = FALSE;
            }
        }
    }

    return 全部成功;
}

// 停止所有转发规则
BOOL 端口转发管理类::停止所有转发()
{
    std::lock_guard<std::mutex> 锁(规则列表锁);

    try
    {
        for (auto* 规则 : 转发规则列表)
        {
            if (规则->运行中)
            {
                // 设置停止标志
                规则->运行中 = false;

                // 关闭监听套接字
                if (规则->监听套接字 != INVALID_SOCKET)
                {
                    closesocket(规则->监听套接字);
                    规则->监听套接字 = INVALID_SOCKET;
                }

                // 等待线程结束
                if (规则->转发线程 && 规则->转发线程->joinable())
                {
                    规则->转发线程->join();
                }

                // 释放线程对象
                if (规则->转发线程)
                {
                    delete 规则->转发线程;
                    规则->转发线程 = nullptr;
                }

                规则->状态 = _T("已停止");
            }
        }

        return TRUE;
    }
    catch (const std::exception& e)
    {
        TRACE(_T("停止所有转发时发生异常: %s\n"), CString(e.what()));
    }

    return TRUE;
}

// 启动单个转发规则
BOOL 端口转发管理类::启动转发规则(int 规则序号)
{
    // 先找到规则，不持有锁
    端口转发规则* 目标规则 = nullptr;
    {
        std::lock_guard<std::mutex> 锁(规则列表锁);
        for (auto* 规则 : 转发规则列表)
        {
            if (规则->序号 == 规则序号)
            {
                目标规则 = 规则;
                break;
            }
        }
    }

    if (!目标规则) return FALSE;
    if (目标规则->运行中) return TRUE;

    try
    {
        // 创建监听socket
        目标规则->监听套接字 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (目标规则->监听套接字 == INVALID_SOCKET)
        {
            TRACE(_T("创建socket失败，错误码: %d\n"), WSAGetLastError());
            return FALSE;
        }

        // 设置socket选项
        int 选项值 = 1;
        setsockopt(目标规则->监听套接字, SOL_SOCKET, SO_REUSEADDR, (char*)&选项值, sizeof(选项值));

        // 绑定地址
        sockaddr_in 服务器地址;
        服务器地址.sin_family = AF_INET;

        // 转换IP地址
        USES_CONVERSION;
        char* 输入IP = T2A(目标规则->输入IP);
        if (inet_pton(AF_INET, 输入IP, &(服务器地址.sin_addr)) != 1)
        {
            TRACE(_T("IP地址转换失败: %s\n"), 目标规则->输入IP);
            closesocket(目标规则->监听套接字);
            目标规则->监听套接字 = INVALID_SOCKET;
            return FALSE;
        }

        服务器地址.sin_port = htons(目标规则->输入端口);

        if (bind(目标规则->监听套接字, (sockaddr*)&服务器地址, sizeof(服务器地址)) == SOCKET_ERROR)
        {
            TRACE(_T("绑定端口失败，错误码: %d\n"), WSAGetLastError());
            closesocket(目标规则->监听套接字);
            目标规则->监听套接字 = INVALID_SOCKET;
            return FALSE;
        }

        // 开始监听
        if (listen(目标规则->监听套接字, 10) == SOCKET_ERROR)
        {
            TRACE(_T("监听失败，错误码: %d\n"), WSAGetLastError());
            closesocket(目标规则->监听套接字);
            目标规则->监听套接字 = INVALID_SOCKET;
            return FALSE;
        }

        // 设置非阻塞模式
        u_long 非阻塞模式 = 1;
        ioctlsocket(目标规则->监听套接字, FIONBIO, &非阻塞模式);

        // 启动转发线程
        目标规则->运行中 = true;
        目标规则->状态 = _T("运行中");
        目标规则->转发线程 = new std::thread(转发线程函数, 目标规则);

        TRACE(_T("端口转发规则 %d 启动成功: %s:%d -> %s:%d\n"),
            目标规则->序号, 目标规则->输入IP, 目标规则->输入端口,
            目标规则->输出IP, 目标规则->输出端口);

        return TRUE;
    }
    catch (const std::exception& e)
    {
        TRACE(_T("启动转发规则时发生异常: %s\n"), CString(e.what()));

        // 清理资源
        if (目标规则->监听套接字 != INVALID_SOCKET)
        {
            closesocket(目标规则->监听套接字);
            目标规则->监听套接字 = INVALID_SOCKET;
        }
        目标规则->运行中 = false;
        目标规则->状态 = _T("启动失败");

        return FALSE;
    }
}

// 停止单个转发规则
BOOL 端口转发管理类::停止转发规则(int 规则序号)
{
    std::lock_guard<std::mutex> 锁(规则列表锁);

    for (auto* 规则 : 转发规则列表)
    {
        if (规则->序号 == 规则序号 && 规则->运行中)
        {
            规则->运行中 = false;
            规则->状态 = _T("已停止");

            if (规则->监听套接字 != INVALID_SOCKET)
            {
                closesocket(规则->监听套接字);
                规则->监听套接字 = INVALID_SOCKET;
            }

            if (规则->转发线程 && 规则->转发线程->joinable())
            {
                规则->转发线程->join();
                delete 规则->转发线程;
                规则->转发线程 = nullptr;
            }

            TRACE(_T("端口转发规则 %d 已停止\n"), 规则序号);
            return TRUE;
        }
    }

    return FALSE;
}

// 添加转发规则
BOOL 端口转发管理类::添加转发规则(const CString& 输入IP, int 输入端口, const CString& 输出IP, int 输出端口)
{
    std::lock_guard<std::mutex> 锁(规则列表锁);

    TRACE(_T("=== 添加转发规则开始 ===\n"));
    TRACE(_T("参数: %s:%d -> %s:%d\n"), 输入IP, 输入端口, 输出IP, 输出端口);

    auto* 新规则 = new 端口转发规则();
    新规则->序号 = static_cast<int>(转发规则列表.size()) + 1;
    新规则->输入IP = 输入IP;
    新规则->输入端口 = 输入端口;
    新规则->输出IP = 输出IP;
    新规则->输出端口 = 输出端口;
    新规则->状态 = _T("已停止");

    转发规则列表.push_back(新规则);

    TRACE(_T("添加转发规则成功，新规则序号: %d\n"), 新规则->序号);
    TRACE(_T("当前规则总数: %d\n"), 转发规则列表.size());
    TRACE(_T("=== 添加转发规则结束 ===\n"));

    return TRUE;
}

// 删除转发规则
BOOL 端口转发管理类::删除转发规则(int 规则序号)
{
    std::lock_guard<std::mutex> 锁(规则列表锁);

    for (auto it = 转发规则列表.begin(); it != 转发规则列表.end(); ++it)
    {
        if ((*it)->序号 == 规则序号)
        {
            // 如果规则正在运行，先停止
            if ((*it)->运行中)
            {
                停止转发规则(规则序号);
            }

            delete* it;
            转发规则列表.erase(it);

            TRACE(_T("删除转发规则: %d\n"), 规则序号);
            return TRUE;
        }
    }
    return FALSE;
}

// 更新转发规则
BOOL 端口转发管理类::更新转发规则(int 规则序号, const CString& 输入IP, int 输入端口,
    const CString& 输出IP, int 输出端口)
{
    std::lock_guard<std::mutex> 锁(规则列表锁);

    for (auto* 规则 : 转发规则列表)
    {
        if (规则->序号 == 规则序号)
        {
            // 如果规则正在运行，先停止再更新
            BOOL 正在运行 = 规则->运行中;

            if (正在运行)
            {
                停止转发规则(规则序号);
            }

            规则->输入IP = 输入IP;
            规则->输入端口 = 输入端口;
            规则->输出IP = 输出IP;
            规则->输出端口 = 输出端口;

            // 如果之前是运行状态，重新启动
            if (正在运行)
            {
                启动转发规则(规则序号);
            }
            return TRUE;
        }
    }
    return FALSE;
}

// 获取规则列表
std::vector<端口转发规则*> 端口转发管理类::获取规则列表() const
{
    std::lock_guard<std::mutex> 锁(规则列表锁);
    return 转发规则列表; // 返回副本
}


// 转发线程函数
void 端口转发管理类::转发线程函数(端口转发规则* 规则)
{
    TRACE(_T("=== 转发线程启动 === 规则: %d\n"), 规则->序号);

    while (规则->运行中)
    {
        sockaddr_in 客户端地址;
        int 客户端地址长度 = sizeof(客户端地址);

        SOCKET 客户端套接字 = accept(规则->监听套接字, (sockaddr*)&客户端地址, &客户端地址长度);

        if (客户端套接字 != INVALID_SOCKET)
        {
            // 解析客户端IP
            char 客户端IP缓冲区[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(客户端地址.sin_addr), 客户端IP缓冲区, sizeof(客户端IP缓冲区));

            TRACE(_T("规则 %d 接受客户端连接: %s\n"), 规则->序号, CString(客户端IP缓冲区));

            // 连接到目标服务器
            SOCKET 目标套接字 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (目标套接字 != INVALID_SOCKET)
            {
                sockaddr_in 目标地址;
                目标地址.sin_family = AF_INET;

                // 使用宽字符转换
                USES_CONVERSION;
                char* 输出IP = T2A(规则->输出IP);
                inet_pton(AF_INET, 输出IP, &(目标地址.sin_addr));
                目标地址.sin_port = htons(规则->输出端口);

                if (connect(目标套接字, (sockaddr*)&目标地址, sizeof(目标地址)) == 0)
                {
                    // 连接成功，启动客户端处理线程
                    {
                        std::lock_guard<std::mutex> 锁(规则->连接数锁);
                        规则->连接数++;
                    }

                    // 使用detach来避免线程管理问题
                    std::thread(客户端处理线程, 客户端套接字, 目标套接字, 规则).detach();
                }
                else
                {
                    TRACE(_T("规则 %d 连接到目标服务器失败: %s:%d\n"),
                        规则->序号, 规则->输出IP, 规则->输出端口);
                    closesocket(目标套接字);
                    closesocket(客户端套接字);
                }
            }
            else
            {
                TRACE(_T("规则 %d 创建目标套接字失败\n"), 规则->序号);
                closesocket(客户端套接字);
            }
        }
        else
        {
            // accept失败，可能是非阻塞模式或错误
            int 错误码 = WSAGetLastError();
            if (错误码 != WSAEWOULDBLOCK)
            {
                TRACE(_T("规则 %d accept失败，错误码: %d\n"), 规则->序号, 错误码);
            }
            Sleep(100);
        }
    }

    TRACE(_T("=== 转发线程退出 === 规则: %d\n"), 规则->序号);
}

// 客户端处理线程
void 端口转发管理类::客户端处理线程(SOCKET 客户端套接字, SOCKET 目标套接字, 端口转发规则* 规则)
{
    TRACE(_T("=== 客户端处理线程启动 === 规则: %d\n"), 规则->序号);

    // 创建两个线程分别处理两个方向的数据转发
    std::thread 客户端到目标线程(转发数据, 客户端套接字, 目标套接字);
    std::thread 目标到客户端线程(转发数据, 目标套接字, 客户端套接字);

    // 等待两个线程结束
    if (客户端到目标线程.joinable())
        客户端到目标线程.join();

    if (目标到客户端线程.joinable())
        目标到客户端线程.join();

    // 关闭连接
    closesocket(客户端套接字);
    closesocket(目标套接字);

    // 减少连接数
    {
        std::lock_guard<std::mutex> 锁(规则->连接数锁);
        规则->连接数--;
    }

    TRACE(_T("=== 客户端处理线程退出 === 规则: %d\n"), 规则->序号);
}

// 数据转发函数
void 端口转发管理类::转发数据(SOCKET 来源套接字, SOCKET 目标套接字)
{
    char 缓冲区[4096];

    while (true)
    {
        int 接收长度 = recv(来源套接字, 缓冲区, sizeof(缓冲区), 0);

        if (接收长度 > 0)
        {
            // 转发数据
            int 发送长度 = send(目标套接字, 缓冲区, 接收长度, 0);

            if (发送长度 == SOCKET_ERROR)
            {
                break;
            }
        }
        else if (接收长度 == 0)
        {
            // 连接正常关闭
            TRACE(_T("连接正常关闭\n"));
            break;
        }
        else
        {
            // 接收错误
            int 错误码 = WSAGetLastError();
            if (错误码 != WSAEWOULDBLOCK)
            {
                break;
            }
        }
    }
}

// 保存配置到注册表
BOOL 端口转发管理类::保存配置()
{
    std::lock_guard<std::mutex> 锁(规则列表锁);

    HKEY hKey;
    LONG lResult = RegCreateKeyEx(HKEY_CURRENT_USER,
        _T("Software\\NageServer\\PortForward"),
        0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);

    if (lResult == ERROR_SUCCESS)
    {
        CString 配置数据;

        for (const auto* 规则 : 转发规则列表)
        {
            CString 单条规则;
            单条规则.Format(_T("%d|%s|%d|%s|%d"),
                规则->序号, 规则->输入IP, 规则->输入端口, 规则->输出IP, 规则->输出端口);

            配置数据 += 单条规则 + _T(";");
        }

        RegSetValueEx(hKey, _T("ForwardRules"), 0, REG_SZ,
            (const BYTE*)(LPCTSTR)配置数据,
            (配置数据.GetLength() + 1) * sizeof(TCHAR));

        RegCloseKey(hKey);

        TRACE(_T("端口转发配置保存成功，规则数量: %d\n"), 转发规则列表.size());
        return TRUE;
    }

    TRACE(_T("端口转发配置保存失败\n"));
    return FALSE;
}

// 从注册表加载配置
BOOL 端口转发管理类::加载配置()
{
    TRACE(_T("=== 加载配置开始 ===\n"));

    std::lock_guard<std::mutex> 锁(规则列表锁);

    // 先清空现有规则
    for (auto* 规则 : 转发规则列表) {
        delete 规则;
    }
    转发规则列表.clear();

    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER,
        _T("Software\\NageServer\\PortForward"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD dwType, dwSize = 4096;
        TCHAR szValue[4096];

        if (RegQueryValueEx(hKey, _T("ForwardRules"), NULL, &dwType,
            (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
        {
            CString 配置数据(szValue);
            TRACE(_T("读取配置数据: %s\n"), 配置数据);

            int 位置 = 0;
            CString 单条规则 = 配置数据.Tokenize(_T(";"), 位置);
            while (!单条规则.IsEmpty())
            {
                CStringArray 规则数组;
                int 子位置 = 0;
                CString 部分 = 单条规则.Tokenize(_T("|"), 子位置);
                while (!部分.IsEmpty())
                {
                    规则数组.Add(部分);
                    部分 = 单条规则.Tokenize(_T("|"), 子位置);
                }

                if (规则数组.GetSize() == 5)
                {
                    int 序号 = _ttoi(规则数组[0]);
                    CString 输入IP = 规则数组[1];
                    int 输入端口 = _ttoi(规则数组[2]);
                    CString 输出IP = 规则数组[3];
                    int 输出端口 = _ttoi(规则数组[4]);

                    // 验证数据有效性
                    if (序号 > 0 && 输入端口 > 0 && 输入端口 <= 65535 &&
                        输出端口 > 0 && 输出端口 <= 65535)
                    {
                        // 直接创建规则对象
                        auto* 新规则 = new 端口转发规则();
                        新规则->序号 = 序号;
                        新规则->输入IP = 输入IP;
                        新规则->输入端口 = 输入端口;
                        新规则->输出IP = 输出IP;
                        新规则->输出端口 = 输出端口;
                        新规则->状态 = _T("已停止");

                        转发规则列表.push_back(新规则);
                        TRACE(_T("添加规则: %s:%d -> %s:%d\n"), 输入IP, 输入端口, 输出IP, 输出端口);
                    }
                }

                单条规则 = 配置数据.Tokenize(_T(";"), 位置);
            }
        }

        RegCloseKey(hKey);

        TRACE(_T("端口转发配置加载成功，规则数量: %d\n"), 转发规则列表.size());
        return TRUE;
    }

    TRACE(_T("端口转发配置加载失败或没有配置\n"));
    return FALSE;
}

BOOL 端口转发管理类::获取规则列表副本(std::vector<端口转发规则*>& 规则列表副本) const
{
    std::lock_guard<std::mutex> 锁(规则列表锁);
    规则列表副本 = 转发规则列表;
    return TRUE;
}

BOOL 端口转发管理类::验证规则参数(const CString& 输入IP, int 输入端口, const CString& 输出IP, int 输出端口)
{
    if (输入端口 <= 0 || 输入端口 > 65535 || 输出端口 <= 0 || 输出端口 > 65535)
        return FALSE;

    // 简单的IP验证
    if (输入IP.IsEmpty() || 输出IP.IsEmpty())
        return FALSE;

    return TRUE;
}
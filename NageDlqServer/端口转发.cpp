//端口转发.cpp

#include "pch.h"
#include "端口转发.h"
#include <winsock2.h>
#include <ws2tcpip.h>

#include <MSTcpIP.h>
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
    TRACE(_T("=== 启动所有转发开始 ===\n"));

    std::vector<端口转发规则*> 临时规则列表;

    {
        std::lock_guard<std::mutex> 锁(规则列表锁);
        临时规则列表 = 转发规则列表;
        TRACE(_T("总规则数量: %d\n"), 临时规则列表.size());

        // 详细输出每个规则
        for (int i = 0; i < 临时规则列表.size(); i++)
        {
            auto* 规则 = 临时规则列表[i];
            TRACE(_T("规则[%d]: 序号=%d, IP=%s:%d -> %s:%d, 运行中=%d\n"),
                i, 规则->序号, 规则->输入IP, 规则->输入端口,
                规则->输出IP, 规则->输出端口, 规则->运行中);
        }
    }

    BOOL 全部成功 = TRUE;
    for (auto* 规则 : 临时规则列表)
    {
        TRACE(_T("\n--- 处理规则 %d ---\n"), 规则->序号);

        if (!规则->运行中)
        {
            TRACE(_T("规则未运行，尝试启动...\n"));
            if (!启动转发规则(规则->序号))
            {
                TRACE(_T("!!! 规则 %d 启动失败 !!!\n"), 规则->序号);
                全部成功 = FALSE;
            }
            else
            {
                TRACE(_T("规则 %d 启动成功\n"), 规则->序号);
            }
        }
        else
        {
            TRACE(_T("规则 %d 已在运行中，跳过\n"), 规则->序号);
        }
    }

    // 统计结果
    int 成功数量 = 0;
    for (auto* 规则 : 临时规则列表)
    {
        if (规则->运行中) 成功数量++;
    }

    TRACE(_T("=== 启动所有转发结束 === 成功: %d/%d, 总体结果: %s\n"),
        成功数量, 临时规则列表.size(), 全部成功 ? _T("成功") : _T("失败"));

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
    端口转发规则* 目标规则 = nullptr;

    // 先找到规则
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

    if (!目标规则)
    {
        TRACE(_T("启动规则失败: 找不到规则 %d\n"), 规则序号);
        return FALSE;
    }

    if (目标规则->运行中)
    {
        TRACE(_T("规则 %d 已经在运行中\n"), 规则序号);
        return TRUE;
    }

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

        // 更新状态
        {
            std::lock_guard<std::mutex> 锁(规则列表锁);
            目标规则->运行中 = true;
            目标规则->状态 = _T("运行中");
        }

        // 启动转发线程
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

        {
            std::lock_guard<std::mutex> 锁(规则列表锁);
            目标规则->运行中 = false;
            目标规则->状态 = _T("启动失败");
        }

        return FALSE;
    }
}

// 停止单个转发规则
BOOL 端口转发管理类::停止转发规则(int 规则序号)
{
    端口转发规则* 目标规则 = nullptr;

    // 先找到规则
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

    if (!目标规则 || !目标规则->运行中)
    {
        return FALSE;
    }

    TRACE(_T("停止转发规则: %d\n"), 规则序号);

    // 设置停止标志
    目标规则->运行中 = false;
    目标规则->状态 = _T("已停止");

    // 关闭监听套接字
    if (目标规则->监听套接字 != INVALID_SOCKET)
    {
        closesocket(目标规则->监听套接字);
        目标规则->监听套接字 = INVALID_SOCKET;
    }

    // 等待线程结束
    if (目标规则->转发线程 && 目标规则->转发线程->joinable())
    {
        目标规则->转发线程->join();
    }

    // 释放线程对象
    if (目标规则->转发线程)
    {
        delete 目标规则->转发线程;
        目标规则->转发线程 = nullptr;
    }

    TRACE(_T("端口转发规则 %d 已停止\n"), 规则序号);
    return TRUE;
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

    TRACE(_T("=== 删除转发规则开始 === 序号: %d\n"), 规则序号);

    for (auto it = 转发规则列表.begin(); it != 转发规则列表.end(); ++it)
    {
        if ((*it)->序号 == 规则序号)
        {
            TRACE(_T("找到要删除的规则: %s:%d -> %s:%d\n"),
                (*it)->输入IP, (*it)->输入端口, (*it)->输出IP, (*it)->输出端口);

            // 如果规则正在运行，先停止
            if ((*it)->运行中)
            {
                TRACE(_T("规则正在运行，先停止规则\n"));

                // 设置停止标志
                (*it)->运行中 = false;

                // 关闭监听套接字
                if ((*it)->监听套接字 != INVALID_SOCKET)
                {
                    closesocket((*it)->监听套接字);
                    (*it)->监听套接字 = INVALID_SOCKET;
                }

                // 等待线程结束
                if ((*it)->转发线程 && (*it)->转发线程->joinable())
                {
                    (*it)->转发线程->join();
                }

                // 释放线程对象
                if ((*it)->转发线程)
                {
                    delete (*it)->转发线程;
                    (*it)->转发线程 = nullptr;
                }
            }

            // 删除规则对象
            delete* it;
            转发规则列表.erase(it);

            TRACE(_T("规则删除成功\n"));

            // 重新编号剩余的规则
            for (size_t i = 0; i < 转发规则列表.size(); i++)
            {
                转发规则列表[i]->序号 = static_cast<int>(i) + 1;
            }

            TRACE(_T("规则重新编号完成，当前规则数量: %d\n"), 转发规则列表.size());
            return TRUE;
        }
    }

    TRACE(_T("未找到要删除的规则: %d\n"), 规则序号);
    return FALSE;
}

// 更新转发规则
BOOL 端口转发管理类::更新转发规则(int 规则序号, const CString& 输入IP, int 输入端口,
    const CString& 输出IP, int 输出端口)
{
    // 先找到要更新的规则
    端口转发规则* 目标规则 = nullptr;
    BOOL 正在运行 = FALSE;

    {
        std::lock_guard<std::mutex> 锁(规则列表锁);
        for (auto* 规则 : 转发规则列表)
        {
            if (规则->序号 == 规则序号)
            {
                目标规则 = 规则;
                正在运行 = 规则->运行中;
                break;
            }
        }
    }

    if (!目标规则)
    {
        TRACE(_T("更新规则失败: 找不到规则 %d\n"), 规则序号);
        return FALSE;
    }

    TRACE(_T("更新规则 %d: %s:%d -> %s:%d, 当前状态: %s\n"),
        规则序号, 输入IP, 输入端口, 输出IP, 输出端口,
        正在运行 ? _T("运行中") : _T("已停止"));

    // 如果规则正在运行，先停止
    if (正在运行)
    {
        TRACE(_T("规则正在运行，先停止规则\n"));
        停止转发规则(规则序号);
    }

    // 更新规则数据
    {
        std::lock_guard<std::mutex> 锁(规则列表锁);
        目标规则->输入IP = 输入IP;
        目标规则->输入端口 = 输入端口;
        目标规则->输出IP = 输出IP;
        目标规则->输出端口 = 输出端口;
    }

    // 如果之前是运行状态，重新启动
    if (正在运行)
    {
        TRACE(_T("重新启动规则\n"));
        启动转发规则(规则序号);
    }
    else
    {
        // 更新状态显示
        std::lock_guard<std::mutex> 锁(规则列表锁);
        目标规则->状态 = _T("已停止");
    }

    TRACE(_T("规则更新完成，立即保存配置\n"));

    // 立即保存配置
    return 保存配置();
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

    DWORD 上次清理时间 = GetTickCount();
    const DWORD 清理间隔 = 30000; // 30秒清理一次无效连接

    DWORD 上次状态检查时间 = GetTickCount();
    const DWORD 状态检查间隔 = 5 * 60 * 1000; // 5分钟检查一次状态

    while (规则->运行中)
    {
        // 定期清理标记为关闭的连接
        DWORD 当前时间 = GetTickCount();
        if (当前时间 - 上次清理时间 > 清理间隔)
        {
            规则->清理无效连接();
            上次清理时间 = 当前时间;
        }

        // 定期记录连接状态（不主动断开）
        if (当前时间 - 上次状态检查时间 > 状态检查间隔)
        {
            定期连接状态检查(规则);
            上次状态检查时间 = 当前时间;
        }

        // 使用select等待连接，设置超时以便定期检查运行状态
        fd_set 读集合;
        FD_ZERO(&读集合);
        FD_SET(规则->监听套接字, &读集合);

        struct timeval 超时;
        超时.tv_sec = 1;  // 1秒超时，便于及时响应停止信号
        超时.tv_usec = 0;

        int 选择结果 = select(0, &读集合, NULL, NULL, &超时);

        if (选择结果 > 0)
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

                // 设置TCP保活机制（让操作系统处理连接维持）
                int 保持活动 = 1;
                setsockopt(客户端套接字, SOL_SOCKET, SO_KEEPALIVE, (char*)&保持活动, sizeof(保持活动));

                // 设置TCP保活参数（Windows）
                tcp_keepalive 保活参数;
                保活参数.onoff = 1;
                保活参数.keepalivetime = 2 * 60 * 60 * 1000;  // 2小时后开始检测
                保活参数.keepaliveinterval = 60 * 1000;       // 1分钟重试间隔

                DWORD 返回字节数 = 0;
                // 使用WSAIoctl设置TCP保活参数
                if (WSAIoctl(客户端套接字, SIO_KEEPALIVE_VALS, &保活参数, sizeof(保活参数),
                    NULL, 0, &返回字节数, NULL, NULL) == SOCKET_ERROR)
                {
                    // 如果设置失败，只记录日志，不影响功能
                    int 错误码 = WSAGetLastError();
                    TRACE(_T("设置TCP保活参数失败，错误码: %d (不影响功能)\n"), 错误码);
                }

                // 连接到目标服务器
                SOCKET 目标套接字 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (目标套接字 != INVALID_SOCKET)
                {
                    sockaddr_in 目标地址;
                    目标地址.sin_family = AF_INET;

                    USES_CONVERSION;
                    char* 输出IP = T2A(规则->输出IP);
                    if (inet_pton(AF_INET, 输出IP, &(目标地址.sin_addr)) != 1)
                    {
                        TRACE(_T("目标IP地址转换失败: %s\n"), 规则->输出IP);
                        closesocket(目标套接字);
                        closesocket(客户端套接字);
                        continue;
                    }
                    目标地址.sin_port = htons(规则->输出端口);

                    // 设置连接超时（仅用于连接阶段）
                    DWORD 连接超时 = 10000; // 10秒
                    setsockopt(目标套接字, SOL_SOCKET, SO_RCVTIMEO, (char*)&连接超时, sizeof(连接超时));
                    setsockopt(目标套接字, SOL_SOCKET, SO_SNDTIMEO, (char*)&连接超时, sizeof(连接超时));

                    if (connect(目标套接字, (sockaddr*)&目标地址, sizeof(目标地址)) == 0)
                    {
                        // 连接成功后，移除发送/接收超时设置
                        DWORD 无超时 = 0;
                        setsockopt(目标套接字, SOL_SOCKET, SO_RCVTIMEO, (char*)&无超时, sizeof(无超时));
                        setsockopt(目标套接字, SOL_SOCKET, SO_SNDTIMEO, (char*)&无超时, sizeof(无超时));

                        // 为服务器端套接字也设置保活
                        保持活动 = 1;
                        setsockopt(目标套接字, SOL_SOCKET, SO_KEEPALIVE, (char*)&保持活动, sizeof(保持活动));

                        // 设置目标套接字的TCP保活参数
                        tcp_keepalive 目标保活参数;
                        目标保活参数.onoff = 1;
                        目标保活参数.keepalivetime = 2 * 60 * 60 * 1000;  // 2小时后开始检测
                        目标保活参数.keepaliveinterval = 60 * 1000;       // 1分钟重试间隔

                        if (WSAIoctl(目标套接字, SIO_KEEPALIVE_VALS, &目标保活参数, sizeof(目标保活参数),
                            NULL, 0, &返回字节数, NULL, NULL) == SOCKET_ERROR)
                        {
                            int 错误码 = WSAGetLastError();
                            TRACE(_T("设置目标套接字TCP保活参数失败，错误码: %d\n"), 错误码);
                        }

                        // 记录连接信息
                        规则->添加活动连接(客户端套接字);
                        规则->添加活动连接(目标套接字);

                        // 启动客户端处理线程
                        std::thread* 处理线程 = new std::thread(客户端处理线程, 客户端套接字, 目标套接字, 规则);
                        处理线程->detach();

                        规则->增加连接数();
                        TRACE(_T("规则 %d: 新连接建立，当前连接数: %d\n"),
                            规则->序号, 规则->获取连接数());
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
        }
        else if (选择结果 == 0)
        {
            // select超时，继续循环
            continue;
        }
        else
        {
            int 错误码 = WSAGetLastError();
            if (错误码 != WSAEINTR) // 不是被中断的错误
            {
                TRACE(_T("规则 %d select错误: %d\n"), 规则->序号, 错误码);
            }
            Sleep(100); // 短暂休眠
        }
    }

    TRACE(_T("=== 转发线程退出 === 规则: %d\n"), 规则->序号);
}

// 客户端处理线程
void 端口转发管理类::客户端处理线程(SOCKET 客户端套接字, SOCKET 目标套接字, 端口转发规则* 规则)
{
    // 设置套接字为阻塞模式（而不是非阻塞）
    u_long 阻塞模式 = 0;
    ioctlsocket(客户端套接字, FIONBIO, &阻塞模式);
    ioctlsocket(目标套接字, FIONBIO, &阻塞模式);

    // 记录连接信息用于日志
    DWORD 连接开始时间 = GetTickCount();

    // 创建两个线程分别处理两个方向的数据转发
    std::thread 客户端到目标线程(转发数据, 客户端套接字, 目标套接字, 规则);
    std::thread 目标到客户端线程(转发数据, 目标套接字, 客户端套接字, 规则);

    // 等待两个线程结束
    auto 等待线程结束 = [](std::thread& 线程) {
        if (线程.joinable())
        {
            // 无限期等待线程结束
            线程.join();
        }
        };

    // 等待线程结束
    等待线程结束(客户端到目标线程);
    等待线程结束(目标到客户端线程);

    // 计算连接持续时间
    DWORD 连接持续时间 = GetTickCount() - 连接开始时间;
    TRACE(_T("规则 %d: 连接结束，持续时间: %.2f小时\n"),
        规则->序号, 连接持续时间 / (3600.0 * 1000.0));

    // 关闭连接
    closesocket(客户端套接字);
    closesocket(目标套接字);

    // 减少连接数
    规则->减少连接数();

    TRACE(_T("规则 %d: 连接处理完成，当前连接数: %d\n"),
        规则->序号, 规则->获取连接数());
}

// 数据转发函数
void 端口转发管理类::转发数据(SOCKET 来源套接字, SOCKET 目标套接字, 端口转发规则* 规则)
{
    char 缓冲区[4096];

    while (规则->运行中)
    {
        // 使用select无限期等待数据（不设置超时）
        fd_set 读集合;
        FD_ZERO(&读集合);
        FD_SET(来源套接字, &读集合);

        // 重要：不设置超时，无限期等待
        int 选择结果 = select(0, &读集合, NULL, NULL, NULL);

        if (选择结果 > 0)
        {
            int 接收长度 = recv(来源套接字, 缓冲区, sizeof(缓冲区), 0);

            if (接收长度 > 0)
            {
                // 更新连接活动时间
                规则->更新连接活动时间(来源套接字);

                // 转发数据
                int 发送长度 = send(目标套接字, 缓冲区, 接收长度, 0);

                if (发送长度 == SOCKET_ERROR)
                {
                    int 错误码 = WSAGetLastError();
                    TRACE(_T("发送数据失败，错误码: %d\n"), 错误码);
                    break;
                }
            }
            else if (接收长度 == 0)
            {
                // 连接正常关闭（对端调用了close/shutdown）
                TRACE(_T("连接正常关闭\n"));
                break;
            }
            else
            {
                // 接收错误
                int 错误码 = WSAGetLastError();
                if (错误码 != WSAEWOULDBLOCK)
                {
                    TRACE(_T("接收数据失败，错误码: %d\n"), 错误码);
                    break;
                }
                // WSAEWOULDBLOCK是非阻塞模式下的正常情况，继续等待
            }
        }
        else if (选择结果 == 0)
        {
            // 理论上不会发生，因为没有设置超时
            continue;
        }
        else
        {
            // select错误
            int 错误码 = WSAGetLastError();
            TRACE(_T("select错误: %d\n"), 错误码);
            break;
        }
    }

    // 标记连接为关闭状态
    规则->移除活动连接(来源套接字);
    规则->移除活动连接(目标套接字);
}

// 保存配置到注册表
BOOL 端口转发管理类::保存配置()
{
    HKEY hKey;
    LONG lResult = RegCreateKeyEx(HKEY_CURRENT_USER,
        _T("Software\\NageServer\\PortForward"),
        0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);

    if (lResult == ERROR_SUCCESS)
    {
        CString 配置数据;

        {
            std::lock_guard<std::mutex> 锁(规则列表锁);
            for (const auto* 规则 : 转发规则列表)
            {
                CString 单条规则;
                单条规则.Format(_T("%d|%s|%d|%s|%d|%s"),
                    规则->序号,
                    规则->输入IP,
                    规则->输入端口,
                    规则->输出IP,
                    规则->输出端口,
                    规则->状态);

                配置数据 += 单条规则 + _T(";");

                TRACE(_T("保存规则: %s\n"), 单条规则);
            }
        }

        TRACE(_T("保存的完整配置: %s\n"), 配置数据);

        // 保存到注册表
        LSTATUS 设置结果 = RegSetValueEx(hKey, _T("ForwardRules"), 0, REG_SZ,
            (const BYTE*)(LPCTSTR)配置数据, (配置数据.GetLength() + 1) * sizeof(TCHAR));

        RegCloseKey(hKey);

        if (设置结果 == ERROR_SUCCESS)
        {
            TRACE(_T("端口转发配置保存成功\n"));
            return TRUE;
        }
        else
        {
            TRACE(_T("RegSetValueEx失败，错误码: %d\n"), 设置结果);
        }
    }
    else
    {
        TRACE(_T("RegCreateKeyEx失败，错误码: %d\n"), lResult);
    }

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

    CString 注册表路径 = _T("Software\\NageServer\\PortForward");
    TRACE(_T("尝试从注册表加载: HKEY_CURRENT_USER\\%s\n"), 注册表路径);

    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, 注册表路径, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        TRACE(_T("成功打开注册表键\n"));

        DWORD dwType, dwSize = 0;

        // 先获取数据大小
        if (RegQueryValueEx(hKey, _T("ForwardRules"), NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS)
        {
            TRACE(_T("找到ForwardRules值，大小: %d 字节\n"), dwSize);

            if (dwSize > 0 && dwType == REG_SZ)
            {
                TCHAR* szValue = new TCHAR[dwSize / sizeof(TCHAR) + 1];
                ZeroMemory(szValue, (dwSize / sizeof(TCHAR) + 1) * sizeof(TCHAR));

                if (RegQueryValueEx(hKey, _T("ForwardRules"), NULL, &dwType,
                    (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
                {
                    CString 配置数据(szValue);
                    TRACE(_T("读取配置数据: %s\n"), 配置数据);

                    int 位置 = 0;
                    CString 单条规则 = 配置数据.Tokenize(_T(";"), 位置);
                    int 规则计数 = 0;

                    while (!单条规则.IsEmpty())
                    {
                        TRACE(_T("处理单条规则[%d]: %s\n"), 规则计数, 单条规则);

                        CStringArray 规则数组;
                        int 子位置 = 0;
                        CString 部分 = 单条规则.Tokenize(_T("|"), 子位置);

                        while (!部分.IsEmpty())
                        {
                            规则数组.Add(部分);
                            TRACE(_T("规则字段[%d]: %s\n"), 规则数组.GetSize() - 1, 部分);
                            部分 = 单条规则.Tokenize(_T("|"), 子位置);
                        }

                        // 现在应该有6个部分：序号|输入IP|输入端口|输出IP|输出端口|状态
                        if (规则数组.GetSize() >= 5)  // 至少要有前5个必需字段
                        {
                            int 序号 = _ttoi(规则数组[0]);
                            CString 输入IP = 规则数组[1];
                            int 输入端口 = _ttoi(规则数组[2]);
                            CString 输出IP = 规则数组[3];
                            int 输出端口 = _ttoi(规则数组[4]);

                            CString 状态;
                            if (规则数组.GetSize() >= 6)
                            {
                                状态 = 规则数组[5];
                            }
                            else
                            {
                                状态 = _T("已停止");
                            }

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
                                新规则->状态 = 状态;
                                新规则->运行中 = (状态 == _T("运行中"));

                                转发规则列表.push_back(新规则);
                                规则计数++;

                                TRACE(_T("成功加载规则[%d]: %s:%d -> %s:%d, 状态: %s\n"),
                                    序号, 输入IP, 输入端口, 输出IP, 输出端口, 状态);
                            }
                            else
                            {
                                TRACE(_T("规则数据无效，跳过: %s\n"), 单条规则);
                            }
                        }
                        else
                        {
                            TRACE(_T("规则格式错误，字段数: %d\n"), 规则数组.GetSize());
                        }

                        单条规则 = 配置数据.Tokenize(_T(";"), 位置);
                    }

                    TRACE(_T("成功加载 %d 条规则\n"), 规则计数);
                }
                else
                {
                    TRACE(_T("读取注册表值失败\n"));
                }

                delete[] szValue;
            }
            else
            {
                TRACE(_T("配置数据大小为0或类型错误\n"));
            }
        }
        else
        {
            TRACE(_T("查询注册表值大小失败\n"));
        }

        RegCloseKey(hKey);

        TRACE(_T("端口转发配置加载完成，规则数量: %d\n"), 转发规则列表.size());

        // 如果成功加载了规则，返回TRUE
        if (!转发规则列表.empty())
        {
            return TRUE;
        }
    }
    else
    {
        TRACE(_T("无法打开注册表键，错误码: %d\n"), GetLastError());
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

// 在端口转发规则结构中已修改，这里添加详细日志
void 端口转发管理类::定期连接状态检查(端口转发规则* 规则)
{
    std::lock_guard<std::mutex> 锁(规则->连接列表锁);
    DWORD 当前时间 = GetTickCount();

    for (auto& 连接 : 规则->活动连接)
    {
        if (!连接->正在关闭)
        {
            DWORD 连接时长 = (当前时间 - 连接->开始时间) / (1000 * 60); // 分钟
            DWORD 空闲时长 = (当前时间 - 连接->最后活动时间) / (1000 * 60); // 分钟

            // 只记录日志，不主动断开
            if (连接时长 > 60) // 连接超过1小时
            {
                TRACE(_T("规则 %d: 连接已持续 %d 分钟，空闲 %d 分钟\n"),
                    规则->序号, 连接时长, 空闲时长);
            }
        }
    }
}
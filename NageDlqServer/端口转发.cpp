//端口转发.cpp

#include "pch.h"
#include "端口转发.h"
#include <MSTcpIP.h>
#include <algorithm>

// 端口转发管理类构造函数
端口转发管理类::端口转发管理类()
{
    // 初始化Winsock（如果需要）
    static bool winsock初始化 = false;
    if (!winsock初始化)
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0)
        {
            winsock初始化 = true;
        }
    }
}

端口转发管理类::~端口转发管理类()
{
    // 停止所有转发
    停止所有转发();

    // 清理所有规则
    std::lock_guard<std::mutex> 锁(规则列表锁);
    for (auto* 规则 : 转发规则列表)
    {
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
        }
    }

    TRACE(_T("=== 启动所有转发结束 ===\n"));
    return 全部成功;
}

// 停止所有转发规则
BOOL 端口转发管理类::停止所有转发()
{
    std::vector<端口转发规则*> 临时规则列表;

    {
        std::lock_guard<std::mutex> 锁(规则列表锁);
        临时规则列表 = 转发规则列表;
    }

    for (auto* 规则 : 临时规则列表)
    {
        if (规则->运行中)
        {
            停止转发规则(规则->序号);
        }
    }

    return TRUE;
}

// 启动单个转发规则
BOOL 端口转发管理类::启动转发规则(int 规则序号)
{
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

    // 设置停止标志（先设标志，让转发线程退出循环）
    目标规则->运行中 = false;
    目标规则->状态 = _T("已停止");

    // 关闭监听套接字（让accept立即返回）
    if (目标规则->监听套接字 != INVALID_SOCKET)
    {
        closesocket(目标规则->监听套接字);
        目标规则->监听套接字 = INVALID_SOCKET;
    }

    // 清理所有活动连接（关闭套接字让select解除阻塞）
    目标规则->清空所有连接();

    // 重置连接数
    目标规则->设置连接数(0);

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

    if (!验证规则参数(输入IP, 输入端口, 输出IP, 输出端口))
    {
        return FALSE;
    }

    for (const auto* 现有规则 : 转发规则列表)
    {
        if (!现有规则)
        {
            continue;
        }

        if (现有规则->输入IP == 输入IP &&
            现有规则->输入端口 == 输入端口 &&
            现有规则->输出IP == 输出IP &&
            现有规则->输出端口 == 输出端口)
        {
            return FALSE;
        }
    }

    auto* 新规则 = new 端口转发规则();
    新规则->序号 = static_cast<int>(转发规则列表.size()) + 1;
    新规则->输入IP = 输入IP;
    新规则->输入端口 = 输入端口;
    新规则->输出IP = 输出IP;
    新规则->输出端口 = 输出端口;
    新规则->状态 = _T("已停止");

    转发规则列表.push_back(新规则);

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
    return 转发规则列表;
}

// ============================================================
// 统一转发线程（select I/O多路复用，单线程管理所有连接）
// 替代原来"每连接3线程"的模式，极大降低内存占用
// ============================================================
void 端口转发管理类::转发线程函数(端口转发规则* 规则)
{
    TRACE(_T("=== 转发线程启动（多路复用模式）=== 规则: %d\n"), 规则->序号);

    // 用于收集超时连接
    std::vector<SOCKET> 待清理列表;

    while (规则->运行中)
    {
        // 构建 fd_set：监听套接字 + 所有活跃连接的读写套接字
        fd_set 读集合;
        FD_ZERO(&读集合);
        int 最大套接字 = 0;

        // 1. 加入监听套接字
        if (规则->监听套接字 != INVALID_SOCKET)
        {
            FD_SET(规则->监听套接字, &读集合);
            最大套接字 = (int)规则->监听套接字;
        }

        // 2. 加入所有活跃连接的套接字
        规则->收集活跃套接字(读集合, 最大套接字);

        // 3. select 等待（带超时，用于定期清理）
        struct timeval 超时;
        超时.tv_sec = 端口转发规则::选择超时毫秒 / 1000;
        超时.tv_usec = (端口转发规则::选择超时毫秒 % 1000) * 1000;

        int 选择结果 = select(最大套接字 + 1, &读集合, NULL, NULL, &超时);

        if (选择结果 == SOCKET_ERROR)
        {
            int 错误码 = WSAGetLastError();
            if (错误码 == WSAENOTSOCK)
            {
                // 有套接字被关闭了，清理后重试
                清理超时连接(规则);
                continue;
            }
            TRACE(_T("规则 %d: select错误 %d，休眠后重试\n"), 规则->序号, 错误码);
            Sleep(100);
            continue;
        }

        // 4. 处理监听套接字的新连接
        if (规则->监听套接字 != INVALID_SOCKET &&
            FD_ISSET(规则->监听套接字, &读集合))
        {
            选择结果--;

            // 检查连接数上限
            if (规则->已达到最大连接数())
            {
                // 达到上限，接受后立即关闭，避免SYN队列堆积
                sockaddr_in 临时地址;
                int 临时长度 = sizeof(临时地址);
                SOCKET 临时套接字 = accept(规则->监听套接字, (sockaddr*)&临时地址, &临时长度);
                if (临时套接字 != INVALID_SOCKET)
                {
                    TRACE(_T("规则 %d: 连接数已达上限 %d，拒绝新连接\n"),
                          规则->序号, 端口转发规则::最大连接数);
                    shutdown(临时套接字, SD_BOTH);
                    closesocket(临时套接字);
                }
            }
            else
            {
                sockaddr_in 客户端地址;
                int 客户端地址长度 = sizeof(客户端地址);

                SOCKET 客户端套接字 = accept(规则->监听套接字,
                    (sockaddr*)&客户端地址, &客户端地址长度);

                if (客户端套接字 != INVALID_SOCKET)
                {
                    // 连接目标服务器
                    SOCKET 目标套接字 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                    if (目标套接字 != INVALID_SOCKET)
                    {
                        sockaddr_in 目标地址;
                        目标地址.sin_family = AF_INET;

                        USES_CONVERSION;
                        char* 输出IP = T2A(规则->输出IP);
                        if (inet_pton(AF_INET, 输出IP, &(目标地址.sin_addr)) == 1)
                        {
                            目标地址.sin_port = htons(规则->输出端口);

                            // 设置目标套接字为非阻塞以支持connect超时
                            u_long 非阻塞 = 1;
                            ioctlsocket(目标套接字, FIONBIO, &非阻塞);

                            connect(目标套接字, (sockaddr*)&目标地址, sizeof(目标地址));

                            // 使用select等待connect完成（最多3秒）
                            fd_set 写集合;
                            FD_ZERO(&写集合);
                            FD_SET(目标套接字, &写集合);
                            struct timeval 连接超时 = { 3, 0 };

                            if (select(0, NULL, &写集合, NULL, &连接超时) > 0)
                            {
                                // connect成功，设置客户端和非阻塞模式
                                ioctlsocket(客户端套接字, FIONBIO, &非阻塞);

                                // 恢复目标套接字为非阻塞
                                // （已经是非阻塞，无需再设）

                                // 记录连接对
                                规则->添加连接对(客户端套接字, 目标套接字);
                                规则->增加连接数();

                                TRACE(_T("规则 %d: 新连接建立，当前连接数: %d\n"),
                                      规则->序号, 规则->获取连接数());
                            }
                            else
                            {
                                TRACE(_T("规则 %d: 连接目标 %s:%d 超时\n"),
                                      规则->序号, 规则->输出IP, 规则->输出端口);
                                closesocket(目标套接字);
                                closesocket(客户端套接字);
                            }
                        }
                        else
                        {
                            closesocket(目标套接字);
                            closesocket(客户端套接字);
                        }
                    }
                    else
                    {
                        closesocket(客户端套接字);
                    }
                }
            }
        }

        // 5. 处理所有活跃连接的数据转发
        if (选择结果 > 0)
        {
            待清理列表.clear();

            // 获取连接映射的快照（避免处理时持有锁）
            std::vector<std::shared_ptr<转发连接对>> 活跃连接快照;
            {
                std::lock_guard<std::mutex> 锁(规则->连接映射锁);
                for (auto& 对 : 规则->连接映射)
                {
                    if (对.second && !对.second->已关闭)
                    {
                        活跃连接快照.push_back(对.second);
                    }
                }
            }

            for (auto& 连接对 : 活跃连接快照)
            {
                if (!连接对 || 连接对->已关闭) continue;

                bool 连接已断 = false;

                // 客户端->目标方向
                if (!连接已断 && 连接对->客户端套接字 != INVALID_SOCKET &&
                    FD_ISSET(连接对->客户端套接字, &读集合))
                {
                    if (!处理单次转发(连接对->客户端套接字, 连接对->目标套接字, 规则))
                    {
                        连接已断 = true;
                    }
                    else
                    {
                        连接对->最后活动时间 = GetTickCount();
                    }
                }

                // 目标->客户端方向
                if (!连接已断 && 连接对->目标套接字 != INVALID_SOCKET &&
                    FD_ISSET(连接对->目标套接字, &读集合))
                {
                    if (!处理单次转发(连接对->目标套接字, 连接对->客户端套接字, 规则))
                    {
                        连接已断 = true;
                    }
                    else
                    {
                        连接对->最后活动时间 = GetTickCount();
                    }
                }

                if (连接已断)
                {
                    待清理列表.push_back(连接对->客户端套接字);
                }
            }

            // 清理断开的连接
            for (SOCKET 客户端 : 待清理列表)
            {
                规则->移除连接对(客户端);
                规则->减少连接数();
            }
            if (!待清理列表.empty())
            {
                TRACE(_T("规则 %d: 清理了 %d 个断开连接，当前连接数: %d\n"),
                      规则->序号, (int)待清理列表.size(), 规则->获取连接数());
            }
        }
        else if (选择结果 == 0)
        {
            // 超时：正好用来清理空闲连接
            清理超时连接(规则);
        }
    }

    // 线程退出前清理所有连接
    规则->清空所有连接();
    TRACE(_T("=== 转发线程退出（多路复用模式）=== 规则: %d\n"), 规则->序号);
}

// 处理单次数据转发：从来源读取，写入目标
// 返回 true=成功, false=连接断开
bool 端口转发管理类::处理单次转发(SOCKET 来源, SOCKET 目标, 端口转发规则* 规则)
{
    if (来源 == INVALID_SOCKET || 目标 == INVALID_SOCKET)
        return false;

    char 缓冲区[8192];

    int 接收长度 = recv(来源, 缓冲区, sizeof(缓冲区), 0);

    if (接收长度 > 0)
    {
        // 发送数据到目标
        int 已发送 = 0;
        int 剩余 = 接收长度;

        while (剩余 > 0)
        {
            int 发送结果 = send(目标, 缓冲区 + 已发送, 剩余, 0);
            if (发送结果 <= 0)
            {
                int 错误码 = WSAGetLastError();
                if (错误码 == WSAEWOULDBLOCK)
                {
                    // 目标缓冲区满，稍后select会再次通知我们
                    // 丢弃本次未发送的数据（简单转发场景可接受）
                    return true;
                }
                return false; // 发送失败
            }
            已发送 += 发送结果;
            剩余 -= 发送结果;
        }
        return true;
    }
    else if (接收长度 == 0)
    {
        // 对端正常关闭
        return false;
    }
    else
    {
        int 错误码 = WSAGetLastError();
        if (错误码 == WSAEWOULDBLOCK)
        {
            return true; // 非阻塞模式下的正常情况
        }
        return false; // 真正的错误
    }
}

// 清理超时连接
void 端口转发管理类::清理超时连接(端口转发规则* 规则)
{
    DWORD 当前时间 = GetTickCount();
    std::vector<SOCKET> 超时列表;

    {
        std::lock_guard<std::mutex> 锁(规则->连接映射锁);
        for (auto& 对 : 规则->连接映射)
        {
            if (对.second && !对.second->已关闭)
            {
                DWORD 空闲时间 = 当前时间 - 对.second->最后活动时间;
                if (空闲时间 > 端口转发规则::连接超时毫秒)
                {
                    超时列表.push_back(对.first);
                }
            }
        }
    }

    for (SOCKET 客户端 : 超时列表)
    {
        规则->移除连接对(客户端);
        规则->减少连接数();
    }

    if (!超时列表.empty())
    {
        TRACE(_T("规则 %d: 清理了 %d 个超时连接（空闲>%d秒），当前连接数: %d\n"),
              规则->序号, (int)超时列表.size(),
              端口转发规则::连接超时毫秒 / 1000,
              规则->获取连接数());
    }
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

// 获取规则列表副本
BOOL 端口转发管理类::获取规则列表副本(std::vector<端口转发规则*>& 规则列表副本) const
{
    std::lock_guard<std::mutex> 锁(规则列表锁);
    规则列表副本 = 转发规则列表;
    return TRUE;
}

// 验证规则参数
BOOL 端口转发管理类::验证规则参数(const CString& 输入IP, int 输入端口, const CString& 输出IP, int 输出端口)
{
    if (输入端口 <= 0 || 输入端口 > 65535 || 输出端口 <= 0 || 输出端口 > 65535)
        return FALSE;

    // 简单的IP验证
    if (输入IP.IsEmpty() || 输出IP.IsEmpty())
        return FALSE;

    return TRUE;
}

// 连接超时清理已集成到转发线程函数中，通过 清理超时连接() 实现
// 不再需要独立的定期检查函数
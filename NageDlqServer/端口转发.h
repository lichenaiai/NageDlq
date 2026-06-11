// 端口转发.h
#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <map>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <afx.h>
#include <MSTcpIP.h>
#pragma comment(lib, "ws2_32.lib")

// 单个转发连接对（客户端<->目标服务器）
struct 转发连接对
{
    SOCKET 客户端套接字;
    SOCKET 目标套接字;
    DWORD 创建时间;
    DWORD 最后活动时间;
    bool 已关闭;

    转发连接对(SOCKET 客户端, SOCKET 目标)
        : 客户端套接字(客户端), 目标套接字(目标),
          创建时间(GetTickCount()), 最后活动时间(GetTickCount()),
          已关闭(false) {
    }

    ~转发连接对()
    {
        关闭();
    }

    void 关闭()
    {
        if (!已关闭)
        {
            已关闭 = true;
            if (客户端套接字 != INVALID_SOCKET)
            {
                shutdown(客户端套接字, SD_BOTH);
                closesocket(客户端套接字);
                客户端套接字 = INVALID_SOCKET;
            }
            if (目标套接字 != INVALID_SOCKET)
            {
                shutdown(目标套接字, SD_BOTH);
                closesocket(目标套接字);
                目标套接字 = INVALID_SOCKET;
            }
        }
    }
};

// 端口转发规则结构
struct 端口转发规则
{
    // 配置常量
    static constexpr int 最大连接数 = 500;       // 单规则最大连接数
    static constexpr DWORD 连接超时毫秒 = 300000; // 5分钟空闲超时
    static constexpr DWORD 选择超时毫秒 = 1000;   // select超时间隔（用于定期清理）

    int 序号;
    CString 输入IP;
    int 输入端口;
    CString 输出IP;
    int 输出端口;
    CString 状态;
    BOOL 运行中;
    SOCKET 监听套接字;
    std::thread* 转发线程;

    // 连接管理（改用map便于O(1)查找）
    int 连接数;
    mutable std::mutex 连接数锁;
    std::map<SOCKET, std::shared_ptr<转发连接对>> 连接映射;  // key=客户端套接字
    mutable std::mutex 连接映射锁;

    // 构造函数
    端口转发规则() : 序号(0), 输入端口(0), 输出端口(0),
        运行中(FALSE), 监听套接字(INVALID_SOCKET),
        转发线程(nullptr), 连接数(0) {
    }

    // 析构函数
    ~端口转发规则()
    {
        if (转发线程 && 转发线程->joinable())
        {
            转发线程->join();
            delete 转发线程;
            转发线程 = nullptr;
        }

        if (监听套接字 != INVALID_SOCKET)
        {
            closesocket(监听套接字);
            监听套接字 = INVALID_SOCKET;
        }

        清空所有连接();
    }

    // 连接数操作
    int 获取连接数() const
    {
        std::lock_guard<std::mutex> 锁(连接数锁);
        return 连接数;
    }

    void 增加连接数()
    {
        std::lock_guard<std::mutex> 锁(连接数锁);
        连接数++;
    }

    void 减少连接数()
    {
        std::lock_guard<std::mutex> 锁(连接数锁);
        if (连接数 > 0) 连接数--;
    }

    void 设置连接数(int 数量)
    {
        std::lock_guard<std::mutex> 锁(连接数锁);
        连接数 = 数量;
    }

    bool 已达到最大连接数() const
    {
        std::lock_guard<std::mutex> 锁(连接数锁);
        return 连接数 >= 最大连接数;
    }

    // 连接管理方法
    void 添加连接对(SOCKET 客户端, SOCKET 目标)
    {
        std::lock_guard<std::mutex> 锁(连接映射锁);
        auto 连接对 = std::make_shared<转发连接对>(客户端, 目标);
        连接映射[客户端] = 连接对;
    }

    void 移除连接对(SOCKET 客户端)
    {
        std::lock_guard<std::mutex> 锁(连接映射锁);
        auto it = 连接映射.find(客户端);
        if (it != 连接映射.end())
        {
            it->second->关闭();
            连接映射.erase(it);
        }
    }

    void 清空所有连接()
    {
        std::lock_guard<std::mutex> 锁(连接映射锁);
        for (auto& 对 : 连接映射)
        {
            if (对.second)
            {
                对.second->关闭();
            }
        }
        连接映射.clear();

        std::lock_guard<std::mutex> 锁2(连接数锁);
        连接数 = 0;
    }

    // 获取连接对副本（线程安全）
    std::shared_ptr<转发连接对> 获取连接对(SOCKET 客户端)
    {
        std::lock_guard<std::mutex> 锁(连接映射锁);
        auto it = 连接映射.find(客户端);
        if (it != 连接映射.end())
        {
            return it->second;
        }
        return nullptr;
    }

    // 收集所有活跃套接字到fd_set（线程安全）
    void 收集活跃套接字(fd_set& 读集合, int& 最大套接字)
    {
        std::lock_guard<std::mutex> 锁(连接映射锁);
        for (auto& 对 : 连接映射)
        {
            if (对.second && !对.second->已关闭)
            {
                SOCKET 客户端 = 对.second->客户端套接字;
                SOCKET 目标 = 对.second->目标套接字;
                if (客户端 != INVALID_SOCKET)
                {
                    FD_SET(客户端, &读集合);
                    if ((int)客户端 > 最大套接字) 最大套接字 = (int)客户端;
                }
                if (目标 != INVALID_SOCKET)
                {
                    FD_SET(目标, &读集合);
                    if ((int)目标 > 最大套接字) 最大套接字 = (int)目标;
                }
            }
        }
    }
};

// 端口转发管理类
class 端口转发管理类
{
public:
    端口转发管理类();
    ~端口转发管理类();

    BOOL 启动所有转发();
    BOOL 停止所有转发();
    BOOL 启动转发规则(int 规则序号);
    BOOL 停止转发规则(int 规则序号);
    BOOL 添加转发规则(const CString& 输入IP, int 输入端口, const CString& 输出IP, int 输出端口);
    BOOL 删除转发规则(int 规则序号);
    BOOL 更新转发规则(int 规则序号, const CString& 输入IP, int 输入端口,
        const CString& 输出IP, int 输出端口);

    // 安全的获取规则列表方法
    std::vector<端口转发规则*> 获取规则列表() const;
    BOOL 获取规则列表副本(std::vector<端口转发规则*>& 规则列表副本) const;
    BOOL 保存配置();
    BOOL 加载配置();
    BOOL 验证规则参数(const CString& 输入IP, int 输入端口, const CString& 输出IP, int 输出端口);

private:
    std::vector<端口转发规则*> 转发规则列表;
    mutable std::mutex 规则列表锁;

    // 统一转发线程：单线程用select管理所有连接（替代原来每连接多线程模式）
    static void 转发线程函数(端口转发规则* 规则);
    // 处理单次数据转发：从来源读取，写入目标
    static bool 处理单次转发(SOCKET 来源, SOCKET 目标, 端口转发规则* 规则);
    // 清理超时连接
    static void 清理超时连接(端口转发规则* 规则);
};
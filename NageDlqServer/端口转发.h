// 端口转发.h
#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <afx.h>
#include <MSTcpIP.h>
#pragma comment(lib, "ws2_32.lib")

struct 连接信息
{
    SOCKET 套接字;
    DWORD 开始时间;
    DWORD 最后活动时间;
    bool 正在关闭;

    连接信息(SOCKET s) : 套接字(s),
        开始时间(GetTickCount()),
        最后活动时间(开始时间),
        正在关闭(false) {
    }
};

// 端口转发规则结构
struct 端口转发规则
{
    int 序号;
    CString 输入IP;
    int 输入端口;
    CString 输出IP;
    int 输出端口;
    CString 状态;
    BOOL 运行中;
    SOCKET 监听套接字;
    std::thread* 转发线程;

    // 连接管理
    int 连接数;
    mutable std::mutex 连接数锁;
    std::vector<std::shared_ptr<连接信息>> 活动连接;
    mutable std::mutex 连接列表锁;

    // 构造函数
    端口转发规则() : 序号(0), 输入端口(0), 输出端口(0),
        运行中(FALSE), 监听套接字(INVALID_SOCKET),
        转发线程(nullptr), 连接数(0) {
    }

    // 析构函数
    ~端口转发规则()
    {
        // 停止转发线程
        if (转发线程 && 转发线程->joinable())
        {
            转发线程->join();
            delete 转发线程;
        }

        // 关闭监听套接字
        if (监听套接字 != INVALID_SOCKET)
        {
            closesocket(监听套接字);
        }

        // 清理所有活动连接
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

    // 连接管理方法
    void 添加活动连接(SOCKET 套接字)
    {
        std::lock_guard<std::mutex> 锁(连接列表锁);
        auto 连接 = std::make_shared<连接信息>(套接字);
        活动连接.push_back(连接);
    }

    void 移除活动连接(SOCKET 套接字)
    {
        std::lock_guard<std::mutex> 锁(连接列表锁);
        for (auto it = 活动连接.begin(); it != 活动连接.end(); ++it)
        {
            if ((*it)->套接字 == 套接字)
            {
                活动连接.erase(it);
                break;
            }
        }
    }

    void 更新连接活动时间(SOCKET 套接字)
    {
        std::lock_guard<std::mutex> 锁(连接列表锁);
        for (auto& 连接 : 活动连接)
        {
            if (连接->套接字 == 套接字)
            {
                连接->最后活动时间 = GetTickCount();
                break;
            }
        }
    }

    void 清空所有连接()
    {
        std::lock_guard<std::mutex> 锁(连接列表锁);

        for (auto& 连接 : 活动连接)
        {
            if (连接->套接字 != INVALID_SOCKET)
            {
                shutdown(连接->套接字, SD_BOTH);
                closesocket(连接->套接字);
            }
        }

        活动连接.clear();

        // 重置连接数
        std::lock_guard<std::mutex> 锁2(连接数锁);
        连接数 = 0;
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

    static void 转发线程函数(端口转发规则* 规则);
    static void 客户端处理线程(SOCKET 客户端套接字, SOCKET 目标套接字, 端口转发规则* 规则);
    static void 转发数据(SOCKET 来源套接字, SOCKET 目标套接字, 端口转发规则* 规则);
    static void 单向转发数据(SOCKET 来源套接字, SOCKET 目标套接字, 端口转发规则* 规则);
    static void 定期连接状态检查(端口转发规则* 规则);
};
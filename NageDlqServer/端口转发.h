// 端口转发.h 
#pragma once

#include <map>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// 端口转发规则结构
struct 端口转发规则
{
    int 序号;
    CString 状态;           // 运行中/已停止
    CString 输入IP;
    int 输入端口;
    CString 输出IP;
    int 输出端口;
    int 连接数;
    bool 运行中;
    SOCKET 监听套接字;
    std::thread* 转发线程;
    std::mutex 连接数锁;

    端口转发规则()
    {
        序号 = 0;
        输入端口 = 0;
        输出端口 = 0;
        运行中 = false;
        监听套接字 = INVALID_SOCKET;
        转发线程 = nullptr;
        连接数 = 0;
    }

    // 禁止拷贝
    端口转发规则(const 端口转发规则&) = delete;
    端口转发规则& operator=(const 端口转发规则&) = delete;

    // 移动构造
    端口转发规则(端口转发规则&& other) noexcept
    {
        *this = std::move(other);
    }

    端口转发规则& operator=(端口转发规则&& other) noexcept
    {
        if (this != &other)
        {
            序号 = other.序号;
            状态 = std::move(other.状态);
            输入IP = std::move(other.输入IP);
            输入端口 = other.输入端口;
            输出IP = std::move(other.输出IP);
            输出端口 = other.输出端口;
            连接数 = other.连接数;
            运行中 = other.运行中;
            监听套接字 = other.监听套接字;
            转发线程 = other.转发线程;

            other.监听套接字 = INVALID_SOCKET;
            other.转发线程 = nullptr;
        }
        return *this;
    }
};

// 端口转发管理类
class 端口转发管理类
{
public:
    端口转发管理类();
    virtual ~端口转发管理类();

    // 启动所有转发规则
    BOOL 启动所有转发();

    // 停止所有转发规则  
    BOOL 停止所有转发();

    // 启动单个转发规则
    BOOL 启动转发规则(int 规则序号);

    // 停止单个转发规则
    BOOL 停止转发规则(int 规则序号);

    // 添加转发规则
    BOOL 添加转发规则(const CString& 输入IP, int 输入端口,
        const CString& 输出IP, int 输出端口);

    // 删除转发规则
    BOOL 删除转发规则(int 规则序号);

    // 更新转发规则
    BOOL 更新转发规则(int 规则序号, const CString& 输入IP, int 输入端口,
        const CString& 输出IP, int 输出端口);

    // 获取规则列表
    std::vector<端口转发规则> 获取规则列表();

    // 配置管理
    BOOL 保存配置();
    BOOL 加载配置();

private:
    std::vector<端口转发规则> 转发规则列表;
    std::mutex 规则列表锁;

    // 转发线程函数
    static void 转发线程函数(端口转发规则* 规则);

    // 客户端处理线程
    static void 客户端处理线程(SOCKET 客户端套接字, SOCKET 目标套接字,
        std::atomic<int>* 连接数指针);

    // 数据转发函数
    static void 转发数据(SOCKET 来源套接字, SOCKET 目标套接字);
};

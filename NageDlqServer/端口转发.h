// 端口转发.h - 修正版本
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
    std::atomic<int> 连接数;
    std::atomic<bool> 运行中;
    SOCKET 监听套接字;
    std::thread* 转发线程;

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

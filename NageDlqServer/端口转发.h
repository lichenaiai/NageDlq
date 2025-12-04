// 端口转发.h
#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

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

    // 修改为普通int加上互斥锁保护
    int 连接数;
    mutable std::mutex 连接数锁;

    SOCKET 监听套接字;
    std::thread* 转发线程;

    // 添加连接数操作的线程安全方法
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

    // 添加构造函数，初始化成员
    端口转发规则() : 序号(0), 输入端口(0), 输出端口(0),
        运行中(FALSE), 连接数(0),
        监听套接字(INVALID_SOCKET),
        转发线程(nullptr) {
    }

    // 添加析构函数
    ~端口转发规则()
    {
        if (转发线程 && 转发线程->joinable())
        {
            转发线程->join();
            delete 转发线程;
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

    static void 转发线程函数(端口转发规则* 规则);
    static void 客户端处理线程(SOCKET 客户端套接字, SOCKET 目标套接字, 端口转发规则* 规则);
    static void 转发数据(SOCKET 来源套接字, SOCKET 目标套接字, 端口转发规则* 规则);
    
};
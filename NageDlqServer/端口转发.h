#pragma once

#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// 端口转发规则结构
struct 端口转发规则
{
    int 序号 = 0;
    CString 状态 = _T("已停止");
    CString 输入IP;
    int 输入端口 = 0;
    CString 输出IP;
    int 输出端口 = 0;
    int 连接数 = 0;
    bool 运行中 = false;
    SOCKET 监听套接字 = INVALID_SOCKET;
    std::thread* 转发线程 = nullptr;
    std::mutex 连接数锁;

    端口转发规则() = default;
};

// 端口转发管理类
class 端口转发管理类
{
public:
    端口转发管理类();
    virtual ~端口转发管理类();

    BOOL 启动所有转发();
    BOOL 停止所有转发();
    BOOL 启动转发规则(int 规则序号);
    BOOL 停止转发规则(int 规则序号);
    BOOL 添加转发规则(const CString& 输入IP, int 输入端口, const CString& 输出IP, int 输出端口);
    BOOL 删除转发规则(int 规则序号);
    BOOL 更新转发规则(int 规则序号, const CString& 输入IP, int 输入端口, const CString& 输出IP, int 输出端口);

    // 修改：返回const引用
    const std::vector<端口转发规则*>& 获取规则列表() const;

    BOOL 保存配置();
    BOOL 加载配置();

private:
    std::vector<端口转发规则*> 转发规则列表;
    std::mutex 规则列表锁;

    static void 转发线程函数(端口转发规则* 规则);
    static void 客户端处理线程(SOCKET 客户端套接字, SOCKET 目标套接字, 端口转发规则* 规则);
    static void 转发数据(SOCKET 来源套接字, SOCKET 目标套接字);
};
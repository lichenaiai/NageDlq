// WebSocket处理类.h
#pragma once

#include <string>
#include <vector>
#include <map>
#include <winsock2.h>
#include <windows.h>
#include <wincrypt.h>  // Windows加密API
#pragma comment(lib, "crypt32.lib")

class WebSocket处理类
{
public:
    WebSocket处理类();
    virtual ~WebSocket处理类();

    // 静态方法
    static bool 是WebSocket握手请求(const std::string& 请求数据);
    static std::string 生成握手响应(const std::string& 请求数据);
    static std::string 生成WebSocketAcceptKey(const std::string& key);

    // WebSocket帧处理
    static std::string 解析WebSocket帧(std::vector<unsigned char>& 缓冲区);
    static std::vector<char> 创建WebSocket帧(const std::string& 消息);
    static std::vector<char> 创建关闭帧();

    // 客户端连接管理
    void 处理WebSocket客户端(SOCKET 客户端套接字, const char* 客户端IP);
    void 发送消息到客户端(SOCKET 客户端套接字, const std::string& 消息);

    // 设置回调函数指针，用于处理业务逻辑
    typedef void (*处理消息回调)(void* 上下文, SOCKET 客户端套接字, const std::string& 消息, const std::string& 客户端IP);

    void 设置回调函数(处理消息回调 回调, void* 上下文);

    // 处理WebSocket消息的核心函数
    void 处理客户端消息(SOCKET 客户端套接字, const std::string& 消息, const std::string& 客户端IP);

    // 向客户端发送响应
    void 发送响应(SOCKET 客户端套接字, const std::string& 响应数据);

private:
	static std::string 提取HTTP头字段(const std::string& 请求数据, const std::string& 字段名);

    // WebSocket帧解析辅助函数
    static bool 解析帧头(const std::vector<unsigned char>& 缓冲区,
        size_t& 帧头长度,
        bool& 是结束帧,
        unsigned char& 操作码,
        bool& 掩码标志,
        unsigned long& 数据长度);

    static std::string 提取帧数据(const std::vector<unsigned char>& 缓冲区,
        size_t 帧头长度,
        unsigned long 数据长度,
        bool 掩码标志);

    // Base64编码
    static std::string Base64编码(const std::vector<unsigned char>& 数据);

    // SHA1哈希计算
    static std::string 计算SHA1(const std::string& 输入);

    // 回调函数
    处理消息回调 消息回调函数;
    void* 回调上下文;
};
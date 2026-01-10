// WebSocket处理类.h
#pragma once
#include <string>
#include <vector>

class WebSocket处理器
{
public:
    WebSocket处理器();
    virtual ~WebSocket处理器();

    // 检查是否是WebSocket握手请求
    static bool 是WebSocket握手请求(const std::string& 请求数据);

    // 处理WebSocket握手
    static std::string 生成握手响应(const std::string& 请求数据);

    // 解析WebSocket数据帧
    static std::string 解析WebSocket帧(const std::vector<char>& 原始数据);

    // 创建WebSocket数据帧
    static std::vector<char> 创建WebSocket帧(const std::string& 消息);

    // 创建WebSocket文本帧
    static std::vector<char> 创建文本帧(const std::string& 消息);

    // 创建WebSocket关闭帧
    static std::vector<char> 创建关闭帧();

    // 计算WebSocket Accept Key
    static std::string 计算AcceptKey(const std::string& 客户端密钥);

private:
    // Base64编码
    static std::string Base64编码(const unsigned char* 数据, size_t 长度);

    // SHA1哈希
    static std::vector<unsigned char> SHA1哈希(const std::string& 输入);
};
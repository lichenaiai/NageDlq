// WebSocket处理类.cpp
#include "pch.h"
#include "WebSocket处理类.h"
#include <winsock2.h>
#include <windows.h>
#include <wincrypt.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

#pragma comment(lib, "crypt32.lib")

WebSocket处理器::WebSocket处理器() {}

WebSocket处理器::~WebSocket处理器() {}

// 检查是否是WebSocket握手请求
bool WebSocket处理器::是WebSocket握手请求(const std::string& 请求数据)
{
    // 检查是否包含WebSocket升级头
    return (请求数据.find("Upgrade: websocket") != std::string::npos &&
        请求数据.find("Connection: Upgrade") != std::string::npos);
}

// 生成握手响应
std::string WebSocket处理器::生成握手响应(const std::string& 请求数据)
{
    // 提取Sec-WebSocket-Key
    size_t key开始位置 = 请求数据.find("Sec-WebSocket-Key: ");
    if (key开始位置 == std::string::npos)
    {
        return "";
    }

    key开始位置 += 19; // "Sec-WebSocket-Key: "的长度
    size_t key结束位置 = 请求数据.find("\r\n", key开始位置);
    if (key结束位置 == std::string::npos)
    {
        return "";
    }

    std::string 客户端密钥 = 请求数据.substr(key开始位置, key结束位置 - key开始位置);
    std::string acceptKey = 计算AcceptKey(客户端密钥);

    if (acceptKey.empty())
    {
        return "";
    }

    // 构建握手响应
    std::string 响应;
    响应 = "HTTP/1.1 101 Switching Protocols\r\n";
    响应 += "Upgrade: websocket\r\n";
    响应 += "Connection: Upgrade\r\n";
    响应 += "Sec-WebSocket-Accept: " + acceptKey + "\r\n";
    响应 += "Sec-WebSocket-Version: 13\r\n";
    响应 += "\r\n";

    return 响应;
}

// 计算Accept Key
std::string WebSocket处理器::计算AcceptKey(const std::string& 客户端密钥)
{
    const std::string 魔法字符串 = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string 组合字符串 = 客户端密钥 + 魔法字符串;

    auto 哈希结果 = SHA1哈希(组合字符串);
    if (哈希结果.empty())
    {
        return "";
    }

    return Base64编码(哈希结果.data(), 哈希结果.size());
}

// SHA1哈希计算
std::vector<unsigned char> WebSocket处理器::SHA1哈希(const std::string& 输入)
{
    HCRYPTPROV 加密提供者 = 0;
    HCRYPTHASH 哈希对象 = 0;
    std::vector<unsigned char> 哈希结果(20);
    DWORD 哈希长度 = 20;

    if (!CryptAcquireContext(&加密提供者, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        return std::vector<unsigned char>();
    }

    if (!CryptCreateHash(加密提供者, CALG_SHA1, 0, 0, &哈希对象))
    {
        CryptReleaseContext(加密提供者, 0);
        return std::vector<unsigned char>();
    }

    if (!CryptHashData(哈希对象, (const BYTE*)输入.c_str(), (DWORD)输入.length(), 0))
    {
        CryptDestroyHash(哈希对象);
        CryptReleaseContext(加密提供者, 0);
        return std::vector<unsigned char>();
    }

    if (!CryptGetHashParam(哈希对象, HP_HASHVAL, 哈希结果.data(), &哈希长度, 0))
    {
        CryptDestroyHash(哈希对象);
        CryptReleaseContext(加密提供者, 0);
        return std::vector<unsigned char>();
    }

    CryptDestroyHash(哈希对象);
    CryptReleaseContext(加密提供者, 0);

    return 哈希结果;
}

// Base64编码
std::string WebSocket处理器::Base64编码(const unsigned char* 数据, size_t 长度)
{
    DWORD base64长度 = 0;

    // 计算所需缓冲区大小
    if (!CryptBinaryToStringA(数据, (DWORD)长度,
        CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &base64长度))
    {
        return "";
    }

    std::vector<char> base64缓冲区(base64长度);

    if (!CryptBinaryToStringA(数据, (DWORD)长度,
        CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, base64缓冲区.data(), &base64长度))
    {
        return "";
    }

    return std::string(base64缓冲区.data(), base64长度 - 1); // 去掉null终止符
}

// 解析WebSocket数据帧
std::string WebSocket处理器::解析WebSocket帧(const std::vector<char>& 原始数据)
{
    if (原始数据.size() < 2)
    {
        return "";
    }

    const unsigned char* 数据指针 = reinterpret_cast<const unsigned char*>(原始数据.data());
    size_t 数据索引 = 0;

    // 读取第一个字节
    unsigned char 字节1 = 数据指针[数据索引++];
    bool 结束帧 = (字节1 & 0x80) != 0; // FIN位
    int 操作码 = 字节1 & 0x0F;

    // 读取第二个字节
    unsigned char 字节2 = 数据指针[数据索引++];
    bool 掩码标志 = (字节2 & 0x80) != 0;
    uint64_t 负载长度 = 字节2 & 0x7F;

    // 处理扩展负载长度
    if (负载长度 == 126)
    {
        if (原始数据.size() < 数据索引 + 2)
        {
            return "";
        }
        负载长度 = (数据指针[数据索引] << 8) | 数据指针[数据索引 + 1];
        数据索引 += 2;
    }
    else if (负载长度 == 127)
    {
        if (原始数据.size() < 数据索引 + 8)
        {
            return "";
        }
        // 只支持32位长度（简化处理）
        负载长度 = 0;
        for (int i = 0; i < 8; i++)
        {
            负载长度 = (负载长度 << 8) | 数据指针[数据索引 + i];
        }
        数据索引 += 8;
    }

    // 读取掩码键
    unsigned char 掩码键[4] = { 0 };
    if (掩码标志)
    {
        if (原始数据.size() < 数据索引 + 4)
        {
            return "";
        }
        for (int i = 0; i < 4; i++)
        {
            掩码键[i] = 数据指针[数据索引++];
        }
    }

    // 检查数据长度是否足够
    if (原始数据.size() < 数据索引 + 负载长度)
    {
        return "";
    }

    // 处理操作码
    if (操作码 == 0x8) // 关闭帧
    {
        return ""; // 返回空字符串表示关闭
    }
    else if (操作码 == 0x9) // Ping帧
    {
        // 应该回复Pong，这里简化处理
        return "";
    }
    else if (操作码 == 0xA) // Pong帧
    {
        return "";
    }
    else if (操作码 != 0x1 && 操作码 != 0x2) // 不是文本或二进制帧
    {
        return "";
    }

    // 读取负载数据
    std::vector<char> 负载数据(负载长度);
    for (size_t i = 0; i < 负载长度; i++)
    {
        负载数据[i] = 数据指针[数据索引++];
    }

    // 应用掩码（如果有）
    if (掩码标志)
    {
        for (size_t i = 0; i < 负载长度; i++)
        {
            负载数据[i] ^= 掩码键[i % 4];
        }
    }

    // 如果是文本帧，转换为字符串
    if (操作码 == 0x1)
    {
        return std::string(负载数据.begin(), 负载数据.end());
    }

    return "";
}

// 创建WebSocket数据帧
std::vector<char> WebSocket处理器::创建WebSocket帧(const std::string& 消息)
{
    return 创建文本帧(消息);
}

// 创建文本帧
std::vector<char> WebSocket处理器::创建文本帧(const std::string& 消息)
{
    std::vector<char> 帧数据;

    // 添加帧头
    // FIN=1, Opcode=1 (文本帧)
    帧数据.push_back(0x81);

    size_t 消息长度 = 消息.length();

    // 添加长度字段
    if (消息长度 <= 125)
    {
        帧数据.push_back(static_cast<char>(消息长度));
    }
    else if (消息长度 <= 65535)
    {
        帧数据.push_back(126);
        帧数据.push_back(static_cast<char>((消息长度 >> 8) & 0xFF));
        帧数据.push_back(static_cast<char>(消息长度 & 0xFF));
    }
    else
    {
        帧数据.push_back(127);
        for (int i = 7; i >= 0; i--)
        {
            帧数据.push_back(static_cast<char>((消息长度 >> (8 * i)) & 0xFF));
        }
    }

    // 添加消息数据
    for (char c : 消息)
    {
        帧数据.push_back(c);
    }

    return 帧数据;
}

// 创建关闭帧
std::vector<char> WebSocket处理器::创建关闭帧()
{
    std::vector<char> 帧数据;

    // FIN=1, Opcode=8 (关闭帧)
    帧数据.push_back(0x88);
    帧数据.push_back(0x00); // 长度0

    return 帧数据;
}
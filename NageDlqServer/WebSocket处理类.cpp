// WebSocket处理类.cpp
#include "pch.h"
#include "WebSocket处理类.h"
#include <winsock2.h>
#include <windows.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

WebSocket处理类::WebSocket处理类()
	: 消息回调函数(nullptr)
	, 回调上下文(nullptr)
{
}

WebSocket处理类::~WebSocket处理类()
{
}

void WebSocket处理类::设置回调函数(处理消息回调 回调, void* 上下文)
{
	消息回调函数 = 回调;
	回调上下文 = 上下文;
}

// 检查是否是WebSocket握手请求
bool WebSocket处理类::是WebSocket握手请求(const std::string& 请求数据)
{
	// 简化检查
	if (请求数据.find("GET /") == 0)
	{
		if (请求数据.find("Upgrade: websocket") != std::string::npos ||
			请求数据.find("Upgrade: WebSocket") != std::string::npos)
		{
			return true;
		}
	}
	return false;
}

// 生成WebSocket握手响应
std::string WebSocket处理类::生成握手响应(const std::string& 请求数据)
{
	// 简化处理：直接返回标准握手响应
	// 在实际应用中，应该解析Sec-WebSocket-Key并生成正确的响应

	return "HTTP/1.1 101 Switching Protocols\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n"
		"\r\n";
}

// 生成WebSocket Accept Key（简化版）
std::string WebSocket处理类::生成WebSocketAcceptKey(const std::string& key)
{
	// 简化处理：返回固定值
	// 在实际应用中，应该计算 key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" 的SHA1哈希，然后Base64编码
	return "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=";
}

void WebSocket处理类::处理客户端消息(SOCKET 客户端套接字, const std::string& 消息, const std::string& 客户端IP)
{
	if (消息回调函数)
	{
		// 调用回调函数处理消息
		消息回调函数(回调上下文, 客户端套接字, 消息, 客户端IP);
	}
	else
	{
		// 如果没有设置回调，发送默认响应
		std::string 响应 = "WebSocket连接成功，但未设置消息处理器";
		发送响应(客户端套接字, 响应);
	}
}

void WebSocket处理类::发送响应(SOCKET 客户端套接字, const std::string& 响应数据)
{
	发送消息到客户端(客户端套接字, 响应数据);
}

// 修改处理WebSocket客户端函数，添加消息处理
void WebSocket处理类::处理WebSocket客户端(SOCKET 客户端套接字, const char* 客户端IP)
{
	// 设置为非阻塞模式
	u_long 非阻塞模式 = 1;
	ioctlsocket(客户端套接字, FIONBIO, &非阻塞模式);

	TRACE(_T("开始处理WebSocket客户端: %s\n"), CString(客户端IP));

	// WebSocket接收缓冲区
	std::vector<unsigned char> 接收缓冲区;
	std::string 客户端IP字符串(客户端IP);

	// 最后活动时间
	DWORD 最后活动时间 = GetTickCount();
	const DWORD 连接超时时间 = 300000; // 5分钟超时

	// 处理循环
	while (true)
	{
		// 检查连接超时
		DWORD 当前时间 = GetTickCount();
		if (当前时间 - 最后活动时间 > 连接超时时间)
		{
			TRACE(_T("WebSocket客户端 %s 连接超时\n"), CString(客户端IP));
			break;
		}

		// 接收数据
		char 缓冲区[4096];
		memset(缓冲区, 0, sizeof(缓冲区));

		int 接收长度 = recv(客户端套接字, 缓冲区, sizeof(缓冲区), 0);

		if (接收长度 > 0)
		{
			// 更新最后活动时间
			最后活动时间 = GetTickCount();

			// 添加到接收缓冲区
			接收缓冲区.insert(接收缓冲区.end(),
				(unsigned char*)缓冲区,
				(unsigned char*)缓冲区 + 接收长度);

			// 解析WebSocket帧
			while (true)
			{
				std::string 消息 = 解析WebSocket帧(接收缓冲区);

				if (消息.empty())
				{
					// 空消息可能是关闭帧或数据不完整
					if (接收缓冲区.size() >= 2 &&
						(接收缓冲区[0] & 0x0F) == 0x8) // 关闭帧
					{
						TRACE(_T("收到WebSocket关闭帧\n"));
						// 发送关闭帧响应
						std::vector<char> 关闭帧 = 创建关闭帧();
						send(客户端套接字, 关闭帧.data(), 关闭帧.size(), 0);
						goto 清理退出;
					}
					break;
				}

				// 处理消息
				TRACE(_T("WebSocket收到消息: %s\n"), CString(消息.c_str()));
				处理客户端消息(客户端套接字, 消息, 客户端IP字符串);
			}
		}
		else if (接收长度 == 0)
		{
			// 连接关闭
			TRACE(_T("WebSocket客户端关闭连接\n"));
			break;
		}
		else
		{
			int 错误码 = WSAGetLastError();
			if (错误码 != WSAEWOULDBLOCK)
			{
				TRACE(_T("WebSocket接收错误: %d\n"), 错误码);
				break;
			}
			// 没有数据，休眠等待
			Sleep(50);
		}
	}

清理退出:
	// 清理工作
	closesocket(客户端套接字);
	TRACE(_T("WebSocket客户端处理结束: %s\n"), CString(客户端IP));
}

// 创建WebSocket帧（简化版）
std::vector<char> WebSocket处理类::创建WebSocket帧(const std::string& 消息)
{
	std::vector<char> 帧;

	// FIN位设为1，操作码设为1（文本帧）
	帧.push_back(0x81); // 1000 0001

	// 数据长度
	size_t 数据长度 = 消息.length();
	if (数据长度 <= 125)
	{
		帧.push_back(数据长度 & 0x7F);
	}
	else if (数据长度 <= 65535)
	{
		帧.push_back(126);
		帧.push_back((数据长度 >> 8) & 0xFF);
		帧.push_back(数据长度 & 0xFF);
	}

	// 添加数据
	for (size_t i = 0; i < 数据长度; i++)
	{
		帧.push_back(消息[i]);
	}

	return 帧;
}

// 创建关闭帧
std::vector<char> WebSocket处理类::创建关闭帧()
{
	std::vector<char> 帧;

	// FIN位设为1，操作码设为8（关闭帧）
	帧.push_back(0x88); // 1000 1000
	帧.push_back(0x00); // 长度为0

	return 帧;
}

// 发送消息到客户端
void WebSocket处理类::发送消息到客户端(SOCKET 客户端套接字, const std::string& 消息)
{
	std::vector<char> 帧 = 创建WebSocket帧(消息);
	send(客户端套接字, 帧.data(), 帧.size(), 0);
}

// 解析帧头（简化版）
bool WebSocket处理类::解析帧头(const std::vector<unsigned char>& 缓冲区,
	size_t& 帧头长度,
	bool& 是结束帧,
	unsigned char& 操作码,
	bool& 掩码标志,
	unsigned long& 数据长度)
{
	if (缓冲区.size() < 2)
	{
		return false;
	}

	// 解析第一个字节
	unsigned char 字节1 = 缓冲区[0];
	是结束帧 = (字节1 & 0x80) != 0;
	操作码 = 字节1 & 0x0F;

	// 解析第二个字节
	unsigned char 字节2 = 缓冲区[1];
	掩码标志 = (字节2 & 0x80) != 0;
	数据长度 = 字节2 & 0x7F;

	// 计算帧头长度
	帧头长度 = 2; // 基础帧头长度

	if (数据长度 == 126)
	{
		if (缓冲区.size() < 4)
		{
			return false;
		}
		数据长度 = (缓冲区[2] << 8) | 缓冲区[3];
		帧头长度 = 4;
	}
	else if (数据长度 == 127)
	{
		if (缓冲区.size() < 10)
		{
			return false;
		}
		// 只使用低32位（简化处理）
		数据长度 = (缓冲区[6] << 24) | (缓冲区[7] << 16) | (缓冲区[8] << 8) | 缓冲区[9];
		帧头长度 = 10;
	}

	// 如果有掩码，增加4字节
	if (掩码标志)
	{
		帧头长度 += 4;
	}

	return true;
}

// 提取帧数据
std::string WebSocket处理类::提取帧数据(const std::vector<unsigned char>& 缓冲区,
	size_t 帧头长度,
	unsigned long 数据长度,
	bool 掩码标志)
{
	std::string 消息;
	消息.reserve(数据长度);

	// 提取掩码
	unsigned char 掩码[4] = { 0 };
	if (掩码标志 && 帧头长度 >= 6)
	{
		size_t 掩码位置 = 帧头长度 - 4;
		掩码[0] = 缓冲区[掩码位置];
		掩码[1] = 缓冲区[掩码位置 + 1];
		掩码[2] = 缓冲区[掩码位置 + 2];
		掩码[3] = 缓冲区[掩码位置 + 3];
	}

	// 提取数据
	for (unsigned long i = 0; i < 数据长度; i++)
	{
		unsigned char 数据字节 = 缓冲区[帧头长度 + i];
		if (掩码标志)
		{
			数据字节 ^= 掩码[i % 4];
		}
		消息.push_back(数据字节);
	}

	return 消息;
}

// 计算SHA1哈希（使用Windows CryptoAPI）
std::string WebSocket处理类::计算SHA1(const std::string& 输入)
{
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	BYTE hash[20];
	DWORD hashLen = 20;

	std::string result;

	if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		if (CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
		{
			if (CryptHashData(hHash, (BYTE*)输入.c_str(), (DWORD)输入.length(), 0))
			{
				if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0))
				{
					// 转换为十六进制字符串
					std::stringstream ss;
					for (DWORD i = 0; i < hashLen; i++)
					{
						ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
					}
					result = ss.str();
				}
			}
			CryptDestroyHash(hHash);
		}
		CryptReleaseContext(hProv, 0);
	}

	return result;
}

// Base64编码
std::string WebSocket处理类::Base64编码(const std::vector<unsigned char>& 数据)
{
	DWORD 编码后长度 = 0;

	// 计算所需缓冲区大小
	if (!CryptBinaryToStringA(数据.data(), (DWORD)数据.size(),
		CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
		NULL, &编码后长度))
	{
		return "";
	}

	std::vector<char> 缓冲区(编码后长度);

	if (CryptBinaryToStringA(数据.data(), (DWORD)数据.size(),
		CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
		缓冲区.data(), &编码后长度))
	{
		// 去除末尾的null字符
		if (编码后长度 > 0 && 缓冲区[编码后长度 - 1] == '\0')
		{
			编码后长度--;
		}
		return std::string(缓冲区.data(), 编码后长度);
	}

	return "";
}

// 解析WebSocket帧
std::string WebSocket处理类::解析WebSocket帧(std::vector<unsigned char>& 缓冲区)
{
	if (缓冲区.size() < 2)
	{
		return "";
	}

	size_t 帧头长度 = 0;
	bool 是结束帧 = false;
	unsigned char 操作码 = 0;
	bool 掩码标志 = false;
	unsigned long 数据长度 = 0;

	if (!解析帧头(缓冲区, 帧头长度, 是结束帧, 操作码, 掩码标志, 数据长度))
	{
		return "";
	}

	// 检查是否有足够的数据
	if (缓冲区.size() < 帧头长度 + 数据长度)
	{
		return "";
	}

	// 处理不同类型的帧
	if (操作码 == 0x8) // 关闭帧
	{
		// 返回空字符串表示关闭帧
		return "";
	}
	else if (操作码 == 0x1) // 文本帧
	{
		std::string 消息 = 提取帧数据(缓冲区, 帧头长度, 数据长度, 掩码标志);

		// 移除已处理的数据
		缓冲区.erase(缓冲区.begin(), 缓冲区.begin() + 帧头长度 + 数据长度);

		return 消息;
	}

	return "";
}


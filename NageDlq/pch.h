// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容

// 添加要在此处预编译的标头
#include "framework.h"
#include <wil/com.h>
#include <WebView2.h>
#include <wrl/client.h>


#endif //PCH_H

#define CLIENT_VERSION "0.80"				   // 客户端版本号 小于数据库版本号时执行更新
#define UPDATE_SERVER "127.2.1.7/nageup/"  // 更新服务器地址  80
#define SERVER_IP "127.2.1.6"              // 登录器服务端IP地址
#define SERVER_PORT 9896						// 登录器服务端端口号
#define WEBSERVER_PORT "127.2.1.7"			//web服务器地址 8080
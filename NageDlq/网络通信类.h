// 网络通信类.h
#pragma once
#include "pch.h"
#include <afxsock.h>  // MFC socket 支持

// 前向声明
class NageDlqDlg;

//typedef void (CWnd::* 消息回调函数类型)(CString);
typedef void (NageDlqDlg::*消息回调函数类型)(CString);

// 网络通信类 - 负责客户端与服务端的通信
class 网络通信类 : public CAsyncSocket
{
    DECLARE_DYNAMIC(网络通信类)

public:
    网络通信类();
    virtual ~网络通信类();

    // 连接服务端
    BOOL 连接服务端(const CString& 服务端地址, UINT 端口号);

    // 发送数据到服务端
    BOOL 发送数据(const CString& 数据);

    // 关闭连接
    void 关闭连接();

    // 检查连接状态
    BOOL 是否已连接() const;

    // 设置回调函数指针
    //void 设置消息回调函数(消息回调函数类型 回调函数, CWnd* 窗口指针);
    //void 设置消息回调函数(void(NageDlqDlg::* 回调函数)(CString), NageDlqDlg* 窗口指针);
    void 设置消息回调函数(消息回调函数类型 回调函数, NageDlqDlg* 窗口指针);

    // Socket事件重写
    virtual void OnConnect(int 错误代码);
    virtual void OnReceive(int 错误代码);
    virtual void OnClose(int 错误代码);
    virtual void OnSend(int 错误代码);

private:
    // 服务端地址和端口
    CString 服务端地址;
    UINT 服务端端口;

    // 连接状态
    BOOL 连接状态;

    // 接收缓冲区
    CString 接收缓冲区;

    // 消息回调函数
    //消息回调函数类型 消息回调函数;
    //CWnd* 回调窗口指针;
    消息回调函数类型 消息回调函数;
    NageDlqDlg* 回调窗口指针;

    // 解析接收到的数据
    void 解析接收数据(const CString& 数据);
};

/*
 * websocket_client.h
 * WebSocket客户端头文件
 * 定义WebSocket客户端的接口和函数
 */

#ifndef __WEBSOCKET_CLIENT_H__
#define __WEBSOCKET_CLIENT_H__

#include <common.h>
#include <os/os.h>

/**
 * @brief 创建WebSocket客户端线程
 * @return BK_OK 成功，其他值失败
 */
int websocket_client_create_thread(void);

#endif /* __WEBSOCKET_CLIENT_H__ */

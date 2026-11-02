/*
 * udp_client.h
 * UDP客户端模块头文件
 * 提供UDP客户端的初始化、连接和数据收发功能
 */

#ifndef __UDP_CLIENT_H__
#define __UDP_CLIENT_H__

#include <os/os.h>
#include <components/log.h>

/**
 * @brief UDP客户端线程函数
 * @param arg 用户参数
 * @return 无
 */
void udp_client_thread(void *arg);

/**
 * @brief 创建UDP客户端线程
 * @return 成功返回BK_OK，失败返回错误码
 */
bk_err_t udp_client_create_thread(void);

#endif /* __UDP_CLIENT_H__ */

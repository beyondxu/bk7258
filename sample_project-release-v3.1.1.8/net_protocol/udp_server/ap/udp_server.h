/*
 * udp_server.h
 * UDP服务器模块头文件
 * 提供UDP服务器的初始化、监听和数据收发功能
 */

#ifndef __UDP_SERVER_H__
#define __UDP_SERVER_H__

#include <os/os.h>
#include <components/log.h>

/**
 * @brief UDP服务器线程函数
 * @param arg 用户参数
 * @return 无
 */
void udp_server_thread(void *arg);

/**
 * @brief 创建UDP服务器线程
 * @return 成功返回BK_OK，失败返回错误码
 */
bk_err_t udp_server_create_thread(void);

#endif /* __UDP_SERVER_H__ */

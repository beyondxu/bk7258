/*
 * tcp_server.h
 * TCP服务器模块头文件
 * 提供TCP服务器的初始化、监听和客户端处理功能
 */

#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include <os/os.h>
#include <components/log.h>

/**
 * @brief TCP服务器线程函数
 * @param arg 用户参数
 * @return 无
 */
void tcp_server_thread(void *arg);

/**
 * @brief 创建TCP服务器线程
 * @return 成功返回BK_OK，失败返回错误码
 */
bk_err_t tcp_server_create_thread(void);

#endif /* __TCP_SERVER_H__ */

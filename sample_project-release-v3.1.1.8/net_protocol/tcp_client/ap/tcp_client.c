/*
 * tcp_client.c
 * TCP客户端模块源文件
 * 实现TCP客户端的初始化、连接和数据收发功能
 */

#include "tcp_client.h"
#include "common.h"
#include <os/mem.h>
#include <os/str.h>
#include <lwip/inet.h>
#include <lwip/sockets.h>
#include <lwip/ip_addr.h>
#include <lwip/inet.h>
#include "wifi_manager.h"
#include "common.h"

/**
 * @brief TCP客户端线程函数
 * 实现TCP客户端的主要逻辑：创建套接字、连接服务器、发送初始消息、循环接收和回复数据
 */
void tcp_client_thread(void *arg)
{
    int sockfd = -1;
    struct sockaddr_in server_addr;
    char buffer[TCP_BUFFER_SIZE];
    int ret;

    TCP_LOGI("TCP client thread started\n");

    // 等待网络连接稳定
    while(1)
    {
        if(wifi_sta_is_link_success()) break;

        rtos_delay_milliseconds(100);
    }

    // 创建TCP套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        TCP_LOGE("socket create failed: %d\n", sockfd);
        goto exit;
    }
    TCP_LOGI("socket create success\n");

    // 设置服务器地址
    os_memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TCP_SERVER_PORT);
    if (inet_pton(AF_INET, TCP_SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        TCP_LOGE("inet_pton failed\n");
        goto exit;
    }

    // 连接服务器
    ret = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        TCP_LOGE("connect failed: %d\n", ret);
        goto exit;
    }
    TCP_LOGI("connect to server %s:%d success\n", TCP_SERVER_IP, TCP_SERVER_PORT);

    // 发送初始消息
    const char *init_msg = "hello, this is beken tcp client";
    ret = send(sockfd, init_msg, os_strlen(init_msg), 0);
    if (ret < 0)
    {
        TCP_LOGE("send failed: %d\n", ret);
        goto exit;
    }
    TCP_LOGI("send init message success: %s\n", init_msg);

    // 循环接收并回复数据
    while (1)
    {
        // 接收服务器数据
        ret = recv(sockfd, buffer, TCP_BUFFER_SIZE - 1, 0);
        if (ret <= 0)
        {
            if (ret == 0)
            {
                TCP_LOGI("server closed connection\n");
            }
            else
            {
                TCP_LOGE("recv failed: %d\n", ret);
            }
            break;
        }

        // 添加字符串结束符
        buffer[ret] = '\0';
        TCP_LOGI("received from server: %s\n", buffer);
        TEST_PASS();

        // 直接回复收到的数据
        ret = send(sockfd, buffer, ret, 0);
        if (ret < 0)
        {
            TCP_LOGE("send reply failed: %d\n", ret);
            break;
        }
        TCP_LOGI("send reply success\n");
    }

exit:
    if (sockfd >= 0)
    {
        close(sockfd);
        TCP_LOGI("socket closed\n");
    }
    rtos_delete_thread(NULL); // 删除当前线程
}

/**
 * @brief 创建TCP客户端线程
 * 创建并启动TCP客户端线程
 */
bk_err_t tcp_client_create_thread(void)
{
    beken_thread_t tcp_thread;
    bk_err_t ret = BK_OK;
    
    do {
        // 创建TCP客户端线程
        ret = rtos_create_thread(&tcp_thread, BEKEN_APPLICATION_PRIORITY, "tcp_client", 
                               tcp_client_thread, 4096, NULL);
        if (ret != BK_OK)
        {
            TCP_LOGE("create tcp client thread failed: %d\n", ret);
            break;
        }
        TCP_LOGI("create tcp client thread success\n");
    } while (0);

    return ret;
}
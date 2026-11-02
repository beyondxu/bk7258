/*
 * tcp_server.c
 * TCP服务器模块源文件
 * 实现TCP服务器的初始化、监听和客户端处理功能
 */

#include "tcp_server.h"
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
 * @brief 处理客户端连接的线程函数
 * 为每个客户端连接创建独立线程，处理数据收发
 */
static void tcp_client_handler(void *arg)
{
    int client_sockfd = (int)arg;
    char buffer[TCP_BUFFER_SIZE];
    int ret;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // 获取客户端地址信息
    getpeername(client_sockfd, (struct sockaddr *)&client_addr, &addr_len);
    TCP_LOGI("client connected from %s:%d\n", 
             inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // 循环接收并回复数据
    while (1)
    {
        // 接收客户端数据
        ret = recv(client_sockfd, buffer, TCP_BUFFER_SIZE - 1, 0);
        if (ret <= 0)
        {
            if (ret == 0)
            {
                TCP_LOGI("client disconnected\n");
            }
            else
            {
                TCP_LOGE("recv failed: %d\n", ret);
            }
            break;
        }

        // 添加字符串结束符
        buffer[ret] = '\0';
        TCP_LOGI("received from client: %s\n", buffer);
        TEST_PASS();

        // 直接回复收到的数据
        ret = send(client_sockfd, buffer, ret, 0);
        if (ret < 0)
        {
            TCP_LOGE("send reply failed: %d\n", ret);
            break;
        }
        TCP_LOGI("send reply success\n");
    }

    // 关闭客户端套接字
    close(client_sockfd);
    TCP_LOGI("client socket closed\n");
    rtos_delete_thread(NULL); // 删除当前线程
}

/**
 * @brief TCP服务器线程函数
 * 实现TCP服务器的主要逻辑：创建监听套接字、绑定、监听、接受客户端连接
 */
void tcp_server_thread(void *arg)
{
    int server_sockfd = -1;
    int client_sockfd = -1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int ret;
    int opt = 1;

    TCP_LOGI("TCP server thread started\n");

    // 等待网络连接稳定
    while(1)
    {
        if(wifi_sta_is_link_success()) break;

        rtos_delay_milliseconds(100);
    }

    // 创建TCP套接字
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd < 0)
    {
        TCP_LOGE("socket create failed: %d\n", server_sockfd);
        goto exit;
    }
    TCP_LOGI("socket create success\n");

    // 设置套接字选项，允许地址重用
    ret = setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret < 0)
    {
        TCP_LOGE("setsockopt failed: %d\n", ret);
        goto exit;
    }

    // 设置服务器地址
    os_memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听所有网络接口
    server_addr.sin_port = htons(TCP_SERVER_PORT);

    // 绑定套接字到指定端口
    ret = bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        TCP_LOGE("bind failed: %d\n", ret);
        goto exit;
    }
    TCP_LOGI("bind to port %d success\n", TCP_SERVER_PORT);

    // 开始监听
    ret = listen(server_sockfd, TCP_MAX_CLIENTS);
    if (ret < 0)
    {
        TCP_LOGE("listen failed: %d\n", ret);
        goto exit;
    }
    TCP_LOGI("listening on port %d, max clients: %d\n", TCP_SERVER_PORT, TCP_MAX_CLIENTS);

    // 循环接受客户端连接
    while (1)
    {
        // 接受客户端连接
        client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sockfd < 0)
        {
            TCP_LOGE("accept failed: %d\n", client_sockfd);
            continue;
        }

        TCP_LOGI("accepted client connection\n");

        // 为每个客户端创建处理线程
        beken_thread_t client_thread;
        ret = rtos_create_thread(&client_thread, BEKEN_APPLICATION_PRIORITY, "tcp_client", 
                               tcp_client_handler, 4096, (void *)client_sockfd);
        if (ret != BK_OK)
        {
            TCP_LOGE("create client handler thread failed: %d\n", ret);
            close(client_sockfd);
            continue;
        }
    }

exit:
    if (server_sockfd >= 0)
    {
        close(server_sockfd);
        TCP_LOGI("server socket closed\n");
    }
    rtos_delete_thread(NULL); // 删除当前线程
}

/**
 * @brief 创建TCP服务器线程
 * 创建并启动TCP服务器线程
 */
bk_err_t tcp_server_create_thread(void)
{
    beken_thread_t tcp_thread;
    bk_err_t ret = BK_OK;
    
    do {
        // 创建TCP服务器线程
        ret = rtos_create_thread(&tcp_thread, BEKEN_APPLICATION_PRIORITY, "tcp_server", 
                               tcp_server_thread, 4096, NULL);
        if (ret != BK_OK)
        {
            TCP_LOGE("create tcp server thread failed: %d\n", ret);
            break;
        }
        TCP_LOGI("create tcp server thread success\n");
    } while (0);

    return ret;
}

/*
 * udp_client.c
 * UDP客户端模块源文件
 * 实现UDP客户端的初始化、连接和数据收发功能
 */

#include "udp_client.h"
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
 * @brief UDP客户端线程函数
 * 实现UDP客户端的主要逻辑：创建套接字、发送数据到服务器
 */
void udp_client_thread(void *arg)
{
    int sockfd = -1;
    struct sockaddr_in server_addr;
    int ret;
    int count = 0;

    UDP_LOGI("UDP client thread started\n");

    // 等待网络连接稳定
    while(1)
    {
        if(wifi_sta_is_link_success()) break;

        rtos_delay_milliseconds(100);
    }

    // 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        UDP_LOGE("socket create failed: %d\n", sockfd);
        goto exit;
    }
    UDP_LOGI("socket create success\n");

    // 设置服务器地址
    os_memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_SERVER_PORT);
    if (inet_pton(AF_INET, UDP_SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        UDP_LOGE("inet_pton failed\n");
        goto exit;
    }

    UDP_LOGI("sending to server %s:%d\n", UDP_SERVER_IP, UDP_SERVER_PORT);

    // 循环发送数据到服务器
    while (1)
    {
        char msg[128];
        os_snprintf(msg, sizeof(msg), "hello from udp client, count=%d", count++);
        
        ret = sendto(sockfd, msg, os_strlen(msg), 0, 
                     (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (ret < 0)
        {
            UDP_LOGE("sendto failed: %d\n", ret);
            rtos_delay_milliseconds(100);
            continue;
        }
        
        UDP_LOGI("send success: %s\n", msg);
        TEST_PASS();
        
        // 延迟1秒后继续发送
        rtos_delay_milliseconds(1000);
    }

exit:
    if (sockfd >= 0)
    {
        close(sockfd);
        UDP_LOGI("socket closed\n");
    }
    rtos_delete_thread(NULL); // 删除当前线程
}

/**
 * @brief 创建UDP客户端线程
 * 创建并启动UDP客户端线程
 */
bk_err_t udp_client_create_thread(void)
{
    beken_thread_t udp_thread;
    bk_err_t ret = BK_OK;
    
    do {
        // 创建UDP客户端线程
        ret = rtos_create_thread(&udp_thread, BEKEN_APPLICATION_PRIORITY, "udp_client", 
                               udp_client_thread, 4096, NULL);
        if (ret != BK_OK)
        {
            UDP_LOGE("create udp client thread failed: %d\n", ret);
            break;
        }
        UDP_LOGI("create udp client thread success\n");
    } while (0);

    return ret;
}

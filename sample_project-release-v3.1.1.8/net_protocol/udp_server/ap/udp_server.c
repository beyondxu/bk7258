/*
 * udp_server.c
 * UDP服务器模块源文件
 * 实现UDP服务器的初始化、监听和数据收发功能
 */

#include "udp_server.h"
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
 * @brief UDP服务器线程函数
 * 实现UDP服务器的主要逻辑：创建UDP套接字、绑定、循环接收和回复数据
 */
void udp_server_thread(void *arg)
{
    int server_sockfd = -1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int ret;
    char buffer[UDP_BUFFER_SIZE];

    UDP_LOGI("UDP server thread started\n");

    // 等待网络连接稳定
    while(1)
    {
        if(wifi_sta_is_link_success()) break;

        rtos_delay_milliseconds(100);
    }

    // 创建UDP套接字
    server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sockfd < 0)
    {
        UDP_LOGE("socket create failed: %d\n", server_sockfd);
        goto exit;
    }
    UDP_LOGI("socket create success\n");

    // 设置服务器地址
    os_memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听所有网络接口
    server_addr.sin_port = htons(UDP_SERVER_PORT);

    // 绑定套接字到指定端口
    ret = bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        UDP_LOGE("bind failed: %d\n", ret);
        goto exit;
    }
    UDP_LOGI("bind to port %d success\n", UDP_SERVER_PORT);

    // 循环接收和回复数据
    while (1)
    {
        // 重置地址长度
        addr_len = sizeof(client_addr);
        
        // 接收客户端数据
        ret = recvfrom(server_sockfd, buffer, UDP_BUFFER_SIZE - 1, 0, 
                      (struct sockaddr *)&client_addr, &addr_len);
        if (ret < 0)
        {
            UDP_LOGE("recvfrom failed: %d\n", ret);
            // 接收失败延迟后继续
            rtos_delay_milliseconds(100);
            continue;
        }
        else if (ret == 0)
        {
            UDP_LOGI("received empty packet from %s:%d\n", 
                    inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            continue;
        }

        // 添加字符串结束符
        buffer[ret] = '\0';
        UDP_LOGI("received from %s:%d: %s\n", 
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);
        TEST_PASS();

        // 直接回复收到的数据
        ret = sendto(server_sockfd, buffer, ret, 0, 
                    (struct sockaddr *)&client_addr, addr_len);
        if (ret < 0)
        {
            UDP_LOGE("sendto failed: %d\n", ret);
            // 发送失败延迟后继续
            rtos_delay_milliseconds(100);
            continue;
        }
        UDP_LOGI("send reply to %s:%d success\n", 
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    }

exit:
    if (server_sockfd >= 0)
    {
        close(server_sockfd);
        UDP_LOGI("server socket closed\n");
    }
    rtos_delete_thread(NULL); // 删除当前线程
}

/**
 * @brief 创建UDP服务器线程
 * 创建并启动UDP服务器线程
 */
bk_err_t udp_server_create_thread(void)
{
    beken_thread_t udp_thread;
    bk_err_t ret = BK_OK;
    
    do {
        // 创建UDP服务器线程
        ret = rtos_create_thread(&udp_thread, BEKEN_APPLICATION_PRIORITY, "udp_server", 
                               udp_server_thread, 4096, NULL);
        if (ret != BK_OK)
        {
            UDP_LOGE("create udp server thread failed: %d\n", ret);
            break;
        }
        UDP_LOGI("create udp server thread success\n");
    } while (0);

    return ret;
}

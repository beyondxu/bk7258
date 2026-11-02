/*
 * ap_main.c
 * 主程序入口文件
 * 整合WiFi管理和TCP客户端模块，实现完整的网络通信功能
 */

#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include <modules/wifi.h>
#include <components/log.h>
#include <components/netif.h>
#include <components/event.h>
#include <bk_private/bk_init.h>
#include <bk_private/bk_wifi.h>
#include <lwip/inet.h>
#include "common.h"
#include "wifi_manager.h"
#include "tcp_client.h"

int main(void)
{
    int ret = 0;

    bk_init();
    
    do {
        // 初始化Wi-Fi STA模式并尝试连接
        ret = wifi_sta_connect(WIFI_AP_SSID, WIFI_AP_PASSWORD);
        if (ret != BK_OK)
        {
            TCP_LOGE("Failed to connect to WiFi: %d\n", ret);
            TEST_FAIL();
            break;
        }

        // 创建TCP客户端线程
        ret = tcp_client_create_thread();
        if (ret != BK_OK)
        {
            TCP_LOGE("create tcp client thread failed: %d\n", ret);
            TEST_FAIL();
            break;
        }
        TCP_LOGI("create tcp client thread success\n");
    } while (0);

    return ret;
}
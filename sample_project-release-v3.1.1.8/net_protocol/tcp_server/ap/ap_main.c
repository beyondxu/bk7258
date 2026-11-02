/*
 * ap_main.c
 * 主程序入口文件
 * 整合WiFi管理和TCP服务器模块，实现完整的网络通信功能
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
#include "tcp_server.h"

int main(void)
{
    int ret = 0;

    bk_init();
    
    do {
        // 初始化Wi-Fi STA模式并连接到AP
        ret = wifi_sta_connect(WIFI_AP_SSID, WIFI_AP_PASSWORD);
        if (ret != BK_OK)
        {
            TCP_LOGE("Failed to connect WiFi: %d\n", ret);
            TEST_FAIL();
            break;
        }

        // 创建TCP服务器线程
        ret = tcp_server_create_thread();
        if (ret != BK_OK)
        {
            TCP_LOGE("create tcp server thread failed: %d\n", ret);
            TEST_FAIL();
            break;
        }
        TCP_LOGI("create tcp server thread success\n");
    } while (0);

    return ret;
}
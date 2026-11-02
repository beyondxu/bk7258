/*
 * ap_main.c
 * 主程序入口文件
 * 整合WiFi管理和WebSocket客户端模块，实现完整的网络通信功能
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
#include "websocket_client.h"

int main(void)
{
    int ret = 0;

    bk_init();
    
    do {
        // 初始化Wi-Fi STA模式并尝试连接
        ret = wifi_sta_connect(WIFI_AP_SSID, WIFI_AP_PASSWORD);
        if (ret != BK_OK)
        {
            WS_LOGE("Failed to connect to WiFi: %d\n", ret);
            TEST_FAIL();
            break;
        }

        // 创建WebSocket客户端线程
        ret = websocket_client_create_thread();
        if (ret != BK_OK)
        {
            WS_LOGE("create websocket client thread failed: %d\n", ret);
            TEST_FAIL();
            break;
        }
        WS_LOGI("create websocket client thread success\n");
    } while (0);

    return ret;
}
/*
 * wifi_manager.c
 * WiFi管理模块源文件
 * 实现WiFi STA模式的初始化、连接和事件处理功能
 */

#include "wifi_manager.h"
#include "common.h"
#include <os/mem.h>
#include <os/str.h>
#include <bk_private/bk_wifi.h>
#include <components/log.h>
#include <components/netif.h>
#include <components/event.h>

/**
 * @brief 网络接口事件回调函数
 * 处理网络接口相关的事件，如获取IP地址
 */
static int wifi_netif_event_cb(void *arg, event_module_t event_module, int event_id, void *event_data)
{
    netif_event_got_ip4_t *got_ip;

    switch (event_id)
    {
        // 处理获取到 IPv4 地址的事件
        case EVENT_NETIF_GOT_IP4:
            got_ip = (netif_event_got_ip4_t *)event_data;
            UDP_LOGI("%s got ip:%s\n", got_ip->netif_if == NETIF_IF_STA ? "STA" : "unknown netif", got_ip->ip);
            break;
        default:
            UDP_LOGI("rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}

/**
 * @brief Wi-Fi STA模式事件回调函数
 * 处理Wi-Fi STA相关的事件，如连接成功和断开连接
 */
static int wifi_sta_event_cb(void *arg, event_module_t event_module, int event_id, void *event_data)
{
    wifi_event_sta_disconnected_t *sta_disconnected;
    wifi_event_sta_connected_t *sta_connected;

    // 根据事件ID处理不同的Wi-Fi STA事件
    switch (event_id)
    {
        // 处理STA连接成功的事件
        case EVENT_WIFI_STA_CONNECTED:
            sta_connected = (wifi_event_sta_connected_t *)event_data;
            UDP_LOGI("STA connected to %s\n", sta_connected->ssid);
            break;

        // 处理STA断开连接的事件
        case EVENT_WIFI_STA_DISCONNECTED:
            sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
            UDP_LOGI("STA disconnected, reason(%d)\n", sta_disconnected->disconnect_reason);
            break;

        default:
            UDP_LOGI("rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}

/**
 * @brief 注册WiFi相关事件回调函数
 * 注册网络接口事件和WiFi事件的回调函数
 */
static bk_err_t wifi_register_events(void)
{
    bk_err_t ret = BK_OK;
    do {
        // 注册WiFi事件回调函数，处理所有WiFi事件
        ret = bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, wifi_sta_event_cb, NULL);
        if (ret != BK_OK)
        {
            UDP_LOGE("Failed to register WiFi event callback: %d\n", ret);
            break;
        }
        
        // 注册网络事件回调函数，处理所有网络接口事件
        ret = bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, wifi_netif_event_cb, NULL);
        if (ret != BK_OK)
        {
            UDP_LOGE("Failed to register netif event callback: %d\n", ret);
            break;
        }
    } while (0);
    
    return ret;
}

/**
 * @brief 初始化Wi-Fi STA模式并尝试连接到指定的Wi-Fi网络
 * 配置WiFi参数并启动STA模式
 * @param ssid WiFi热点名称
 * @param password WiFi密码
 * @return 操作结果，成功返回BK_OK，失败返回错误码
 */
int wifi_sta_connect(const char *ssid, const char *password)
{
    // 使用默认配置初始化Wi-Fi STA配置结构体
    wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();
    int ret = BK_OK;

    do {
        // 参数有效性检查
        if (ssid == NULL)
        {
            UDP_LOGE("Invalid WiFi credentials\n");
            ret = BK_ERR_PARAM;
            break;
        }

        // 设置WiFi SSID和密码
        os_memset(sta_config.ssid, 0, sizeof(sta_config.ssid));
        os_strncpy(sta_config.ssid, ssid, os_strlen(ssid));
        os_memset(sta_config.password, 0, sizeof(sta_config.password));
        if(password)
        {
            os_strncpy(sta_config.password, password, os_strlen(password));
        }

        // 设置WiFi配置
        ret = bk_wifi_sta_set_config(&sta_config);
        if (ret != BK_OK)
        {
            UDP_LOGE("Failed to set WiFi config: %d\n", ret);
            break;
        }

        // 注册WiFi事件回调
        ret = wifi_register_events();
        if (ret != BK_OK)
        {
            UDP_LOGE("Failed to register WiFi events: %d\n", ret);
            break;
        }

        // 启动WiFi STA模式
        ret = bk_wifi_sta_start();
        if (ret != BK_OK)
        {
            UDP_LOGE("Failed to start WiFi STA: %d\n", ret);
            break;
        }

        UDP_LOGI("WiFi connecting to SSID: %s\n", sta_config.ssid);
    } while (0);
    
    return ret;
}
/**
 * @brief wifi是否连接成功

 * @param 无
 * @return true 连接成功  false 正在连接
 */
bool wifi_sta_is_link_success(void)
{
    wifi_link_status_t link_status = {0};

    bk_wifi_sta_get_link_status(&link_status);

    return link_status.state == WIFI_LINKSTATE_STA_GOT_IP;
}


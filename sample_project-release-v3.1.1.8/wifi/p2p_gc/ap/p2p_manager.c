#include "common.h"
#include <os/mem.h>
#include <os/str.h>
#include <bk_private/bk_wifi.h>
#include <components/log.h>
#include <components/netif.h>
#include <components/event.h>
#include <modules/wifi.h>

static int p2p_netif_event_cb(void *arg, event_module_t event_module, int event_id, void *event_data)
{
    netif_event_got_ip4_t *got_ip;

    switch (event_id)
    {
        case EVENT_NETIF_GOT_IP4:
            got_ip = (netif_event_got_ip4_t *)event_data;
            P2P_LOGI("%s got ip:%s\n", got_ip->netif_if == NETIF_IF_STA ? "STA" : "unknown netif", got_ip->ip);
            break;
        default:
            P2P_LOGI("rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}

static int p2p_wifi_event_cb(void *arg, event_module_t event_module, int event_id, void *event_data)
{
    wifi_event_sta_disconnected_t *sta_disconnected;
    wifi_event_sta_connected_t *sta_connected;
    wifi_event_ap_disconnected_t *ap_disconnected;
    wifi_event_ap_connected_t *ap_connected;

    switch (event_id)
    {
        case EVENT_WIFI_STA_CONNECTED:
            sta_connected = (wifi_event_sta_connected_t *)event_data;
            P2P_LOGI("[%s][%d] P2P connected success:%s\n", __FUNCTION__, __LINE__, sta_connected->ssid);
            break;

        case EVENT_WIFI_STA_DISCONNECTED:
            sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
            P2P_LOGI("[%s][%d] P2P STA disconnected, reason(%d)\n", __FUNCTION__, __LINE__, sta_disconnected->disconnect_reason);
            break;

        case EVENT_WIFI_AP_CONNECTED:
            ap_connected = (wifi_event_ap_connected_t *)event_data;
            P2P_LOGI("[%s][%d]" BK_MAC_FORMAT" connected to BK AP\n", __FUNCTION__, __LINE__, BK_MAC_STR(ap_connected->mac));
            break;

        case EVENT_WIFI_AP_DISCONNECTED:
            ap_disconnected = (wifi_event_ap_disconnected_t *)event_data;
            P2P_LOGI("[%s][%d] "BK_MAC_FORMAT" disconnected from BK AP\n", __FUNCTION__, __LINE__, BK_MAC_STR(ap_disconnected->mac));
            break;

        case EVENT_WIFI_GO_DISCONNECTED:
            P2P_LOGI("[%s][%d] p2p go disconnected\n", __FUNCTION__, __LINE__);
            break;

        default:
            P2P_LOGI("rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}

static bk_err_t p2p_register_events(void)
{
    bk_err_t ret = BK_OK;
    do {
        ret = bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, p2p_wifi_event_cb, NULL);
        if (ret != BK_OK)
        {
            P2P_LOGE("Failed to register WiFi event callback: %d\n", ret);
            break;
        }
        
        ret = bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, p2p_netif_event_cb, NULL);
        if (ret != BK_OK)
        {
            P2P_LOGE("Failed to register netif event callback: %d\n", ret);
            break;
        }
    } while (0);
    
    return ret;
}

/**
 * @brief 启动P2P GC（Group Client）模式并开始设备发现
 * @param ssid P2P组SSID名称
 * @return BK_OK成功，其他错误码表示失败
 */
int app_p2p_gc_connect(const char *ssid)
{
    int ret = BK_OK;

    do {
        if (ssid == NULL)
        {
            P2P_LOGE("Invalid P2P SSID\n");
            ret = BK_ERR_PARAM;
            break;
        }

        ret = p2p_register_events();
        if (ret != BK_OK)
        {
            P2P_LOGE("Failed to register P2P events: %d\n", ret);
            break;
        }

        ret = bk_wifi_p2p_enable_with_intent(ssid, 0);
        if (ret != BK_OK)
        {
            P2P_LOGE("Failed to enable P2P: %d\n", ret);
            break;
        }

        ret = bk_wifi_p2p_find();
        if (ret != BK_OK)
        {
            P2P_LOGE("Failed to start P2P find: %d\n", ret);
            break;
        }

        P2P_LOGI("P2P GC connecting with SSID: %s\n", ssid);
    } while (0);
    
    return ret;
}

/**
 * @brief 启动P2P GO（Group Owner）模式并开始设备发现
 * @param ssid P2P组SSID名称
 * @return BK_OK成功，其他错误码表示失败
 */
int app_p2p_go_start(const char *ssid)
{
    int ret = BK_OK;

    do {
        if (ssid == NULL)
        {
            P2P_LOGE("Invalid P2P SSID\n");
            ret = BK_ERR_PARAM;
            break;
        }

        ret = p2p_register_events();
        if (ret != BK_OK)
        {
            P2P_LOGE("Failed to register P2P events: %d\n", ret);
            break;
        }

        ret = bk_wifi_p2p_enable_with_intent(ssid, 15);
        if (ret != BK_OK)
        {
            P2P_LOGE("Failed to enable P2P: %d\n", ret);
            break;
        }

        ret = bk_wifi_p2p_find();
        if (ret != BK_OK)
        {
            P2P_LOGE("Failed to start P2P find: %d\n", ret);
            break;
        }

        P2P_LOGI("P2P GC connecting with SSID: %s\n", ssid);
    } while (0);
    
    return ret;
}

/**
 * @brief 断开P2P GO（Group Owner）或GC（Group Client）连接
 * @return BK_OK成功，其他错误码表示失败
 */
int app_p2p_disconnect(void)
{
    int ret = BK_OK;

    ret = bk_wifi_p2p_disable();
    if (ret != BK_OK)
    {
        P2P_LOGE("Failed to disable P2P: %d\n", ret);
    }
    else
    {
        P2P_LOGI("P2P GO or GC disconnected\n");
    }
    
    return ret;
}

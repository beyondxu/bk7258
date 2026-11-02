#include <os/os.h>
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


#define TAG "wifi_AP"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

#define WIFI_AP_SSID         "sample_project_ap"
#define WIFI_AP_PASSWORD     "12345678"


// 网络接口事件回调函数，用于处理网络接口相关的事件
static int wifi_netif_event_cb(void *arg, event_module_t event_module,
                               int event_id, void *event_data)
{
    netif_event_got_ip4_t *got_ip;

    switch (event_id)
    {
        // 处理获取到 IPv4 地址的事件
        case EVENT_NETIF_GOT_IP4:
            TEST_PASS();
            got_ip = (netif_event_got_ip4_t *)event_data;
            LOGI("%s got ip:%s\n", got_ip->netif_if == NETIF_IF_STA ? "STA" : "unknown netif", got_ip->ip);
            break;
        default:
            LOGI("rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}

// Wi-Fi STA 模式事件回调函数，用于处理 Wi-Fi STA 相关的事件
static int wifi_sta_event_cb(void *arg, event_module_t event_module,
                             int event_id, void *event_data)
{
    wifi_event_sta_disconnected_t *sta_disconnected;
    wifi_event_sta_connected_t *sta_connected;

    // 根据事件 ID 处理不同的 Wi-Fi STA 事件
    switch (event_id)
    {
        // 处理 STA 连接成功的事件
        case EVENT_WIFI_STA_CONNECTED:
            sta_connected = (wifi_event_sta_connected_t *)event_data;
            LOGI("STA connected to %s\n", sta_connected->ssid);
            break;

        // 处理 STA 断开连接的事件
        case EVENT_WIFI_STA_DISCONNECTED:
            sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
            LOGI("STA disconnected, reason(%d)\n", sta_disconnected->disconnect_reason);
            break;

        default:
            LOGI("rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}

// 初始化 Wi-Fi STA 模式并尝试连接到指定的 Wi-Fi 网络
static void wifi_sta_init_and_connect(void)
{
    // 使用默认配置初始化 Wi-Fi STA 配置结构体
    wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();

    os_memset(sta_config.ssid, 0, sizeof(sta_config.ssid));
    os_strncpy(sta_config.ssid, WIFI_AP_SSID, os_strlen(WIFI_AP_SSID));
    os_memset(sta_config.password, 0, sizeof(sta_config.password));
    os_strncpy(sta_config.password, WIFI_AP_PASSWORD, os_strlen(WIFI_AP_PASSWORD));
    BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
    BK_LOG_ON_ERR(bk_wifi_sta_start()); // 启动 Wi-Fi STA 模式，并在出错时记录日志
    
    LOGI("ssid:%s password:%s\n", sta_config.ssid, sta_config.password);
}

int main(void)
{
    bk_init();
    media_service_init();

    // 注册 Wi-Fi 事件回调函数，处理所有 Wi-Fi 事件，网络接口事件
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, wifi_sta_event_cb, NULL));
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, wifi_netif_event_cb, NULL));

    // 初始化 Wi-Fi STA 模式并尝试连接
    wifi_sta_init_and_connect();

    return 0;
}

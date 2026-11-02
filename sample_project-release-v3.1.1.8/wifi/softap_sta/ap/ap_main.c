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

#define WIFI_AP_SSID                  "sample_project_ap"    //AP模式热点名
#define WIFI_AP_PASSWORD              "12345678"
#define DEFAULT_IP                    "192.168.0.1"
#define DEFAULT_MASK                  "255.255.255.0"
#define DEFAULT_GATEWAY               "192.168.0.1"
#define DEFAULT_DNS                   "192.168.0.1"

#define WIFI_STA_CONNECT_SSID         "E0_xiao"              //sta模式连接的热点名
#define WIFI_STA_CONNECT_PASSWORD     "12345678"

/**
 * @brief 打印连接到 AP 的 STA 设备信息
 *
 * @param stas 指向 wlan_ap_stas_t 结构体的指针，包含连接到 AP 的 STA 设备信息
 */
static void ap_print_sta_info(wlan_ap_stas_t *stas)
{
    int i;

    // 打印连接到 AP 的 STA 设备数量
    LOGI("sta_num: %d\n", stas->num);

    // 遍历所有连接到 AP 的 STA 设备
    for (i = 0; i < stas->num; i++)
    {
        LOGI("[%02d]Mac addr: %02x:%02x:%02x:%02x:%02x:%02x, ",
                       i + 1, stas->sta[i].addr[0], stas->sta[i].addr[1],
                       stas->sta[i].addr[2], stas->sta[i].addr[3],
                       stas->sta[i].addr[4], stas->sta[i].addr[5]);
        LOGI("ip: %s, rssi %d\n", inet_ntoa(stas->sta[i].ipaddr), stas->sta[i].rssi);
    }
}

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

// Wi-Fi 事件回调函数，用于 Wi-Fi STA 和 AP 相关的事件
static int wifi_event_cb(void *arg, event_module_t event_module,
                             int event_id, void *event_data)
{
    wifi_event_sta_disconnected_t *sta_disconnected;
    wifi_event_sta_connected_t *sta_connected;
    wifi_event_ap_disconnected_t *ap_disconnected;
    wifi_event_ap_connected_t *ap_connected;

    // 根据事件 ID 处理不同的 Wi-Fi STA 事件
    switch (event_id)
    {
        case EVENT_WIFI_AP_CONNECTED:
            ap_connected = (wifi_event_ap_connected_t *)event_data;
            LOGI(BK_MAC_FORMAT" connected to AP\n", BK_MAC_STR(ap_connected->mac));
            TEST_PASS();
            // 等待设备DHCP 地址分配完成
            rtos_delay_milliseconds(1000);
            
            wlan_ap_stas_t stas;
            os_memset(&stas, 0, sizeof(stas));
            if (BK_OK == bk_wifi_ap_get_sta_list(&stas))
            {
                ap_print_sta_info(&stas);
            }

            if (stas.sta)
            {
                os_free(stas.sta);
            }
            break;

        case EVENT_WIFI_AP_DISCONNECTED:
            ap_disconnected = (wifi_event_ap_disconnected_t *)event_data;
            LOGI(BK_MAC_FORMAT" disconnected from AP\n", BK_MAC_STR(ap_disconnected->mac));
            break;

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
    os_strncpy(sta_config.ssid, WIFI_STA_CONNECT_SSID, os_strlen(WIFI_STA_CONNECT_SSID));
    os_memset(sta_config.password, 0, sizeof(sta_config.password));
    os_strncpy(sta_config.password, WIFI_STA_CONNECT_PASSWORD, os_strlen(WIFI_STA_CONNECT_PASSWORD));
    BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
    BK_LOG_ON_ERR(bk_wifi_sta_start()); // 启动 Wi-Fi STA 模式，并在出错时记录日志
    
    LOGI("ssid:%s password:%s\n", sta_config.ssid, sta_config.password);
}

static void wifi_softap_start(void)
{
    // 使用默认配置初始化 AP 配置结构体
    wifi_ap_config_t ap_config = WIFI_DEFAULT_AP_CONFIG();
    netif_ip4_config_t ip4_config = {0};

    os_memset(&ip4_config, 0, sizeof(ip4_config));
    os_strcpy(ip4_config.ip, DEFAULT_IP);
    os_strcpy(ip4_config.mask, DEFAULT_MASK);
    os_strcpy(ip4_config.gateway, DEFAULT_GATEWAY);
    os_strcpy(ip4_config.dns, DEFAULT_DNS);
    BK_LOG_ON_ERR(bk_netif_set_ip4_config(NETIF_IF_AP, &ip4_config));     // 设置 AP 网络接口的 IPv4 配置

    // 设置 AP 的 SSID
    os_memset(ap_config.ssid, 0, sizeof(ap_config.ssid));
    os_strncpy(ap_config.ssid, WIFI_AP_SSID, os_strlen(WIFI_AP_SSID));
    // 设置 AP 的密码
    os_memset(ap_config.password, 0, sizeof(ap_config.password));
    os_strncpy(ap_config.password, WIFI_AP_PASSWORD, os_strlen(WIFI_AP_PASSWORD));

    LOGI("ssid:%s  key:%s\r\n", ap_config.ssid, ap_config.password);
    BK_LOG_ON_ERR(bk_wifi_ap_set_config(&ap_config));
    BK_LOG_ON_ERR(bk_wifi_ap_start());                                    // 启动 AP
}

int main(void)
{
    bk_init();
    media_service_init();

    // 注册 Wi-Fi 事件回调函数，处理所有 Wi-Fi 事件，网络接口事件
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, wifi_event_cb, NULL));
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, wifi_netif_event_cb, NULL));

    // 初始化 Wi-Fi STA 模式并尝试连接
    wifi_sta_init_and_connect();

    // 启动 WiFi SoftAP 功能
    wifi_softap_start();

    return 0;
}

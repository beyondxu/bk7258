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
#define DEFAULT_IP           "192.168.0.1"
#define DEFAULT_MASK         "255.255.255.0"
#define DEFAULT_GATEWAY      "192.168.0.1"
#define DEFAULT_DNS          "192.168.0.1"

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


/**
 * @brief WiFi SoftAP 事件回调函数，处理 WiFi AP 相关事件
 *
 * @param arg 回调函数参数
 * @param event_module 事件模块
 * @param event_id 事件 ID
 * @param event_data 事件数据
 * @return int 返回 BK_OK 表示处理成功
 */
static int wifi_softap_event_cb(void *arg, event_module_t event_module,
                                int event_id, void *event_data)
{
    wifi_event_ap_disconnected_t *ap_disconnected;
    wifi_event_ap_connected_t *ap_connected;

    switch (event_id)
    {
        // 当有STA 设备连接时触发连接事件
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

        // 当有STA 设备断开时触发断开事件
        case EVENT_WIFI_AP_DISCONNECTED:
            ap_disconnected = (wifi_event_ap_disconnected_t *)event_data;
            LOGI(BK_MAC_FORMAT" disconnected from AP\n", BK_MAC_STR(ap_disconnected->mac));
            break;

        default:
            LOGI("rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}

/**
 * @brief 启动 WiFi SoftAP 功能
 *
 * 该函数初始化 AP 配置结构体，设置 IPv4 地址、子网掩码、网关和 DNS 地址，
 * 并启动 AP 功能。
 */
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

    // 注册 WiFi 事件回调函数，监听所有 WiFi 事件，若失败则记录日志
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, wifi_softap_event_cb, NULL));

    // 启动 WiFi SoftAP 功能
    wifi_softap_start();

    return 0;
}

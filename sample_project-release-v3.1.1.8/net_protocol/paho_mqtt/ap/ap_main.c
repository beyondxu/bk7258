#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>
#include <components/shell_task.h>
#include <modules/wifi.h>
#include <components/netif.h>
#include <components/event.h>
#include <string.h>
#include "paho_mqtt_demo.h"

#define CONFIG_EXAMPLE_WIFI_STA_SSID        "sample_project_wifi"
#define CONFIG_EXAMPLE_WIFI_STA_PASSWORD    "12345678"
#define CONFIG_MQTT_TEST_SERVER_URI         "tcp://broker.emqx.io:1883"

static beken_thread_t mqtt_client_handle = NULL;
static beken_semaphore_t g_wifi_cmd_sema = NULL;

// 处理网络接口事件的回调函数
static int wifi_netif_event_cb(void *arg, event_module_t event_module,
                               int event_id, void *event_data)
{
    // 打印接收到的事件信息
    BK_EXAMPLE_LOG(TAG,"rx event <%d %d>\n", event_module, event_id);
    netif_event_got_ip4_t *got_ip;

    switch (event_id)
    {
        // 处理获取到 IPv4 地址的事件
        case EVENT_NETIF_GOT_IP4:
            got_ip = (netif_event_got_ip4_t *)event_data;
            netif_ip4_config_t ip4_info = {0};
            BK_EXAMPLE_LOG(TAG,"%s got ip:%s\n", got_ip->netif_if == NETIF_IF_STA ? "STA" : "unknown netif", got_ip->ip);

            BK_RETURN_ON_ERR(bk_netif_get_ip4_config(NETIF_IF_STA, &ip4_info));
            BK_EXAMPLE_LOG(TAG, "ip=%s,gate=%s,mask=%s,dns=%s\r\n",
                           ip4_info.ip, ip4_info.gateway, ip4_info.mask, ip4_info.dns);

            if (g_wifi_cmd_sema != NULL)
            {
                rtos_set_semaphore(&g_wifi_cmd_sema);
            }
            break;

        default:
            BK_EXAMPLE_LOG(TAG,"rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}

// 处理 Wi-Fi STA 事件的回调函数
static int wifi_sta_event_cb(void *arg, event_module_t event_module,
                             int event_id, void *event_data)
{
    wifi_event_sta_disconnected_t *sta_disconnected;
    wifi_event_sta_connected_t *sta_connected;

    switch (event_id)
    {
        // 处理 STA 连接成功事件
        case EVENT_WIFI_STA_CONNECTED:
            sta_connected = (wifi_event_sta_connected_t *)event_data;
            // 打印 STA 连接的 SSID 信息
            BK_EXAMPLE_LOG(TAG,"STA connected to %s\n", sta_connected->ssid);
            break;

        // 处理 STA 断开连接事件
        case EVENT_WIFI_STA_DISCONNECTED:
            sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
            // 打印 STA 断开连接的原因
            BK_EXAMPLE_LOG(TAG,"STA disconnected, reason(%d)\n", sta_disconnected->disconnect_reason);
            break;

        default:
            BK_EXAMPLE_LOG(TAG,"rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}

// 初始化 Wi-Fi STA 并尝试连接
static void wifi_sta_init_and_connect(void)
{
    // 使用默认配置初始化 Wi-Fi STA 配置结构体
    wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();

    // 注册 Wi-Fi 事件回调函数
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, wifi_sta_event_cb, NULL));
    // 注册网络接口事件回调函数
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, wifi_netif_event_cb, NULL));

    // 设置ssid password
    strncpy(sta_config.ssid, CONFIG_EXAMPLE_WIFI_STA_SSID, WIFI_SSID_STR_LEN);
    strncpy(sta_config.password, CONFIG_EXAMPLE_WIFI_STA_PASSWORD, WIFI_PASSWORD_LEN);

    BK_EXAMPLE_LOG(TAG,"ssid:%s password:%s\n", sta_config.ssid, sta_config.password);
    BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
    // 启动 Wi-Fi STA
    BK_LOG_ON_ERR(bk_wifi_sta_start());
}

// MQTT 客户端线程函数
static void mqtt_client_thread(void *arg)
{
    int err;

    // 初始化 Wi-Fi STA 并尝试连接
    wifi_sta_init_and_connect();

    // 等待信号量，永久等待
    err = rtos_get_semaphore(&g_wifi_cmd_sema, BEKEN_WAIT_FOREVER);
    if (err != kNoErr)
    {
        goto _exit;
    }

    // 启动 MQTT 演示
    mqtt_demo_start(CONFIG_MQTT_TEST_SERVER_URI);
    // 启动 MQTT 发布演示
    mqtt_demo_publish_start();

    while(1)
    {
        rtos_delay_milliseconds(50);
    }

_exit:
    rtos_deinit_semaphore(&g_wifi_cmd_sema);

    mqtt_client_handle = NULL;
    rtos_delete_thread(NULL);
}

// 主函数，程序入口
int main(void)
{
    int err;

    // 系统初始化
    bk_init();
    // 设置 shell 日志级别
    shell_set_log_level(BK_LOG_INFO);
    err = rtos_init_semaphore(&g_wifi_cmd_sema, 1);
    if (err != kNoErr)
    {
        BK_EXAMPLE_LOG(TAG,"create g_wifi_cmd_sema fail.\r\n");
        return -1;
    }

    // 创建 MQTT 客户端线程
    err = rtos_create_thread(&mqtt_client_handle, 5, "mqtt_client",
                             (beken_thread_function_t)mqtt_client_thread, 4096, 0);

    if (err != kNoErr)
    {
        BK_EXAMPLE_LOG(TAG,"create mqtt_client thread fail.\r\n");
        return -1;
    }

    return 0;
}
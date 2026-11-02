#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include <modules/wifi.h>
#include <components/log.h>
#include <components/netif.h>
#include <components/event.h>
#include <bk_private/bk_init.h>

#define TAG "wifi_scan"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define CONFIG_EXAMPLE_SCAN_ALL_SSID  1

#if !CONFIG_EXAMPLE_SCAN_ALL_SSID
#define CONFIG_EXAMPLE_WIFI_SACN_SSID   "sample_project_wifi"
#endif

#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

// 定义 WiFi 扫描事件回调函数，当 WiFi 扫描完成时会触发此函数
static int wifi_scan_event_cb(void *arg, event_module_t event_module,
                              int event_id, void *event_data)
{
    wifi_scan_result_t scan_result = {0};

    // 获取 WiFi 扫描结果，若出错则记录日志
    BK_LOG_ON_ERR(bk_wifi_scan_get_result(&scan_result));
    // 打印 WiFi 扫描结果，若出错则记录日志
    BK_LOG_ON_ERR(bk_wifi_scan_dump_result(&scan_result));
    bk_wifi_scan_free_result(&scan_result);
    TEST_PASS();

    return BK_OK;
}

// 定义启动 WiFi 扫描的函数
static void wifi_scan_start(void)
{
#if CONFIG_EXAMPLE_SCAN_ALL_SSID
    // 配置扫描所有 SSID，开始扫描
    BK_LOG_ON_ERR(bk_wifi_scan_start(NULL));
#else
    wifi_scan_config_t scan_config = {0};
    // 将目标 SSID 复制到扫描配置中
    os_strncpy(scan_config.ssid, CONFIG_EXAMPLE_WIFI_SACN_SSID, WIFI_SSID_STR_LEN);
    BK_LOG_ON_ERR(bk_wifi_scan_start(&scan_config));
#endif
}

int main(void)
{
    bk_init();
    media_service_init();

    // 注册 WiFi 扫描完成事件的回调函数
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_SCAN_DONE,wifi_scan_event_cb, NULL));
    wifi_scan_start();

    LOGI("start wifi scan\r\n");
    
    return 0;
}

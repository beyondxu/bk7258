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
#include <components/webclient.h>

#define TAG "webclient"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

#define WIFI_AP_SSID         "Redmi_B5F9"
#define WIFI_AP_PASSWORD     "12345678"
#define LINK_OVER_TIME       10000


// 网络接口事件回调函数，用于处理网络接口相关的事件
static int wifi_netif_event_cb(void *arg, event_module_t event_module,
                               int event_id, void *event_data)
{
    netif_event_got_ip4_t *got_ip;

    switch (event_id)
    {
        // 处理获取到 IPv4 地址的事件
        case EVENT_NETIF_GOT_IP4:
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

#define RCV_BUF_SIZE                1024*2
#define SEND_HEADER_SIZE              1024
int webclient_http_post(void)
{
    char buf_url[] = "http://www.rt-thread.com/service/echo";
    char post_data[] = "devicesCode=120002000814&userKey=6b6d71e72c70d136179177ddd231af7d&devicesName=bk7256&random=ZWVUTR&token=2c70426994";
    char *uri;
    uri = os_strdup(buf_url);
    size_t data_len;
    char rep_data[2*1024] = {0};
    data_len = sizeof(post_data);
    struct webclient_session* session = NULL;
    unsigned char *buffer = NULL;
    int  ret = 0;
    int bytes_read, resp_status;

    buffer = (unsigned char *) web_malloc(RCV_BUF_SIZE);
    if (buffer == NULL)
    {
        BK_LOGE(TAG,"no memory for receive response buffer.\n");
        ret = -5;
        goto __exit;
    }

    /* create webclient session and set header response size */
    session = webclient_session_create(SEND_HEADER_SIZE);
    if (session == NULL)
    {
        ret = -5;
        goto __exit;
    }
    char version_buf[32] ="7256000";

    /* build header for upload */
    webclient_header_fields_add(session, "Content-Length: %d\r\n", strlen(post_data));
    webclient_header_fields_add(session, "Content-Type: application/x-www-form-urlencoded\r\n");
    webclient_header_fields_add(session, "%s\r\n",version_buf);

    /* send POST request by default header */
    if ((resp_status = webclient_post(session, uri, post_data, data_len)) != 200)
    {
        LOGE("webclient POST request failed, response(%d) error.\n", resp_status);
        ret = -1;
        goto __exit;
    }

    BK_LOGD(TAG,"webclient post response data: \n");
    do
    {
        bytes_read = webclient_read(session, buffer, RCV_BUF_SIZE);
        if (bytes_read <= 0)
        {
            break;
        }
        strncat(rep_data,(char*)buffer,bytes_read);
    } while (1);

    ret = BK_OK;
    LOGI("[%s][%d] rep_data %s.\n", __FUNCTION__, __LINE__, rep_data);

__exit:
    if (session)
    {
        webclient_close(session);
    }

    if (buffer)
    {
        web_free(buffer);
    }

    return ret;
}

int main(void)
{
    wifi_link_status_t link_status = {0};
    bk_err_t ret = BK_FAIL;
    uint32 ticks = 0;
    
    bk_init();
    media_service_init();

    // 注册 Wi-Fi 事件回调函数，处理所有 Wi-Fi 事件，网络接口事件
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, wifi_sta_event_cb, NULL));
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, wifi_netif_event_cb, NULL));

    // 初始化 Wi-Fi STA 模式并尝试连接
    wifi_sta_init_and_connect();

    ticks = rtos_get_time();
    while(1)
    {
        ret = bk_wifi_sta_get_link_status(&link_status);

        if(/*WIFI_LINKSTATE_STA_CONNECTED == link_status.state || */WIFI_LINKSTATE_STA_GOT_IP == link_status.state)
        {
            LOGI("[%s][%d] wifi link success\n", __FUNCTION__, __LINE__);
            break;
        }

        if((rtos_get_time() - ticks) >= LINK_OVER_TIME)
        {
            LOGI("[%s][%d] wifi link fail\n", __FUNCTION__, __LINE__);
            TEST_FAIL();
            return BK_FAIL;
            break;
        }

        rtos_thread_msleep(100);
    }

    //rtos_thread_msleep(1000);
    ret = webclient_http_post();
    if(BK_OK != ret)
    {
        LOGI("[%s][%d] post fail\n", __FUNCTION__, __LINE__);
        TEST_FAIL();
    }
    else
    {
        LOGI("[%s][%d] post success\n", __FUNCTION__, __LINE__);
        TEST_PASS();
    }

    return 0;
}

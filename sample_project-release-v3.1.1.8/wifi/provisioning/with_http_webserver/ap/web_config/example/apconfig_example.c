#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <modules/wifi.h>
#include <components/event.h>
#include <components/netif.h>
#include <string.h>
#include <components/log.h>

#include "webnet.h"
#include "wn_module.h"
#include "apconfig_example.h"

#define CONFIG_EXAMPLE_IP                   "192.168.1.1"
#define CONFIG_EXAMPLE_MASK                 "255.255.255.0"
#define CONFIG_EXAMPLE_GW                   "192.168.1.1"
#define CONFIG_EXAMPLE_DNS                  "192.168.1.1"
#define CONFIG_EXAMPLE_WIFI_AP_SSID         "web_config"
#define CONFIG_EXAMPLE_WIFI_AP_PASSWORD     "12345678"

static beken_thread_t wifi_cfg_thread;
static beken_queue_t  wifi_notify_que;
static net_info_t net_info = {0};
static struct udp_pcb *dns_pcb = NULL;

static int wifi_config_ok_notify(void);
static int wifi_scan_result_callback(void);

static int wifi_send_notify(note_msg_t *arg)
{
    int ret = 0;
    note_msg_t *msg= arg;
    if(wifi_notify_que != NULL)
    {
        ret=rtos_push_to_queue(&wifi_notify_que, (void *)msg, BEKEN_WAIT_FOREVER);
        if(ret != BK_OK)
        {
            BK_EXAMPLE_LOG(TAG,"wifi queue send notify fial \n");
        }
    }
    return ret;
}

static int wifi_softap_event_cb(void *arg, event_module_t event_module,
                                int event_id, void *event_data)
{

    wifi_event_ap_disconnected_t *ap_disconnected;
    wifi_event_ap_connected_t *ap_connected;

    wifi_event_sta_disconnected_t *sta_disconnected;
    wifi_event_sta_connected_t *sta_connected;

    switch (event_id)
    {
        case EVENT_WIFI_AP_CONNECTED:
            ap_connected = (wifi_event_ap_connected_t *)event_data;
            BK_EXAMPLE_LOG(TAG, BK_MAC_FORMAT" connected to AP\n", BK_MAC_STR(ap_connected->mac));

            break;

        case EVENT_WIFI_AP_DISCONNECTED:
            ap_disconnected = (wifi_event_ap_disconnected_t *)event_data;
            BK_EXAMPLE_LOG(TAG, BK_MAC_FORMAT" disconnected from AP\n", BK_MAC_STR(ap_disconnected->mac));
            break;

        case EVENT_WIFI_STA_CONNECTED:
            sta_connected = (wifi_event_sta_connected_t *)event_data;
            BK_EXAMPLE_LOG(TAG, "STA connected to %s\n", sta_connected->ssid);
            break;

        case EVENT_WIFI_STA_DISCONNECTED:
            TEST_FAIL();
            sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
            BK_EXAMPLE_LOG(TAG, "STA disconnected, reason(%d)\n", sta_disconnected->disconnect_reason);
            break;

        case EVENT_WIFI_SCAN_DONE:
            wifi_scan_result_callback();
            break;

        default:
            BK_EXAMPLE_LOG(TAG, "rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}


static int wifi_netif_event_cb(void *arg, event_module_t event_module,
                               int event_id, void *event_data)
{
    netif_event_got_ip4_t *got_ip;

    switch (event_id)
    {
        case EVENT_NETIF_GOT_IP4:
            got_ip = (netif_event_got_ip4_t *)event_data;
            BK_EXAMPLE_LOG(TAG, "%s got ip\n", got_ip->netif_if == NETIF_IF_STA ? "STA" : "unknown netif");
            wifi_config_ok_notify();
            break;
        default:
            BK_EXAMPLE_LOG(TAG, "rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}
static void wifi_event_handler_init(void)
{
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, wifi_softap_event_cb, NULL));
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, wifi_netif_event_cb, NULL));
}
static void wifi_event_handler_deinit(void)
{
    BK_LOG_ON_ERR(bk_event_unregister_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, wifi_softap_event_cb));
    BK_LOG_ON_ERR(bk_event_unregister_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, wifi_netif_event_cb));
}

static void wifi_softap_init(void)
{
    bk_wifi_sta_stop();
    wifi_ap_config_t ap_config = WIFI_DEFAULT_AP_CONFIG();
    netif_ip4_config_t ip4_config = {0};

    strncpy(ip4_config.ip, CONFIG_EXAMPLE_IP, NETIF_IP4_STR_LEN);
    strncpy(ip4_config.mask, CONFIG_EXAMPLE_MASK, NETIF_IP4_STR_LEN);
    strncpy(ip4_config.gateway, CONFIG_EXAMPLE_GW, NETIF_IP4_STR_LEN);
    strncpy(ip4_config.dns, CONFIG_EXAMPLE_DNS, NETIF_IP4_STR_LEN);
    BK_LOG_ON_ERR(bk_netif_set_ip4_config(NETIF_IF_AP, &ip4_config));


    strncpy(ap_config.ssid, CONFIG_EXAMPLE_WIFI_AP_SSID, WIFI_SSID_STR_LEN);
    strncpy(ap_config.password, CONFIG_EXAMPLE_WIFI_AP_PASSWORD, WIFI_PASSWORD_LEN);

    BK_EXAMPLE_LOG(TAG, "ssid:%s  key:%s\r\n", ap_config.ssid, ap_config.password);
    BK_LOG_ON_ERR(bk_wifi_ap_set_config(&ap_config));
    BK_LOG_ON_ERR(bk_wifi_ap_start());
}


static void wifi_sta_init(void)
{
    wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();

    strncpy(sta_config.ssid, net_info.ssid, WIFI_SSID_STR_LEN);
    strncpy(sta_config.password, net_info.pwd, WIFI_PASSWORD_LEN);

    BK_EXAMPLE_LOG(TAG, "ssid:%s password:%s\n", sta_config.ssid, sta_config.password);
    BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
    BK_LOG_ON_ERR(bk_wifi_sta_start());
}

static wifi_scan_result_t g_scan_result = {0};

static int netif_get_ip_event_cb(void *arg, event_module_t event_module,
                                 int event_id, void *event_data)
{
    wifi_config_ok_notify();
    return 0;
}

static void bk_wifi_scan_sort_result(wifi_scan_result_t *scan_result)
{
    if (scan_result->ap_num == 0)
        return;

    int unique_ap_num = 0;
    
    // 合并步骤：移除空SSID并保留相同SSID中信号最强的一个
    for (int i = 0; i < scan_result->ap_num; i++)
    {
        // 跳过空SSID的AP
        if (scan_result->aps[i].ssid[0] == '\0')
            continue;
        
        // 检查当前AP的SSID是否已经存在于结果中
        int exists = 0;
        for (int j = 0; j < unique_ap_num; j++)
        {
            if (strcmp(scan_result->aps[i].ssid, scan_result->aps[j].ssid) == 0)
            {
                exists = 1;
                // 如果当前AP的信号强度更强，则替换已存在的AP
                if (scan_result->aps[i].rssi > scan_result->aps[j].rssi)
                {
                    scan_result->aps[j] = scan_result->aps[i];
                }
                break;
            }
        }
        
        // 如果SSID不存在，则添加到结果中
        if (!exists)
        {
            if (unique_ap_num != i)
            {
                scan_result->aps[unique_ap_num] = scan_result->aps[i];
            }
            unique_ap_num++;
        }
    }
    
    // 更新AP数量
    scan_result->ap_num = unique_ap_num;

    // 如果没有AP了，直接返回
    if (scan_result->ap_num == 0)
        return;

    // 按信号强度降序排序
    for (int i = 0; i < scan_result->ap_num - 1; i++)
    {
        for (int j = 0; j < scan_result->ap_num - i - 1; j++)
        {
            if (scan_result->aps[j].rssi < scan_result->aps[j + 1].rssi)
            {
                wifi_scan_ap_info_t temp = scan_result->aps[j];
                scan_result->aps[j] = scan_result->aps[j + 1];
                scan_result->aps[j + 1] = temp;
            }
        }
    }
}

static int wifi_scan_result_callback(void)
{
    wifi_scan_result_t scan_result = {0};

    bk_wifi_scan_get_result(&scan_result);
    bk_wifi_scan_sort_result(&scan_result);
    BK_LOG_ON_ERR(bk_wifi_scan_dump_result(&scan_result)); //打印扫描结果

    if(scan_result.ap_num > 0)
    {
        if(!g_scan_result.aps)
        {
            os_printf("[%s]scan cb malloc !\r\n", __func__);
            g_scan_result.aps = (wifi_scan_ap_info_t *)os_malloc(MAX_WIFI_SCAN_AP_NUM*sizeof(wifi_scan_ap_info_t));
        }

        if(g_scan_result.aps)
        {
            bk_printf("[%s] get scan result:%d\r\n", __func__, scan_result.ap_num);
            os_memset(g_scan_result.aps, 0, MAX_WIFI_SCAN_AP_NUM*sizeof(wifi_scan_ap_info_t));
            g_scan_result.ap_num = scan_result.ap_num > MAX_WIFI_SCAN_AP_NUM ? MAX_WIFI_SCAN_AP_NUM:scan_result.ap_num;
            os_memcpy(g_scan_result.aps, scan_result.aps, g_scan_result.ap_num*sizeof(wifi_scan_ap_info_t));
        }
    }

    bk_wifi_scan_free_result(&scan_result);

    return BK_OK;
}

static void cgi_web_ap_config_handler(struct webnet_session* session)
{
    const char* mimetype;
    struct webnet_request* request;
    BK_ASSERT(session !=NULL);
    request = session->request;
    BK_ASSERT(request != NULL);
    BK_EXAMPLE_LOG(TAG, "%s session:%x\r\n",__FUNCTION__, session);

    mimetype = mime_get_type(".html");
    session->request->result_code = 200;
    webnet_session_set_header(session, mimetype, 200, "Ok", -1);

    if(request->method == WEBNET_GET)
    {
        webnet_session_write(session, (const uint8_t*)PAGE_HEADER, strlen(PAGE_HEADER));
        webnet_session_write(session, (const uint8_t*)PAGE_BODY_GET, strlen(PAGE_BODY_GET));
    }
    else if(request->method == WEBNET_POST)
    {
        webnet_session_write(session, (const uint8_t*)HEADER_GET, strlen(HEADER_GET));
        webnet_session_write(session, (const uint8_t*)BODY_POST, strlen(BODY_POST));
    }
    if (request->query_counter && request->method == WEBNET_POST)
    {
        const char *ssid_value, *password_value;
        ssid_value = webnet_request_get_query(request, "ssid");
        password_value = webnet_request_get_query(request, "password");

        if(ssid_value)
        {
            memcpy(net_info.ssid,ssid_value,strlen(ssid_value)+1);
        }
        else
        {
            memset(net_info.ssid, 0, sizeof(net_info.ssid));
        }

        if(password_value && strlen(password_value))
        {
            memcpy(net_info.pwd,password_value,strlen(password_value)+1);
        }
        else
        {
            memset(net_info.pwd, 0, sizeof(net_info.pwd));
        }

        BK_EXAMPLE_LOG(TAG, "ssid:%s,password:%s\r\n",net_info.ssid,net_info.pwd);

        note_msg_t msg;
        msg.event = ACK_WIFI_CONNECT_START;
        wifi_send_notify(&msg);

    }

    return;

}

static void _scan_free_result(void)
{
    if(g_scan_result.aps)
    {
        os_free(g_scan_result.aps);
        g_scan_result.aps = NULL;
        g_scan_result.ap_num = 0;
    }
}
static void cgi_web_scan_handler(struct webnet_session* session)
{
    const char* mimetype;
    struct webnet_request* request;
    char *res_scan_buf = NULL;
    uint32 cur_time = rtos_get_time();

    BK_ASSERT(session !=NULL);
    request = session->request;
    BK_ASSERT(request != NULL);

    BK_EXAMPLE_LOG(TAG, "%s session:%x\r\n",__FUNCTION__, session);

    mimetype = mime_get_type(".html");
    session->request->result_code = 200;
    webnet_session_set_header(session, mimetype, 200, "Ok", -1);

    res_scan_buf = os_malloc(SCAN_BUFF_SIZE);
    if(!res_scan_buf)
    {
        BK_EXAMPLE_LOG(TAG, "[%s][%d] malloc mail\r\n", __FUNCTION__, __LINE__);
        return ;
    }

    if(request->method == WEBNET_GET)
    {
        _scan_free_result();

        bk_wifi_scan_start(NULL);

        while(1)
        {
            if((rtos_get_time() - cur_time) > SCAN_OVER_TIME)
            {
                BK_EXAMPLE_LOG(TAG, "[%s][%d] scan over time\r\n", __FUNCTION__, __LINE__);
                bk_wifi_scan_stop();
            }

            if(g_scan_result.ap_num > 0 && g_scan_result.aps)
            {
                os_memset(res_scan_buf, 0, SCAN_BUFF_SIZE);
                os_snprintf(res_scan_buf, SCAN_BUFF_SIZE, "[  ");
                for(int i=0; i<g_scan_result.ap_num; i++)
                {
                    if((i+1) == g_scan_result.ap_num)
                    {
                        os_snprintf(res_scan_buf+os_strlen(res_scan_buf), SCAN_BUFF_SIZE-os_strlen(res_scan_buf), "{\"name\": \"%s\"  }]", g_scan_result.aps[i].ssid);
                    }
                    else
                    {
                        os_snprintf(res_scan_buf+os_strlen(res_scan_buf), SCAN_BUFF_SIZE-os_strlen(res_scan_buf), "{\"name\": \"%s\"  },", g_scan_result.aps[i].ssid);
                    }
                }
                break;
            }

            rtos_delay_milliseconds(200);
        }

        BK_EXAMPLE_LOG(TAG, "[%s][%d] scan %s\r\n", __FUNCTION__, __LINE__, res_scan_buf);
        webnet_session_write(session, (const uint8_t*)res_scan_buf, strlen(res_scan_buf));
    }

    if(res_scan_buf)
    {
        psram_free(res_scan_buf);
    }
    return;

}

static void cgi_web_status_handler(struct webnet_session* session)
{
    const char* mimetype;
    struct webnet_request* request;
    char status_buf[128] = {0};

    BK_ASSERT(session !=NULL);
    request = session->request;
    BK_ASSERT(request != NULL);

    BK_EXAMPLE_LOG(TAG, "%s session:%x\r\n",__FUNCTION__, session);

    mimetype = mime_get_type(".json");
    session->request->result_code = 200;
    webnet_session_set_header(session, mimetype, 200, "Ok", -1);

    if(request->method == WEBNET_GET)
    {
        // 检查Wi-Fi连接状态
        wifi_link_status_t sta_status;
        int connect_state = 0;

        // 获取STA状态

        if (bk_wifi_sta_get_link_status(&sta_status) == BK_OK)
        {
            if (sta_status.state == WIFI_LINKSTATE_STA_IDLE)
            {
                connect_state = 0;
            }
            else if (sta_status.state == WIFI_LINKSTATE_STA_GOT_IP)
            {
                connect_state = 2;
            }
            else
            {
                connect_state = 1;
            }
        }

        // 返回JSON格式的状态信息
        os_snprintf(status_buf, sizeof(status_buf),
                    "{\"connect_state\": %d, \"ssid\": \"%s\"}",
                    connect_state, net_info.ssid);

        BK_EXAMPLE_LOG(TAG, "[%s][%d] status: %s\r\n", __FUNCTION__, __LINE__, status_buf);
        webnet_session_write(session, (const uint8_t*)status_buf, strlen(status_buf));
    }

    return;
}

static int wifi_config_ok_notify(void)
{
    note_msg_t msg;

    BK_EXAMPLE_LOG(TAG, "[%s][%d] enter\n", __FUNCTION__, __LINE__);
    msg.event = ACK_WIFI_CONNECTED_CFG_FIN;
    wifi_send_notify(&msg);

    return BK_OK;
}

static int parse_dns_query(const uint8_t *payload, uint16_t len, uint8_t *domain_buf)
{
    if (len < DNS_HEADER_LEN + 5) return 0;

    const uint8_t *ptr = payload + DNS_HEADER_LEN;
    const uint8_t *end = payload + len;
    uint8_t *buf_ptr = domain_buf;
    uint8_t label_len;

    while (ptr < end && (label_len = *ptr++) != 0)
    {
        if (label_len > 63 || (ptr + label_len) > end)
        {
            return 0;
        }

        *buf_ptr++ = label_len;
        memcpy(buf_ptr, ptr, label_len);
        buf_ptr += label_len;
        ptr += label_len;
    }

    if ((end - ptr) < 4) return 0;

    *buf_ptr = 0;

    return 1;
}

static void dns_recv_cb(void *arg,struct udp_pcb *pcb, struct pbuf *p,
                        const ip_addr_t *addr, u16_t port)
{
    BK_EXAMPLE_LOG(TAG, "%s:%d\r\n",__func__,__LINE__);
    uint8_t *payload = (uint8_t *)p->payload;
    struct pbuf *resp = NULL;
    char domain[MAX_DOMAIN_LEN + 1];

    if (p->tot_len <= DNS_HEADER_LEN) goto cleanup;

    dns_header_t *hdr = (dns_header_t *)payload;
    if ((ntohs(hdr->flags) & 0x8000) || (ntohs(hdr->qdcount) != 1))
    {
        goto cleanup;
    }

    if (!parse_dns_query(payload, p->tot_len, (uint8_t *)domain))
    {
        goto cleanup;
    }

    uint16_t qname_len = strlen(domain)+2; // include length byte and null terminator
    uint16_t total_len = DNS_HEADER_LEN + qname_len + sizeof(dns_query_t) + sizeof(dns_answer_t);

    resp = pbuf_alloc(PBUF_TRANSPORT, total_len, PBUF_RAM);
    if (!resp) goto cleanup;

    dns_header_t *rhdr = (dns_header_t *)resp->payload;
    memset(rhdr, 0, DNS_HEADER_LEN);
    rhdr->id = hdr->id;
    rhdr->flags = htons(0x8100); // QR=1, AA=1
    rhdr->qdcount = htons(1);
    rhdr->ancount = htons(1);

    uint8_t *rptr = (uint8_t *)(rhdr + 1);
    memcpy(rptr, domain, strlen(domain));
    rptr += strlen(domain);
    *rptr++ = 0;

    dns_query_t *query = (dns_query_t *)rptr;
    query->type = htons(1); // A record
    query->class = htons(1); // IN class
    rptr += sizeof(dns_query_t);

    dns_answer_t *ans = (dns_answer_t *)rptr;
    ans->name_ptr = htons(0xC00C);
    ans->type = htons(1);
    ans->class = htons(1);
    ans->ttl = htonl(DNS_TTL);
    ans->rdlength = htons(4);

    ans->addr = inet_addr((char *)CONFIG_EXAMPLE_IP);
    BK_EXAMPLE_LOG(TAG, "%s:%d domain:%s,qname_len:%d\r\n",__func__,__LINE__,domain,qname_len);
    udp_sendto(pcb, resp, addr, port);

cleanup:
    if (resp) pbuf_free(resp);
    pbuf_free(p);

}

static void captive_dns_init(void)
{
    if(!dns_pcb)
        dns_pcb = udp_new();

    if (dns_pcb)
    {
        int ret = udp_bind(dns_pcb, IP_ADDR_ANY, DNS_PORT);
        BK_EXAMPLE_LOG(TAG, "[%s][%d] bind ret:%d\r\n", __FUNCTION__, __LINE__, ret);
        udp_recv(dns_pcb, dns_recv_cb, NULL);
    }
}

static void captive_dns_deinit(void)
{
    if(dns_pcb)
    {
        udp_remove(dns_pcb);
        dns_pcb = NULL;
    }
}

static void wifi_softap_cfg_thread(beken_thread_arg_t *arg)
{
    int ret= 0;
    note_msg_t msg;
    while(1)
    {
        rtos_delay_milliseconds(500);
        ret = rtos_pop_from_queue(&wifi_notify_que, (void *)&msg, BEKEN_WAIT_FOREVER);
        if(ret == BK_OK)
        {
            BK_EXAMPLE_LOG(TAG, "get msg :%d \n",msg.event);
            switch (msg.event)
            {
                case ACK_WIFI_CONNECT_START:
                    wifi_sta_init();
                    break;

                case ACK_WIFI_CONNECTED_CFG_FIN:
                    TEST_PASS();
                    rtos_delay_milliseconds(1000);
                case ACK_WIFI_CFG_STOP:
                    goto exit;
                    break;

                default:
                    break;
            }
        }
    }
exit:

    bk_wifi_ap_stop();
    webnet_deinit();

    if(wifi_notify_que!= NULL)
    {
        rtos_deinit_queue(&wifi_notify_que);
        wifi_notify_que = NULL;
    }
    webnet_cgi_unregister();
    wifi_event_handler_deinit();
    wifi_cfg_thread = NULL;

    _scan_free_result();

    captive_dns_deinit();
    BK_EXAMPLE_LOG(TAG, "[%s][%d] exit\n", __FUNCTION__, __LINE__);

    rtos_delete_thread(NULL);

}

void wifi_provisioning_web_config_start(void)
{
    int ret = 0;

    wifi_event_handler_init();
    wifi_softap_init();
    captive_dns_init();

    #ifdef WEBNET_USING_CGI
    webnet_cgi_set_root("/");
    webnet_cgi_register("", cgi_web_ap_config_handler);
    webnet_cgi_register("scan", cgi_web_scan_handler);
    webnet_cgi_register("status", cgi_web_status_handler);
    #endif

    webnet_init();

    ret = rtos_init_queue(&wifi_notify_que,
                          "dwifi_notify",
                          sizeof(note_msg_t),
                          5);

    if (ret != BK_OK)
    {
        BK_EXAMPLE_LOG(TAG, "%s, init queue failed\r\n", __func__);
        return ;
    }
    else
    {
        BK_EXAMPLE_LOG(TAG, "%s, init queue done\r\n", __func__);
    }
    ret = rtos_create_thread(&wifi_cfg_thread,
                             5,
                             "wificonfig",
                             (beken_thread_function_t) wifi_softap_cfg_thread,
                             4096,
                             (beken_thread_arg_t)NULL);

    if (ret != BK_OK)
    {
        BK_EXAMPLE_LOG(TAG, "%s, create thread failed\r\n", __func__);
    }

}

void wifi_provisioning_web_config_stop(void)
{
    note_msg_t msg;

    BK_EXAMPLE_LOG(TAG, "[%s][%d] enter\n", __FUNCTION__, __LINE__);
    msg.event = ACK_WIFI_CFG_STOP;
    wifi_send_notify(&msg);
}
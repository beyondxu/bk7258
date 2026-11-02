/*
 * websocket_client.c
 * WebSocket客户端实现文件
 * 实现WebSocket客户端的连接、数据发送和事件处理功能
 */

#include <os/mem.h>
#include <os/str.h>
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
#include "bk_websocket_client.h"

static beken_thread_t websocket_client_handle = NULL;
static char *g_recv_data = NULL;

/**
 * @brief 打印十六进制数据
 * @param data 数据指针
 * @param len 数据长度
 */
static void print_hex_data(const uint8_t *data, int len)
{
    if (data == NULL || len <= 0)
    {
        return;
    }

    char buffer[64] = {0};  // 16字节 * 3字符/字节 = 48字符 + 空间
    int buffer_pos = 0;

    for (int i = 0; i < len; i++)
    {
        // 使用os_snprintf格式化到buffer
        buffer_pos += os_snprintf(buffer + buffer_pos, sizeof(buffer) - buffer_pos, "%02X ", data[i]);

        // 每16字节输出一次
        if ((i + 1) % 16 == 0)
        {
            WS_LOGI("%s\n", buffer);
            os_memset(buffer, 0, sizeof(buffer));
            buffer_pos = 0;
        }
    }

    // 输出剩余的数据（不足16字节的部分）
    if (buffer_pos > 0)
    {
        WS_LOGI("%s\n", buffer);
    }
}

/**
 * @brief WebSocket事件处理函数
 * @param event_handler_arg 事件处理参数
 * @param event_base 事件基础
 * @param event_id 事件ID
 * @param event_data 事件数据
 */
static void websocket_event_handler(void *event_handler_arg, char *event_base, 
                                    int32_t event_id, void *event_data)
{
    bk_websocket_event_data_t *ws_event_data = (bk_websocket_event_data_t *)event_data;

    switch (event_id)
    {
        case WEBSOCKET_EVENT_CONNECTED:
            WS_LOGI("Connected to WebSocket server\n");
            TEST_PASS();
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            WS_LOGI("Disconnected from WebSocket server\n");
            break;

        case WEBSOCKET_EVENT_DATA:
            if (ws_event_data != NULL)
            {
                // 根据opcode判断是文本还是二进制数据
                if (ws_event_data->op_code == WS_TRANSPORT_OPCODES_TEXT)
                {
                    if(!g_recv_data)
                    {
                        g_recv_data = psram_malloc(ws_event_data->payload_len + 1);
                        BK_ASSERT(g_recv_data);
                        os_memset(g_recv_data, 0, ws_event_data->payload_len+1);
                    }

                    os_memcpy(g_recv_data+ws_event_data->payload_offset, ws_event_data->data_ptr, ws_event_data->data_len);
                    if ((ws_event_data->payload_offset + ws_event_data->data_len) >= ws_event_data->payload_len
                         && ws_event_data->data_len > 0 && ws_event_data->data_ptr != NULL)
                    {
                        WS_LOGI("Received TEXT data, len:%d, data:%s\n", ws_event_data->payload_len, g_recv_data);

                        //处理接收的文本数据

                        //释放文本数据内存
                        psram_free(g_recv_data);
                        g_recv_data = NULL;
                    }
                    
                }
                else if (ws_event_data->op_code == WS_TRANSPORT_OPCODES_BINARY)
                {
                    //释放文本数据内存
                    if(g_recv_data)
                    {
                        psram_free(g_recv_data);
                        g_recv_data = NULL;
                    }
                    
                    WS_LOGI("Received BINARY data, len:%d\n", ws_event_data->data_len);
                    // 打印二进制数据的十六进制表示
                    print_hex_data((const uint8_t *)ws_event_data->data_ptr, ws_event_data->data_len);
                }
                else
                {
                    WS_LOGI("Received data, opcode:%d, len:%d\n", 
                            ws_event_data->op_code, ws_event_data->data_len);
                }
            }
            break;

        case WEBSOCKET_EVENT_ERROR:
            WS_LOGE("WebSocket error occurred\n");
            TEST_FAIL();
            break;

        case WEBSOCKET_EVENT_CLOSED:
            WS_LOGI("WebSocket connection closed\n");
            break;

        default:
            WS_LOGI("Unknown WebSocket event: %d\n", event_id);
            break;
    }
}

/**
 * @brief WebSocket客户端线程函数
 * @param arg 线程参数
 */
static void websocket_client_thread(void *arg)
{
    int err;
    transport client = NULL;
    websocket_client_input_t websocket_cfg = {0};
    char uri[128] = {0};

    WS_LOGI("WebSocket client thread started\n");

    // 等待WiFi连接稳定
    while(1)
    {
        if(wifi_sta_is_link_success()) break;
        rtos_delay_milliseconds(100);
    }

    // 构建WebSocket服务器URI
    snprintf(uri, sizeof(uri), "ws://%s:%d", WEBSOCKET_SERVER_IP, WEBSOCKET_SERVER_PORT);
    websocket_cfg.uri = uri;
    websocket_cfg.enable_auto_reconnect = true;                   // 自动重连
    websocket_cfg.ws_event_handler = websocket_event_handler;     // 注册WebSocket事件处理函数
    WS_LOGI("Connecting to WebSocket server: %s\n", websocket_cfg.uri);

    // 初始化WebSocket客户端
    client = websocket_client_init(&websocket_cfg);
    if (client == NULL)
    {
        WS_LOGE("WebSocket client init failed\n");
        TEST_FAIL();
        goto exit;
    }

    // 启动WebSocket客户端
    err = websocket_client_start(client);
    if (err != 0)
    {
        WS_LOGE("WebSocket client start failed: %d\n", err);
        TEST_FAIL();
        goto exit;
    }

    // 等待连接建立
    while (!websocket_client_is_connected(client))
    {
        rtos_delay_milliseconds(100);
    }

    WS_LOGI("WebSocket connection established, sending data...\n");

    // 发送文本消息
    const char *text_msg = "hello, this is bk websocket client";
    err = websocket_client_send_text(client, text_msg, strlen(text_msg), 5000);
    if (err < 0)
    {
        WS_LOGE("Failed to send text message: %d\n", err);
        TEST_FAIL();
    }
    else
    {
        WS_LOGI("Sent text message: %s\n", text_msg);
        TEST_PASS();
    }

    // 延迟一小段时间
    rtos_delay_milliseconds(1000);

    // 发送二进制数据 \x01\x02\x03\x04
    uint8_t binary_data[] = {0x01, 0x02, 0x03, 0x04};
    err = websocket_client_send_binary(client, (const char *)binary_data, 
                                        sizeof(binary_data), 5000);
    if (err < 0)
    {
        WS_LOGE("Failed to send binary data: %d\n", err);
        TEST_FAIL();
    }
    else
    {
        WS_LOGI("Sent binary data: 01 02 03 04\n");
        TEST_PASS();
    }

    // 保持连接，等待服务器数据
    WS_LOGI("Waiting for server data...\n");
    while (1)
    {
        rtos_delay_milliseconds(100);
    }

exit:
    if (client != NULL)
    {
        websocket_client_destroy(client);
        WS_LOGI("WebSocket client destroyed\n");
    }
    rtos_delete_thread(NULL);
}

/**
 * @brief 创建WebSocket客户端线程
 * @return BK_OK 成功，其他值失败
 */
int websocket_client_create_thread(void)
{
    int ret;

    ret = rtos_create_thread(&websocket_client_handle,
                             BEKEN_APPLICATION_PRIORITY,
                             "websocket_client",
                             (beken_thread_function_t)websocket_client_thread,
                             4096,
                             NULL);

    if (ret != kNoErr)
    {
        WS_LOGE("Failed to create websocket client thread: %d\n", ret);
        return BK_FAIL;
    }

    WS_LOGI("Create websocket client thread success\n");
    return BK_OK;
}

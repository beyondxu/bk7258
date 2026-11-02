# WebSocket客户端示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述

本示例基于 Wi-Fi 组件和 WebSocket 协议，演示了一个简单的 WebSocket 客户端实现。它具体展示了如何：

1. 配置 Wi-Fi STA 参数，启动 STA 功能并连接到指定的 Wi-Fi 网络
2. 使用 os.h rtos_create_thread 接口创建线程
3. 使用 bk_websocket 模块实现 WebSocket 客户端功能
4. 连接到 WebSocket 服务器并发送文本和二进制数据
5. 接收服务器下发的文本和二进制数据

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_WEBSOCKET          | WebSocket客户端使能配置，配置为y                                         |
| CONFIG_WEBSOCKET_TLS    | WebSocket客户端支持TLS使能配置，本示例配置为n|
| CONFIG_PSA_MBEDTLS    | 新版本mbedtls使能，支持硬件加速，本示例配置为y|
| CONFIG_TRUSTENGINE    | 加解密使用硬件加速使能，默认开启，与CONFIG_SW_CRYPTO只能二选一，本示例配置为n|
| CONFIG_SW_CRYPTO    | 加解密使用软件算法，与CONFIG_TRUSTENGINE只能二选一，本示例配置为y|

## 项目结构

```
websocket/
├── ap/
│   ├── ap_main.c          # 主程序入口
│   ├── wifi_manager.c     # Wi-Fi管理模块
│   ├── wifi_manager.h     # Wi-Fi管理模块头文件
│   ├── websocket_client.c # WebSocket客户端实现
│   ├── websocket_client.h # WebSocket客户端头文件
│   └── common.h           # 公共配置和日志接口
├── cp/
├── partitions/
├── build/
├── CMakeLists.txt
├── Makefile
└── README.md
```

## 公共配置文件（common.h）

common.h 文件包含了项目的公共配置和统一的日志接口：

1. **Wi-Fi配置**：
   - `WIFI_AP_SSID`：要连接的Wi-Fi网络名称（默认为"sample_project_ap"）
   - `WIFI_AP_PASSWORD`：Wi-Fi网络密码（默认为"12345678"）

2. **WebSocket客户端配置**：
   - `WEBSOCKET_SERVER_IP`：WebSocket服务器IP地址（默认为"192.168.2.37"）
   - `WEBSOCKET_SERVER_PORT`：WebSocket服务器端口（默认为8888）
   - `WEBSOCKET_BUFFER_SIZE`：WebSocket数据缓冲区大小（默认为1024字节）

3. **统一日志接口**：
   - `WS_TAG`：日志标签（"websocket_client"）
   - `WS_LOGI(...)`：信息日志
   - `WS_LOGE(...)`：错误日志
   - `WS_LOGD(...)`：调试日志
   - `WS_LOGV(...)`：详细日志

所有模块都应包含此文件以使用统一的配置和日志接口。

## 常用接口

### Wi-Fi接口
更多接口请查看wifi.h。

``` C
    //配置STA参数
    bk_err_t bk_wifi_sta_set_config(const wifi_sta_config_t *sta_config);

    //启动sta
    bk_err_t bk_wifi_sta_start(void);

    //获取sta连接状态、连接的ap、rssi等信息
    bk_err_t bk_wifi_sta_get_link_status(wifi_link_status_t *link_status);

    //停止sta
    bk_err_t bk_wifi_sta_stop(void);

    //注册wi-fi事件回调函数
    bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, wifi_sta_event_cb, NULL);

    //注册网络事件回调函数
    bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, wifi_netif_event_cb, NULL);
```

### RTOS接口
更多接口请查看os.h。

``` C
    //创建线程
    bk_err_t rtos_create_thread(beken_thread_t *thread, uint8_t priority, const char *name, 
                               beken_thread_function_t function, uint32_t stack_size, 
                               beken_thread_arg_t arg);

    //删除线程
    bk_err_t rtos_delete_thread(beken_thread_t *thread);

    //延迟函数
    void rtos_delay_milliseconds(uint32_t ms);
```

### WebSocket接口
更多接口请查看bk_websocket_client.h。

``` C
    //初始化websocket相关资源，如申请接收、发送buffer等
    typedef struct {
        const char         *uri;                    //后台url，用户需关注
        bool               disable_auto_reconnect;  //是否启用自动重连
        int                rx_retry;
        void               *user_context;
        int                buffer_size;
        const char         *subprotocol;
        const char         *user_agent;
        const char         *authorization;
        const char         *headers;
        bk_event_handler_t ws_event_handler;       //连接成功、断开、接收数据等事件上送，用户需关注
    } websocket_client_input_t;

    //初始化WebSocket客户端
    transport websocket_client_init(const websocket_client_input_t *input);

    //启动WebSocket客户端
    int websocket_client_start(transport client);

    //检查连接状态
    bool websocket_client_is_connected(transport client);

    //发送文本数据
    int websocket_client_send_text(transport client, const char *data, int len, int timeout);

    //发送二进制数据
    int websocket_client_send_binary(transport client, const char *data, int len, int timeout);

    //销毁WebSocket客户端
    bk_err_t websocket_client_destroy(transport client);
```

## 相关代码

### 主程序入口（ap_main.c）
``` C
int main(void)
{
    int ret = 0;

    bk_init();
    
    do {
        // 初始化Wi-Fi STA模式并尝试连接
        ret = wifi_sta_connect(WIFI_AP_SSID, WIFI_AP_PASSWORD);
        if (ret != BK_OK)
        {
            WS_LOGE("Failed to connect to WiFi: %d\n", ret);
            break;
        }

        // 创建WebSocket客户端线程
        ret = websocket_client_create_thread();
        if (ret != BK_OK)
        {
            WS_LOGE("create websocket client thread failed: %d\n", ret);
            break;
        }
        WS_LOGI("create websocket client thread success\n");
    } while (0);

    return ret;
}
```

### WebSocket客户端线程（websocket_client.c）
``` C
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
```

### WebSocket事件处理函数（websocket_client.c）

> **注意** ：后台推送的websocket数据，会通过初始化注册的ws_event_handler callback上送，在处理文本数据时，用户需像下面这样判断接收的数据是否达到payload_len，以处理拆包的特殊场景。

``` C
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
                    LOGD("WS_TRANSPORT_OPCODES_TEXT\r\n");
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
```

## 操作说明
### 编译

编译命令如下：

```bash
cd sample_project/net_protocol/websocket
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录build文件夹里的all-app.bin;
- 确保"sample_project_ap"热点已启动，且密码为"12345678"；
- 确保WebSocket服务器在192.168.2.37:8888端口监听;
- 板子上电运行后，板子会自动连接"sample_project_ap"的热点，连接成功后会创建WebSocket客户端线程；
- WebSocket客户端连接成功后，会先发送文本消息"hello, this is bk websocket client"，然后发送二进制数据\x01\x02\x03\x04；
- 客户端会持续监听服务器下发的数据，根据数据类型（文本或二进制）进行相应的处理和日志输出。

以下是上电运行后的关键串口日志：

```
[2025-12-30 19:54:50.945]  ap1:WEBSOCKE:E(1605):websocket connected to ws://192.168.50.207:8888
[2025-12-30 19:54:50.945]  ap1:websocke:I(1605):Connected to WebSocket server  //连上websocket服务器
[2025-12-30 19:54:50.945]  ap1:websocke:I(1605):/homeb/honghui.xiao/7258/avdk_smp/sample_project/net_protocol/websocket/ap/websocket_client.c:76:websocket_event_handler:PASS
[2025-12-30 19:54:50.945]  ap0:websocke:I(1611):WebSocket connection established, sending data...
[2025-12-30 19:54:50.945]  ap0:websocke:I(1613):Sent text message: hello, this is bk websocket client   //给服务器发送文本数据
[2025-12-30 19:54:50.945]  ap0:websocke:I(1613):/homeb/honghui.xiao/7258/avdk_smp/sample_project/net_protocol/websocket/ap/websocket_client.c:210:websocket_client_thread:PASS
[2025-12-30 19:54:51.048]  ap1:websocke:I(1815):Received TEXT data, len:34, data:hello, this is bk websocket client
[2025-12-30 19:54:51.048]  ap1:WEBSOCKE:E(1816):Received text frame[len=34]: 
[2025-12-30 19:54:51.834]  ap0:websocke:I(2615):Sent binary data: 01 02 03 04   //给服务器发送二进制数据
[2025-12-30 19:54:51.834]  ap0:websocke:I(2615):/homeb/honghui.xiao/7258/avdk_smp/sample_project/net_protocol/websocket/ap/websocket_client.c:228:websocket_client_thread:PASS
[2025-12-30 19:54:51.834]  ap0:websocke:I(2616):Waiting for server data...
[2025-12-30 19:54:52.018]  ap1:websocke:I(2696):Received BINARY data, len:4   //收到服务器发送的二进制数据
[2025-12-30 19:54:52.018]  ap1:websocke:I(2696):01 02 03 04 

```

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。
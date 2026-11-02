| 支持的芯片 | BK7258 | BK7257 |
| --------- | ------- | ------- |

# Paho MQTT 客户端示例


## 概述

本示例基于 paho_mqtt 组件，演示了 MQTT 客户端的实现。它具体展示了如何：

1. 配置 MQTT 连接参数，连接到 MQTT 服务器
2. 如何向 MQTT 服务器发布消息
3. 如何从 MQTT 服务器订阅主题
4. 如何处理 MQTT 服务器的响应


## 核心配置文件

### paho_mqtt_demo.h

定义了 MQTT 演示的相关宏和函数声明：

```c
// MQTT 测试配置参数
#define CONFIG_MQTT_CLIENTID                "beken_mqtt_test"
#define CONFIG_MQTT_USERNAME                "admin"
#define CONFIG_MQTT_PASSWORD                "password"
#define CONFIG_MQTT_SUBTOPIC                "/mqtt/sub_test"
#define CONFIG_MQTT_PUBTOPIC                "/mqtt/pub_test"
#define CONFIG_MQTT_WILLMSG                 "Goodbye!"  

#define MQTT_TEST_QOS               0
#define MQTT_PUB_SUB_BUF_SIZE       1024
#define TEST_DATA_SIZE              256

// 函数声明
void mqtt_demo_start(char *url);
void mqtt_demo_publish_start(void);
void mqtt_demo_stop(void);
```

### ap_main.c

定义了 WiFi STA 配置和 MQTT 客户端线程：

```c
// WiFi STA 配置
#define CONFIG_EXAMPLE_WIFI_STA_SSID        "sample_project_wifi"
#define CONFIG_EXAMPLE_WIFI_STA_PASSWORD    "12345678"
#define CONFIG_MQTT_TEST_SERVER_URI         "tcp://broker.emqx.io:1883"
```

## 相关代码

### MQTT 客户端初始化

```c
void mqtt_demo_start(char *url)
{
    MQTTPacket_connectData condata = MQTTPacket_connectData_initializer;
    os_memset(&mqtt_client, 0, sizeof(MQTT_CLIENT_T));
    mqtt_demo_stats.mqtt_stopping = false;

    BK_EXAMPLE_LOG(TAG,"mqtt_demo_start\r\n");

    // 设置 MQTT 服务器地址
    mqtt_client.uri = url;
    memcpy(&mqtt_client.condata, &condata, sizeof(condata));
    // 设置 MQTT 客户端 ID
    mqtt_client.condata.clientID.cstring = CONFIG_MQTT_CLIENTID;
    // 设置 MQTT 保活间隔时间
    mqtt_client.condata.keepAliveInterval = 60;
    // 设置 MQTT 清理会话标志
    mqtt_client.condata.cleansession = 1;
    // 设置 MQTT 用户名
    mqtt_client.condata.username.cstring = CONFIG_MQTT_USERNAME;
    // 设置 MQTT 密码
    mqtt_client.condata.password.cstring = CONFIG_MQTT_PASSWORD;
    // 设置遗嘱消息标志
    mqtt_client.condata.willFlag = 1;
    // 设置遗嘱消息的 QoS 等级
    mqtt_client.condata.will.qos = MQTT_TEST_QOS;
    // 设置遗嘱消息不保留
    mqtt_client.condata.will.retained = 0;
    // 设置遗嘱消息的主题
    mqtt_client.condata.will.topicName.cstring = CONFIG_MQTT_PUBTOPIC;
    // 设置遗嘱消息的内容
    mqtt_client.condata.will.message.cstring = CONFIG_MQTT_WILLMSG;
    // 设置 MQTT 发送和接收缓冲区大小
    mqtt_client.buf_size = mqtt_client.readbuf_size = MQTT_PUB_SUB_BUF_SIZE;
    // 分配 MQTT 发送缓冲区内存
    mqtt_client.buf = os_malloc(mqtt_client.buf_size);
    // 分配 MQTT 接收缓冲区内存
    mqtt_client.readbuf = os_malloc(mqtt_client.readbuf_size);

    // 设置 MQTT 连接成功回调函数
    mqtt_client.connect_callback = mqtt_connect_callback;
    // 设置 MQTT 上线回调函数
    mqtt_client.online_callback = mqtt_online_callback;
    // 设置 MQTT 下线回调函数
    mqtt_client.offline_callback = mqtt_offline_callback;
    // 复制订阅主题
    mqtt_client.messageHandlers[0].topicFilter = os_strdup(CONFIG_MQTT_SUBTOPIC);
    // 设置订阅消息回调函数
    mqtt_client.messageHandlers[0].callback = mqtt_sub_callback;
    // 设置订阅消息的 QoS 等级
    mqtt_client.messageHandlers[0].qos = MQTT_TEST_QOS;
    // 设置默认消息回调函数
    mqtt_client.defaultMessageHandler = mqtt_sub_callback;

    // 启动 Paho MQTT 客户端
    paho_mqtt_start(&mqtt_client);
}
```

### WiFi STA 初始化和连接

```c
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
```

### MQTT 发布线程

```c
static void mqtt_publish_handler(void *parameter)
{
    char *pub_data = os_malloc(TEST_DATA_SIZE * sizeof(char));

    if (!pub_data)
    {
        BK_EXAMPLE_LOG(TAG,"no memory for pub_data\n");
        return;
    }

    memset(pub_data, 0, TEST_DATA_SIZE * sizeof(char));
    strcpy(pub_data, "Hello MQTT! This is BK7258");

    mqtt_demo_stats.start_tick = bk_get_tick();
    BK_EXAMPLE_LOG(TAG,"mqtt_pub start at '%d'\r\n", mqtt_demo_stats.start_tick);

    while (1)
    {
        rtos_get_semaphore(&g_mqtt_pub_sema, 10*1000);

        if(mqtt_demo_stats.mqtt_stopping)
        {
            goto _exit;
        }

        if (!mqtt_demo_publish(pub_data,strlen(pub_data)))
        {
            mqtt_demo_stats.pub_count++;
        }

        test_show_info();
    }

_exit:
    if (pub_data)
    {
        os_free(pub_data);
        pub_data = NULL;
    }

    mqtt_pub_thread = NULL;
    rtos_delete_thread(NULL);
}
```

## 项目配置、编译、烧录

### 项目配置

项目配置文件位于 `ap/config/` 目录下，根据不同的芯片型号有不同的配置文件：

- BK7258: `ap/config/bk7258_ap/config`
- BK7257: `ap/config/bk7257_ap/config`

主要配置项包括：

```
# 使能PAHO_MQTT组件
CONFIG_PAHO_MQTT=y

# 使能LWIP网络协议栈
CONFIG_LWIP=y
CONFIG_LWIP_V2_1=y

# 设置MQTT相关参数（在代码中直接定义）
```

### 项目编译

运行以下命令进行编译构建：

```bash
# 设置SDK路径环境变量,根据实际路径设置
export SDK_DIR=/home/user/armino/bk_idk

# 进入示例项目目录
cd sample_project/net_protocol/paho_mqtt

# 使用CMake构建
make clean
make bk7258
```

### 项目烧录

使用烧录工具（BKFIL）进行烧录，以BK7258为例，烧录目标固件：

```
build/bk7258/paho_mqtt/package/all-app.bin
```

注意：
1. 请确保烧录工具版本与芯片版本匹配，如不确定，请升级最新版本。
2. 请确保烧录工具已连接到开发板。

## 测试步骤

1. 编译并烧录示例项目到开发板
2. 开发板上电运行，会连接到指定的WiFi AP "sample_project_wifi", 默认密码：12345678
3. 开发板连接到指定的MQTT服务器 "broker.emqx.io"
4. 开发板会周期性发布消息到指定的MQTT主题 "/mqtt/pub_test"
5. 开发板会订阅指定的MQTT主题 "/mqtt/sub_test"，收到消息后会打印到串口log
6. 可使用`ap/tools/mqtt_pub_sub.py`脚本进行辅助测试：
   - 该脚本同时实现了MQTT发布和订阅功能
   - 订阅主题：`/mqtt/pub_test`（与开发板发布的主题相同）
   - 发布主题：`/mqtt/sub_test`（与开发板订阅的主题相同）
   - 运行方法：
     ```bash
     # 安装依赖
     pip install paho-mqtt
     
     # 运行脚本
     cd sample_project/net_protocol/paho_mqtt/ap/tools
     python mqtt_pub_sub.py
     ```
   - 脚本功能：
     - 接收开发板发布到`/mqtt/pub_test`的消息
     - 向开发板订阅的`/mqtt/sub_test`主题发布消息
     - 每5秒发布一条消息，共发布1000条
     - 实时显示所有接收和发布的消息

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。
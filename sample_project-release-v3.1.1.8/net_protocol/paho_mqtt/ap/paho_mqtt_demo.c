#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "paho_mqtt.h"
#include "os/os.h"
#include "os/str.h"
#include "paho_mqtt_demo.h"

#define CONFIG_MQTT_CLIENTID                "beken_mqtt_test"
#define CONFIG_MQTT_USERNAME                "admin"
#define CONFIG_MQTT_PASSWORD                "password"
#define CONFIG_MQTT_SUBTOPIC                "/mqtt/sub_test"
#define CONFIG_MQTT_PUBTOPIC                "/mqtt/pub_test"
#define CONFIG_MQTT_WILLMSG                 "Goodbye!"  

#define MQTT_TEST_QOS               0
#define MQTT_PUB_SUB_BUF_SIZE       1024
#define TEST_DATA_SIZE              256

#define TEST_PASS() 	BK_EXAMPLE_LOG(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL() 	BK_EXAMPLE_LOG(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

static beken_semaphore_t g_mqtt_pub_sema = NULL;
static beken_thread_t mqtt_pub_thread = NULL;
static MQTT_CLIENT_T mqtt_client;

// 定义 MQTT 测试配置结构体
typedef struct
{
    uint32_t pub_count;   // 发布消息计数
    uint32_t sub_count;   // 订阅消息计数
    uint32_t recon_count;   // 重新连接计数
    uint32_t start_tick;    // 测试开始时间
    bool     mqtt_stopping;    // 测试是否停止
} mqtt_demo_stats_T;


static mqtt_demo_stats_T mqtt_demo_stats = {0};
// MQTT 订阅消息回调函数，当接收到订阅主题的消息时被调用
static void mqtt_sub_callback(MQTT_CLIENT_T *c, MessageData *msg_data)
{
    mqtt_demo_stats.sub_count ++;
    BK_EXAMPLE_LOG(TAG,"mqtt_sub_callback len %d\r\n",msg_data->message->payloadlen);
    BK_EXAMPLE_LOG(TAG,"payload:%s\r\n",msg_data->message->payload);

    return;
}

// MQTT 连接成功回调函数，当 MQTT 连接成功时被调用
static void mqtt_connect_callback(MQTT_CLIENT_T *c)
{
    BK_EXAMPLE_LOG(TAG,"mqtt_connect_callback\r\n");
    return;
}
// MQTT 上线回调函数，当 MQTT 客户端上线时被调用
static void mqtt_online_callback(MQTT_CLIENT_T *c)
{
    mqtt_demo_stats.recon_count ++;
    BK_EXAMPLE_LOG(TAG,"mqtt_online_callback\r\n");
    TEST_PASS();
    return;
}
// MQTT 下线回调函数，当 MQTT 客户端下线时被调用// MQTT 下线回调函数，当 MQTT 客户端下线时被调用
static void mqtt_offline_callback(MQTT_CLIENT_T *c)
{
    BK_EXAMPLE_LOG(TAG,"mqtt_offline_callback\r\n");
    TEST_FAIL();
    return;
}

// MQTT 测试发布函数，用于向指定主题发布消息
static int mqtt_demo_publish(const char *send_str,int len)
{
    MQTTMessage message;
    const char *msg_str = send_str;
    const char *topic = CONFIG_MQTT_PUBTOPIC;

    message.qos = MQTT_TEST_QOS;
    message.retained = 0;
    message.payload = (void *)msg_str;
    message.payloadlen = len;

    bk_printf("message.payloadlen:%d\r\n",message.payloadlen);

    return mqtt_publish_with_topic(&mqtt_client, topic, &message);
}

// 显示 MQTT 测试信息的函数
static void test_show_info(void)
{
    BK_EXAMPLE_LOG(TAG,"==== MQTT test info ====\n");
    BK_EXAMPLE_LOG(TAG,"Server: %s\n",mqtt_client.uri);
    BK_EXAMPLE_LOG(TAG,"QoS   : %d\n", MQTT_TEST_QOS);
    BK_EXAMPLE_LOG(TAG,"Test duration(tick)            : %d\n", bk_get_tick() - mqtt_demo_stats.start_tick);
    BK_EXAMPLE_LOG(TAG,"Number of published  packages : %d\n", mqtt_demo_stats.pub_count);
    BK_EXAMPLE_LOG(TAG,"Number of subscribed packages : %d\n", mqtt_demo_stats.sub_count);
    BK_EXAMPLE_LOG(TAG,"Number of reconnections       : %d\n", mqtt_demo_stats.recon_count);
}

// MQTT 发布处理线程函数
static void mqtt_publish_handler(void *parameter)
{
    char *pub_data = os_malloc(TEST_DATA_SIZE * sizeof(char));

    if (!pub_data)
    {
        BK_EXAMPLE_LOG(TAG,"no memory for pub_data\n");
        return;
    }

    memset(pub_data, 0, TEST_DATA_SIZE * sizeof(char));
    strcpy(pub_data, "Hello MQTT! This is BK7239N");

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

// 启动 MQTT 发布线程的函数
void mqtt_demo_publish_start(void)
{
    int ret;

    if(mqtt_pub_thread)
    {
        BK_EXAMPLE_LOG(TAG,"mqtt_pub_thread is running\r\n");
        return;
    }

    ret = rtos_init_semaphore(&g_mqtt_pub_sema, 1);
    BK_ASSERT(kNoErr == ret);

    ret = rtos_create_thread(&mqtt_pub_thread,
                             5,
                             "publish_thread",
                             (beken_thread_function_t)mqtt_publish_handler,
                             (unsigned short)1024 * 4,
                             (beken_thread_arg_t)0);
    BK_ASSERT(kNoErr == ret);

    return;
}


// 启动 MQTT 测试的函数
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

    if (!(mqtt_client.buf && mqtt_client.readbuf))
    {
        BK_EXAMPLE_LOG(TAG,"no memory for MQTT mqtt_client buffer!\n");
        goto _exit;
    }

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

    BK_EXAMPLE_LOG(TAG,"paho_mqtt_start\r\n");
    // 启动 Paho MQTT 客户端
    paho_mqtt_start(&mqtt_client);
    return;
_exit:
    if (mqtt_client.buf)
    {
        os_free(mqtt_client.buf);
        mqtt_client.buf = NULL;
    }

    if (mqtt_client.readbuf)
    {
        os_free(mqtt_client.readbuf);
        mqtt_client.readbuf = NULL;
    }

    return;
}

// 停止 MQTT 测试的函数
void mqtt_demo_stop(void)
{
    MQTT_CLIENT_T *local_client = &mqtt_client;

    mqtt_demo_stats.mqtt_stopping = true;

    if (mqtt_demo_stats.mqtt_stopping)
    {
        rtos_set_semaphore(&g_mqtt_pub_sema);
    }

    if (local_client)
    {
        paho_mqtt_stop(local_client);
    }

    test_show_info();
    memset(&mqtt_demo_stats, 0, sizeof(mqtt_demo_stats));

    BK_EXAMPLE_LOG(TAG,"==== MQTT test stop ====\n");
}


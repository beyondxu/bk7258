#ifndef __PAHO_MQTT_H__
#define __PAHO_MQTT_H__

#include <stdint.h>
#include "MQTTPacket.h"
#include "MQTTFreeRTOS.h"

#define MQTT_USING_TLS          1

#define MQTT_DEBUG
#ifdef MQTT_DEBUG
#define  debug_printf           os_printf("[MQTT] ");os_printf
#else
#define  debug_printf           os_null_printf
#endif

#ifdef MQTT_USING_TLS
#if CONFIG_MBEDTLS
#include <tls_client.h>
#elif CONFIG_PSA_MBEDTLS
#include "tls_connect.h"
#endif
#endif

#define MQTT_SW_VERSION         "1.1.0"
#define MQTT_SW_VERSION_NUM     0x10100

#ifndef PKG_PAHOMQTT_SUBSCRIBE_HANDLERS
#define MAX_MESSAGE_HANDLERS    1 /* redefinable - how many subscriptions do you want? */
#else
#define MAX_MESSAGE_HANDLERS    PKG_PAHOMQTT_SUBSCRIBE_HANDLERS
#endif

#define MAX_PACKET_ID           65535 /* according to the MQTT specification - do not change! */

#define MQTT_SOCKET_TIMEO       6000

#ifdef MQTT_USING_TLS
#define MQTT_TLS_READ_BUFFER    4096
#endif

enum QoS
{
    QOS0,
    QOS1,
    QOS2
};

/* all failure return codes must be negative */
enum returnCode { PAHO_BUFFER_OVERFLOW = -2, PAHO_FAILURE = -1, PAHO_SUCCESS = 0 };

enum mqttControl
{
    MQTT_CTRL_SET_CONN_TIMEO = 0,      /* set mqtt connect timeout */
    MQTT_CTRL_SET_RECONN_INTERVAL,     /* set reconnect interval */
    MQTT_CTRL_SET_KEEPALIVE_INTERVAL,  /* set keepalive interval */
    MQTT_CTRL_PUBLISH_BLOCK,           /* publish data block or nonblock */
};

typedef struct MQTTMessage
{
    enum QoS qos;
    unsigned char retained;
    unsigned char dup;
    unsigned short id;
    void *payload;
    size_t payloadlen;
} MQTTMessage;

typedef struct MessageData
{
    MQTTMessage *message;
    MQTTString *topicName;
} MessageData;

typedef struct MQTT_CLIENT_S MQTT_CLIENT_T;

struct MQTT_CLIENT_S
{
    const char *uri;
    int sock;
    uint8_t is_quit;

    MQTTPacket_connectData condata;

    unsigned int next_packetid, command_timeout_ms;
    size_t buf_size, readbuf_size;
    unsigned char *buf, *readbuf;
    unsigned int keepAliveInterval;
    int connect_timeout;
    int reconnect_interval;
    int isblocking;
    int is_connected;
    uint32_t tick_ping;

    void (*connect_callback)(MQTT_CLIENT_T *);
    void (*online_callback)(MQTT_CLIENT_T *);
    void (*offline_callback)(MQTT_CLIENT_T *);

    struct MessageHandlers
    {
        char *topicFilter;
        void (*callback)(MQTT_CLIENT_T *, MessageData *);
        enum QoS qos;
    } messageHandlers[MAX_MESSAGE_HANDLERS]; /* Message handlers are indexed by subscription topic */

    void (*defaultMessageHandler)(MQTT_CLIENT_T *, MessageData *);

    /* publish interface */
    Mutex* pub_mutex;             /* publish data mutex for blocking */

    #if defined(RT_USING_POSIX) && (defined(RT_USING_DFS_NET) || defined(SAL_USING_POSIX))
    struct rt_pipe_device* pipe_device;
    int pub_pipe[2];
    #else
    int pub_sock;
    int pub_port;
    #endif /* RT_USING_POSIX && (RT_USING_DFS_NET || SAL_USING_POSIX) */

    #ifdef MQTT_USING_TLS
    MbedTLSSession *tls_session;      /* mbedtls session struct */
    #endif

    void *user_data;                  /* user-specific data */
};

/* subscribe topic receive data callback */
typedef void (*subscribe_cb)(MQTT_CLIENT_T *client, MessageData *data);

/**
 * This function start a mqtt worker thread.
 *
 * @param client the pointer of MQTT context structure
 *
 * @return the error code, 0 on start successfully.
 */
int paho_mqtt_start(MQTT_CLIENT_T *client);

/**
 * This function publish message to specified mqtt topic.
 * @note it will be discarded, recommend to use "paho_mqtt_publish"
 *
 * @param c the pointer of MQTT context structure
 * @param topicName topic  name
 * @param message the pointer of MQTTMessage structure
 *
 * @return the error code, 0 on subscribe successfully.
 */

int mqtt_publish_with_topic(MQTT_CLIENT_T *c, const char *topicName, MQTTMessage *message);


/**
 * This function stop mqtt worker thread and free mqtt client object.
 *
 * @param client the pointer of MQTT context structure
 *
 * @return the error code, 0 on start successfully.
 */
int paho_mqtt_stop(MQTT_CLIENT_T *client);

#endif /* __PAHO_MQTT_H__ */

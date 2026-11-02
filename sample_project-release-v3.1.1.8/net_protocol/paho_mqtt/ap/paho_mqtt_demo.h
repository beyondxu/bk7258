#ifndef __MQTT_DEMO_H__
#define __MQTT_DEMO_H__

#define BK_EXAMPLE_LOG   BK_LOGI
#define TAG "paho-mqtt"

void mqtt_demo_start(char *url);
void mqtt_demo_stop(void);
void mqtt_demo_publish_start(void);

#endif /*__MQTT_DEMO_H__*/



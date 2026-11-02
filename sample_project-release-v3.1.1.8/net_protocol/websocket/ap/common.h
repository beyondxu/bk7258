/*
 * common.h
 * 公共头文件
 * 包含项目通用的定义和接口
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <components/log.h>

// WiFi接入点配置
#define WIFI_AP_SSID         "sample_project_ap"    // WiFi SSID
#define WIFI_AP_PASSWORD     "12345678"             // WiFi密码

/**
 * @brief WebSocket客户端配置
 */
#define WEBSOCKET_SERVER_IP       "192.168.2.37"      // WebSocket服务器IP地址
#define WEBSOCKET_SERVER_PORT     8888               // WebSocket服务器端口
#define WEBSOCKET_BUFFER_SIZE     1024               // WebSocket数据缓冲区大小

// 统一日志接口
#define WS_TAG "websocket_client"
#define WS_LOGI(...) BK_LOGI(WS_TAG, ##__VA_ARGS__)
#define WS_LOGE(...) BK_LOGE(WS_TAG, ##__VA_ARGS__)
#define WS_LOGD(...) BK_LOGD(WS_TAG, ##__VA_ARGS__)
#define WS_LOGV(...) BK_LOGV(WS_TAG, ##__VA_ARGS__)

#define TEST_PASS()     BK_LOGI(WS_TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(WS_TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")


#endif /* __COMMON_H__ */

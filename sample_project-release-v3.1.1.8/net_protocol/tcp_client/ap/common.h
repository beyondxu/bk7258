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
 * @brief TCP客户端配置
 */
#define TCP_SERVER_IP       "192.168.2.37"      // TCP服务器IP地址
#define TCP_SERVER_PORT     8888               // TCP服务器端口
#define TCP_BUFFER_SIZE     1024               // TCP数据缓冲区大小

// 统一日志接口
#define TCP_TAG "tcp_client"
#define TCP_LOGI(...) BK_LOGI(TCP_TAG, ##__VA_ARGS__)
#define TCP_LOGE(...) BK_LOGE(TCP_TAG, ##__VA_ARGS__)
#define TCP_LOGD(...) BK_LOGD(TCP_TAG, ##__VA_ARGS__)
#define TCP_LOGV(...) BK_LOGV(TCP_TAG, ##__VA_ARGS__)

#define TEST_PASS()     BK_LOGI(TCP_TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TCP_TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")


#endif /* __COMMON_H__ */

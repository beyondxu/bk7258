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
 * @brief UDP客户端配置
 */
#define UDP_SERVER_IP       "192.168.2.37"      // UDP服务器IP地址
#define UDP_SERVER_PORT     8888               // UDP服务器端口
#define UDP_BUFFER_SIZE     1024               // UDP数据缓冲区大小

// 统一日志接口
#define UDP_TAG "udp_client"
#define UDP_LOGI(...) BK_LOGI(UDP_TAG, ##__VA_ARGS__)
#define UDP_LOGE(...) BK_LOGE(UDP_TAG, ##__VA_ARGS__)
#define UDP_LOGD(...) BK_LOGD(UDP_TAG, ##__VA_ARGS__)
#define UDP_LOGV(...) BK_LOGV(UDP_TAG, ##__VA_ARGS__)

#define TEST_PASS()     BK_LOGI(UDP_TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(UDP_TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")


#endif /* __COMMON_H__ */

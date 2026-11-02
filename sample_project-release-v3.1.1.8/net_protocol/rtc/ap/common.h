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

// 统一日志接口
#define RTC_TAG "rtc"
#define RTC_LOGI(...) BK_LOGI(RTC_TAG, ##__VA_ARGS__)
#define RTC_LOGE(...) BK_LOGE(RTC_TAG, ##__VA_ARGS__)
#define RTC_LOGD(...) BK_LOGD(RTC_TAG, ##__VA_ARGS__)
#define RTC_LOGV(...) BK_LOGV(RTC_TAG, ##__VA_ARGS__)

#define TEST_PASS()     BK_LOGI(RTC_TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(RTC_TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")


#endif /* __COMMON_H__ */

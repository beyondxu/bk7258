/*
 * ap_main.c
 * 主程序入口文件
 * 整合WiFi管理和RTC时间获取功能
 */

#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include <modules/wifi.h>
#include <components/log.h>
#include <components/netif.h>
#include <components/event.h>
#include <bk_private/bk_init.h>
#include <bk_private/bk_wifi.h>
#include <lwip/inet.h>
#include "time/time.h"
#include "common.h"
#include "wifi_manager.h"

int main(void)
{
    int ret = 0;
    struct tm rtc_time;

    bk_init();
    
    do {
        // 获取RTC时间
        ret = datetime_get(&rtc_time);
        if (ret == 0)
        {
            RTC_LOGI("RTC Time: %04d-%02d-%02d %02d:%02d:%02d\n", 
                    rtc_time.tm_year + 1900,
                    rtc_time.tm_mon + 1,
                    rtc_time.tm_mday,
                    rtc_time.tm_hour,
                    rtc_time.tm_min,
                    rtc_time.tm_sec);
        }
        else
        {
            RTC_LOGE("Failed to get RTC time: %d\n", ret);
            TEST_FAIL();
        }
        
        // 初始化Wi-Fi STA模式并尝试连接
        ret = wifi_sta_connect(WIFI_AP_SSID, WIFI_AP_PASSWORD);
        if (ret != BK_OK)
        {
            RTC_LOGE("Failed to connect to WiFi: %d\n", ret);
            TEST_FAIL();
            break;
        }

        // 等待WiFi连接稳定
        while(1)
        {
            if(wifi_sta_is_link_success()) break;
            rtos_delay_milliseconds(100);
        }

        // 等3秒让ntp同步时间成功再获取
        rtos_delay_milliseconds(3000);
        
        // 获取RTC时间
        ret = datetime_get(&rtc_time);
        if (ret == 0)
        {
            RTC_LOGI("RTC Time: %04d-%02d-%02d %02d:%02d:%02d\n", 
                    rtc_time.tm_year + 1900,
                    rtc_time.tm_mon + 1,
                    rtc_time.tm_mday,
                    rtc_time.tm_hour,
                    rtc_time.tm_min,
                    rtc_time.tm_sec);
            TEST_PASS();
        }
        else
        {
            RTC_LOGE("Failed to get RTC time: %d\n", ret);
            TEST_FAIL();
        }
    } while (0);

    return ret;
}
# RTC时间获取示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述

> **备注** ：BK7258 RTC 精度为1000 PPM，建议周期性通过网络同步RTC时间。

本示例基于 Wi-Fi 组件和 RTC 时间模块，演示了如何在 Wi-Fi 连接成功后，通过ntp自动更新RTC时间，并获取 RTC 时间。它具体展示了如何：

1. 配置 Wi-Fi STA 参数，启动 STA 功能并连接到指定的 Wi-Fi 网络
2. 等待 Wi-Fi 连接稳定
3. 使用 datetime_get 接口获取 RTC 时间
4. 以标准格式输出时间信息

## 相关配置

| 配置                     | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_NTP_SYNC_RTC         | time模块使能配置|


## 接口

> **备注** ：依赖模块为app，调用头文件为#include "time/time.h"。

接口代码位于bk_avdk_smp/ap/components/app/time/time.c，引用头文件"time.h"，然后调用下面接口，可获取和设置时间。

```c
	//设置时间
	int datetime_set(time_t  sec)   
	
	//获取时间
	int datetime_get(struct tm *t)
	
	//加上时区获取时间，zone单位为妙
	int datetime_get_with_zone(struct tm *t, int zone) 
```

## 注意事项
- sdk默认开启了ntp对时，如果不需要ntp对时，可屏蔽app_time_rtc_ntp_sync_init里的ntp_sync_task线程；
- sdk ntp对时默认是按上海市区8小时对时，如果需要改为标准的rtc时间，可把ntp.c 里的**NTP_TIMEZONE定义改为0**；
- sdk 默认的ntp服务器为："ntp.aliyun.com","ntp.tencent.com","time.windows.com"，如需修改，可修改ntp_get_local_time函数里的数组定义；


## 项目结构

```
rtc/
├── ap/
│   ├── ap_main.c          # 主程序入口
│   ├── wifi_manager.c     # Wi-Fi管理模块
│   ├── wifi_manager.h     # Wi-Fi管理模块头文件
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

2. **统一日志接口**：
   - `RTC_TAG`：日志标签（"rtc"）
   - `RTC_LOGI(...)`：信息日志
   - `RTC_LOGE(...)`：错误日志
   - `RTC_LOGD(...)`：调试日志
   - `RTC_LOGV(...)`：详细日志

所有模块都应包含此文件以使用统一的配置和日志接口。

## 相关代码

### 主程序入口（ap_main.c）
``` C
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
```

### Wi-Fi STA初始化（wifi_manager.c）
``` C
int wifi_sta_connect(const char *ssid, const char *password)
{
    wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();
    int ret = BK_OK;

    do {
        // 参数有效性检查
        if (ssid == NULL || password == NULL)
        {
            RTC_LOGE("Invalid WiFi credentials\n");
            ret = BK_ERR_PARAM;
            break;
        }

        // 设置WiFi SSID和密码
        os_memset(sta_config.ssid, 0, sizeof(sta_config.ssid));
        os_strncpy(sta_config.ssid, ssid, os_strlen(ssid));
        os_memset(sta_config.password, 0, sizeof(sta_config.password));
        os_strncpy(sta_config.password, password, os_strlen(password));

        // 设置WiFi配置
        ret = bk_wifi_sta_set_config(&sta_config);
        if (ret != BK_OK)
        {
            RTC_LOGE("Failed to set WiFi config: %d\n", ret);
            break;
        }

        // 注册WiFi事件回调
        ret = wifi_register_events();
        if (ret != BK_OK)
        {
            RTC_LOGE("Failed to register WiFi events: %d\n", ret);
            break;
        }

        // 启动WiFi STA模式
        ret = bk_wifi_sta_start();
        if (ret != BK_OK)
        {
            RTC_LOGE("Failed to start WiFi STA: %d\n", ret);
            break;
        }

        RTC_LOGI("WiFi connecting to SSID: %s\n", sta_config.ssid);
    } while (0);
    
    return ret;
}
```

## 编译命令

```bash
cd sample_project/net_protocol/rtc
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258
```

## 操作说明

- 参考[如何烧录](/BURN.md)，烧录build文件夹里的all-app.bin;
- 确保"sample_project_ap"热点已启动，且密码为"12345678"；
- 板子上电运行后，板子会自动连接"sample_project_ap"的热点，连接成功后获取RTC时间；

以下是上电运行后的关键串口日志：

```
[2025-12-30 20:47:45.798]  ap0:rtc:I(431):RTC Time: 2025-12-30 20:46:55  //上电运行获取时间
[2025-12-30 20:47:46.125]  ap0:lwip:I(664):sta ip start
[2025-12-30 20:47:46.125]  ap0:cli:W(665):BK STA connected sample_project_ap
[2025-12-30 20:47:46.125]  ap0:rtc:I(665):STA connected to sample_project_ap
[2025-12-30 20:47:46.125]  ap0:cli:W(665):BK STA got ip
[2025-12-30 20:47:46.125]  ap0:rtc:I(665):STA got ip:192.168.43.3          //wifi连接成功，并获取ip
[2025-12-30 20:47:49.273]  ap0:rtc:I(3759):RTC Time: 2025-12-30 20:46:59   //等3秒ntp同步时间成功后，获取时间

```

## 注意事项

1. **NTP对时**：如果设备未进行NTP对时，RTC时间可能不准确
2. **时区设置**：默认时区为东八区，如需修改请参考注意事项中的说明
3. **时间格式**：时间输出格式为 `YYYY-MM-DD HH:MM:SS`，年份需要加1900（因为tm_year是从1900年开始计算的）

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。
# webclient 请求http短数据示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 相关章节

 - [Wi-Fi STA示例](/wifi/sta_connect/README.md)
 - [webclient官网介绍](https://github.com/RT-Thread-packages/webclient/blob/2.2.0/README_ZH.md)

## 概述

webclient是开源模块，可以基于此模块实现http或https相关的功能，如http下载文件、http post提交等，webclient有以下特点：
- 支持 IPV4/IPV6 地址；
- 支持 GET/POST 请求方法；
- 支持文件的上传和下载功能；
- 支持 HTTPS 加密传输；
- 完善的头部数据添加和处理方式。

本示例基于 WIFI 组件，演示了wifi联网成功后，如何通过http获取短数据。它具体实现了以下功能：
1. 配置 Wi-Fi STA 参数，启动 STA 功能
2. 配置相关事件回调函数，处理 STA 功能相关事件，如连接成功、断开等
3. 访问http连接，获取返回的数据

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_WEBCLIENT          | webclient使能配置，配置为y                                         |
| CONFIG_WEBCLIENT_TLS    | webclient支持TLS使能配置，需支持https时配置为y，本示例配置为n|
| CONFIG_PSA_MBEDTLS    | 新版本mbedtls使能，支持硬件加速，本示例配置为n|
| CONFIG_TRUSTENGINE    | 加解密使用硬件加速使能，默认开启，与CONFIG_SW_CRYPTO只能二选一，本示例配置为n|
| CONFIG_SW_CRYPTO    | 加解密使用软件算法，与CONFIG_TRUSTENGINE只能二选一，本示例配置为n|

## 相关代码

``` C
    static int webclient_get_short_data(const char *uri)
    {
        char *response = NULL;
        size_t resp_len = 0;
        int index;

        // 请求url，返回的数据为response
        if (webclient_request(uri, NULL, NULL, 0, (void **)&response, &resp_len) < 0)
        {
            LOGE(TAG,"webclient send get request failed.\n");
            return BK_FAIL;
        }

        // 打印返回数据
        LOGI("[%s][%d] webclient get response data: \n", __FUNCTION__, __LINE__);
        for (index = 0; index < os_strlen(response); index++)
        {
            BK_LOG_RAW("%c", response[index]);
        }
        LOGI("\r\n");

        TEST_PASS();

        // 释放返回数据内存
        if (response)
        {
            web_free(response);
        }

        return 0;
    }
```

## 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/net_protocol/webclient/http_get_shorter_data
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录工程下build文件夹里的all-app.bin;
- 确保“Redmi_B5F9”热点已启动，且密码为“12345678”；
- 板子上电运行后，板子会自动连接“Redmi_B5F9”的热点，连上后会有sta的连接事件打印;
- 请求http连接，并打印返回数据;

以下是上电运行到关键日志：

```
[2025-12-15 15:07:20.327]  ap0:lwip:I(2657):sta ip start    //sta连接成功
[2025-12-15 15:07:20.327]  ap0:cli:W(2659):BK STA got ip    //sta获取到ip
[2025-12-15 15:07:23.466]  ap1:webclien:I(5811):[webclient_get_short_data][109] webclient get response data: 
[2025-12-15 15:07:23.593]  RT-Thread is an open source IoT operating system from China, which has strong scalability: from a tiny kernel running on a tiny core, for example ARM Cortex-M0, or Cortex-M3/4/7, to a rich feature system running on MIPS32, ARM Cortex-A8, ARM Cortex-A9 DualCore etc.                  //http返回数据
```

## 常见问题
### 程序运行提示wifi link fail

SDK V3.1.1.6及之前版本 获取wifi状态接口bk_wifi_sta_get_link_status有问题，在wifi连上时，state正常应该是WIFI_LINKSTATE_STA_GOT_IP，可临时在main函数判断wifi是否连上条件加上WIFI_LINKSTATE_STA_CONNECTED == link_status.state。

```c
    while(1)
    {
        ret = bk_wifi_sta_get_link_status(&link_status);

        if(WIFI_LINKSTATE_STA_CONNECTED == link_status.state || WIFI_LINKSTATE_STA_GOT_IP == link_status.state)
        {
            LOGI("[%s][%d] wifi link success\n", __FUNCTION__, __LINE__);
            break;
        }

        if((rtos_get_time() - ticks) >= LINK_OVER_TIME)
        {
            LOGI("[%s][%d] wifi link fail\n", __FUNCTION__, __LINE__);
            TEST_FAIL();
            return BK_FAIL;
            break;
        }

        rtos_thread_msleep(100);
    }
```
## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

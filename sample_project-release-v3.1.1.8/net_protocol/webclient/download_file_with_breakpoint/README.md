# webclient 断点续传下载文件示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 相关章节

 - [Wi-Fi STA示例](/wifi/sta_connect/README.md)
 - [webclient官网介绍](https://github.com/RT-Thread-packages/webclient/blob/2.2.0/README_ZH.md)
 - [webclient 下载整个文件示例](/net_protocol/webclient/download_file/README.md)

## 概述

> **注意** ：
旧版本SDK https不支持超时退出和断点续传，可确认sdk是否包含bk_webclient_set_timeout接口，如不包含，可将download_file/sdk_patch拷贝到开发工程里，然后在根目录Makefile加入以下shell命令：
``` shell
SDK_DIR ?= $(abspath ../../../bk_avdk_smp)

# 拷贝sdk_patch文件到sdk目录
$(if $(shell cp -vfr sdk_patch/* $(SDK_DIR)/ 2>&1),,$(error "Failed to copy sdk_patch files"))

PROJECT_MAKE_FILE := $(SDK_DIR)/tools/build_tools/build_files/project_main.mk
ifeq ($(wildcard $(PROJECT_MAKE_FILE)),)
    $(error "$(PROJECT_MAKE_FILE) not exist, please check sdk directory.")
endif
include $(PROJECT_MAKE_FILE)
``` 

webclient是开源模块，可以基于此模块实现http或https相关的功能，如http下载文件、http post提交等，webclient有以下特点：
- 支持 IPV4/IPV6 地址；
- 支持 GET/POST 请求方法；
- 支持文件的上传和下载功能；
- 支持 HTTPS 加密传输；
- 完善的头部数据添加和处理方式。

本示例实现了以下功能：
1. 配置 Wi-Fi STA 参数，启动 STA 功能
2. 配置相关事件回调函数，处理 STA 功能相关事件，如连接成功、断开等
3. 通过断点续传接口，下载https文件最后100字节数据，并存于OTA分区偏移4K位置

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_WEBCLIENT          | webclient使能配置，配置为y                                         |
| CONFIG_WEBCLIENT_TLS    | webclient支持TLS使能配置，需支持https时配置为y，本示例配置为y|
| CONFIG_PSA_MBEDTLS    | 新版本mbedtls使能，支持硬件加速，本示例配置为y|
| CONFIG_TRUSTENGINE    | 加解密使用硬件加速使能，默认开启，与CONFIG_SW_CRYPTO只能二选一，本示例配置为n|
| CONFIG_SW_CRYPTO    | 加解密使用软件算法，与CONFIG_TRUSTENGINE只能二选一，本示例配置为y|

## 相关接口

``` C
    /**
    * @brief 通过webclient下载http或者https文件（断点续传模式）
    * 
    * 使用HTTP断点续传功能通过webclient接口下载文件，支持从指定位置开始下载。
    * 下载的数据通过回调函数返回给调用者进行处理，适用于大文件下载或网络不稳定的场景。
    * 
    * @param uri 下载文件的URI地址，支持http或https    
    * @param start_pos 开始下载的位置（文件偏移量）
    * @param per_pack_size 每次下载的数据块大小
    * @param handle_function 下载数据的处理回调函数
    *        @param buffer 下载到的数据缓冲区
    *        @param size 下载到的数据长度
    *        @return 处理结果，0表示成功
    * @return 成功返回WEBCLIENT_OK，失败返回错误码
    *         - BK_ERR_PARAM: 参数无效
    *         - BK_ERR_NO_MEM: 内存分配失败
    *         - BK_ERR_NOT_FOUND: 无法获取文件长度
    */
    int download_file_with_breakppoint_by_webclient(const char *uri, uint32_t start_pos, int per_pack_size, int (*handle_function)(char *buffer, int size))
```

## 相关代码

``` C
    extern void bk_webclient_set_timeout(int timeout_sec);
    bk_webclient_set_timeout(5);   //设置超时时间，5秒

    //断点续传文件末尾100字节，下载数据通过download_handle_func返回
    ret = download_file_with_breakppoint_by_webclient("https://downloadtest.trenditadmin.com/app_pack_V119.rbl", 
                                                      1542144-100,
                                                      1024,
                                                      download_handle_func);
```

## 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/net_protocol/webclient/download_file_with_breakpoint
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录工程下build文件夹里的all-app.bin;
- 确保“Redmi_B5F9”热点已启动，且密码为“12345678”；
- 板子上电运行后，板子会自动连接“Redmi_B5F9”的热点，连上后会有sta的连接事件打印;
- 自动下载网络文件;

以下是上电运行到关键日志：

```
[2025-12-16 20:40:30.268]  ap0:webclien:I(1032):STA got ip:192.168.50.201    //sta获取到ip
[2025-12-16 20:40:31.366]  ap1:TLS:I(2149):Connected downloadtest.trenditadmin.com:443 success...  //https连接成功
[2025-12-16 20:40:32.406]  ap1:webclien:I(3230):[download_handle_func][130] filelen is :1542144    //文件长度
[2025-12-16 20:40:33.300]  ap0:webclien:I(4003):[download_handle_func][136] buffer_length :100  //download_handle_func回调打印的下载的数据长度
[2025-12-16 20:40:33.300]  ap1:webclien:I(4011):[main][202] download success   //下载成功
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

### 下载失败

在板子RF有校准，并且下载url及wifi网络是正常的情况下，还是会断点续传下载失败，可能原因如下：
- download_file_with_breakppoint_by_webclient接口的callback函数，完成后是否返回成功0，webclient内部在这个回调函数返回非0时，会认为失败然后退出下载；
- download_file_with_breakppoint_by_webclient接口的callback执行函数里，是否释放有释放buffer内存，不释放会导致内存泄漏，最后内存申请失败导致下载失败；

如果还有失败，日志等级可以调整为6，查看webclient更详细的日志信息。

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

# Wi-Fi P2P GC示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 相关资料
- [Wi-Fi P2P 使用指南](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/developer-guide/wifi/bk_wifi_p2p.html) 

## 概述

> **注意** ：
1. AP 和 P2P模式不能共存，只能选择其中一种模式，应用程序使用时需注意互斥；
2. sdk V3.1.1.7及之前版本不支持P2P GO模式，如需使用请升级到V3.1.1.8及以上版本；

可先查阅[Wi-Fi P2P 使用指南](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/developer-guide/wifi/bk_wifi_p2p.html) 了解 P2P 功能的基本原理和使用方法， SDK默认是GO模式（类似AP模式），**默认ip地址为192.168.49.1**。

本示例基于 Wi-Fi P2P 组件，演示了 Wi-Fi P2P GC（Group Client）模式的实现。它具体展示了如何：

1. 启动 P2P GC 模式，创建 P2P 热点
2. 配置相关事件回调函数，处理 P2P 功能相关事件，如连接成功、获取IP等
3. 手机端可在 WiFi 设置中发现并连接此 P2P 热点

## 相关配置

可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

### AP核配置

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_P2P          | P2P功能使能配置，配置为y                                         |

### CP核配置

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_P2P          | P2P功能使能配置，配置为y                                         |

## 常用接口

更多接口请查看wifi.h。

> **注意** ：P2P GO模式和GC模式，主要调用的启动接口不一样，其它都是一样的。
    
``` C
    //启用P2P GO模式第一种方法
    bk_err_t bk_wifi_p2p_enable(const char *ssid);

    //启用P2P GO模式第二种方法
    //intent传入15， 强制启用P2P GO模式
    bk_err_t bk_wifi_p2p_enable_with_intent(const char *ssid, int intent);

    //intent传入0， 强制启用P2P GC模式
    bk_err_t bk_wifi_p2p_enable_with_intent(const char *ssid, int intent);

    //开始P2P设备发现
    bk_err_t bk_wifi_p2p_find(void);

    //禁用P2P模式
    bk_err_t bk_wifi_p2p_disable(void);

    //注册wi-fi事件回调函数
    bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, p2p_wifi_event_cb, NULL);

    //注册网络事件回调函数
    bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, p2p_netif_event_cb, NULL);
```

## 相关代码

``` C
    // 注册 P2P 事件回调函数，处理所有 Wi-Fi 事件，网络接口事件
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, p2p_wifi_event_cb, NULL));
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, p2p_netif_event_cb, NULL));

    // 启用 P2P GC 模式
    BK_LOG_ON_ERR(bk_wifi_p2p_enable_with_intent(P2P_DEVICE_NAME, 0));
    
    // 开始 P2P 设备发现
    BK_LOG_ON_ERR(bk_wifi_p2p_find());
```

## 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/wifi/p2p_gc
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录build文件夹里的all-app.bin;
- 板子上电运行后，板子会启动 P2P GC 模式；
- 在手机“设置”/"WLAN", "WLAN直连"里搜索（不同手机可能不一样，去到WLAN直连界面），并连接到 "P2P_GC_Device" 热点；
- 连接成功后，板子端会输出连接成功和获取IP的事件日志。

以下是上电运行到手机连接成功后的log输出：

```
 ap0:lwip:D(47216):ip_addr: 192.168.49.227      // 板子端获取IP地址
 ap0:cli:W(47217):BK STA connected 
 ap0:p2p_gc:I(47217):[p2p_wifi_event_cb][39] P2P connected success:   //应用代码收到P2P 连接成功事件
 ap0:cli:W(47217):unknown netif got ip
 ap1:p2p_gc:I(47218):unknown netif got ip:192.168.49.227  //应用代码收到获取IP地址事件

```

## 更多WIFI开发指南请参考 [技术文档](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/developer-guide/wifi/index.html)。

## 常见问题
### 项目上到底使用GO模式还是GC模式？

一般推荐使用GO模式，板子的ip固定为192.168.49.1，而GC模式的ip是动态的，由对端设备分配。这样GC模式对端设备上的应用程序无法知道板子的ip是什么，只能通过其它方式（如ble或者网络广播）来交互获取板子的ip地址，操作相比GO模式要复杂一些。

### 板子wifi P2P一直连接不成功

可以尝试以下方法：
- 有些手机mcc没做好，可以将手机的wifi不连接热点（将连过的2.4G wifi断开），或者连接5G wifi，然后重新搜索连接；
- 通过memshow指令（如果提示指令不存在，可以参考[menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改)，将CP核的CONFIG_DEBUG_VERSION打开），确认CP核的sram内存是否够(heap行minimum列内容，如果接近0表示sram内存峰值使用快没了)，不够可参考 [官网文档如何调整内存](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/smp_doc/bk7258/zh_CN/v3.1.1/developer-guide/memory_resource/index.html) 可适当增加sram内存大小；

``` c
name    total   free    minimum   peak 
heap	280576	263024	258496	22080
psram	655360	638624	638624	16736
``` 

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

# Wi-Fi AP示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述

本示例基于 Wi-Fi 组件 SoftAP 示例，演示了 Wi-Fi SoftAP 的实现。它具体展示了如何：

1. 配置 Wi-Fi SoftAP 参数，启动 SoftAP；
2. 如何处理连接到 SoftAP 的 STA 设备事件；
3. 如何获取连接到 SoftAP 的 STA 设备信息；

## 相关代码

``` C
    // 使用默认配置初始化 AP 配置结构体
    wifi_ap_config_t ap_config = WIFI_DEFAULT_AP_CONFIG();
    netif_ip4_config_t ip4_config = {0};

    os_memset(&ip4_config, 0, sizeof(ip4_config));
    os_strcpy(ip4_config.ip, DEFAULT_IP);
    os_strcpy(ip4_config.mask, DEFAULT_MASK);
    os_strcpy(ip4_config.gateway, DEFAULT_GATEWAY);
    os_strcpy(ip4_config.dns, DEFAULT_DNS);
    BK_LOG_ON_ERR(bk_netif_set_ip4_config(NETIF_IF_AP, &ip4_config));     // 设置 AP 网络接口的 IPv4 配置

    // 设置 AP 的 SSID
    os_strncpy(ap_config.ssid, WIFI_AP_SSID, os_strlen(WIFI_AP_SSID));
    // 设置 AP 的密码
    os_strncpy(ap_config.password, WIFI_AP_PASSWORD, os_strlen(WIFI_AP_PASSWORD));

    LOGI("ssid:%s  key:%s\r\n", ap_config.ssid, ap_config.password);
    BK_LOG_ON_ERR(bk_wifi_ap_set_config(&ap_config));
    BK_LOG_ON_ERR(bk_wifi_ap_start());                                    // 启动 AP

    // 注册 WiFi 事件回调函数，监听所有 WiFi 事件，若失败则记录日志
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, wifi_softap_event_cb, NULL));
```

## 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/wifi/softap
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/wifi/softap/build/bk7258/softap/package/all-app.bin；
- 上电运行后，板子会启动“sample_project_ap”的热点；
- 手机或者其他板子连接sample_project_ap热点，会有sta的连接事件打印。  

以下是上电运行后的串口log 输出以及手机连接热点后的log输出

```
ef:I(7):EasyFlash V4.1.0 is initialize success.
cal:W(18):temp in otp is:560
xtal_cali:58
cal:I(95):idx:40=40+(0),r:54,xtal:58,pwr_gain:a0ab7128
psram:W(106):psram type(16MB) not match CONFIG_PSRAM_CAPACITY 0X00800000
ap1:bk_ipc:D(0):bk_ipc_core_init
ap1:bk_ipc:D(0):open channel: 23 on CPU: 1
ap1:bk_ipc:D(0):channel: gpio_ipc
ap1:OS:D(0):create ipc thread, tcb=60720b08, stack=[607202e0-60720ae0:2048], prio=5, xCoreID=-1
ap1:bk_ipc:D(0):bk_ipc_core_init success
ap1:os:D(0):
ap1:os:D(0):mem_type start    end      size    
ap1:os:D(0):-------- -------- -------- --------
ap1:os:D(0):itcm     0x20     0x80     96      
ap1:os:D(0):dtcm     0x20000004 0x20000004 0       
ap1:os:D(0):ram      0x28010480 0x28061ff8 334712  
ap1:os:D(0):non_heap 0x28010480 0x2801d898 54296   
ap1:os:D(0):iram     0x8010000 0x8010480 1152    
ap1:os:D(0):data     0x28010480 0x28011a14 5524    
ap1:os:D(0):bss      0x28012800 0x2801d894 45204   
ap1:os:D(0):heap     0x2801d898 0x28061ff8 280416  
ap1:os:D(0):psram    0x60720000 0x607c0000 655360  
ap1:ef:I(0):ENV start address is 0x007FC000, size is 8192 bytes.
ap0:ef:I(268):EasyFlash V4.1.0 is initialize success.
ap0:D(268):ret_code444444444=0
ap0:D(350):driver_init end
ap0:init:D(350):reason - power on
ap0:init:D(350):regs - 0, 0, 0
ap0:init:D(351):armino rev: 
ap0:init:D(351):armino soc id:53434647_72360101
ap0:bk_init:D(351):armino app init: Nov 18 2025 17:30:42
ap0:bk_init:D(351):verify id: D13970
ap0:bk_init:D(351):APP Version: unknown
ap0:OS:D(351):create event, tcb=280058e8, stack=[28005958-28006558:3072], prio=1, xCoreID=-1

$ap0:lwip:D(352):init TCP/IP
ap0:OS:D(352):create tcp/ip, tcb=607213c8, stack=[60720ba0-607213a0:2048], prio=7, xCoreID=-1
ap1:WDRV:D(353):bk_wifi_init, 174
ap1:WDRV:D(353):wdrv_init
ap1:D(353):wdrv_ipc_init
ap1:WDRV:D(353):wdrv_ipc_env_init
ap1:WDRV:W(354):wdrv_tx_cmd_buffer_init, addr[0]=0x28011a18, pattern_addr=0x28011a14
ap1:WDRV:W(354):wdrv_tx_cmd_buffer_init, addr[1]=0x28011d38, pattern_addr=0x28011d34
ap1:WDRV:W(354):wdrv_tx_cmd_buffer_init, addr[2]=0x28012058, pattern_addr=0x28012054
ap1:OS:D(354):create wdrv_thread, tcb=60722488, stack=[60721460-60722460:4096], prio=7, xCoreID=-1
ap1:WDRV:D(355):wdrv_host_cmd_id : 514
ap1:WDRV:D(355):wdrv_tx_msg: msg_id:0x202 len:12 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(355):wdrv_rx_handle_msg id:8202 cfm_sn:0 len:6
ap0:WDRV:D(356):wdrv_tx_msg: msg_id:0x366 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(356):wdrv_rx_handle_msg id:8366 cfm_sn:0 len:4
ap0:WDRV:D(356):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(357):wdrv_rx_handle_msg id:8366 cfm_sn:0 len:4
ap0:WDRV:D(357):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(358):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(358):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:D(359):wdrv_rx_handle_msg id:8366 cfm_sn:0 len:4
ap0:WDRV:D(359):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(360):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(360):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:D(360):wdrv_rx_handle_msg id:8366 cfm_sn:0 len:4
ap0:WDRV:D(361):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(361):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(361):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap1:WDRV:D(362):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap1:WDRV:D(362):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:D(362):wifi inited(1)
ap1:gpio:D(363):bk_gpio_driver_init:has inited 
wakeup
ap0:D(367):WIFI AT CMDS INIT OK
ap1:bluetoot:D(367):bk_bluetooth_init start, 0 
ap0:bt_ipc:D(368):open channel: 19 on CPU
ap0:bt_ipc:D(368):bt_ipc_init success
bt_ipc:W(471):bt_ipc_init bt ipc already initialised
wakeup
rwnx_cal_set_rfconfig(0x101) phy on; rf off
ap0:ble:D(376):bt_feature_adapter_init _is_gatt_discovery_auto 1
ap0:ble:D(376):bt_feature_adapter_init _ignore_smp_key_distr_all_zero 0
ap0:ble:D(376):bt_feature_adapter_init _ignore_smp_already_pair 0
ap0:OS:D(377):create ble_host, tcb=280227b8, stack=[28021b90-28022790:3072], prio=5, xCoreID=-1
ap0:ble:D(378):timer_init
ap0:ble:D(378):kernel_mem_init type 0 size 3396
ap0:ble:D(378):kernel_mem_init type 1 size 3084
ap0:ble:D(379):kernel_mem_init type 2 size 6156
ap0:ble:D(379):kernel_mem_init type 3 size 12
ap0:ble:D(379):timer_set_timeout, to 0 ms
ap0:ble:D(380):timer_set_timeout, to 0 ms
ap0:ble:D(420):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x1,status:0x0
ap0:ble:D(446):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x3,status:0x0
ap0:ble:D(454):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x1a,status:0x0
ap0:ble:D(461):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x1a,status:0x0
ap0:ble:D(461):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x28,status:0x0
ap0:ble:D(462):gapm_cmp_evt:BLE_STACK_OK
ap1:AT_BLE:D(464):BLE AT CMDS INIT OK
ap1:bluetoot:D(464):bk_bluetooth_init ok
ap0:wifi_AP:I(465):ssid:sample_project_ap  key:12345678
ap0:WDRV:D(465):ap configuring
ap0:WDRV:D(465):wdrv_tx_msg: msg_id:0x323 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(466):wdrv_rx_handle_msg id:8323 cfm_sn:0 len:4
ap0:WDRV:D(466):wdrv_rx_handle_cmd_confirm,460,ID:0x323
ap0:WDRV:D(467):wdrv_tx_msg: msg_id:0x367 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(468):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(468):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap1:WDRV:W(468):the max conn num is zero, set it is default
ap1:WDRV:D(469):wdrv_tx_msg: msg_id:0x320 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
wifi:W(572):the max conn num is zero, set it is default
ap0:WDRV:D(469):wdrv_rx_handle_msg id:8320 cfm_sn:0 len:4
ap0:WDRV:D(470):wdrv_rx_handle_cmd_confirm,460,ID:0x320
ap1:WDRV:D(470):ap configured
ap1:WDRV:D(470):bk_wifi_ap_init, 326
ap1:WDRV:D(470):wifi already init, reinit anyway!
ap1:WDRV:D(471):wdrv_tx_msg: msg_id:0x367 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(471):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(472):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap1:WDRV:D(472):wdrv_tx_msg: msg_id:0x366 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(473):wdrv_rx_handle_msg id:8366 cfm_sn:0 len:4
ap0:WDRV:D(473):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap1:WDRV:D(473):wdrv_tx_msg: msg_id:0x367 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(474):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(474):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap1:WDRV:D(475):wdrv_tx_msg: msg_id:0x367 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(476):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(476):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap1:WDRV:D(476):wifi inited(21)
ap1:WDRV:D(476):wdrv_tx_msg: msg_id:0x321 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
wifid:I(592):mm_add_if_req: vif_idx=1, type=2, p2p=0, status=0
wifid:I(593):chan_ctxt_add: CTXT0,freq2462MHz,bw20MHz,pwr0dBm
wifid:I(593):chan_reg_fix:VIF1,CTXT0,type2,ctxt_s0,nb_vif0
wifid:I(597):[KW:]mm_set_vif_state,vif=1,vif_type=2,is_active=1, aid=0x0,rssi=0
hitf:I(597):start apm success, vif1, channel0, bcmc3
hitf:I(1821):add CCMP
lwip:I(1823):mac c8:47:8c:91:bd:d0
ap0:WDRV:D(1722):wdrv_rx_handle_msg id:8321 cfm_sn:0 len:4
ap0:WDRV:D(1723):wdrv_rx_handle_cmd_confirm,460,ID:0x321
ap1:WDRV:D(1723):ap started
ap1:lwip:D(1723):configuring iface ap (with Static IP)
ap1:lwip:D(1725):lwip_socket, main create socked:1
ap1:lwip:D(1726):lwip_socket, main create socked:2
ap1:lwip:D(1727):lwip_socket, main create socked:3
ap1:OS:D(1728):create dhcp-server, tcb=607240c8, stack=[60723aa0-607240a0:1536], prio=2, xCoreID=-1
wifid:I(22131):mm_sta_add:vif 1,sta 0,status 0
hitf:I(22132):add sta, sta=0, pm state=0
hitf:I(22140):add CCMP
wpa:I(22152):WPA: TK a581e6df5dc7847c82af8887c9d15c6b
wpa:I(22153):WPA: GTK 6c40d0560680749f0e1c0e3bde12101d
hitf:I(22160):add CCMP
ap0:WDRV:D(22059):wdrv_rx_handle_msg id:4 cfm_sn:0 len:6
ap0:WDRV:D(22059):wdrv_rx_handle_event,584
ap0:WDRV:D(22059):wdrv_rx_handle_wifi_cntrl_event,472,4
ap0:WDRV:D(22060):AP-INDICATE: e2:29:7c:3b:4e:e4 connected
ap1:cli:W(22060):e2:29:7c:3b:4e:e4 connected to BK AP
ap1:wifi_AP:I(22060):e2:29:7c:3b:4e:e4 connected to AP
ap1:wifi_AP:I(22061):/homeb/honghui.xiao/7258/avdk_smp/sample_project/wifi/softap/ap/ap_main.c:76:wifi_softap_event_cb:PASS
lwip:I(22304):ap:DHCP should send offer
lwip:I(22480):ap:DHCP should send ack
ap0:WDRV:D(23061):wdrv_tx_msg: msg_id:0x327 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(23061):wdrv_rx_handle_msg id:8327 cfm_sn:0 len:4
ap0:WDRV:D(23062):wdrv_rx_handle_cmd_confirm,460,ID:0x327
ap0:WDRV:D(23062):wdrv_tx_msg: msg_id:0x328 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(23062):wdrv_rx_handle_msg id:8328 cfm_sn:0 len:4
ap0:WDRV:D(23062):wdrv_rx_handle_cmd_confirm,460,ID:0x328
ap0:wifi_AP:I(23062):sta_num: 1
ap0:wifi_AP:I(23063):[01]Mac addr: e2:29:7c:3b:4e:e4, ap0:wifi_AP:I(23063):ip: 192.168.188.100, rssi -37
```
## 更多WIFI开发指南请参考 [技术文档](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/developer-guide/wifi/index.html)。


## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

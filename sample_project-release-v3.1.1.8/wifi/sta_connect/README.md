# Wi-Fi STA示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述

本示例基于 Wi-Fi 组件，演示了 Wi-Fi STA 连接的实现。它具体展示了如何：

1. 配置 Wi-Fi STA 参数，启动 STA 功能
2. 配置相关事件回调函数，处理 STA 功能相关事件，如连接成功、断开等
3. 连接到指定的 Wi-Fi 网络

## 常用接口

更多接口请查看wifi.h。

    
``` C
    //配置STA参数
    bk_err_t bk_wifi_sta_set_config(const wifi_sta_config_t *sta_config);

    //启动sta
    bk_err_t bk_wifi_sta_start(void);

    //获取sta连接状态、连接的ap、rssi等信息
    bk_err_t bk_wifi_sta_get_link_status(wifi_link_status_t *link_status);

    //停止sta
    bk_err_t bk_wifi_sta_stop(void);

    //注册wi-fi事件回调函数
    bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, wifi_sta_event_cb, NULL);

    //注册网络事件回调函数
    bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, wifi_netif_event_cb, NULL);
```

## 相关代码

``` C
    // 注册 Wi-Fi 事件回调函数，处理所有 Wi-Fi 事件，网络接口事件
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, wifi_sta_event_cb, NULL));
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, wifi_netif_event_cb, NULL));

    wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();

    os_strncpy(sta_config.ssid, WIFI_AP_SSID, os_strlen(WIFI_AP_SSID));
    os_strncpy(sta_config.password, WIFI_AP_PASSWORD, os_strlen(WIFI_AP_PASSWORD));
    BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
    BK_LOG_ON_ERR(bk_wifi_sta_start()); // 启动 Wi-Fi STA 模式，并在出错时记录日志
```

## 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/wifi/sta_connect
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/wifi/sta_connect/build/bk7258/sta_connect/package/all-app.bin;
- 确保“sample_project_ap”热点已启动，且密码为“12345678”；
- 板子上电运行后，板子会自动连接“sample_project_ap”的热点，连上后会有sta的连接事件打印。 

以下是上电运行到连上热点“sample_project_ap”后的log输出：

```
ef:I(7):EasyFlash V4.1.0 is initialize success.
cal:W(18):temp in otp is:560
xtal_cali:58
cal:I(95):idx:40=40+(0),r:54,xtal:58,pwr_gain:a0ab7128
psram:W(105):psram type(16MB) not match CONFIG_PSRAM_CAPACITY 0X00800000
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
ap0:D(349):driver_init end
ap0:init:D(349):reason - power on
ap0:init:D(349):regs - 0, 0, 0
ap0:init:D(349):armino rev: 
ap0:init:D(349):armino soc id:53434647_72360101
ap0:bk_init:D(349):armino app init: Nov 18 2025 16:55:55
ap0:bk_init:D(349):verify id: D13970

$ap0:bk_init:D(349):APP Version: unknown
ap0:OS:D(349):create event, tcb=280058e8, stack=[28005958-28006558:3072], prio=1, xCoreID=-1
ap0:lwip:D(350):init TCP/IP
ap0:OS:D(351):create tcp/ip, tcb=607213c8, stack=[60720ba0-607213a0:2048], prio=7, xCoreID=-1
ap1:WDRV:D(351):bk_wifi_init, 174
ap1:WDRV:D(351):wdrv_init
ap1:D(351):wdrv_ipc_init
ap1:WDRV:D(352):wdrv_ipc_env_init
ap1:WDRV:W(352):wdrv_tx_cmd_buffer_init, addr[0]=0x28011a18, pattern_addr=0x28011a14
ap1:WDRV:W(352):wdrv_tx_cmd_buffer_init, addr[1]=0x28011d38, pattern_addr=0x28011d34
ap1:WDRV:W(352):wdrv_tx_cmd_buffer_init, addr[2]=0x28012058, pattern_addr=0x28012054
ap1:OS:D(352):create wdrv_thread, tcb=60722488, stack=[60721460-60722460:4096], prio=7, xCoreID=-1
ap0:WDRV:D(353):wdrv_host_cmd_id : 514
ap0:WDRV:D(353):wdrv_tx_msg: msg_id:0x202 len:12 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(354):wdrv_rx_handle_msg id:8202 cfm_sn:0 len:6
ap0:WDRV:D(354):wdrv_tx_msg: msg_id:0x366 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(354):wdrv_rx_handle_msg id:8366 cfm_sn:0 len:4
ap0:WDRV:D(354):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(355):wdrv_rx_handle_msg id:8366 cfm_sn:0 len:4
ap0:WDRV:D(355):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(356):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(356):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:D(357):wdrv_rx_handle_msg id:8366 cfm_sn:0 len:4
ap0:WDRV:D(357):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(358):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(358):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:D(358):wdrv_rx_handle_msg id:8366 cfm_sn:0 len:4
ap0:WDRV:D(359):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(359):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(359):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:D(360):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(360):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:D(361):wifi inited(1)
ap1:gpio:D(361):bk_gpio_driver_init:has inited 
wakeup
ap0:D(365):WIFI AT CMDS INIT OK
ap1:bluetoot:D(365):bk_bluetooth_init start, 0 
ap1:bt_ipc:D(366):open channel: 19 on CPU
ap1:bt_ipc:D(366):bt_ipc_init success
bt_ipc:W(470):bt_ipc_init bt ipc already initialised
wakeup
rwnx_cal_set_rfconfig(0x101) phy on; rf off
ap0:ble:D(374):bt_feature_adapter_init _is_gatt_discovery_auto 1
ap0:ble:D(374):bt_feature_adapter_init _ignore_smp_key_distr_all_zero 0
ap0:ble:D(375):bt_feature_adapter_init _ignore_smp_already_pair 0
ap0:OS:D(375):create ble_host, tcb=280227b8, stack=[28021b90-28022790:3072], prio=5, xCoreID=-1
ap0:ble:D(376):timer_init
ap0:ble:D(376):kernel_mem_init type 0 size 3396
ap0:ble:D(377):kernel_mem_init type 1 size 3084
ap0:ble:D(377):kernel_mem_init type 2 size 6156
ap0:ble:D(377):kernel_mem_init type 3 size 12
ap0:ble:D(377):timer_set_timeout, to 0 ms
ap0:ble:D(379):timer_set_timeout, to 0 ms
ap0:ble:D(419):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x1,status:0x0
ap0:ble:D(445):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x3,status:0x0
ap0:ble:D(452):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x1a,status:0x0
ap0:ble:D(459):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x1a,status:0x0
ap0:ble:D(459):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x28,status:0x0
ap0:ble:D(460):gapm_cmp_evt:BLE_STACK_OK
ap1:AT_BLE:D(462):BLE AT CMDS INIT OK
ap1:bluetoot:D(462):bk_bluetooth_init ok
ap0:WDRV:D(463):sta configuring
ap0:WDRV:D(463):sta config, ssid=sample_project_ap password=12345678 security=12
ap0:WDRV:D(463):sta configured(5)
ap0:WDRV:D(463):wdrv_tx_msg: msg_id:0x310 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(464):wdrv_rx_handle_msg id:8310 cfm_sn:0 len:4
ap0:WDRV:D(465):wdrv_rx_handle_cmd_confirm,460,ID:0x310
ap1:WDRV:D(465):sta starting
ap1:WDRV:D(465):wdrv_tx_msg: msg_id:0x366 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(466):wdrv_rx_handle_msg id:8366 cfm_sn:0 len:4
ap0:WDRV:D(466):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(467):wdrv mac addr:c8:47:8c:91:bd:d1
ap0:WDRV:D(467):sta connecting
ap0:WDRV:D(467):sta connected(f)
ap0:WDRV:D(467):wdrv_tx_msg: msg_id:0x311 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
wifid:I(585):mm_add_if_req: vif_idx=0, type=0, p2p=0, status=0
wpa:I(591):State: DISCONNECTED -> INACTIVE
wpa:I(591):State: INACTIVE -> DISCONNECTED
lwip:I(591):mac c8:47:8c:91:bd:d1
wpa:I(593):State: DISCONNECTED -> INACTIVE
wpa:I(593):Setting scan request: 0.000000 sec
wpa:I(594):State: INACTIVE -> SCANNING
ap0:WDRV:D(491):wdrv_rx_handle_msg id:8311 cfm_sn:0 len:4
ap0:WDRV:D(491):wdrv_rx_handle_cmd_confirm,460,ID:0x311
ap0:wifi_AP:I(492):ssid:sample_project_ap password:12345678
wifid:I(595):[KW:]scanu_start_req:src_id=15,vif=0,dur=0,chan_cnt=1,ssid_len=17,bssid=0xffff-ffff-ffff
wifid:I(595):[KW:]scanu_start_req:ssid=sample_project_ap
wifid:I(596):[KW:]scanu_start_req:chan_idx=7
os:I(604):psram:0x60700000,size:131072
wifid:I(638):chan_unregister_once_req: VIF0,type=11
wifid:I(639):[KW:]scanu_confirm:status=0,req_type=0,upload_cnt=3,recv_cnt=3,time=44006us,result=1,rfcli=0
wpa:I(640):Scan completed in 0.042000 seconds
hitf:I(640):get scan result:1
wpa:I(642):State: SCANNING -> ASSOCIATING
wifid:I(642):[KW:]conn vif0-0,auth_type:0,bssid:2fb4-cfa4-9314,ssid:sample_project_ap,is encryp:8.
wifid:I(644):chan_ctxt_add: CTXT0,freq2442MHz,bw20MHz,pwr27dBm
wifid:I(644):chan_reg_fix:VIF0,CTXT0,type3,ctxt_s0,nb_vif0
wifid:I(645):mm_sta_add:vif 0,sta 0,status 0
wifid:I(647):[KW:]auth_send:seq1, txtype0, auth_type0, seq4
wifid:I(662):[KW:]sm_auth_handler: status code 0, tx status 0x80800000
wifid:I(662):[KW:]assoc_req_send:is ht, seq_num:5
wifid:I(671):[KW:]assoc_rsp:status0,tx_s0x80800000
wifid:I(671):[KW:]mm_set_vif_state,vif=0,vif_type=0,is_active=1, aid=0x3,rssi=-40
wpa:I(672):State: ASSOCIATING -> ASSOCIATED
wifid:I(686):[KW:]sm_disassoc_handler: reason code 2, state 0
wifid:I(686):[KW:]mm_set_vif_state,vif=0,vif_type=0,is_active=0, aid=0x0,rssi=-40
wifid:I(686):sta_del_req 0
wifid:I(687):chan_unreg_fix:VIF0,CTXT0,nb_vif1
wpa:I(687):Setting scan request: 0.100000 sec
wpa:I(688):State: ASSOCIATED -> DISCONNECTED
ap0:WDRV:D(585):wdrv_rx_handle_msg id:2 cfm_sn:0 len:8
ap0:WDRV:D(585):wdrv_rx_handle_event,584
ap0:WDRV:D(586):wdrv_rx_handle_wifi_cntrl_event,472,2
ap0:WDRV:D(586):WLAN-INDICATE: disconected and stop send data
ap1:cli:W(586):BK STA disconnected, reason(2)
ap1:wifi_AP:I(586):STA disconnected, reason(2)
wpa:I(788):State: DISCONNECTED -> SCANNING
wifid:I(788):[KW:]scanu_start_req:src_id=15,vif=0,dur=0,chan_cnt=14,ssid_len=17,bssid=0xffff-ffff-ffff
wifid:I(789):[KW:]scanu_start_req:ssid=sample_project_ap
wifid:I(2493):[KW:]scanu_confirm:status=0,req_type=0,upload_cnt=13,recv_cnt=73,time=1705618us,result=1,rfcli=0
wpa:I(2494):Scan completed in 1.704000 seconds
hitf:I(2494):get scan result:1
wpa:I(2495):State: SCANNING -> ASSOCIATING
wifid:I(2495):[KW:]conn vif0-0,auth_type:0,bssid:2fb4-cfa4-9314,ssid:sample_project_ap,is encryp:8.
wifid:I(2496):chan_ctxt_add: CTXT1,freq2442MHz,bw20MHz,pwr27dBm
wifid:I(2496):chan_reg_fix:VIF0,CTXT1,type3,ctxt_s0,nb_vif0
wifid:I(2496):mm_sta_add:vif 0,sta 1,status 0
wifid:I(2498):[KW:]auth_send:seq1, txtype0, auth_type0, seq50
wifid:I(2511):[KW:]sm_auth_handler: status code 0, tx status 0x80800000
wifid:I(2511):[KW:]assoc_req_send:is ht, seq_num:51
wifid:I(2519):[KW:]assoc_rsp:status0,tx_s0x80800000
wifid:I(2519):[KW:]mm_set_vif_state,vif=0,vif_type=0,is_active=1, aid=0x1,rssi=-46
wpa:I(2520):State: ASSOCIATING -> ASSOCIATED
wpa:I(2536):State: ASSOCIATED -> 4WAY_HANDSHAKE
wpa:I(2536):WPA: TK 5351711108057c6c03a6eaf93c87e486
wpa:I(2548):State: 4WAY_HANDSHAKE -> 4WAY_HANDSHAKE
hitf:I(2558):add CCMP
wpa:I(2558):State: 4WAY_HANDSHAKE -> GROUP_HANDSHAKE
hitf:I(2559):add CCMP
wpa:I(2559):State: GROUP_HANDSHAKE -> COMPLETED
lwip:I(2577):sta ip start
lwip:I(2578):[KW:]sta:DHCP_DISCOVER()
lwip:I(2587):[KW:]sta:DHCP_OFFER received in DHCP_STATE_SELECTING state
lwip:I(2587):[KW:]sta:DHCP_REQUEST(netif=0x280650a0) en   1
lwip:I(2596):[KW:]sta:DHCP_ACK received
wifid:I(2597):[KW:]me dhcp done vif:0
ap0:WDRV:D(2498):wdrv_rx_handle_msg id:1 cfm_sn:0 len:56
ap0:WDRV:D(2498):wdrv_rx_handle_event,584
ap0:WDRV:D(2498):wdrv_rx_handle_wifi_cntrl_event,472,1
ap0:WDRV:D(2499):wdrv_cntrlap1:lwip:I(2499):sta ip start
ap1:lwip:D(2500):configuring iface sta (with Static IP)
ap1:lwip:D(2500):using static ip...
ap1:lwip:D(2500):ip_addr: 192.168.43.3 
ap0:cli:W(2501):BK STA connected sample_project_ap
ap0:wifi_AP:I(2501):STA connected to sample_project_ap
ap0:cli:W(2501):BK STA got ip
ap0:wifi_AP:I(2502):/homeb/honghui.xiao/7258/avdk_smp/sample_project/wifi/sta_connect/ap/ap_main.c:38:wifi_netif_event_cb:PASS
ap0:wifi_AP:I(2502):STA got ip:192.168.43.3
```
## 更多WIFI开发指南请参考 [技术文档](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/developer-guide/wifi/index.html)。


## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

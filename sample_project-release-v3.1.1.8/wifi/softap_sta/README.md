# Wi-Fi AP示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述

本示例基于 Wi-Fi 组件，演示了 Wi-Fi AP/STA 共存的实现。它具体展示了如何：

1. 配置 Wi-Fi AP 参数，启动 AP 功能
2. 配置 Wi-Fi STA 参数，启动 STA 功能
3. 配置相关事件回调函数，处理 AP/STA 功能相关事件

以上功能主要是将AP示例和STA示例组合到一起，实现AP和STA共存的功能，具体实现可参考如下：
- [Wi-Fi AP示例](/wifi/softap/README.md)
- [Wi-Fi STA示例](/wifi/sta_connect/README.md)

## 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/wifi/softap_sta
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/wifi/softap_sta/build/bk7258/softap_sta/package/all-app.bin；
- 上电后，板子会启动“sample_project_ap”的热点，手机或者其他板子连接sample_project_ap热点，会有sta的连接事件打印；
- 确保“E0_xiao”热点已启动，且密码为“12345678”，板子上电运行后，板子会自动以sta连接“E0_xiao”的热点，连上后会有sta的连接事件打印；

以下是上电运行，上面操作后的日志打印：

```
ef:I(7):EasyFlash V4.1.0 is initialize success.
cal:W(18):temp in otp is:560
xtal_cali:58
cal:I(103):idx:40=40+(0),r:54,xtal:58,pwr_gain:a0ab7128
psram:W(113):psram type(16MB) not match CONFIG_PSRAM_CAPACITY 0X00800000
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
ap0:ef:I(273):EasyFlash V4.1.0 is initialize success.
ap0:D(273):ret_code444444444=0
ap0:D(361):driver_init end
ap0:init:D(361):reason - power on
ap0:init:D(362):regs - 0, 0, 0
ap0:init:D(362):armino rev: 
ap0:init:D(362):armino soc id:53434647_72360101
ap0:bk_init:D(362):armino app init: Nov 18 2025 19:19:19
ap0:bk_init:D(362):verify id: D13970

$ap0:bk_init:D(362):APP Version: unknown
ap0:OS:D(362):create event, tcb=280058e8, stack=[28005958-28006558:3072], prio=1, xCoreID=-1
ap0:lwip:D(363):init TCP/IP
ap0:OS:D(363):create tcp/ip, tcb=607213c8, stack=[60720ba0-607213a0:2048], prio=7, xCoreID=-1
ap1:WDRV:D(364):bk_wifi_init, 174
ap1:WDRV:D(364):wdrv_init
ap1:D(364):wdrv_ipc_init
ap1:WDRV:D(364):wdrv_ipc_env_init
ap1:WDRV:W(364):wdrv_tx_cmd_buffer_init, addr[0]=0x28011a18, pattern_addr=0x28011a14
ap1:WDRV:W(364):wdrv_tx_cmd_buffer_init, addr[1]=0x28011d38, pattern_addr=0x28011d34
ap1:WDRV:W(365):wdrv_tx_cmd_buffer_init, addr[2]=0x28012058, pattern_addr=0x28012054
ap1:OS:D(365):create wdrv_thread, tcb=60722488, stack=[60721460-60722460:4096], prio=7, xCoreID=-1
ap1:WDRV:D(365):wdrv_host_cmd_id : 514
ap1:WDRV:D(365):wdrv_tx_msg: msg_id:0x202 len:12 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(366):wdrv_rx_handle_msg id:8202 cfm_sn:0 len:6
ap0:WDRV:D(366):wdrv_tx_msg: msg_id:0x366 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(367):wdrv_rx_handle_msg id:8366 cfm_sn:0 len:4
ap0:WDRV:D(367):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(368):wdrv_rx_handle_msg id:8366 cfm_sn:0 len:4
ap0:WDRV:D(368):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(368):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(368):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap1:WDRV:D(369):wdrv_rx_handle_msg id:8366 cfm_sn:0 len:4
ap1:WDRV:D(369):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(370):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(370):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap1:WDRV:D(370):wdrv_rx_handle_ms
INSRT-S:os:I(627):psram:0x60700000,size:131072
g id:8366 cfm_sn:0 len:4
ap1:WDRV:D(371):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(371):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(371):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:D(372):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(372):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:D(372):wifi inited(1)
ap0:gpio:D(373):bk_gpio_driver_init:has inited 
wakeup
ap0:D(376):WIFI AT CMDS INIT OK
ap1:bluetoot:D(377):bk_bluetooth_init start, 0 
ap1:bt_ipc:D(377):open channel: 19 on CPU
ap1:bt_ipc:D(378):bt_ipc_init success
bt_ipc:W(488):bt_ipc_init bt ipc already initialised
wakeup
rwnx_cal_set_rfconfig(0x101) phy on; rf off
ap0:ble:D(386):bt_feature_adapter_init _is_gatt_discovery_auto 1
ap0:ble:D(386):bt_feature_adapter_init _ignore_smp_key_distr_all_zero 0
ap0:ble:D(387):bt_feature_adapter_init _ignore_smp_already_pair 0
ap0:OS:D(387):create ble_host, tcb=280227b8, stack=[28021b90-28022790:3072], prio=5, xCoreID=-1
ap0:ble:D(388):timer_init
ap0:ble:D(388):kernel_mem_init type 0 size 3396
ap0:ble:D(388):kernel_mem_init type 1 size 3084
ap0:ble:D(389):kernel_mem_init type 2 size 6156
ap0:ble:D(389):kernel_mem_init type 3 size 12
ap0:ble:D(389):timer_set_timeout, to 0 ms
ap0:ble:D(391):timer_set_timeout, to 0 ms
ap0:ble:D(431):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x1,status:0x0
ap0:ble:D(457):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x3,status:0x0
ap1:ble:D(464):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x1a,status:0x0
ap1:ble:D(471):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x1a,status:0x0
ap1:ble:D(471):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x28,status:0x0
ap1:ble:D(472):gapm_cmp_evt:BLE_STACK_OK
ap0:AT_BLE:D(474):BLE AT CMDS INIT OK
ap0:bluetoot:D(474):bk_bluetooth_init ok
ap0:WDRV:D(475):sta configuring
ap0:WDRV:D(475):sta config, ssid=E0_xiao password=12345678 security=12
ap0:WDRV:D(475):sta configured(5)
ap1:WDRV:D(476):wdrv_tx_msg: msg_id:0x310 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(476):wdrv_rx_handle_msg id:8310 cfm_sn:0 len:4
ap0:WDRV:D(477):wdrv_rx_handle_cmd_confirm,460,ID:0x310
ap1:WDRV:D(477):sta starting
ap1:WDRV:D(477):wdrv_tx_msg: msg_id:0x366 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(479):wdrv_rx_handle_msg id:8366 cfm_sn:0 len:4
ap0:WDRV:D(479):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap1:WDRV:D(479):wdrv mac addr:c8:47:8c:91:bd:d1
ap1:WDRV:D(479):sta connecting
ap1:WDRV:D(480):sta connected(f)
ap1:WDRV:D(480):wdrv_tx_msg: msg_id:0x311 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
wifid:I(601):mm_add_if_req: vif_idx=0, type=0, p2p=0, status=0
wpa:I(606):State: DISCONNECTED -> INACTIVE
wpa:I(607):State: INACTIVE -> DISCONNECTED
lwip:I(607):mac c8:47:8c:91:bd:d1
wpa:I(609):State: DISCONNECTED -> INACTIVE
wpa:I(610):Setting scan request: 0.000000 sec
wpa:I(610):State: INACTIVE -> SCANNING
ap0:WDRV:D(502):wdrv_rx_handle_msg id:8311 cfm_sn:0 len:4
ap0:WDRV:D(502):wdrv_rx_handle_cmd_confirm,460,ID:0x311
ap1:wifi_AP:I(502):ssid:E0_xiao password:12345678
ap1:wifi_AP:I(503):ssid:sample_project_ap  key:12345678
ap1:WDRV:D(503):ap configuring
wifid:I(612):[KW:]scanu_start_req:src_id=15,vif=0,dur=0,chan_cnt=1,ssid_len=7,bssid=0xffff-ffff-ffff
wifid:I(612):[KW:]scanu_start_req:ssid=E0_xiao
wifid:I(612):[KW:]scanu_start_req:chan_idx=11
ap1:WDRV:D(503):wdrv_tx_msg: msg_id:0x323 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(504):wdrv_rx_handle_msg id:8323 cfm_sn:0 len:4
ap0:WDRV:D(505):wdrv_rx_handle_cmd_confirm,460,ID:0x323
ap1:WDRV:D(505):wdrv_tx_msg: msg_id:0x367 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(506):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(507):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:W(507):the max conn num is zero, set it is default
ap0:WDRV:D(508):wdrv_tx_msg: msg_id:0x320 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
wifi:W(617):the max conn num is zero, set it is default
ap0:WDRV:D(509):wdrv_rx_handle_msg id:8320 cfm_sn:0 len:4
ap0:WDRV:D(509):wdrv_rx_handle_cmd_confirm,460,ID:0x320
ap1:WDRV:D(509):ap configured
ap1:WDRV:D(509):bk_wifi_ap_init, 326
ap1:WDRV:D(510):wifi already init, reinit anyway!
ap1:WDRV:D(510):wdrv_tx_msg: msg_id:0x367 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(511):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(511):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap1:WDRV:D(511):wdrv_tx_msg: msg_id:0x366 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(512):wdrv_rx_handle_msg id:8366 cfm_sn:0 len:4
ap0:WDRV:D(512):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap1:WDRV:D(512):wdrv_tx_msg: msg_id:0x367 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(513):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(513):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap1:WDRV:D(514):wdrv_tx_msg: msg_id:0x367 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(514):wdrv_rx_handle_msg id:8367 cfm_sn:0 len:4
ap0:WDRV:D(514):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:D(515):wifi inited(2f)
ap0:WDRV:D(515):wdrv_tx_msg: msg_id:0x321 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
wifid:I(626):mm_add_if_req: vif_idx=1, type=2, p2p=0, status=0
wifid:I(627):chan_ctxt_add: CTXT0,freq2462MHz,bw20MHz,pwr0dBm
wifid:I(627):chan_reg_fix:VIF1,CTXT0,type2,ctxt_s0,nb_vif0
hitf:I(632):start apm success, vif1, channel0, bcmc3
wifid:I(635):[KW:]mm_set_vif_state,vif=1,vif_type=2,is_active=1, aid=0x0,rssi=0
wifid:I(673):chan_unregister_once_req: VIF0,type=11
wifid:I(674):[KW:]scanu_confirm:status=0,req_type=0,upload_cnt=1,recv_cnt=2,time=61742us,result=1,rfcli=0
hitf:I(1877):add CCMP
lwip:I(1878):mac c8:47:8c:91:bd:d0
ap0:WDRV:D(1773):wdrv_rx_handle_msg id:8321 cfm_sn:0 len:4
ap0:WDRV:D(1773):wdrv_rx_handle_cmd_confirm,460,ID:0x321
wpa:I(1882):Scan completed in 1.265000 seconds
hitf:I(1883):get scan result:1
ap1:WDRV:D(1773):ap started
ap1:lwip:D(1773):configuring iface ap (with Static IP)
ap1:lwip:D(1775):lwip_socket, main create socked:1
ap0:lwip:D(1777):lwip_socket, main create socked:2
wpa:I(1886):State: SCANNING -> ASSOCIATING
wifid:I(1887):[KW:]conn vif0-0,auth_type:3,bssid:4204-bc1a-e03d,ssid:E0_xiao,is encryp:8.
ap0:lwip:D(1778):lwip_socket, main create socked:3
ap0:OS:D(1779):create dhcp-server, tcb=607240c8, stack=[60723aa0-607240a0:1536], prio=2, xCoreID=-1
wifid:I(1888):chan_reg_fix:VIF0,CTXT0,type3,ctxt_s4,nb_vif1
wifid:I(1889):mm_sta_add:vif 0,sta 0,status 0
wifid:I(2390):[KW:]assoc_req_send:is ht, seq_num:29
wifid:I(2400):[KW:]assoc_rsp:status0,tx_s0x80800000
wifid:I(2400):[KW:]mm_set_vif_state,vif=0,vif_type=0,is_active=1, aid=0x10,rssi=-29
hitf:W(2401):ioctl fail, cmd=30 ret=-6409
wpa:I(2402):State: ASSOCIATING -> ASSOCIATED
wpa:I(2412):State: ASSOCIATED -> 4WAY_HANDSHAKE
wpa:I(2413):WPA: TK 1974fdd56bba692fd7ea7926bc253dec
wpa:I(2419):State: 4WAY_HANDSHAKE -> 4WAY_HANDSHAKE
hitf:I(2420):add CCMP
wpa:I(2420):State: 4WAY_HANDSHAKE -> GROUP_HANDSHAKE
hitf:I(2420):add CCMP
wpa:I(2421):State: GROUP_HANDSHAKE -> COMPLETED
lwip:I(2441):sta ip start
lwip:I(2441):[KW:]sta:DHCP_DISCOVER()
lwip:I(2453):[KW:]sta:DHCP_OFFER received in DHCP_STATE_SELECTING state
lwip:I(2453):[KW:]sta:DHCP_REQUEST(netif=0x280650a0) en   1
lwip:I(2457):[KW:]sta:DHCP_ACK received
wifid:I(2458):[KW:]me dhcp done vif:0
ap0:WDRV:D(2351):wdrv_rx_handle_msg id:1 cfm_sn:0 len:56
ap0:WDRV:D(2351):wdrv_rx_handle_event,584
ap0:WDRV:D(2352):wdrv_rx_handle_wifi_cntrl_event,472,1
ap0:WDRV:D(2352):wdrv_cntrlap1:lwip:I(2353):sta ip start
ap1:lwip:D(2353):configuring iface sta (with Static IP)
ap1:lwip:D(2353):using static ip...
ap1:lwip:D(2353):ip_addr: 192.168.50.199 
ap1:cli:W(2354):BK STA connected E0_xiao
ap1:wifi_AP:I(2354):STA connected to E0_xiao
ap1:cli:W(2354):BK STA got ip
ap1:wifi_AP:I(2354):/homeb/honghui.xiao/7258/avdk_smp/sample_project/wifi/softap_sta/ap/ap_main.c:67:wifi_netif_event_cb:PASS
ap1:wifi_AP:I(2355):STA got ip:192.168.50.199
wifid:I(17579):mm_sta_add:vif 1,sta 1,status 0
hitf:I(17580):add sta, sta=1, pm state=0
hitf:I(17586):add CCMP
wpa:I(17601):WPA: TK 231d9f667e2d957df828f61be36b7aa4
wpa:I(17601):WPA: GTK 9d8640819b89a34e478225b4f2b5a5e3
hitf:I(17618):add CCMP
ap0:WDRV:D(17510):wdrv_rx_handle_msg id:4 cfm_sn:0 len:6
ap0:WDRV:D(17511):wdrv_rx_handle_event,584
ap0:WDRV:D(17511):wdrv_rx_handle_wifi_cntrl_event,472,4
ap0:WDRV:D(17511):AP-INDICATE: e2:29:7c:3b:4e:e4 connected
ap1:cli:W(17512):e2:29:7c:3b:4e:e4 connected to BK AP
ap1:wifi_AP:I(17512):e2:29:7c:3b:4e:e4 connected to AP
ap1:wifi_AP:I(17512):/homeb/honghui.xiao/7258/avdk_smp/sample_project/wifi/softap_sta/ap/ap_main.c:94:wifi_event_cb:PASS
lwip:I(17779):ap:DHCP should send offer
lwip:I(17798):ap:DHCP should send ack
wifid:I(17804):DHCP RELET START
ap0:WDRV:D(18512):wdrv_tx_msg: msg_id:0x327 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(18512):wdrv_rx_handle_msg id:8327 cfm_sn:0 len:4
ap0:WDRV:D(18512):wdrv_rx_handle_cmd_confirm,460,ID:0x327
ap0:WDRV:D(18513):wdrv_tx_msg: msg_id:0x328 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(18513):wdrv_rx_handle_msg id:8328 cfm_sn:0 len:4
ap0:WDRV:D(18513):wdrv_rx_handle_cmd_confirm,460,ID:0x328
ap0:wifi_AP:I(18513):sta_num: 1
ap0:wifi_AP:I(18513):[01]Mac addr: e2:29:7c:3b:4e:e4, ap0:wifi_AP:I(18513):ip: 192.168.188.100, rssi -32
```
## 更多WIFI开发指南请参考 [技术文档](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/developer-guide/wifi/index.html)。


## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

# Wi-Fi 扫描示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述

本示例基于 Wi-Fi 组件 扫描示例，演示了 Wi-Fi 扫描的实现。它具体展示了如何：

1. 配置 Wi-Fi 扫描参数，扫描所有信道的 Wi-Fi 热点；
2. 配置 Wi-Fi 扫描参数，扫描指定 SSID 的 Wi-Fi 热点（ap_main.c中CONFIG_EXAMPLE_SCAN_ALL_SSID设置为0，CONFIG_EXAMPLE_WIFI_SACN_SSID设置为指定的SSID）；

## 相关代码

``` C
    #include <modules/wifi.h>
    #include <components/netif.h>
    #include <components/event.h>
    //设置扫描参数并开始扫描
    bk_err_t bk_wifi_scan_start(const wifi_scan_config_t *scan_config);

    //注册WiFi扫描完成事件回调函数
    bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_SCAN_DONE,wifi_scan_event_cb, NULL);
```

## 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/wifi/scan
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/wifi/scan/build/bk7258/scan/package/all-app.bin；
- 上电运行后，板子会自动扫描到周围的 wifi 热点，然后打印出扫描到的热点信息;

以下是上电运行后的串口log 输出：

```
ef:I(7):EasyFlash V4.1.0 is initialize success.
cal:W(18):temp in otp is:560
xtal_cali:58
cal:I(97):idx:40=40+(0),r:54,xtal:58,pwr_gain:a0ab7128
psram:W(107):psram type(16MB) not match CONFIG_PSRAM_CAPACITY 0X00800000
ap1:ef:I(0):ENV start address is 0x007FC000, size is 8192 bytes.
ap0:ef:I(272):EasyFlash V4.1.0 is initialize success.
ap0:WDRV:W(362):wdrv_tx_cmd_buffer_init, addr[0]=0x28011a18, pattern_addr=0x28011a14
ap0:WDRV:W(363):wdrv_tx_cmd_buffer_init, addr[1]=0x28011d38, pattern_addr=0x28011d34
ap0:WDRV:W(363):wdrv_tx_cmd_buffer_init, addr[2]=0x28012058, pattern_addr=0x28012054
wakeup
bt_ipc:W(474):bt_ipc_init bt ipc already initialised

$rwnx_cal_set_rfconfig(0x101) phy on; rf off
wakeup
wifid:I(574):mm_add_if_req: vif_idx=0, type=0, p2p=0, status=0
wpa:I(580):State: DISCONNECTED -> INACTIVE
wpa:I(580):State: INACTIVE -> DISCONNECTED
lwip:I(581):mac c8:47:8c:91:bd:d1
wifi:I(581):scan all APs
wpa:I(582):Setting scan request: 0.100000 sec
ap0:wifi_sca:I(477):start wifi scan
xhhprint start wifi scan
wpa:I(682):State: DISCONNECTED -> SCANNING
wifid:I(683):[KW:]scanu_start_req:src_id=15,vif=0,dur=0,chan_cnt=14,ssid_len=0,bssid=0xffff-ffff-ffff
os:I(691):psram:0x60700000,size:131072
wifid:I(2419):[KW:]scanu_confirm:status=0,req_type=0,upload_cnt=124,recv_cnt=124,time=1736153us,result=36,rfcli=0
wpa:I(2420):Scan completed in 1.735000 seconds
hitf:I(2420):get scan result:36
wpa:I(2427):State: SCANNING -> DISCONNECTED
ap0:WDRV:I(2323):BK_EVT_SCAN_WIFI_IND
ap0:cli:W(2324):rx event <1 0>
scan found 36 AP
              SSID                     BSSID         RSSI chan security
-------------------------------- -----------------   ---- ---- ---------
bktest                           50:0f:f5:51:3d:1b    -37 13 WPA2-AES
huawei_test                      02:27:80:8d:4a:da    -39  1 WPA2-AES
SZ-Guest                         4c:ef:56:08:74:be    -42  1 WPA2-AES
BK-SZ                            4c:ef:56:10:74:be    -42  1 WPA2-AES
Bekencorp-WIFI                   4c:ef:56:18:74:be    -42  1 WPA2-AES
BK-SZ-2.4G                       4c:ef:56:00:74:be    -42  1 WPA2-AES
SZ-Guest                         a8:0c:ca:0a:9a:97    -49  6 WPA2-AES
H3C_1CC7FA                       48:73:97:1c:c7:fc    -49  6 WPA2-AES
BK-SZ                            a8:0c:ca:12:9a:97    -49  6 WPA2-AES
BK-SZ-2.4G                       a8:0c:ca:02:9a:97    -49  6 WPA2-AES
Bekencorp-WIFI                   a8:0c:ca:1a:9a:97    -50  6 WPA2-AES
TP_LINK_6D51                     78:60:5b:43:6d:51    -51  4 WPA2-AES
sample_project_wifi              04:95:e6:ab:53:b4    -54  6 WPA2-AES
test123                          c8:5b:a0:ee:87:47    -54 11 WPA2-AES
H3C_NX15                         38:ca:81:00:4b:75    -55  1 WPA2-AES
BK-SZ-2.4G                       a8:0c:ca:02:9a:9a    -58  8 WPA2-AES
Bekencorp-WIFI                   a8:0c:ca:1a:9a:9a    -59  8 WPA2-AES
SZ-Guest                         a8:0c:ca:0a:9a:9a    -59  8 WPA2-AES
BK-SZ                            a8:0c:ca:12:9a:9a    -59  8 WPA2-AES
Govee-31F-2.4g                   54:ec:2f:18:63:08    -60  1 WPA2-AES
iHoment-HK-31F-2.4g              54:ec:2f:58:63:08    -63  1 WPA2-AES
BK-SZ-2.4G                       4c:ef:56:00:74:a3    -66  3 WPA2-AES
BK-SZ                            a8:0c:ca:12:99:d5    -68  8 WPA2-AES
iHoment_HK-2.4                   70:ca:97:57:ad:a8    -69 11 WPA2-AES
Govee-2.4                        70:ca:97:17:ad:a8    -70 11 WPA2-AES
BK-SZ                            70:3a:73:13:24:ed    -70 13 WPA2-AES
BK-SZ-2.4G                       70:3a:73:03:24:ed    -71 13 WPA2-AES
Govee-2.4                        8c:7a:15:13:13:18    -71 11 WPA2-AES
iHoment_HK-2.4                   8c:7a:15:53:13:18    -75 11 WPA2-AES
iHoment_HK-2.4                   8c:7a:15:4b:04:98    -78  1 WPA2-AES
SD-WLAN_UK-XQD                   78:60:5b:a0:d2:ec    -80 11 WPA2-AES
iHoment-HK-31F-2.4g              34:20:e3:54:b2:a8    -80 11 WPA2-AES
A_T2                             a4:93:3f:eb:49:f8    -52 11 NONE
ap0:wifi_sca:I(2707):/homeb/honghui.xiao/7258/avdk_smp/sample_project/wifi/scan/ap/ap_main.c:39:wifi_scan_event_cb:PASS
```
## 更多WIFI开发指南请参考 [技术文档](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/developer-guide/wifi/index.html)。


## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

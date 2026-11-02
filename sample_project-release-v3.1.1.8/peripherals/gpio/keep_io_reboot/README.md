# GPIO 软重启 io 保持示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述

GPIO（通用输入输出）模块是芯片上用于外部设备信号通信的核心模块。BK7258芯片有56个通用输入输出管脚（GPIO Pin），每个管脚都可用作一个通用IO，或连接一个内部的外设信号。

芯片刚开始启动时，所有GPIO都是高阻态，程序启动后，会依据GPIO table表将gpio初始化配置为对应的属性。
1.  创建 usr_gpio_cfg.h 文件
    - 对于 ap 侧：
        1: 在工程目录 ap/config/bk7258_ap 目录下创建 usr_gpio_cfg.h 文件
        2: 从标准 sdk 的 projects/app 工程的 ap/config/bk7258_ap 目录下复制 usr_gpio_cfg.h 文件到工程的 ap/config/bk7258_ap 目录下
    - 对于 cp 侧：
        1: 在工程 cp/config/bk7258 目录下创建 usr_gpio_cfg.h 文件
        2: 从标准 sdk 的 projects/app 工程的 cp/config/bk7258 目录下复制 usr_gpio_cfg.h 文件到工程的 cp/config/bk7258 目录下
2. 工程 config 配置打开 CONFIG_USR_GPIO_CFG_EN 宏，使能用户自定义 GPIO_DEFAULT_DEV_CONFIG 表。
3. 对于需要在软重启过程中保持状态的 GPIO 管脚，将在 GPIO_DEFAULT_DEV_CONFIG 表中配置 GPIO_INIT 字段为 GPIO_INIT_DISABLE。

本示例基于 GPIO 驱动，演示了 GPIO 如何在软件重启过程中保持io状态的实现。它具体展示了如何：
1. 配置 GPIO 为输出模式，输出高电平；
2. 调用 bk_reboot 函数重启芯片；
3. 重启过程中 GPIO_25 的输出电平是否保持为高电平；

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
|CONFIG_GPIO_DEFAULT_SET_SUPPORT| GPIO 默认设置使能配置，配置为y                                         |
|CONFIG_USR_GPIO_CFG_EN| 用户自定义 GPIO_DEFAULT_DEV_CONFIG 表使能配置，配置为y                                         |
## 相关代码

``` C
    #define GPIO_DEFAULT_DEV_CONFIG  \
    {\
    ...
        {GPIO_25, GPIO_SECOND_FUNC_ENABLE, GPIO_DEV_LCD_G6, GPIO_IO_DISABLE, GPIO_PULL_DISABLE, GPIO_INT_DISABLE, GPIO_INT_TYPE_LOW_LEVEL, GPIO_LOW_POWER_DISCARD_IO_STATUS, GPIO_DRIVER_CAPACITY_1,GPIO_INIT_DISABLE,GPIO_TIME_SHARING_MULTIPLEX_ENABLE},\
    ...
    }\
    #include "driver/gpio.h"
    #include "gpio_driver.h"

    // 对比 GPIO_24 和 GPIO_25 的输出电平
    gpio_output_config(GPIO_24);
    gpio_output_config(GPIO_25);

    bk_gpio_set_output_high(GPIO_24);
    bk_gpio_set_output_high(GPIO_25);

    rtos_delay_milliseconds(2000);
    // 进入软重启
    bk_reboot();
     
```

## 编译


运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/peripherals/gpio/keep_io_reboot
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/peripherals/gpio/keep_io_reboot/build/bk7258/keep_io_reboot/package/all-app.bin；
- 上电运行后，板子运行程序,会先输出高电平到 GPIO_24 和 GPIO_25，等待两秒后，调用 bk_reboot 函数重启芯片，重启过程中，GPIO_25 的输出电平保持为高电平；

以下是上电运行后的串口log 输出：

```
[15:17:13.426]收←◆ef:I(3):ENV start address is 0x007FA000, size is 8192 bytes.
ef:I(6):EasyFlash V4.1.0 is initialize success.
cal:W(16):temp in otp is:565

[15:17:13.538]收←◆flash txpwr table:0xf
xtal_cali:56
cal:I(108):idx:40=40+(0),r:54,xtal:56,pwr_gain:a8ab7128
psram:W(118):psram type(16MB) not match CONFIG_PSRAM_CAPACITY 0X00800000

[15:17:13.771]收←◆ap1:bk_ipc:D(0):bk_ipc_core_init
ap1:bk_ipc:D(0):open channel: 23 on CPU: 1
ap1:bk_ipc:D(0):channel: gpio_ipc
ap1:bk_ipc:D(0):channel: bk_api_rpc
ap1:OS:D(0):create ipc thread, tcb=60720b08, stack=[607202e0-60720ae0:2048], prio=5, xCoreID=-1
ap1:bk_ipc:D(0):bk_ipc_core_init success
ap1:os:D(0):
ap1:os:D(0):mem_type start    end      size    
ap1:os:D(0):-------- -------- -------- --------
ap1:os:D(0):itcm     0x20     0x80     96      
ap1:os:D(0):dtcm     0x20000004 0x20000004 0       
ap1:os:D(0):ram      0x28010480 0x28061ff8 334712  
ap1:os:D(0):non_heap 0x28010480 0x2801f340 61120   
ap1:os:D(0):iram     0x8010000 0x8010480 1152    
ap1:os:D(0):data     0x28010480 0x28011b2c 5804    
ap1:os:D(0):bss      0x28012920 0x2801f33c 51740   
ap1:os:D(0):heap     0x2801f340 0x28061ff8 273592  
ap1:os:D(0):psram    0x60720000 0x607c0000 655360  
ap1:ef:I(0):ENV start address is 0x007FC000, size is 8192 bytes.
ap0:ef:I(299):EasyFlash V4.1.0 is initialize success.
ap0:D(300):ret_code444444444=0

[15:17:13.967]收←◆ap0:D(421):driver_init end
ap0:init:D(421):reason - power on
ap0:init:D(421):regs - 0, 0, 0
ap0:init:D(421):armino rev: 
ap0:init:D(421):armino soc id:53434647_72360101
ap0:bk_init:D(422):armino app init: Dec 22 2025 15:13:19
ap0:bk_init:D(422):verify id: D14731

$ap0:bk_init:D(422):APP Version: unknown
ap0:OS:D(422):create event, tcb=280058e8, stack=[28005958-28006558:3072], prio=1, xCoreID=-1
ap0:lwip:D(422):init TCP/IP
ap0:OS:D(423):create tcp/ip, tcb=607213c8, stack=[60720ba0-607213a0:2048], prio=7, xCoreID=-1
ap1:WDRV:D(423):bk_wifi_init, 174
ap1:WDRV:D(423):wdrv_init
ap1:D(423):wdrv_ipc_init
ap1:WDRV:D(424):wdrv_ipc_env_init
ap1:WDRV:W(424):wdrv_tx_cmd_buffer_init, addr[0]=0x28011b30, pattern_addr=0x28011b2c
ap1:WDRV:W(424):wdrv_tx_cmd_buffer_init, addr[1]=0x28011e50, pattern_addr=0x28011e4c
ap1:WDRV:W(424):wdrv_tx_cmd_buffer_init, addr[2]=0x28012170, pattern_addr=0x2801216c
ap1:OS:D(424):create wdrv_thread, tcb=60722488, stack=[60721460-60722460:4096], prio=7, xCoreID=-1
ap1:WDRV:D(424):wdrv_host_cmd_id : 514
ap1:WDRV:D(425):wdrv_tx_msg: msg_id:0x202 len:12 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(425):wdrv_tx_msg: msg_id:0x366 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(426):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(427):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(427):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:D(428):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(429):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:D(429):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(430):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:D(430):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:D(431):wifi inited(1)
ap0:gpio:D(431):bk_gpio_driver_init:has inited 
wakeup
ap0:D(435):WIFI AT CMDS INIT OK
ap1:bluetoot:D(436):bk_bluetooth_init start, 0 
ap1:bt_ipc:D(436):open channel: 19 on CPU
ap1:bt_ipc:D(436):bt_ipc_init success
wakeup
bt_ipc:W(551):bt_ipc_init bt ipc already initialised
rwnx_cal_set_rfconfig(0x101) phy on; rf off
ap0:ble:D(444):bt_feature_adapter_init _is_gatt_discovery_auto 1
ap0:ble:D(444):bt_feature_adapter_init _ignore_smp_key_distr_all_zero 0
ap0:ble:D(445):bt_feature_adapter_init _ignore_smp_already_pair 0
ap0:OS:D(445):create ble_host, tcb=28024340, stack=[28023718-28024318:3072], prio=5, xCoreID=-1
ap0:ble:D(446):timer_init
ap0:ble:D(446):kernel_mem_init type 0 size 3396
ap0:ble:D(446):kernel_mem_init type 1 size 3084
ap0:ble:D(446):kernel_mem_init type 2 size 6156
ap0:ble:D(447):kernel_mem_init type 3 size 12
ap0:ble:D(447):timer_set_timeout, to 0 ms
ap0:ble:D(448):timer_set_timeout, to 0 ms
ap1:ble:D(488):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x1,status:0x0
ap0:ble:D(514):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x3,status:0x0
ap0:ble:D(522):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x1a,status:0x0
ap0:ble:D(529):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x1a,status:0x0
ap0:ble:D(529):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x28,status:0x0
ap0:ble:D(529):gapm_cmp_evt:BLE_STACK_OK
ap0:AT_BLE:D(531):BLE AT CMDS INIT OK
ap0:bluetoot:D(531):bk_bluetooth_init ok
ap0:hal:W(532):gpio: 24 is used.Please confirm unmap isn't impact is working module.!

[15:17:16.119]收←◆set黣f:I(3):ENV start address is 0x007FA000, size is 8192 bytes.
ef:I(6):EasyFlash V4.1.0 is initialize success.
cal:W(20):temp in otp is:565

[15:17:16.228]收←◆flash txpwr table:0xf
xtal_cali:56
cal:I(108):idx:40=40+(0),r:54,xtal:56,pwr_gain:a8ab7128
psram:W(118):psram type(16MB) not match CONFIG_PSRAM_CAPACITY 0X00800000

[15:17:16.462]收←◆ap1:bk_ipc:D(0):bk_ipc_core_init
ap1:bk_ipc:D(0):open channel: 23 on CPU: 1
ap1:bk_ipc:D(0):channel: gpio_ipc
ap1:bk_ipc:D(0):channel: bk_api_rpc
ap1:OS:D(0):create ipc thread, tcb=60720b08, stack=[607202e0-60720ae0:2048], prio=5, xCoreID=-1
ap1:bk_ipc:D(0):bk_ipc_core_init success
ap1:os:D(0):
ap1:os:D(0):mem_type start    end      size    
ap1:os:D(0):-------- -------- -------- --------
ap1:os:D(0):itcm     0x20     0x80     96      
ap1:os:D(0):dtcm     0x20000004 0x20000004 0       
ap1:os:D(0):ram      0x28010480 0x28061ff8 334712  
ap1:os:D(0):non_heap 0x28010480 0x2801f340 61120   
ap1:os:D(0):iram     0x8010000 0x8010480 1152    
ap1:os:D(0):data     0x28010480 0x28011b2c 5804    
ap1:os:D(0):bss      0x28012920 0x2801f33c 51740   
ap1:os:D(0):heap     0x2801f340 0x28061ff8 273592  
ap1:os:D(0):psram    0x60720000 0x607c0000 655360  
ap0:ef:I(226):ENV start address is 0x007FC000, size is 8192 bytes.
ap0:ef:I(300):EasyFlash V4.1.0 is initialize success.
ap0:D(300):ret_code444444444=0

[15:17:16.656]收←◆ap0:D(419):driver_init end
ap0:init:D(419):reason - software reboot        // 软件重启
ap0:init:D(419):regs - 1, 0, 0
ap0:init:D(420):armino rev: 
ap0:init:D(420):armino soc id:53434647_72360101
ap0:bk_init:D(420):armino app init: Dec 22 2025 15:13:19
ap0:bk_init:D(420):verify id: D14731

```
## 更多GPIO开发指南请参考 [技术文档](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/developer-guide/peripheral/bk_gpio.html)。


## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

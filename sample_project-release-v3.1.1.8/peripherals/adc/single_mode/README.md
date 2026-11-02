# ADC SINGLE MODE 示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述
ADC（模数转换器）模块将模拟信号转换为数字信号，支持：
1. 12位分辨率，采样率可编程：12.5 kHz – 812.5 kHz  
2. 多达11路外部模拟输入通道（ADC1/2/3/4/5/6/10/12/13/14/15）  
3. 5路内部专用通道：  
   - VBAT 监测（VBAT/2、/3、/5、/7，对应 ADC0）  
   - 片内温度传感器（TEMP，对应 ADC7）  
   - TSSIO（对应 ADC8）  
   - Touch OUT_TD（对应 ADC9）  
   - 内部调试通道（对应 ADC11）  
4. 三种转换模式：单次（One-shot）、软件控制、连续采样

本示例基于 ADC 组件，演示了 ADC 单通道单次采样模式。它具体展示了如何：
1. 配置 ADC 为单通道单次模式，选择采样通道，设置采样周期；
2. 启动 ADC 采样并等待转换完成；
3. 读取转换结果并打印电压值；

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

**ADC 配置**
| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_SARADC    | SAR ADC示例使能配置，配置为y         |


## 相关代码

``` C

    // Acquire ADC resource
    ret = bk_adc_acquire();
    if (ret != BK_OK) 
    {
        BK_LOGI(TAG, "ADC acquire failed: %d\r\n", ret);
        bk_adc_release();
        return cali_value;
    }

    // Configure ADC parameters
    adc_config.chan = adc_chan;
    adc_config.adc_mode = ADC_CONTINUOUS_MODE;          //Continuous mode requires manual ADC stop for single-step read
    adc_config.src_clk = ADC_SCLK_XTAL_26M;
    adc_config.clk = 0x30A0C5;
    adc_config.saturate_mode = ADC_SATURATE_MODE_3;
    adc_config.steady_ctrl = 7;
    adc_config.adc_filter = 0;
    adc_config.sample_rate = 0;
    adc_config.is_open = 0;
    adc_config.vol_div = ADC_VOL_DIV_NONE;//adc internal voltage division ratio

    // Initialize ADC channel
    ret = bk_adc_init(adc_chan);
    if (ret != BK_OK) 
    {
        BK_LOGI(TAG, "ADC init failed: %d\r\n", ret);
        bk_adc_deinit(adc_chan);
        bk_adc_release();
        return cali_value;
    }

    ret = bk_adc_set_config(&adc_config);
    if (ret != BK_OK) 
    {
        BK_LOGI(TAG, "ADC set config failed: %d\r\n", ret);
        bk_adc_deinit(adc_chan);
        bk_adc_release();
        return cali_value;
    }

    bk_adc_enable_bypass_clalibration();
    if (ret != BK_OK) 
    {
        BK_LOGI(TAG, "ADC bypass cali failed: %d\r\n", ret);
        bk_adc_deinit(adc_chan);
        bk_adc_release();
        return cali_value;
    }

    // Step 4: Start ADC conversion
    ret = bk_adc_start();
    if (ret != BK_OK) 
    {
        BK_LOGI(TAG, "ADC start failed: %d\r\n", ret);
        bk_adc_stop();
        bk_adc_deinit(adc_chan);
        bk_adc_release();
        return cali_value;
    }

    // Step 5: Read ADC data
    ret = bk_adc_read(&adc_data, SARADC_EXAMPLE_TIMEOUT);
    if (ret != BK_OK) 
    {
        BK_LOGI(TAG, "ADC read raw failed: %d\r\n", ret);
        bk_adc_stop();
        bk_adc_deinit(adc_chan);
        bk_adc_release();
        return cali_value;
    }

    cali_value = bk_adc_data_calculate(adc_data, adc_chan);
    BK_LOGE(TAG, "ADC data = %d, volt = %d(mv)\r\n", adc_data, cali_value);
     
```

## 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //如果Makefile中未指定SDK_DIR，或者路径错误，设置smp sdk路径到环境变量
cd sample_project/peripherals/adc/single_mode
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/peripherals/adc/single_mode/build/bk7258/single_mode/package/all-app.bin；
- 上电运行后，板子运行程序，每一秒钟定时检测GPIO_12的电平。

以下是上电运行后的串口log 输出：

```

[17:48:29.182]收←◆萣k_ipc:D(2):bk_ipc_core_init
bk_ipc:D(3):open channel: 71 on CPU: 0
bk_ipc:D(3):channel: gpio_ipc
bk_ipc:D(3):channel: bk_api_rpc
os:D(3):create ipc thread, tcb=280861e8, stack=[280859c0-280861c0:2048], prio=5
bk_ipc:D(3):bk_ipc_core_init success
os:D(3):
os:D(3):mem_type start    end      size    
os:D(4):-------- -------- -------- --------
os:D(4):itcm     0x20     0x3eb0   16016   
os:D(4):dtcm     0x20000000 0x20003db8 15800   
os:D(4):ram      0x28064d80 0x2809e6f8 235896  
os:D(4):non_heap 0x28064d80 0x28082420 120480  
os:D(4):iram     0x8064000 0x8064d80 3456    
os:D(4):data     0x28064d80 0x28065fb8 4664    
os:D(4):bss      0x28065fe0 0x2808241c 115772  
os:D(5):heap     0x28082420 0x2809e6f8 115416  
os:D(5):psram    0x60700000 0x60720000 131072  
os:D(5):create flash_svr, tcb=280868a0, stack=[28086278-28086878:1536], prio=3
flash:D(5):id=0xc86517
ef:I(6):ENV start address is 0x007FA000, size is 8192 bytes.
ef:I(8):EasyFlash V4.1.0 is initialize success.
os:D(9):create saradc_svr, tcb=28086fd0, stack=[280869a8-28086fa8:1536], prio=3
D(9):ate enabled is 0
D(9):driver_init end
sensor:D(9):saradc low value:[9b1]
sensor:D(9):saradc high value:[136a]
sensor:D(9):sdmadc low value:[6e89]
sensor:D(9):sdmadc high value:[8951]
os:D(10):create phy_svr, tcb=28087848, stack=[28087420-28087820:1024], prio=3
init:D(10):reason - power on
init:D(10):regs - 0, 3, 0
init:D(10):armino rev: 
init:D(10):armino soc id:53434647_72360101
bk_init:D(10):armino app init: Jan  7 2026 15:17:09
bk_init:D(10):verify id: D14731
bk_init:D(10):APP Version: unknown
nv:D(10):vnd_cal_version = 24-04-10 00:00:00
os:D(11):create event, tcb=280881b8, stack=[28087990-28088190:2048], prio=1
lwip:D(11):init TCP/IP
os:D(11):create tcp/ip, tcb=28088e88, stack=[28088660-28088e60:2048], prio=7
wifi:D(12):wifi initing
os:D(12):create syswq, tcb=280897f0, stack=[280893c8-280897c8:1024], prio=6
cal:D(17):get rfcali_mode:1
cal:D(20):cali bias(e)
tempd:D(21):get temperature 563
cal:D(21):temp_code=22,dmodldosel=4,Dgmdc=7
cal:D(30):retry_count=4,dlypc_i=2,stateI=[0,60,31,0],dlypc_q=2,stateQ=[0,59,30,0]
cal:D(101):cali bias(e)
cal:D(101):calibration_main(7faf19e) over
cal:D(101):rf cali flag in flash: valid
flash txpwr table:0xf
tempd:D(102):init temperature 547
os:D(102):create tempd, tcb=2808b668, stack=[2808b040-2808b640:1536], prio=5
xtal_cali:53
cal:D(102):--init_xtal = 53
cal:D(103):gtx_dcorPA=0xd->0xd
cal:D(103):-----pwr_gain:40, rate=54, g_idx:40, shift_b:0, shift_g:0
cal:I(103):idx:40=40+(0),r:54,xtal:53,pwr_gain:98ab7128
os:D(103):create kmsgbk, tcb=2808ca90, stack=[2808ba68-2808ca68:4096], prio=6
os:D(103):create core_thread, tcb=2808d9c0, stack=[2808d198-2808d998:2048], prio=7
os:D(104):create wpas_thread, tcb=2808f2f0, stack=[2808dec8-2808f2c8:5120], prio=4
os:D(104):create cif_thread, tcb=28090960, stack=[2808f938-28090938:4096], prio=7
CIF:D(104):cif_init
wifi:D(104):wifi inited(1) ret(0)
AT_WLAN:D(105):WIFI AT CMDS INIT OK
tempd:D(105):xtal inital:547, 70, 53
ble:D(106):bt_feature_adapter_init _is_gatt_discovery_auto 1
ble:D(106):bt_feature_adapter_init _ignore_smp_key_distr_all_zero 0
ble:D(106):bt_feature_adapter_init _ignore_smp_already_pair 0
os:D(106):create bt_ipc_thd, tcb=28091978, stack=[28091150-28091950:2048], prio=5
bt_ipc:D(106):open channel: 67 on CPU
bt_ipc:D(106):bt_ipc_init success
bluetoot:D(106):bk_bluetooth_init init ipc ok
os:D(106):create rosc_calib, tcb=28092aa8, stack=[28091a80-28092a80:4096], prio=1
D(107):user app entry(0x20b312d)
D(107):boot_cp1 9 0 0x0 [0][0x28069044]E_1
D(107):boot_cp1 9 0 0x0 [0]E_2
D(107):reset_cpu1_core at: 02150000, start=1
D(110):====mailbox reset!!4c, 4c
psram:D(112):Starting PSRAM init...
psram:D(114):bk_psram_init, chip_id:0
cal:D(115):temp_code:21 - adc_code:567 - adc_trend:[13]:547->[12]:570
cal:D(115):-----pwr_gain:40, rate=54, g_idx:40, shift_b:-1, shift_g:-1
cal:I(115):idx:39=40+(-1),r:54,xtal:54,pwr_gain:98ab7120
psram:D(115):bk_psram_init, 8d08-0
psram:W(115):psram type(16MB) not match CONFIG_PSRAM_CAPACITY 0X00800000
psram:D(115):PSRAM init success
bk_ipc:D(337):recv cpu1 state changed: 1
ap1:bk_ipc:D(0):bk_ipc_core_init
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
ap1:os:D(0):non_heap 0x28010480 0x2801f258 60888   
ap1:os:D(0):iram     0x8010000 0x8010480 1152    
ap1:os:D(0):data     0x28010480 0x28011a7c 5628    
ap1:os:D(0):bss      0x28012880 0x2801f258 51672   
ap1:os:D(0):heap     0x2801f258 0x28061ff8 273824  
ap1:os:D(0):psram    0x60720000 0x607c0000 655360  
ap1:ef:I(0):ENV start address is 0x007FC000, size is 8192 bytes.
ap0:ef:I(279):EasyFlash V4.1.0 is initialize success.
ap0:D(279):ret_code444444444=0
ap0:D(339):driver_init end
ap0:init:D(339):reason - power on
ap0:init:D(339):regs - 0, 0, 0
ap0:init:D(339):armino rev: 
ap0:init:D(339):armino soc id:53434647_72360101
ap0:bk_init:D(339):armino app init: Jan  7 2026 15:16:51
ap0:bk_init:D(339):verify id: D14731

$ap0:bk_init:D(339):APP Version: unknown
ap0:OS:D(339):create event, tcb=280058e8, stack=[28005958-28006558:3072], prio=1, xCoreID=-1
ap0:lwip:D(340):init TCP/IP
ap0:OS:D(340):create tcp/ip, tcb=607213c8, stack=[60720ba0-607213a0:2048], prio=7, xCoreID=-1
ap1:WDRV:D(341):bk_wifi_init, 174
ap1:WDRV:D(341):wdrv_init
ap1:D(341):wdrv_ipc_init
ap1:WDRV:D(341):wdrv_ipc_env_init
ap1:WDRV:W(341):wdrv_tx_cmd_buffer_init, addr[0]=0x28011a80, pattern_addr=0x28011a7c
ap1:WDRV:W(341):wdrv_tx_cmd_buffer_init, addr[1]=0x28011da0, pattern_addr=0x28011d9c
ap1:WDRV:W(342):wdrv_tx_cmd_buffer_init, addr[2]=0x280120c0, pattern_addr=0x280120bc
ap1:OS:D(342):create wdrv_thread, tcb=60722488, stack=[60721460-60722460:4096], prio=7, xCoreID=-1
ap1:WDRV:D(342):wdrv_host_cmd_id : 514
ap1:WDRV:D(342):wdrv_tx_msg: msg_id:0x202 len:12 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
D(453):cif_handle_bk_cmd_get_mac_addr_req
mac:D(454):sync_mac_record, base mac:c8:47:8c:df:34:22
mac:W(454):sync_base_mac_record, saved records is more than 1, free index(5).
mac:D(454):sync_mac_record, saved base mac:c8:47:8c:df:34:22
D(454):base mac: c8:47:8c:df:34:22
ap0:WDRV:D(344):wdrv_tx_msg: msg_id:0x366 len:64 sn:0 waitcfm:1 cfm_id:0 cfm_sn:0 
ap0:WDRV:D(344):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(345):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(345):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:D(346):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(346):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:D(347):wdrv_rx_handle_cmd_confirm,460,ID:0x366
ap0:WDRV:D(347):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:D(348):wdrv_rx_handle_cmd_confirm,460,ID:0x367
ap0:WDRV:D(348):wifi inited(1)
ap0:gpio:D(349):bk_gpio_driver_init:has inited 
wakeup
ap0:D(352):WIFI AT CMDS INIT OK
D(463):cpu0 receive the cpu1 boot success event [1]
ap1:bluetoot:D(352):bk_bluetooth_init start, 0 
rosc:D(463):ckmn calib rosc thread start
ap1:bt_ipc:D(353):open channel: 19 on CPU
ap1:bt_ipc:D(353):bt_ipc_init success
gpio:D(465):bk_gpio_driver_init:has inited 
os:D(467):create cli, tcb=280937d8, stack=[280931b0-280937b0:1536], prio=5
os:D(467):create log_hanlder, tcb=28093d08, stack=[280938e0-28093ce0:1024], prio=5
os:D(468):create atsvr-handler, tcb=28094b38, stack=[28093f10-28094b10:3072], prio=3
bt_ipc:D(468):op :0x0001
ble:D(468):bt_feature_adapter_init _is_gatt_discovery_auto 1
wakeup
ble:D(468):bt_feature_adapter_init _ignore_smp_key_distr_all_zero 0
ble:D(469):bt_feature_adapter_init _ignore_smp_already_pair 0
bt_ipc:W(469):bt_ipc_init bt ipc already initialised
bluetoot:D(469):bk_bluetooth_init ipc already init
os:D(473):create ble, tcb=28095c10, stack=[28094fe8-28095be8:3072], prio=5
ble:D(473):ble_entry run ble task with stack size 3072
rwnx_cal_set_rfconfig(0x101) phy on; rf off
ble:D(474):ble mac: c8:47:8c:df:34:23
ble:D(475):kernel_mem_init type 0 size 3144
ble:D(475):kernel_mem_init type 1 size 14508
ble:D(475):kernel_mem_init type 2 size 668
ble:D(476):ble_thread_main cali_ready_status:0xf, ble_cali_staus: 8
ble:D(476):enter normal mode
os:D(476):create ble_hci_to_host_thread, tcb=28096840, stack=[28096018-28096818:2048], prio=5
bluetoot:D(477):bk_bluetooth_init ok
ap0:ble:D(367):bt_feature_adapter_init _is_gatt_discovery_auto 1
ap0:ble:D(367):bt_feature_adapter_init _ignore_smp_key_distr_all_zero 0
ap0:ble:D(367):bt_feature_adapter_init _ignore_smp_already_pair 0
ap0:OS:D(367):create ble_host, tcb=28024258, stack=[28023630-28024230:3072], prio=5, xCoreID=-1
ap1:ble:D(368):timer_init
ap1:ble:D(368):kernel_mem_init type 0 size 3396
ap1:ble:D(368):kernel_mem_init type 1 size 3084
ap1:ble:D(369):kernel_mem_init type 2 size 6156
ap1:ble:D(369):kernel_mem_init type 3 size 12
ap1:ble:D(369):timer_set_timeout, to 0 ms
ap1:ble:D(370):timer_set_timeout, to 0 ms
psram:D(483):Auto detect:No PSRAM_CHIP_ID INFO, ret:0
ap0:ble:D(412):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x1,status:0x0
rosc:D(542):rosc calib complete 82 243
ap0:ble:D(439):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x3,status:0x0
ap0:ble:D(446):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x1a,status:0x0
ap0:ble:D(453):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x1a,status:0x0
ap0:ble:D(454):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x28,status:0x0
ap0:ble:D(454):gapm_cmp_evt:BLE_STACK_OK
ap0:AT_BLE:D(456):BLE AT CMDS INIT OK
ap0:bluetoot:D(456):bk_bluetooth_init ok
ap0:ap_main:I(456):
-----------------ap_main: start-----------------
ap0:OS:D(456):create adc_thread, tcb=607242c8, stack=[60723aa0-607242a0:2048], prio=4, xCoreID=-1
ap0:ap_main:E(466):ADC data = 8190, volt = 1100(mv)
ap0:ap_main:I(473):[adc_thread_entry][155]cali_value = 1100

[17:48:30.790]收←◆ap0:ap_main:E(1482):ADC data = 8190, volt = 1100(mv)
ap0:ap_main:I(1489):[adc_thread_entry][155]cali_value = 1100

[17:48:31.806]收←◆ap0:ap_main:E(2499):ADC data = 8190, volt = 1100(mv)
ap0:ap_main:I(2505):[adc_thread_entry][155]cali_value = 1100

```

## 常见问题

- **7258 saradc 精度**<br>
采样精度12bit<br>

- **7258 采样精度 12bit，采样值为什么会超过 4096**<br>
内部做了累加处理，所以采样值会超过 4096<br>

- **7258 saradc 电压检测范围**<br>
chan0范围0-5.5V，其他0-3V<br>

- **7258 休眠唤醒后 adc 采样值不准**<br>
请确认休眠时 GPI0 是否被下电，如果是唤醒时 GPI0 会有个充电过程，这个
过程会被 adc 采到。<br>

- **saradc每次采样会打印 gpio ummap 或 busy**<br>
只有第一次 adc 采样有这样的打印是正常的，后续还有就是异常，需要确认这个
gpio口有没有被映射成其他功能，gpio口只允许映射为一种功能。<br>

- **saradc 在射频工作时，采样抖动问题**<br>
射频对 adc 干扰分为两部分:<br>
(1)干扰到 saradc 电压参考源，I版本芯片改版后解决，还未量产量<br>
(2)射频工作导致电流发生变化，电流导致 vbat 电压在跳变，saradc如果采 vbat
就能看到明显抖动。该问题建议客户在上层射频不工作时再去采。<br>

- **用连续模式采样时，如何手动关闭adc采样**<br>
参考saradc示例，使用ADC_CONTINUOUS_MODE连续模式，使用连续模式进行单步操作，读完值后需要手动关闭连续模式。<br>

- **如何配置adc分压量程**<br>
(1)7258 ADC_CHAN_0 支持分压配置 Div2 Div3 Div5 Div7<br>
(2)7258 ADC_CHAN_1~ADC_CHAN_15 支持分压配置 Div1 Div2 Div3 Div4<br>
(3)分压配置用于调整adc量程，量程=1.1VxDiv，Div为分压配置值，例如如果配置为Div2，量程为1.1Vx2=2.2V<br>

## 更多adc开发指南请参考 [技术文档](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/developer-guide/peripheral/bk_adc.html?highlight=adc)。

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

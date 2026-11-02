# GPIO 低压休眠 io 保持示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述

GPIO（通用输入输出）模块是芯片上用于外部设备信号通信的核心模块。BK7258芯片有56个通用输入输出管脚（GPIO Pin），每个管脚都可用作一个通用IO，或连接一个内部的外设信号。

芯片刚开始启动时，所有GPIO都是高阻态，程序启动后，会依据GPIO table表将gpio初始化配置为对应的属性。

用户可按以下步骤操作，配置自己设备对应的GPIO table表：
1.  创建 usr_gpio_cfg.h 文件
    - 对于 ap 侧：
        1: 在工程目录 ap/config/bk7258_ap 目录下创建 usr_gpio_cfg.h 文件
        2: 从标准 sdk 的 projects/app 工程的 ap/config/bk7258_ap 目录下复制 usr_gpio_cfg.h 文件到工程的 ap/config/bk7258_ap 目录下
    - 对于 cp 侧：
        1: 在工程 cp/config/bk7258 目录下创建 usr_gpio_cfg.h 文件
        2: 从标准 sdk 的 projects/app 工程的 cp/config/bk7258 目录下复制 usr_gpio_cfg.h 文件到工程的 cp/config/bk7258 目录下
2. 工程 config 配置打开 CONFIG_USR_GPIO_CFG_EN 宏，使能用户自定义 GPIO_DEFAULT_DEV_CONFIG 表。

当你需要在低功耗模式下保持 GPIO 的输出状态时，可以按照以下配置：
1. 使用 bk_gpio_register_lowpower_keep_status() 函数注册。目前默认数量为4，可使用CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT修改数量）
2. 使用 bk_gpio_unregister_lowpower_keep_status（）函数可以取消之前注册需要保持的GPIO。

本示例基于 GPIO 驱动，演示了 GPIO 如何在低压休眠下保持io状态的实现。它具体展示了如何：

1. 配置 GPIO 为输出模式，输出高电平；
2. 启动pm休眠任务，配置 GPIO_25 为低功耗保持状态；
3. 进入低功耗休眠模式，等待 RTC 时钟触发唤醒 或 IO 引脚触发唤醒；


## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
|CONFIG_GPIO_DEFAULT_SET_SUPPORT| GPIO 默认设置使能配置，配置为y                                         |
|CONFIG_USR_GPIO_CFG_EN| 自定义 GPIO_DEFAULT_DEV_CONFIG 表使能配置，配置为y                                         |
|CONFIG_GPIO_WAKEUP_SUPPORT          | GPIO 唤醒使能配置，配置为y                                         |
|CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT| 动态唤醒源使能配置，配置为y                                         |
|CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT| 动态唤醒源最大数量，默认配置为4                                         |
|CONFIG_PSRAM_AS_SYS_MEMORY| PSRAM 作为系统内存使能配置，配置为n                                         |

## 相关代码

``` C
    #include "driver/pm_ap_core.h"
    #include <modules/pm.h>
    #include "driver/gpio.h"
    #include "gpio_driver.h"

   // 对比 GPIO_24 和 GPIO_25 的输出电平
    gpio_output_config(GPIO_24);
    gpio_output_config(GPIO_25);

    bk_gpio_set_output_high(GPIO_24);
    bk_gpio_set_output_high(GPIO_25);

    // 关闭蓝牙
    bk_bluetooth_deinit();
    // psram 没使用的时候默认会下电，使用了psram 后电流会高大约 200 ua
    // 初始化PM任务
    ret = pm_task_init();
    if(ret != BK_OK)
    {
        LOGE("pm_task_init failed\r\n");
        TEST_FAIL();
        return ret;
    }

    // 配置 GPIO_25 为低功耗保持状态
    gpio_config_t io_config ;
    io_config.io_mode = GPIO_OUTPUT_ENABLE;
    io_config.pull_mode = GPIO_PULL_DISABLE;
    io_config.func_mode = GPIO_SECOND_FUNC_DISABLE;
    bk_gpio_register_lowpower_keep_status(GPIO_25, &io_config);

    // 进入低功耗模式
    ret = app_enter_lowpower(30000, 1, GPIO_8, GPIO_INT_TYPE_FALLING_EDGE);
     
```

## 编译
>**注意：**
进入低压休眠前需要把蓝牙关闭，如需要wifi保活，请在进入低功耗前连接网络，设置dtim值。 psram 默认没有使用到会自动下电。系统进入低压休眠低电流为 120 ua左右，如需使用到 psram，psram不掉电电流会多 200 ua左右。

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/peripherals/gpio/keep_io_lowerpower
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/peripherals/gpio/keep_io_lowerpower/build/bk7258/keep_io_lowerpower/package/all-app.bin；
- 上电运行后，板子运行程序,会先输出高电平到 GPIO_24 和 GPIO_25，然后配置 GPIO_25 为低功耗保持状态，进入低功耗休眠模式，配置睡眠时间30s，唤醒IO 为GPIO_8（P8脚），等待 RTC 时钟触发唤醒 或 IO 引脚（P8脚）触发唤醒；

以下是上电运行后的串口log 输出：

```
p1:hal:W(390):gpio: 24 is used.Please confirm unmap isn't impact is working module.!            //配置GPIO
ap1:hal:W(390):gpio: 25 is used.Please confirm unmap isn't impact is working module.!
ap1:OS:D(391):create pm_process_thread, tcb=28028878, stack=[28028050-28028850:2048], prio=3, xCoreID=-1    //创建pm任务线程
ap1:bluetoot:D(391):bk_bluetooth_deinit start, 1 
ap0:ble:D(391):timer_deinit
rwnx_cal_set_rfconfig(0x102) phy on; rf off
ap0:bluetoot:D(399):bk_bluetooth_deinit ok          //关闭蓝牙

ap1:pm_ap:D(2400):Reg system_wakeup_cb:0x21889c5,3,0x2,1        //注册唤醒回调函数
ap1:AP:D(2400):wakeup data:671179888
ap0:keep_io_:I(2400):/home/qintao.shen/work/7258_smp/sdk_sample_project/sample_project/peripherals/gpio/keep_io_lowerpower/ap/ap_main.c:82:main:PASS
ap1:gpio:D(2400):bk_gpio_register_wakeup_source:register wakeup source gpio_id = 8, int_type = 3     //注册io唤醒源
ap1:pm_demo:I(2401):[pm_proecess_task][234]enter low power mode
ap1:pm_demo:I(12737):[pm_proecess_task][242]wakeup mode 1 wakeup source 0           //io唤醒

```
## 更多GPIO开发指南请参考 [技术文档](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/developer-guide/peripheral/bk_gpio.html)。
## 低功耗相关请参考 [技术文档](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/developer-guide/power_save/bk_fsm.html)。

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

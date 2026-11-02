# GPIO 输入示例

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

本示例基于 GPIO 驱动，演示了 GPIO 输入的实现。它具体展示了如何：

1. 配置 GPIO 为输入模式, 配置输入下拉, 配置 GPIO 中断类型为上升沿触发，中断事件中触发中断回调；
2. 在任务中间隔 1s 读取一次 GPIO 输入值；

## 相关代码

``` C
    #include "driver/gpio.h"
    #include "gpio_driver.h"

    // 配置GPIO为输入模式, 配置输入下拉, 配置GPIO中断类型为上升沿触发
    gpio_input_config(GPIO_25, GPIO_PULL_DOWN_EN, GPIO_INT_TYPE_RISING_EDGE);
    // 读取一次GPIO输入值
    uint32_t input_val = bk_gpio_get_input(GPIO_25);
    LOGI("gpio input test: input_val: %d \r\n", input_val);
     
```

## 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/peripherals/gpio/input
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/peripherals/gpio/input/build/bk7258/input/package/all-app.bin；
- 上电运行后，板子运行程序，在任务中间隔 1s 读取一次 GPIO（P25脚）的输入值，打印出 GPIO 输入值;

以下是上电运行后的串口log 输出：

```
ap0:OS:D(528):create gpio_input_thread, tcb=607242c8, stack=[60723aa0-607242a0:2048], prio=3, xCoreID=-1        // 创建gpio输入线程
ap1:hal:W(529):gpio: 25 is used.Please confirm unmap isn't impact is working module.!                          
ap0:gpio_inp:I(529):/home/qintao.shen/work/7258_smp/sdk_sample_project/sample_project/peripherals/gpio/input/ap/ap_main.c:89:main:PASS
ap0:gpio_inp:I(529):gpio input test: input_val: 0 
ap0:gpio_inp:I(1529):gpio input test: input_val: 0 
ap0:gpio_inp:I(2529):gpio input test: input_val: 0 
ap0:gpio_inp:I(3529):gpio input test: input_val: 0      // gpio 读取输入电平
ap0:gpio_inp:I(4529):gpio input test: input_val: 0 
ap0:gpio_inp:I(5321):gpio_int_isr: 25
ap0:gpio_inp:I(5322):gpio_int_isr: 25
ap0:gpio_inp:I(5323):gpio_int_isr: 25                   // gpio 触发中断
ap0:gpio_inp:I(5324):gpio_int_isr: 25
ap0:gpio_inp:I(5325):gpio_int_isr: 25
ap0:gpio_inp:I(5326):gpio_int_isr: 25
ap0:gpio_inp:I(5326):gpio_int_isr: 25
ap0:gpio_inp:I(5327):gpio_int_isr: 25
ap0:gpio_inp:I(5329):gpio_int_isr: 25
ap0:gpio_inp:I(5529):gpio input test: input_val: 1 

```
## 更多GPIO开发指南请参考 [技术文档](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/developer-guide/peripheral/bk_gpio.html)。


## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

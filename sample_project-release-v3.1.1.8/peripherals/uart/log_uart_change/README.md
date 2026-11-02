# 日志串口如何换为其它串口

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 参考章节
- [官网 uart介绍](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/developer-guide/peripheral/bk_uart.html)
- [官网 uart示例](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/examples/peripheral/bk_uart.html)

## 概述

BK7258 芯片有三组UART串口，分别为UART0、UART1、UART2，串口日志默认通过uart0输出，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 修改CP核的CONFIG_UART_PRINT_PORT改为其它串口。

**BK7258 RX、TX、CTS、RTS管脚只能按芯片datasheet定义的管脚严格使用**，不能换为其它管脚，也不能管脚互换（如将RX管脚当TX管脚使用）。

本示例演示了如何在BK7258芯片上将SDK默认的日志打印口由串口0调整为串口1。

## 硬件环境

- 核心板：BK7258_QFN88_V3

![bk_vfs](/multimedia/boot_anim/v3_demo_board.jpg)

## SDK 日志打印改为uart1

### 改CP核配置

以下配置在CP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_USR_GPIO_CFG_EN          | 用户自定义GPIO配置使能，必须配置置为y，否则会使用sdk默认gpio导致管脚配置不对 |
| CONFIG_UART_PRINT_PORT          | UART打印口配置，默认配置为0，改为1调整为uart1 |
| CONFIG_UART_ATE_PORT          | ATE测试串口，默认配置为0，改为1调整为uart1 |
| CONFIG_UART_ATE_PRINT_PORT          | ATE测试打印串口，默认配置为0，改为1调整为uart1 |

### 改AP核配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_USR_GPIO_CFG_EN          | 用户自定义GPIO配置使能，必须配置置为y，否则会使用sdk默认gpio导致管脚配置不对 |

### 改CP的usr_gpio_cfg.h

> **注意** ：需去掉UART0 GPIO_10和GPIO_11的配置，避免和AP核初始化冲突。

在use_with_hw_flow_control/cp/config/bk7258/usr_gpio_cfg.h加入uart1 TX和RX管脚配置，并将其配置为串口功能，如下所示：
```c
       {GPIO_0, GPIO_SECOND_FUNC_DISABLE, GPIO_DEV_UART1_TXD, GPIO_IO_DISABLE, GPIO_PULL_UP_EN, GPIO_INT_DISABLE, GPIO_INT_TYPE_LOW_LEVEL, GPIO_LOW_POWER_DISCARD_IO_STATUS, GPIO_DRIVER_CAPACITY_0,GPIO_INIT_ENABLE,GPIO_TIME_SHARING_MULTIPLEX_DISABLE},\
	{GPIO_1, GPIO_SECOND_FUNC_DISABLE, GPIO_DEV_UART1_RXD, GPIO_IO_DISABLE, GPIO_PULL_UP_EN, GPIO_INT_DISABLE, GPIO_INT_TYPE_LOW_LEVEL, GPIO_LOW_POWER_DISCARD_IO_STATUS, GPIO_DRIVER_CAPACITY_0,GPIO_INIT_ENABLE,GPIO_TIME_SHARING_MULTIPLEX_DISABLE},\
```

### 改AP的usr_gpio_cfg.h

> **注意** ：需去掉UART1 GPIO_0和GPIO_1的配置，避免和CP核初始化冲突。

在use_with_hw_flow_control/ap/config/bk7258_ap/usr_gpio_cfg.h加入uart1 TX、RX管脚配置，并将其配置为串口功能，如下所示：
```c
       {GPIO_10, GPIO_SECOND_FUNC_DISABLE, GPIO_DEV_UART0_RXD, GPIO_IO_DISABLE, GPIO_PULL_UP_EN, GPIO_INT_DISABLE, GPIO_INT_TYPE_LOW_LEVEL, GPIO_LOW_POWER_DISCARD_IO_STATUS, GPIO_DRIVER_CAPACITY_0,GPIO_INIT_ENABLE,GPIO_TIME_SHARING_MULTIPLEX_DISABLE},\
	{GPIO_11, GPIO_SECOND_FUNC_DISABLE, GPIO_DEV_UART0_TXD, GPIO_IO_DISABLE, GPIO_PULL_UP_EN, GPIO_INT_DISABLE, GPIO_INT_TYPE_LOW_LEVEL, GPIO_LOW_POWER_DISCARD_IO_STATUS, GPIO_DRIVER_CAPACITY_0,GPIO_INIT_ENABLE,GPIO_TIME_SHARING_MULTIPLEX_DISABLE},\
```

## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/peripherals/uart/log_uart_change/
vim Makefile  //修改SDK_DIR为实际smp sdk路径
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录工程build下面的all-app.bin；
- 将板子UART1的接电脑，用于输出打印日志；
- 上电后，日志打印“hello world, this is bk7258”；

以下是上电运行后的关键串口log 输出：

```
ap1:uart:I(429):[main][39] hello world, this is bk7258   //打印日志信息
ap1:uart:I(429):/homeb/honghui.xiao/7258/avdk_smp/sample_project_git/peripherals/uart/log_uart_change/ap/ap_main.c:40:main:PASS
```

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

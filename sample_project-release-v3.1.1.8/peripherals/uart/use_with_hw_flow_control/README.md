# 串口硬件流控使用示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 参考章节
- [官网 uart介绍](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/developer-guide/peripheral/bk_uart.html)
- [官网 uart示例](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/examples/peripheral/bk_uart.html)
- [串口收发数据使用示例](/peripherals/uart/general_use/README.md)
- [串口软件流控使用示例](/peripherals/uart/use_with_sw_flow_control/README.md)

## 概述

BK7258 芯片有三组UART串口，分别为UART0、UART1、UART2，串口日志默认通过uart0输出，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 修改CP核的CONFIG_UART_PRINT_PORT改为其它串口。

**BK7258 RX、TX、CTS、RTS管脚只能按芯片datasheet定义的管脚严格使用**，不能换为其它管脚，也不能管脚互换（如将RX管脚当TX管脚使用）。

BK7258 只有uart0有硬件流控，如果uart1、uart2需要流控，可以使用软流控方式实现。

本示例演示了如何在BK7258芯片上使用UART0串口配合硬件流控进行通信，实现以下功能：
1. 将SDK 默认打印口调整为uart1
2. UART0串口的初始化和配置
3. 串口数据的发送和接收
4. 使用中断和信号量实现异步通信
5. 与PC端串口工具进行通信测试

## 硬件环境

- 核心板：BK7258_QFN88_V3

![bk_vfs](/multimedia/boot_anim/v3_demo_board.jpg)

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_UART_RX_DMA          | UART接收DMA使能配置，配置为y启用DMA接收，本示例配置为y |
| CONFIG_UART_TX_DMA          | UART发送DMA使能配置，配置为y启用DMA发送，本示例配置为y |
| CONFIG_USR_GPIO_CFG_EN          | 用户自定义GPIO配置使能，必须配置置为y，否则会使用sdk默认gpio导致管脚配置不对|
| CONFIG_UART0_FLOW_CTRL          | UART0硬件流控使能配置，本示例配置为y |

## SDK 日志打印改为uart1，uart0改为有流控配置

### 改CP核配置
以下配置在CP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_UART_PRINT_PORT          | UART打印口配置，默认配置为0，改为1调整为uart1 |
| CONFIG_UART_ATE_PORT          | ATE测试串口，默认配置为0，改为1调整为uart1 |
| CONFIG_UART_ATE_PRINT_PORT          | ATE测试打印串口，默认配置为0，改为1调整为uart1 |
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

在use_with_hw_flow_control/ap/config/bk7258_ap/usr_gpio_cfg.h加入uart1 TX、RX、RTS、CTS管脚配置，并将其配置为串口功能，如下所示：
```c
       {GPIO_10, GPIO_SECOND_FUNC_DISABLE, GPIO_DEV_UART0_RXD, GPIO_IO_DISABLE, GPIO_PULL_UP_EN, GPIO_INT_DISABLE, GPIO_INT_TYPE_LOW_LEVEL, GPIO_LOW_POWER_DISCARD_IO_STATUS, GPIO_DRIVER_CAPACITY_0,GPIO_INIT_ENABLE,GPIO_TIME_SHARING_MULTIPLEX_DISABLE},\
	{GPIO_11, GPIO_SECOND_FUNC_DISABLE, GPIO_DEV_UART0_TXD, GPIO_IO_DISABLE, GPIO_PULL_UP_EN, GPIO_INT_DISABLE, GPIO_INT_TYPE_LOW_LEVEL, GPIO_LOW_POWER_DISCARD_IO_STATUS, GPIO_DRIVER_CAPACITY_0,GPIO_INIT_ENABLE,GPIO_TIME_SHARING_MULTIPLEX_DISABLE},\
	{GPIO_12, GPIO_SECOND_FUNC_DISABLE, GPIO_DEV_UART0_RTS, GPIO_IO_DISABLE, GPIO_PULL_DISABLE, GPIO_INT_DISABLE, GPIO_INT_TYPE_LOW_LEVEL, GPIO_LOW_POWER_DISCARD_IO_STATUS, GPIO_DRIVER_CAPACITY_0,GPIO_INIT_ENABLE,GPIO_TIME_SHARING_MULTIPLEX_ENABLE},\
	{GPIO_13, GPIO_SECOND_FUNC_DISABLE, GPIO_DEV_UART0_CTS, GPIO_IO_DISABLE, GPIO_PULL_DISABLE, GPIO_INT_DISABLE, GPIO_INT_TYPE_LOW_LEVEL, GPIO_LOW_POWER_DISCARD_IO_STATUS, GPIO_DRIVER_CAPACITY_0,GPIO_INIT_ENABLE,GPIO_TIME_SHARING_MULTIPLEX_ENABLE},\
```

## 代码结构

本示例在[串口收发数据使用示例](/peripherals/uart/general_use/README.md)，只是按上面章节修改配置和管脚，然后将代码测试串口改为uart0，代码解释详见[串口收发数据使用示例](/peripherals/uart/general_use/README.md)。


## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/peripherals/uart/use_with_hw_flow_control/
vim Makefile  //修改SDK_DIR为实际smp sdk路径
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录工程build下面的all-app.bin；
- 将板子UART1的接电脑，用于输出打印日志；
- 将板子UART0的TX、RX、CTS、RTS、GND通过usb转接头和电脑连到一起，**电脑端的RTS**接板子上的P13（板子右下角靠近uart2），**电脑端的CTS**接P12管脚(板子SD卡槽左边)；
- 电脑端打开串口工具，如sscom32，需勾选“RTS”、“DTR”；
- 上电后，程序会通过uart0 发送“hello, this message is sent from bk7258”数据到电脑，电脑可以发送任意数据给BK7258,BK7258收到并会打印；

以下是上电运行后的关键串口log 输出：

```
ap0:uart:I(430):[dev_uart_open][69] uart 0 open success   //串口打开成功
ap0:uart:I(435):[app_uart_test][105] uart 0 recv:hello, this message is from PC   //收到电脑发送的数据
ap0:uart:I(435):/homeb/honghui.xiao/7258/avdk_smp/sample_project_git/peripherals/uart/general_use/ap/ap_main.c:114:app_uart_test:PASS
```

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

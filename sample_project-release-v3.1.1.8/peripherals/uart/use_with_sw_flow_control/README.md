# 串口软件流控使用示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 参考章节
- [官网 uart介绍](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/developer-guide/peripheral/bk_uart.html)
- [官网 uart示例](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/examples/peripheral/bk_uart.html)
- [串口收发数据使用示例](/peripherals/uart/general_use/README.md)
- [串口硬件流控使用示例](/peripherals/uart/use_with_hw_flow_control/README.md)

## 概述

BK7258 芯片有三组UART串口，分别为UART0、UART1、UART2，串口日志默认通过uart0输出，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 修改CP核的CONFIG_UART_PRINT_PORT改为其它串口。

**BK7258 RX、TX、CTS、RTS管脚只能按芯片datasheet定义的管脚严格使用**，不能换为其它管脚，也不能管脚互换（如将RX管脚当TX管脚使用）。

BK7258 只有uart0有硬件流控，如果uart1、uart2需要流控，可以使用软流控方式实现。

本示例演示了如何在BK7258芯片上使用UART1串口配合软件流控进行通信，实现以下功能：
1. UART串口的初始化和配置
2. 串口数据的发送和接收
3. 使用中断和信号量实现异步通信
4. 与PC端串口工具进行通信测试

## 硬件环境

- 核心板：BK7258_QFN88_V3

![bk_vfs](/multimedia/boot_anim/v3_demo_board.jpg)

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_UART_RX_DMA          | UART接收DMA使能配置，配置为y启用DMA接收，本示例配置为y |
| CONFIG_UART_TX_DMA          | UART发送DMA使能配置，配置为y启用DMA发送，本示例配置为y |
| CONFIG_UART_SW_FLOW_CTRL          | UART软件流控使能配置，本示例配置为y |


## 代码结构

本示例在[串口收发数据使用示例](/peripherals/uart/general_use/README.md)，只是增加了软件流控功能，可先查阅[串口收发数据使用示例](/peripherals/uart/general_use/README.md)，新增关键代码如下：

```c
     // UART配置参数结构体
    uart_config_t config =
    {
            .baud_rate = UART_BAUDRATE_115200,  // 波特率：115200
            .data_bits = UART_DATA_8_BITS,      // 数据位：8位
            .parity = UART_PARITY_NONE,         // 校验位：无
            .stop_bits = UART_STOP_BITS_1,      // 停止位：1位
            .flow_ctrl = UART_FLOWCTRL_DISABLE, // 流控：禁用
            .src_clk = UART_SCLK_XTAL_26M       // 源时钟：26M晶振
    };

    do{
        // 启用软件流控
        config.enable_sw_flow_ctrl = true;
        config.cts_gpio = GPIO_34;
        config.rts_gpio = GPIO_35;

        // 打开UART1设备，配置参数并注册中断回调函数
        ret = dev_uart_open(UART_ID_1, &config, _uart_rx_can_read_isr, _uart_tx_can_send_isr);
```

## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/peripherals/uart/use_with_sw_flow_control/
vim Makefile  //修改SDK_DIR为实际smp sdk路径
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录工程build下面的all-app.bin；
- 将板子UART1的TX、RX、CTS、RTS、GND通过usb转接头和电脑连到一起，**电脑端的RTS**接板子上的P34，**电脑端的CTS**接P35管脚；
- 电脑端打开串口工具，如sscom32，需勾选“RTS”、“DTR”；
- 上电后，程序会通过uart1 发送“hello, this message is sent from bk7258”数据到电脑，电脑可以发送任意数据给BK7258,BK7258收到并会打印；

以下是上电运行后的关键串口log 输出：

```
ap0:uart:I(430):[dev_uart_open][69] uart 1 open success   //串口打开成功
ap0:uart:I(435):[app_uart_test][105] uart 1 recv:hello, this message is from PC   //收到电脑发送的数据
ap0:uart:I(435):/homeb/honghui.xiao/7258/avdk_smp/sample_project_git/peripherals/uart/general_use/ap/ap_main.c:114:app_uart_test:PASS
```

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

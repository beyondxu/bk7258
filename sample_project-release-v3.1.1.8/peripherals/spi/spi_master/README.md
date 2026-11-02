# 硬件SPI master示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述
本示例演示了如何在BK7258芯片上使用SPI（串行外设接口）主设备模式。示例基于SPI组件，展示了SPI主设备的初始化、数据传输、DMA模式等功能。

本示例基于 SPI驱动，它具体展示了如何：

1. SPI 主设备初始化配置, SPI配置参数设置（模式、位宽、时钟等）
2. SPI 普通模式数据传输（发送/接收）
3. SPI DMA模式数据传输
4. SPI 全双工通信

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置项 | 描述 |
| ------ | ---- |
|CONFIG_SPI     |使能SPI驱动|
|CONFIG_SPI_DMA |使能SPI DMA模式|
|CONFIG_SPI_BYTE_INTERVAL |SPI字节间隔时间|

## SPI 引脚
>**注意**
SPI0 SDK 默认使用组O的引脚，若需要使用组1或组2的引脚，需修改SDK 文件bk_avdk_smp/ap/middleware/soc/bk7258_ap/hal/spi_ll.h中 SPI0_LL_CSN_PIN、SPI0_LL_SCK_PIN、SPI0_LL_MOSI_PIN、SPI0_LL_MISO_PIN等定义引脚。

SPI0对应的硬件管脚如下
|SPI0 功能 | 组O| 组1| 组2 |
| ------ | ---- | ---- | ---- |
| SPI0_CLK | GPIO_14 | GPIO_44 | GPIO_33 |
| SPI0_CS  | GPIO_15 | GPIO_45 | GPIO_34 |
| SPI0_MOSI | GPIO_16 | GPIO_46 | GPIO_35 |
| SPI0_MISO | GPIO_17 | GPIO_47 | GPIO_36 |

SPI1对应的硬件管脚如下
|SPI1 功能 | 管脚 |
| ------ | ---- |
| SPI1_CLK | GPIO_2 |
| SPI1_CS  | GPIO_3 |
| SPI1_MOSI | GPIO_4 |
| SPI1_MISO | GPIO_5 |

## 相关代码

``` C
    // 相关头文件
    #include <driver/spi.h>
    #include <driver/dma.h>
    
    //关键函数
    // 初始化SPI设备
    bk_err_t bk_spi_init(spi_id_t id, const spi_config_t *config);
    
    // 反初始化SPI设备
    bk_err_t bk_spi_deinit(spi_id_t id);

    // 发送SPI数据（普通模式）
    bk_err_t bk_spi_write_bytes(spi_id_t id, const void *data, uint32_t size);

    // 接收SPI数据（普通模式）
    bk_err_t bk_spi_read_bytes(spi_id_t id, void *data, uint32_t size);
    
    // 发送SPI数据（DMA模式）
    bk_err_t bk_spi_dma_write_bytes(spi_id_t id, const void *data, uint32_t size);
    
    // 接收SPI数据（DMA模式）
    bk_err_t bk_spi_dma_read_bytes(spi_id_t id, void *data, uint32_t size);

    // 全双工SPI数据传输（DMA模式）
    bk_err_t bk_spi_dma_duplex_xfer(spi_id_t id, const void *tx_data, uint32_t tx_size, void *rx_data, uint32_t rx_size);
```
## SPI普通模式初始化
``` C
    // SPI普通模式初始化
    spi_config_t config = {0};
    config.role = SPI_ROLE_MASTER;                      // 角色：主设备
    config.bit_width = SPI_BIT_WIDTH_8BITS;             // 数据位宽：8位
    config.polarity = SPI_POLARITY_HIGH;                // 时钟极性：高电平有效
    config.phase = SPI_PHASE_2ND_EDGE;                  // 时钟相位：第二个时钟沿
    config.wire_mode = SPI_4WIRE_MODE;                  // 4线模式
    config.baud_rate = SPI_BAUD_RATE;                   // 波特率：10MHz
    config.bit_order = SPI_MSB_FIRST;                   // 数据位序：MSB优先
    #if (CONFIG_SPI_BYTE_INTERVAL)
    config.byte_interval = 1;                           // 字节间隔：1个时钟周期
    #endif
    config.dma_mode = SPI_DMA_MODE_DISABLE;             // 禁用DMA模式
    int ret = bk_spi_init(SPI_CHAN_ID, &config);
    LOGI("spi init, spi_chan_id=%d\n", SPI_CHAN_ID);
```
## SPI DMA模式初始化
``` C
    // SPI DMA模式初始化
    spi_config_t config = {0};
    config.role = SPI_ROLE_MASTER;                      // 角色：主设备
    config.bit_width = SPI_BIT_WIDTH_8BITS;             // 数据位宽：8位
    config.polarity = SPI_POLARITY_HIGH;                // 时钟极性：高电平有效
    config.phase = SPI_PHASE_2ND_EDGE;                  // 时钟相位：第二个时钟沿
    config.wire_mode = SPI_4WIRE_MODE;                  // 4线模式
    config.baud_rate = SPI_BAUD_RATE;                   // 波特率：10MHz
    config.bit_order = SPI_MSB_FIRST;                   // 数据位序：MSB优先
    #if (CONFIG_SPI_BYTE_INTERVAL)
    config.byte_interval = 1;                           // 字节间隔：1个时钟周期
    #endif
    #if (CONFIG_SPI_DMA)
    config.dma_mode = SPI_DMA_MODE_ENABLE;              // 启用DMA模式
    spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI1);      // 分配SPI_TX DMA通道
    spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI1_RX);   // 分配SPI_RX DMA通道
    config.spi_tx_dma_chan = spi_tx_dma_chan;           // 设置SPI_TX DMA通道
    config.spi_rx_dma_chan = spi_rx_dma_chan;           // 设置SPI_RX DMA通道
    config.spi_tx_dma_width = DMA_DATA_WIDTH_8BITS;     // 设置SPI_TX DMA数据宽度为8位
    config.spi_rx_dma_width = DMA_DATA_WIDTH_8BITS;     // 设置SPI_RX DMA数据宽度为8位
    #endif

    int ret = bk_spi_init(SPI_CHAN_ID, &config);
    LOGI("spi dma init, spi_chan_id=%d\n", SPI_CHAN_ID);

``` 
## 测试任务
``` C
    int ret = BK_OK;
    rtos_delay_milliseconds(3000);
    LOGI("spi_example_task start\n");

    //配置 SPI 普通模式
    ret = spi_master_init();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }
    
    //单工发送,等待 slave 就绪
    rtos_delay_milliseconds(2000);
    ret = spi_master_send_data();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }

    //单工接受,等待 slave 就绪
    rtos_delay_milliseconds(2000);
    ret = spi_master_recv_data();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }

    ret = spi_master_deinit();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }
    rtos_delay_milliseconds(200);


    //配置SPI DMA模式
    ret = spi_master_dma_init();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }

    //单工发送,等待 slave 就绪
    rtos_delay_milliseconds(2000);
    ret = spi_master_dma_send_data();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }
    //单工接收,等待 slave 就绪
    rtos_delay_milliseconds(2000);
    ret = spi_master_dma_recv_data();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }
    //双工收发,等待 slave 就绪
    rtos_delay_milliseconds(2000);
    ret = spi_master_dma_transmit_data();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }
    ret = spi_master_dma_deinit();
    if(ret != BK_OK)
    {
        TEST_FAIL();
        goto error;
    }   
    
    TEST_PASS();
``` 

## 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/peripherals/spi/spi_master/
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/peripherals/spi/spi_master/output/build/bk7258/output/package/all-app.bin；  
- 准备两块开发板，一块作为主设备(编译烧录 sample_project/peripherals/spi/spi_master)，一块作为从设备(编译烧录 sample_project/peripherals/spi/spi_slave).本示例使用了SPI1 作为通信接口。

- SPI引脚连接：
  - SCK：时钟信号线 （P2脚），连接到从设备的SCK引脚（P2脚）
  - MOSI：主设备输出从设备输入 （P4脚），连接到从设备的MOSI引脚 （P4脚）
  - MISO：主设备输入从设备输出 （P5脚），连接到从设备的MISO引脚 （P5脚）
  - CS：片选信号线 （P3脚），连接到从设备的CS引脚 （P3脚）
  - GND：地引脚，连接到从设备的GND引脚
- 启动测试：
  - 主机先上电，从机后上电
- 观察日志：
  - 主设备串口输出日志显示测试进度和结果
  - 从设备串口输出日志显示接收到的数据
- 检查数据：
  - 验证主设备发送的数据是否与从设备接收到的数据一致
  - 检查数据传输是否符合预期

以下是上电运行后的串口log 输出：

```
[15:51:10.521]收←◆ap0:spi_mast:I(3469):spi_example_task start
ap0:hal:W(3469):gpio: 3 is used.Please confirm unmap isn't impact is working module.!
ap0:hal:W(3469):gpio: 2 is used.Please confirm unmap isn't impact is working module.!
ap0:hal:W(3470):gpio: 4 is used.Please confirm unmap isn't impact is working module.!
ap0:hal:W(3470):gpio: 5 is used.Please confirm unmap isn't impact is working module.!
ap0:hal:D(3470):spi select src_clk xtal
ap0:spi_mast:I(3470):spi init, spi_chan_id=1                                // 初始化SPI

spi_mast:I(7470):example_recv_buf[0]=0x72                                   // 接受从设备发送的数据
ap0:spi_mast:I(7470):example_recv_buf[1]=0x00
ap0:spi_mast:I(7470):example_recv_buf[2]=0x01
ap0:spi_mast:I(7470):example_recv_buf[3]=0x02
ap0:spi_mast:I(7471):example_recv_buf[4]=0x03
ap0:spi_mast:I(7471):example_recv_buf[5]=0x04
ap0:spi_mast:I(7471):example_recv_buf[6]=0x05
ap0:spi_mast:I(7471):example_recv_buf[7]=0x06
ap0:spi_mast:I(7471):example_recv_buf[8]=0x07
ap0:spi_mast:I(7471):example_recv_buf[9]=0x08
ap0:spi_mast:I(7472):example_recv_buf[10]=0x09
ap0:spi_mast:I(7472):example_recv_buf[11]=0x0a
ap0:spi_mast:I(7472):example_recv_buf[12]=0x0b
ap0:spi_mast:I(7472):example_recv_buf[13]=0x0c
ap0:spi_mast:I(7472):example_recv_buf[14]=0x0d
......
ap0:spi_mast:I(13826):/home/qintao.shen/work/7258_smp/sdk_sample_project/sample_project/peripherals/spi/spi_master/ap/ap_main.c:286:spi_example_task:PASS
```
## 更多SPI开发指南请参考 [技术文档](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/examples/peripheral/bk_spi.html)。

## 常见问题
1. SPI通信失败：检查引脚连接是否正确，确保时钟极性、相位配置正确
2. 数据传输错误：检查波特率设置是否超出芯片支持范围
3. DMA模式不可用：确认项目配置中已启用SPI DMA支持
4. 从设备无响应：检查片选信号是否正确控制

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

# 模拟 SPI 输出示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述
模拟SPI（Software SPI） 是用通用GPIO引脚通过软件编程模拟SPI通信协议的方式。通过代码控制GPIO引脚的电平变化（时钟SCLK、数据MOSI/MISO、片选CS），模拟SPI时序。

本示例基于模拟 SPI 驱动，演示了模拟 SPI 输出的实现。它具体展示了如何：

1. 配置模拟 SPI 初始化
2. 发送模拟 SPI 数据/命令

## 相关代码

``` C
    //相关头文件
    #include <driver/sim_spi.h>
    // 定义模拟SPI引脚
    #define SIM_SPI_SCK     GPIO_20
    #define SIM_SPI_CS      GPIO_21
    #define SIM_SPI_SDA     GPIO_22
    #define SIM_SPI_RST     GPIO_23
``` 
### 模拟SPI 操作示例
``` C
int sim_spi_example(void)
{
    int ret = BK_OK;
    lcd_spi_io_t sim_spi_io = {
        .clk = SIM_SPI_SCK,
        .cs = SIM_SPI_CS,
        .sda = SIM_SPI_SDA,
        .rst = SIM_SPI_RST,
    };

    // 注册模拟SPI总线
    sim_spi_handle = lcd_spi_bus_io_register(&sim_spi_io);
    if(sim_spi_handle == NULL)
    {
        LOGE("sim_spi_bus_io_register fail");
        TEST_FAIL();
        return BK_FAIL;
    }

    // 初始化模拟SPI总线, 使用spi3线模式，时钟极性高电平，相位极性为第二个时钟沿
    ret = sim_spi_handle->init(sim_spi_handle);
    if(ret != BK_OK)
    {
        LOGE("sim_spi_handle->init fail");
        TEST_FAIL();
        return BK_FAIL;
    }
    // 3 线串行数据包 包含控制位 D/CX 和传输字节。一共有 9 bits，第一个bit 为 D/CX，
    // 0 表示命令，1 表示数据。后面 8 bits 为传输字节。

    // 发送数据 0x00 ~ 0xFF
    rtos_delay_milliseconds(100);
    for (int i = 0; i < 255; i++)
    {
        i = i & 0xFF;
        sim_spi_handle->write_data(sim_spi_handle, i);
    }
    LOGI("write_data 0x00 ~ 0xFF done\r\n");
   
    // 发送命令 0xFF ~ 0x00
    rtos_delay_milliseconds(100);
    for (int i = 255; i >= 0; i--)
    {
        i = i & 0xFF;
        sim_spi_handle->write_cmd(sim_spi_handle, i);
    }
    LOGI("write_cmd 0xFF ~ 0x00 done\r\n");

    // 反初始化模拟SPI总线
    ret = sim_spi_handle->deinit(sim_spi_handle);
    if(ret != BK_OK)
    {
        LOGE("sim_spi_handle->deinit fail");
        TEST_FAIL();
        return BK_FAIL;
    }
    TEST_PASS();
    return BK_OK;
}
```

## 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/peripherals/spi/sim_spi
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/peripherals/spi/sim_spi/build/bk7258/output/package/all-app.bin；
- 上电运行后，板子运行程序，使用逻辑分析仪分析模拟 SPI 输出信号，是否符合逻辑；
1. 模拟spi 发送 0x00 ~ 0xFF 数据，验证是否符合逻辑；
2. 模拟spi 发送 0xFF ~ 0x00 命令，验证是否符合逻辑；

以下是上电运行后的串口log 输出：

```
ap1:OS:D(446):create sim_spi_example_thread, tcb=607242c8, stack=[60723aa0-607242a0:2048], prio=3, xCoreID=-1
ap1:sim_spi:I(447):rtos_create_thread success
ap0:hal:W(447):gpio: 23 is used.Please confirm unmap isn't impact is working module.!
ap0:hal:W(447):gpio: 20 is used.Please confirm unmap isn't impact is working module.!
ap0:hal:W(448):gpio: 21 is used.Please confirm unmap isn't impact is working module.!
ap0:hal:W(448):gpio: 22 is used.Please confirm unmap isn't impact is working module.!
ap0:st7796s:D(448):lcd_spi_io_init_by_handle clk: 20, cs: 21, sda: 22, rst: 23 
ap0:sim_spi:I(628):write_data 0x00 ~ 0xFF done
ap1:sim_spi:I(808):write_cmd 0xFF ~ 0x00 done
ap1:sim_spi:I(809):/home/qintao.shen/work/7258_smp/sdk_sample_project/sample_project/peripherals/spi/sim_spi/ap/ap_main.c:88:sim_spi_example:PASS

```

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

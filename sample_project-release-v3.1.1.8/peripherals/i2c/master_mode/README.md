# 硬件I2C master示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  
## 硬件环境

- 核心板：BK7258_QFN88_V3

## 概述
- I2C 是一种串行同步半双工通信协议，总线上可以同时挂载多个主机和从机。I2C 总线由串行数据线 (SDA) 和串行时钟线 (SCL) 线构成。这些线都需要上拉电阻.
- BK7258 芯片通常具有 2 个 I2C 控制器 ( I2C0 和 I2C1 )； BK7258 的 I2C 既可工作在主机模式下，也可工作在从机模式下。无论主机还是从机，读写之前需要先调用 bk_i2c_init() 配置 I2C。 bk_i2c_init() 主要配置 I2C SCL 的时钟频率，如果工作在从机模式下，还需要配置从机的 I2C 的器件地址。 I2C 接口的每次传输都是由主器件 (Master) 发起和控制。

- I2C 驱动支持硬件 I2C 和软件模拟 I2C（Simulated I2C）的共存使用。I2C ID 用于区分不同的 I2C 实例
    1. 硬件 I2C：使用 SoC 硬件 I2C 外设 ，对于 BK7258：id = 0, 1 对应 I2C0 和 I2C1。特点：使用硬件寄存器，支持中断和 DMA，性能高
    2. 模拟 I2C：使用 GPIO 软件模拟 I2C 时序 - ID 范围 id >= 2 为模拟 I2C。特点：通过 GPIO 控制实现，灵活配置任意 GPIO 作为 SDA/SCL

本示例基于 I2C 驱动，演示了 I2C master设备的读写操作。它具体实现以下功能：

1. 如何打开I2C设备，配置I2C参数；
2. 如何读写I2C设备；

## I2C 引脚
I2C0对应的硬件管脚如下
| 引脚名 | 管脚 |
| ------ | ---- |
| SCL0 | GPIO_20 |
| SDA0 | GPIO_21 |

I2C1对应的硬件管脚如下
| 引脚名 | 管脚 |
| ------ | ---- |
| SCL1 | GPIO_0 |
| SDA1 | GPIO_1 |

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

**I2C相关配置：**
| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_I2C | i2c 使能配置，需要配置为y |
| CONFIG_I2C_SUPPORT_ID_BITS | i2c 支持的设备id位数，配置为2 |
| CONFIG_SIM_I2C| 使能模拟I2C，需要配置为y |
| CONFIG_SIM_I2C0_SDA_GPIO | 配置模拟I2C0的SDA GPIO |
| CONFIG_SIM_I2C0_SCL_GPIO | 配置模拟I2C0的SCL GPIO |

>**注意**
1. 使用I2C通信时，要 把usr_gpio_cfg.c 中配置 I2C 对应的 GPIO 引脚的第二功能并使能第二功能，如果硬件没有外接上拉电阻，同时要把 GPIO 引脚的上拉功能使能。
2. 使用7258 demo 板 I2C1 时，需要将GPIO_28拉高，才能正常使用I2C1，否则I2C1 会被拉低，导致I2C1 无法正常工作。
3. 使用7258 demo板 两块板子进行板间通信，master 模式需要修改sdk ：bk_avdk_smp/ap/middleware/soc/common/hal/i2c_hal.c 下的bk_err_t i2c_hal_configure(i2c_hal_t *hal, const i2c_config_t *cfg)函数，将函数中的i2c_ll_enable_addr_output(&hal->hw, hal->id); 和 i2c_ll_enable_data_output(&hal->hw, hal->id); 注释掉。这部分是增强 i2c 驱动能力的配置，有些设备可能会导致ack失败, 将其关闭提高Master稳定性。

## 关键函数

``` C
    #include <driver/i2c.h>
    #define I2C_SLAVE_ADDR          0x5A
    #define I2C_ID                  I2C_ID_0
    #define I2C_EXAMPLE_DATA_SIZE   255
    #define I2C_READ_TIMEOUT_MS     1000
    #define I2C_WRITE_TIMEOUT_MS    1000
``` 
### 初始化I2C master
``` C
static int i2c_master_init()
{
    int ret = BK_OK;
    // 初始化I2C驱动
    bk_i2c_driver_init();
    // 初始化I2C配置
    i2c_config_t i2c_cfg = {0};
    i2c_cfg.baud_rate = 400000;
    i2c_cfg.addr_mode = I2C_ADDR_MODE_7BIT;
    i2c_cfg.slave_addr = I2C_SLAVE_ADDR;
    ret = bk_i2c_init(I2C_ID, &i2c_cfg);
    if (ret != BK_OK) {
        LOGE("i2c(%d) init failed, ret = %d\n", I2C_ID, ret);
        return ret;
    }
    LOGI("i2c(%d) init\n", I2C_ID);
    return ret;
}
``` 
### 发送数据
``` C
static int i2c_master_example_send()
{
    int ret = BK_OK;
    uint8_t data_buf[I2C_EXAMPLE_DATA_SIZE] = {0};
    for (uint32_t i = 0; i < I2C_EXAMPLE_DATA_SIZE; i++)
    {
        data_buf[i] = i & 0xff;
    }
    ret = bk_i2c_master_write(I2C_ID, I2C_SLAVE_ADDR, data_buf, I2C_EXAMPLE_DATA_SIZE, I2C_WRITE_TIMEOUT_MS);
    if (ret != BK_OK) {
        LOGE("i2c(%d) master write failed, ret = %d\n", I2C_ID, ret);
        return ret;
    }
    return ret;
}
``` 
### 接收数据
``` C
static int i2c_master_example_recv()
{
    int ret = BK_OK;
    uint8_t recv_buf[I2C_EXAMPLE_DATA_SIZE] = {0};
    ret = bk_i2c_master_read(I2C_ID, I2C_SLAVE_ADDR, recv_buf, I2C_EXAMPLE_DATA_SIZE, I2C_READ_TIMEOUT_MS);
    if (ret != BK_OK) {
        LOGE("i2c(%d) master read failed, ret = %d\n", I2C_ID, ret);
        return ret;
    }
    for (uint32_t i = 0; i < I2C_EXAMPLE_DATA_SIZE; i++) {
        LOGI("i2c_master read 0x%x,\n", recv_buf[i]);
    }
    return ret;
}
```

## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/peripherals/i2c/master_mode/
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/peripherals/i2c/master_mode/build/bk7258/master_mode/package/all-app.bin；
- 准备两块开发板，一块作为主设备(编译烧录 sample_project/peripherals/i2c/master_mode)，一块作为从设备(编译烧录 sample_project/peripherals/i2c/slave_mode). 本示例使用了I2C0 作为通信接口。

- I2C引脚连接：
  - SCL：时钟信号线（P20脚），连接到从设备的SCL引脚（P20脚）
  - SDA：数据信号线（P21脚），连接到从设备的SDA引脚（P21脚）
  - GND：地引脚，连接到从设备的GND引脚
- 启动测试：
  - 主机先上电，从机后上电
- 观察日志：
  - 主设备串口输出日志显示测试进度和结果
  - 从设备串口输出日志显示接收到的数据
- 检查数据：
  - 验证主设备发送的数据是否与从设备接收到的数据一致
  - 检查数据传输是否符合预期
以下是上电运行，上面操作后的日志打印：

```
ap0:hal:W(440):gpio: 28 is used.Please confirm unmap isn't impact is working module.!

ap0:i2c:D(441):I2C(0) init ok, baud_rate:400000             // 主机I2C0初始化
ap0:i2c_slav:I(441):i2c(0) init                         

ap0:i2c_slav:I(1029):i2c_master read 0x0,                   // 主机I2C0读取从机I2C0数据
ap0:i2c_slav:I(1029):i2c_master read 0x1,
ap0:i2c_slav:I(1029):i2c_master read 0x2,
ap0:i2c_slav:I(1030):i2c_master read 0x3,
ap0:i2c_slav:I(1030):i2c_master read 0x4,
ap0:i2c_slav:I(1030):i2c_master read 0x5,
ap0:i2c_slav:I(1030):i2c_master read 0x6,
ap0:i2c_slav:I(1030):i2c_master read 0x7,
ap0:i2c_slav:I(1030):i2c_master read 0x8,
ap0:i2c_slav:I(1031):i2c_master read 0x9,
......
ap0:i2c_mast:I(3090):/home/qintao.shen/work/7258_smp/sdk_sample_project/sample_project/peripherals/i2c/master_mode/ap/ap_main.c:108:i2c_master_task:PASS
```

## 更多I2C开发指南请参考 [技术文档](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/examples/peripheral/bk_i2c.html)。

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

# lvgl V8.3.9 显示jpeg硬解码预先解的图片

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 相关章节
 - [bk_vfs文件系统](/storage/internal_flash_littlefs/README.md)
 - [jpeg硬解输出yuv](/peripherals/jpeg/hardware_decode/decode_jpg_to_yuv422/README.md)
 - [jpeg硬解输出rgb565小端](/peripherals/jpeg/hardware_decode/decode_jpg_to_rgb565_little_endian/README.md)
 - [jpeg硬解输出rgb565大端](/peripherals/jpeg/hardware_decode/decode_jpg_to_rgb565_big_endian/README.md)
 - [lvgl如何显示jpeg图片](/multimedia/lvgl/V8.X/jpeg_pre_hw_display/README.md)

## 概述

> **备注** ：
1、屏内部如果有控制器IC，如SPI或者QSPI接口屏，也可修改初始指令（常见的ST7789、ILI9341等，一般为0x36)，由控制器IC旋转，实现UI显示的旋转；
2、BK7258旋转，内存使用方面，lvgl的drawbuffer比不旋转会多申请一块sram（sdk默认是屏幕十分之一大小），另外因是通过软件算法旋转，帧率对比不旋转，大致会有20%的帧率下降；

当屏物理刷屏的方向与UI显示方向不一致时，如屏物理是480X800的竖屏，而UI显示需要显示为800X480的横屏，则可参考此章节，由BK7258的芯片端将UI绘制的内存转换为物理刷屏的内存排序，实现UI的旋转。

本示例是是基于 **[lvgl如何提前硬解显示一张图片](/multimedia/lvgl/V8.X/jpeg_pre_hw_decode_display/README.md)**，直接修改lvgl初始化的旋转参数，模拟实现物理横屏，UI竖屏逆时针90度旋转显示一张图片：
1. 如何挂载文件系统；
2. 提前预解码图片；
3. 如何打开lcd；
4. lvgl初始化；
5. lvgl显示预解码图片；

## 硬件环境

- 核心板：BK7258_QFN88_V3
- 显示屏：st7701s 480X480 RGB屏幕

> **备注** ：如果更换为其它屏幕，需修改dev_lcd.c中的屏幕参数: rgb_config里的屏的lcd_device，相关io管脚：ldo、背光、复位、cs、clk、sda；

![bk_vfs](/multimedia/boot_anim/v3_demo_board.jpg)

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

**文件系统配置：**
| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_VFS          | bk_vfs使能配置，配置为y                                         |
| CONFIG_VFS_MAX_FD_NUM    | bk_vfs同时打开文件的最大个数，如32         |
| CONFIG_LITTLEFS | littlefs使能配置，配置为y |

**硬解码配置：**
| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_JPEG_DECODER          | jpeg硬解码使能配置，配置为y                                         |
| CONFIG_JPEG_DECODER_HW    | jpeg硬解码使能配置，配置为y         |
| CONFIG_JPEGENC_HW | jpeg硬编码使能配置，硬解码有接口依赖，配置为y |
| CONFIG_DMA2D | bk7258 jpeg硬解码只能输出yuyv格式，如果需要通过dma2d将yuyv格式数据转换为rgb565格式，需要配置为y |

**lcd以及lvgl配置：**
| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_LCD | lcd 使能配置，需要配置为y |
| CONFIG_LCD_ST7701S | st7701s屏驱使能配置，配置为y，换其它屏配置为n |
| CONFIG_LVGL | lvgl使能配置，配置为y |
| CONFIG_LVGL_V8 | lvgl V8版本使能配置，配置为y |
| CONFIG_LV_COLOR_DEPTH | lvgl 颜色深度，依据屏驱输出fmt配置，如屏驱输出fmt为RGB565，则配置为16 |
| CONFIG_LVGL_FRAME_BUFFER_NUM | 刷屏的frame buffer数量，默认2个，用于解决lvgl RGB和MCU屏切屏问题，有内存的SPI或QSPI屏可以配置为1|
| CONFIG_LV_FRAME_DMA2D_COPY | DMA2D拷贝使能配置，刷屏的frame buffer数量为2时，用于两个buffer同步的拷贝方式，默认配置为y |
| CONFIG_LV_IMG_UTILITY_CUSTOMIZE | bk自定义lvgl jpeg提前软解和硬解接口，配置为y |

## 关键代码
``` c
    typedef enum {
        ROTATE_NONE = 0,  /**< 不旋转 */
        ROTATE_90,        /**< 逆时针旋转90度 */
        ROTATE_180,       /**< 旋转180度 */
        ROTATE_270,       /**< 逆时针旋转270度 */
    } media_rotate_t;

    //lvgl初始化
    lvgl_app_init(dev_lcd_get_device_handle(),
                    dev_lcd_get_width(), 
                    dev_lcd_get_height(), 
                    ROTATE_90);   //旋转90度
```

## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/multimedia/lvgl/V8.X/rotate_lcd_display
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录工程build下的all-app.bin；
- sample_project/tools/littlefs/bk.bin烧录到user_config分区，即0x003da000地址；
- 上电后，lcd屏上会逆时针旋转90度显示图片（左下角为原点）；

以下是上电运行后的关键串口log 输出：

```
[2025-12-13 11:17:04.304]  ap0:main:I(501):[_mount_fs][51] mount ret:0, start_addr:0x3da000, flash_size:0x100000  //文件系统挂载成功
[2025-12-13 11:17:04.412]  ap0:rgb_ctlr:D(508):rgb_display_ctlr_open 387 lcd 'st7701s' 480 X 480     //打开lcd屏
[2025-12-13 11:17:04.810]  ap0:lvgl:D(1077):lv_vendor_init complete      //lvgl初始化成功
[2025-12-13 11:17:04.810]  ap1:lvgl:I(1086):[dev_lcd_backlight_ctrl][80] on  //背光开启
```

## 常见问题

### 图片没有显示

sdk 3.1.1.5版本及之前版本，lv_jpeg_hw_decode_init和lv_jpeg_hw_decode_deinit有问题，接口默认都返回失败，导致解码失败了，可将接口改为返回成功或者升级sdk到3.1.1.6及以上版本。

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

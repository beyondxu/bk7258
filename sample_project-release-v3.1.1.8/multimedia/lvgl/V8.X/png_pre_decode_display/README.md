# lvgl V8.3.9 显示预解码的png图片

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 相关章节
 - [bk_vfs文件系统](/storage/internal_flash_littlefs/README.md)
 - [jpeg硬解输出yuv](/peripherals/jpeg/hardware_decode/decode_jpg_to_yuv422/README.md)
 - [jpeg硬解输出rgb565小端](/peripherals/jpeg/hardware_decode/decode_jpg_to_rgb565_little_endian/README.md)
 - [jpeg硬解输出rgb565大端](/peripherals/jpeg/hardware_decode/decode_jpg_to_rgb565_big_endian/README.md)
 - [lvgl如何提前软解显示jpeg图片](/multimedia/lvgl/V8.X/jpeg_pre_sw_decode_display/README.md)

## 概述

本示例是利用lvgl V8.3.9版本，显示预解码的一张png图片，包含以下功能：
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
| CONFIG_DMA2D | dma2d使能配置，双buffer dma2d拷贝需要，需要配置为y |
| CONFIG_LV_IMG_UTILITY_CUSTOMIZE | bk自定义lvgl jpeg提前软解和硬解接口，配置为y |

## 修改lvgl头文件支持PNG

### 拷贝sdk lv_conf.h到应用工程

**支持png解码需要修改lv_conf.h**，为避免修改SDK，可以将SDK_DIR/ap/components/lvgl/lvgl_v8/lv_conf.h，按sdk文件夹结构拷贝到工程根目录下，可参考png_pre_decode_display/sdk_patch文件夹，然后lv_conf.h 以下配置改为1：

``` c
    /*PNG decoder library*/
    #define LV_USE_PNG 1
    #if LV_USE_PNG
        #define LV_PNG_USE_PSRAM 1          /* 0: use default sram memory, 1: use psram memory. */
    #endif
```

### 根目录Makefile加入shell命令覆盖sdk lv_conf.h

在根目录Makefile加入以下shell命令，覆盖sdk lv_conf.h：
``` shell
SDK_DIR ?= $(abspath ../../../bk_avdk_smp)

# 拷贝sdk_patch文件到sdk目录
$(if $(shell cp -vfr sdk_patch/*  $(SDK_DIR)/ 2>&1),,$(error "Failed to copy sdk_patch files"))

PROJECT_MAKE_FILE := $(SDK_DIR)/tools/build_tools/build_files/project_main.mk
ifeq ($(wildcard $(PROJECT_MAKE_FILE)),)
    $(error "$(PROJECT_MAKE_FILE) not exist, please check sdk directory.")
endif
include $(PROJECT_MAKE_FILE)
``` 

## 相关接口

png解码接口如下， 位于lv_img_utility.c文件中：
``` c
    //解码文件系统的png
    bk_err_t lv_png_img_load(char *filename, lv_img_dsc_t *img_dst);

    //释放png解码占用的内存
    void lv_img_decode_unload(lv_img_dsc_t *img_dst);
```

## 相关代码
``` c
    //文件系统初始化
    if(BK_OK != _mount_fs(VFS_MOUNT_PATH))
    {
        LOGE("[%s][%d] fs mount fail\r\n", __FUNCTION__, __LINE__);
        TEST_FAIL();
        return BK_FAIL;
    }

    dev_lcd_open();                    //打开lcd

    //lvgl初始化
    lvgl_app_init(dev_lcd_get_device_handle(),
                    dev_lcd_get_width(), 
                    dev_lcd_get_height(), 
                    ROTATE_NONE);

    /* png解码必须放lvgl初始化后，因为png解码是依赖于lvgl的png解码*/
    ui_resource_preload();             //提前预加载ui图片等资源

    // 第一个ui界面加载，因为这段代码运行不属于lvgl线程，所以需要加互斥锁
    lv_vendor_disp_lock();
    ui_screen1_init();                 //首界面初始化
    lv_disp_load_scr(ui_Screen1);      //显示首界面
    lv_vendor_disp_unlock();

    rtos_delay_milliseconds(100);      //加点延时，让ui绘制完
    dev_lcd_backlight_ctrl(100);       //延后开背光，规避屏内部初始化花屏等问题
```

## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/multimedia/lvgl/V8.X/png_pre_decode_display
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录工程build下的all-app.bin；
- sample_project/tools/littlefs/bk.bin烧录到user_config分区，即0x003da000地址；
- 上电后，lcd屏上会显示黑色背景，中间一个蓝色小圆形中间显示png的图片；

以下是上电运行后的关键串口log 输出：

```
[2025-12-13 11:17:04.304]  ap0:main:I(501):[_mount_fs][51] mount ret:0, start_addr:0x3da000, flash_size:0x100000  //文件系统挂载成功
[2025-12-13 11:17:04.412]  ap0:rgb_ctlr:D(508):rgb_display_ctlr_open 387 lcd 'st7701s' 480 X 480     //打开lcd屏
[2025-12-13 11:17:04.810]  ap0:lvgl:D(1077):lv_vendor_init complete      //lvgl初始化成功
[2025-12-13 11:17:04.810]  ap1:lvgl:I(1086):[dev_lcd_backlight_ctrl][80] on  //背光开启
```

## 常见问题

### 图片没有显示

png解码是依赖于lvgl的png解码，所以必须在lvgl初始化后调用lv_png_img_load接口，否则会解码失败。

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

# lvgl V8.3.9 显示jpeg/png路径的图片

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 相关章节
 - [bk_vfs文件系统](/storage/internal_flash_littlefs/README.md)
 - [jpeg硬解输出yuv](/peripherals/jpeg/hardware_decode/decode_jpg_to_yuv422/README.md)
 - [jpeg硬解输出rgb565小端](/peripherals/jpeg/hardware_decode/decode_jpg_to_rgb565_little_endian/README.md)
 - [jpeg硬解输出rgb565大端](/peripherals/jpeg/hardware_decode/decode_jpg_to_rgb565_big_endian/README.md)
 - [lvgl V8.3.9 显示jpeg硬解码预先解的图片](/multimedia/lvgl/V8.X/jpeg_pre_hw_decode_display/README.md)

## 概述

sdk lvgl内部已适配了bk_vfs虚拟文件系统，jpeg、png、gif图片可以直接传入路径给lvgl，由lvgl内部的解码库完成图片的解码显示。因为传入图片路径的方式显示，会存在读flash和解码的时间消耗，所以一般需配合lvgl的 **[图片cache](https://docs.lvgl.io/8.4/overview/image.html#cache-size)** 使用，第一次图片加载后，lvgl内部会依据lifetime决定图片是否释放。

帧率方面，提前预解码显示，帧率是最快的。一般建议大图提前预解码，小图用传路径+cache的方式。以下两种实现方式的对比，用户可根据实际场景选择合适的解码方式。
| 图片显示方式 | 优点 | 缺点 | 
| --------- |  ------- |  ------- |
| 传入路径显示 | 不需要用户管理图片的内存，由lvgl 内部cache管理 | 1、第一次显示需要解码；<br>2、**jpeg图片lvgl没有缓存，每次显示都需要解码**；<br>3、超出lifetime释放后，再次显示需要重新解码； |
| jpg、png提前预解码显示 | 没有解码时间消耗，直接显示 | 需用户依据ui页面的优先级，管理对应ui图片的预加载和释放 | 

本示例是利用lvgl V8.3.9版本，显示预解码的一张jpeg图片，包含以下功能：
1. 如何挂载文件系统；
2. 如何打开lcd；
3. lvgl初始化；
4. lvgl显示jpeg和png路径图片；

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

**硬解码配置：** 本示例以下不需要开启，实际项目大图需要提前预解码，需要根据实际使用开启：
| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_JPEG_DECODER          | jpeg硬解码使能配置，配置为y                                         |
| CONFIG_JPEG_DECODER_HW    | jpeg硬解码使能配置，配置为y         |
| CONFIG_JPEGENC_HW | jpeg硬编码使能配置，硬解码有接口依赖，配置为y |

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
| CONFIG_LV_IMG_UTILITY_CUSTOMIZE | bk自定义lvgl jpeg提前软解和硬解接口，此示例工程不需要，配置为n |

## 修改lvgl头文件支持JPEG、PNG

### 拷贝sdk lv_conf.h到应用工程

**支持png解码需要修改lv_conf.h**，为避免修改SDK，可以将SDK_DIR/ap/components/lvgl/lvgl_v8/lv_conf.h，按sdk文件夹结构拷贝到工程根目录下，可参考picture_direct_display/sdk_patch文件夹，然后lv_conf.h 以下配置改为1：

``` c
    /*PNG decoder library*/
    #define LV_USE_PNG 1
    #if LV_USE_PNG
        #define LV_PNG_USE_PSRAM 1          /* 0: use default sram memory, 1: use psram memory. */
    #endif

    /*BMP decoder library*/
    #define LV_USE_BMP 0

    /* JPG + split JPG decoder library.
    * Split JPG is a custom format optimized for embedded systems. */
    #define LV_USE_SJPG 1
    #if LV_USE_SJPG
        #define LV_SJPG_USE_PSRAM 1          /* 0: use default sram memory, 1: use psram memory. */
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

## 相关代码
``` c
    void ui_screen1_init(void)
    {
        ui_Screen1 = lv_obj_create(NULL);
        
        lv_obj_clear_flag( ui_Screen1, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
        lv_obj_set_style_radius(ui_Screen1, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_Screen1, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT );
        lv_obj_set_style_bg_opa(ui_Screen1, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
        lv_obj_set_width(ui_Screen1, LV_HOR_RES);
        lv_obj_set_height(ui_Screen1, LV_VER_RES);

        lv_obj_t *ui_img = lv_img_create(ui_Screen1);
        lv_img_set_src(ui_img, VFS_MOUNT_PATH"/img/yuv422/example_jpg422.jpg");     //显示jpeg图片
        lv_obj_set_width( ui_img, LV_SIZE_CONTENT);  /// 1
        lv_obj_set_height( ui_img, LV_SIZE_CONTENT);   /// 1
        lv_obj_set_x( ui_img, 0 );
        lv_obj_set_y( ui_img, 0 );
        lv_obj_add_flag( ui_img, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
        lv_obj_clear_flag( ui_img, LV_OBJ_FLAG_SCROLLABLE );    /// Flags

        lv_obj_t *ui_icon = lv_img_create(ui_Screen1);
        lv_img_set_src(ui_icon, VFS_MOUNT_PATH"/img/icon.png");                     //显示png图片
        lv_obj_set_width( ui_icon, LV_SIZE_CONTENT);  /// 1
        lv_obj_set_height( ui_icon, LV_SIZE_CONTENT);   /// 1
        lv_obj_set_align( ui_icon, LV_ALIGN_CENTER);
        lv_obj_add_flag( ui_icon, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
        lv_obj_clear_flag( ui_icon, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
    }

```

## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/multimedia/lvgl/V8.X/picture_direct_display
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录工程build下的all-app.bin；
- sample_project/tools/littlefs/bk.bin烧录到user_config分区，即0x003da000地址；
- 上电后，lcd屏上会显示一张全屏背景图片，居中显示蓝色小圆形图标；

以下是上电运行后的关键串口log 输出：

```
[2025-12-13 11:17:04.304]  ap0:main:I(501):[_mount_fs][51] mount ret:0, start_addr:0x3da000, flash_size:0x100000  //文件系统挂载成功
[2025-12-13 11:17:04.412]  ap0:rgb_ctlr:D(508):rgb_display_ctlr_open 387 lcd 'st7701s' 480 X 480     //打开lcd屏
[2025-12-13 11:17:04.810]  ap0:lvgl:D(1077):lv_vendor_init complete      //lvgl初始化成功
[2025-12-13 11:17:04.810]  ap1:lvgl:I(1086):[dev_lcd_backlight_ctrl][80] on  //背光开启
```

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

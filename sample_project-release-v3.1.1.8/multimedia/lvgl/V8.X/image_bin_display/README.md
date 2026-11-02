# lvgl V8.3.9 如何显示解码图片bin

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 相关章节
 - [lvgl 图片解码.c如何保存为bin文件](/tools/lvgl_C_file_to_image_bin/README.md)
 - [bk_vfs littlefs文件系统](/storage/internal_flash_littlefs/README.md)
 - [lcd如何显示jpeg图片](/peripherals/lcd/rgb666/display_jpg/README.md)
 - [lvgl V8.3.9 显示jpeg软解码的图片片](/multimedia/lvgl/V8.X/jpeg_pre_sw_decode_display/README.md)

## 概述

lvgl图片解码显示，一般常用方法是将图片放到文件系统，然后解码显示。这种方式在png图片多，并且图片大时，解码耗费的时间比较长，会导致开机时间久（png一张480X480图片实时解码需要几百ms），这种方式可以考虑将png图片解码为bin文件，省去运行时解码的耗时。

图片解码bin使用步骤大致如下：
1. 参考 **[lvgl 图片解码.c如何保存为bin文件](/tools/lvgl_C_file_to_image_bin/README.md)** ，将图片解码为bin文件；
2. 参考 **[bk_vfs littlefs文件系统](/storage/internal_flash_littlefs/README.md)** ，将图片解码bin文件放到文件系统；
3. ui_resource.c和头文件中添加图片解码bin信息到g_ui_resource_infor数组；
4. 根据情况修改ui_resource_preload，看哪些图片需提前加载到psram，哪些图片在运行时才加载；
5. ui代码调用ui_resource_get_image_data获取图片解码bin数据；

本示例是利用lvgl V8.3.9版本，显示文件系统一张jpeg、一张png的解码bin文件，包含以下功能：
1. 如何挂载文件系统；
2. 从flash读取图片解码bin，并存到psram；
3. 如何打开lcd；
4. lvgl初始化；
5. lvgl显示图片；

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
| CONFIG_DMA2D | CONFIG_LV_FRAME_DMA2D_COPY开启，需要配置为y |

## 相关接口
``` c
    /**
     * @brief 根据图片名称获取LVGL图片数据结构
     * 
     * 通过图片名称查找对应的图片资源，并返回LVGL图片数据结构指针，用于在LVGL界面中显示图片。
     * 
     * @param image_name 图片名称，用于标识要获取的图片资源
     * @return 成功返回LVGL图片数据结构指针，失败返回NULL
     */
    lv_img_dsc_t *ui_resource_get_image_data(const char *image_name);

    /**
     * @brief 根据图片名称释放LVGL图片数据结构占用的内存
     * 
     * 通过图片名称查找对应的图片资源，并释放LVGL图片数据结构占用的内存。
     * 
     * @param image_name 图片名称，用于标识要释放的图片资源
     * @return 成功返回BK_OK，失败返回BK_FAIL
     */
    s32 ui_resource_free_image_data(const char *image_name);

    /**
     * @brief 预加载ui需显示的图片，后续ui用时，不用再从flash读取装载psram，可提升帧率
     * @return 成功返回BK_OK
     */
    s32 ui_resource_preload(void);
```

## 相关代码
``` c
    // 每个元素包含图片名称、像素格式、尺寸、文件路径和LVGL图片数据结构
    ui_resource_infor_t g_ui_resource_infor[] = {
        {IMG_BACKGROUND_NAME,      PIXEL_FMT_RGB565_LE, 480, 480, VFS_MOUNT_PATH"/img_bin/1_jpg.bin",      {{0}}},
        {IMG_ICON_NAME,            PIXEL_FMT_ARGB8888,  100, 100, VFS_MOUNT_PATH"/img_bin/icon_png.bin",   {{0}}},
    };

    //文件系统初始化
    if(BK_OK != _mount_fs(VFS_MOUNT_PATH))
    {
        LOGE("[%s][%d] fs mount fail\r\n", __FUNCTION__, __LINE__);
        TEST_FAIL();
        return BK_FAIL;
    }

    ui_resource_preload();             //提前预加载ui图片等资源
    
    dev_lcd_open();                    //打开lcd

    //lvgl初始化
    lvgl_app_init(dev_lcd_get_device_handle(),
                    dev_lcd_get_width(), 
                    dev_lcd_get_height(), 
                    ROTATE_NONE);

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
cd sample_project/multimedia/lvgl/V8.X/image_bin_display
vim Makefile  //修改SDK_DIR为实际smp sdk路径，默认已配置好了
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录工程build下的all-app.bin；
- sample_project/multimedia/lvgl/V8.X/image_bin_display/tools/littlefs/bk.bin烧录到user_config分区，即0x003da000地址；
- 上电后，lcd屏上会显示背景图片，中间显示png图片；

以下是上电运行后的关键串口log 输出：

```
[2025-12-18 10:06:35.493]  ap0:main:I(528):[_mount_fs][51] mount ret:0, start_addr:0x3da000, flash_size:0x100000  //文件系统挂载成功
[2025-12-18 10:06:36.931]  ap0:res:I(1981):[ui_resource_preload][130] image 1.jpg pre load success, width:480, height:480   //1.jpg图片解码bin预加载成功
[2025-12-18 10:06:37.040]  ap0:res:I(2121):[ui_resource_preload][130] image icon.png pre load success, width:100, height:100   //icon.png图片解码bin预加载成功
[2025-12-18 10:06:37.148]  ap0:rgb_ctlr:D(2125):rgb_display_ctlr_open 387 lcd 'st7701s' 480 X 480      //打开lcd屏
[2025-12-18 10:06:37.648]  ap0:lvgl:D(2692):lv_vendor_init complete      //lvgl初始化成功
[2025-12-18 10:06:37.648]  ap1:lvgl:I(1086):[dev_lcd_backlight_ctrl][80] on  //背光开启
```

## 常见问题

### 图片没有显示

sdk 3.1.1.5版本及之前版本，lv_jpeg_hw_decode_init和lv_jpeg_hw_decode_deinit有问题，接口默认都返回失败，导致解码失败了，可将接口改为返回成功或者升级sdk到3.1.1.6及以上版本。

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

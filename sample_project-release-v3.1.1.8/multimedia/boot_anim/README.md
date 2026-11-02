# jpeg开机动画显示

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 相关章节

 - [bk_vfs文件系统](/storage/internal_flash_littlefs/README.md)
 - [jpeg硬解输出yuv](/peripherals/jpeg/hardware_decode/decode_jpg_to_yuv422/README.md)
 - [jpeg硬解输出rgb565小端](/peripherals/jpeg/hardware_decode/decode_jpg_to_rgb565_little_endian/README.md)
 - [jpeg硬解输出rgb565大端](/peripherals/jpeg/hardware_decode/decode_jpg_to_rgb565_big_endian/README.md)

## 硬件环境

- 核心板：BK7258_QFN88_V3
- 显示屏：st7701s 480X480 RGB屏幕

> **备注** ：如果更换为其它屏幕，需修改dev_lcd.c中的屏幕参数: rgb_config里的屏的lcd_device，相关io管脚：ldo、背光、复位、cs、clk、sda。另外如果lcd分辨率不是480X480，也需按照硬解要求，修改sample_project/multimedia/boot_anim/tools/littlefs/bk/anim下的图片；

![bk_vfs](/multimedia/boot_anim/v3_demo_board.jpg)

## 概述

本示例是将**内部flash的user_config分区**作为littlefs分区，结合sample_project/components下的jpeg_file_animation和hw_jpeg_decode模块，实现以下功能：
1. 如何挂载文件系统；
2. 如何打开lcd；
3. 如何播放开机动画；

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_VFS          | bk_vfs使能配置，配置为y                                         |
| CONFIG_VFS_MAX_FD_NUM    | bk_vfs同时打开文件的最大个数，如32         |
| CONFIG_LITTLEFS | littlefs使能配置，配置为y |
| CONFIG_JPEG_DECODER          | jpeg硬解码使能配置，配置为y                                         |
| CONFIG_JPEG_DECODER_HW    | jpeg硬解码使能配置，配置为y         |
| CONFIG_JPEGENC_HW | jpeg硬编码使能配置，硬解码有接口依赖，配置为y |
| CONFIG_DMA2D | bk7258 jpeg硬解码只能输出yuyv格式，如果需要通过dma2d将yuyv格式数据转换为rgb565格式，需要配置为y |
| CONFIG_LCD | lcd 使能配置，需要配置为y |
| CONFIG_LCD_ST7701S | lcd st7701s使能配置，测测试demo需要配置为y |

## 依赖模块

- sample_project/components/app_jpeg_hw_decode
- sample_project/components/app_jpeg_file_animation

**根目录下的 CMakeLists.txt** 中需要添加以下内容（放入自己工程需留意自己工程相对components的路径）：

``` cmake
set(EXTRA_COMPONENTS_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../components/app_jpeg_hw_decode
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../components/app_jpeg_file_animation
)
```
**ap/CMakeLists.txt** PRIV_REQUIRES中需加入app_jpeg_hw_decode和app_jpeg_file_animation依赖，这样可直接调用这两个模块头文件：

``` cmake
set(priv_req  media_service multimedia bk_network_provisioning bk_jpeg_decoder app_jpeg_hw_decode app_jpeg_file_animation)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
```

## 关键函数

> **备注** ：jpeg图片及解码后的内存申请和释放，是调用frame_buffer_app.c中的函数，默认是从frame_buffer内存分配，如果psram内存调整为所有都作为堆内存(**[多媒体相关的几个PSRAM大小都配置为0](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/developer-guide/peripheral/bk_psram.html?highlight=psram#id8)**)，可以将frame_buffer_app.c里的USE_PSRAM_MALLOC配置为1，即从psram堆分配内存。

``` c
    s32 boot_animation(void)
    {
        bk_err_t ret = BK_FAIL;
        animation_module_config_t animation_config = {0};

        do{
            //jpeg 硬解初始化
            ret = _jpeg_hw_decode_init();
            if(BK_OK != ret)
            {
                LOGE("[%s][%d] decode init fail\r\n", __FUNCTION__, __LINE__);
                break;
            }

            //动画图片解码器配置参数
            animation_config.jpeg_hw_decode_config.jpeg_decode_handle = g_jpeg_decode_handle;
            animation_config.jpeg_hw_decode_config.jpeg_frame_malloc = jpeg_frame_malloc;
            animation_config.jpeg_hw_decode_config.jpeg_frame_free = jpeg_frame_free;
            animation_config.jpeg_hw_decode_config.decode_frame_malloc = decode_frame_malloc;
            animation_config.jpeg_hw_decode_config.decode_frame_free = decode_frame_free;
            animation_config.jpeg_hw_decode_config.output_width = 0;
            animation_config.jpeg_hw_decode_config.output_height = 0;
            animation_config.jpeg_hw_decode_config.use_dma2d = false;
            animation_config.jpeg_hw_decode_config.byte_swap = false;
            
            animation_config.decode_format = ANIMATION_DECODE_FORMAT_YUV422;  //rgb屏，BK7258硬件支持输入源是yuv格式数据
            animation_config.lcd_cb = _app_lcd_display;                        //刷屏函数
            animation_config.finish_cb = _animation_finish_cb;                                    //动画完成回调

            // 动画模块初始化
            ret = animation_module_init(&animation_config);
            if(BK_OK != ret)
            {
                LOGE("[%s][%d] init fail\r\n", __FUNCTION__, __LINE__);
                break;
            }

            // 播放动画
            LOGI("[%s][%d] boot animation play start\r\n", __FUNCTION__, __LINE__);
            ret = animation_start_play(VFS_MOUNT_PATH"/anim/anim-", 30, 1, false);
            if(BK_OK != ret)
            {
                LOGE("[%s][%d] boot animation fail\r\n", __FUNCTION__, __LINE__);
                break;
            }

            //等播放完成
            g_play_over = false;
            while(!g_play_over)
            {
                rtos_delay_milliseconds(100);
            }
        }while(0);

        _jpeg_hw_decode_deinit();    //jpeg 硬解反初始化
        animation_module_deinit();   //动画模块反初始化

        LOGI("[%s][%d] boot animation play over\r\n", __FUNCTION__, __LINE__);

        return ret;
    }
```

## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/multimedia/boot_anim
vim Makefile  //修改SDK_DIR为实际smp sdk路径
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/multimedia/boot_anim/build/bk7258/boot_anim/package/all-app.bin；
- sample_project/multimedia/boot_anim/tools/littlefs/bk.bin烧录到user_config分区，即0x003da000地址；
- 上电后，会将文件系统anim文件夹下的图片，以动画的方式逐帧解码，并显示在lcd显示；

以下是上电运行后的关键串口log 输出：

```
[2025-12-01 15:52:39.831]  ap1:anim:I(583):[animation_module_init][288] Animation module initialized successfully      //动画模块初始化成功
[2025-12-01 15:52:39.831]  ap1:anim:I(584):[animation_start_play][356] Animation play started: prefix=/littlefs/anim/anim-, frames=30, repeat=1, loop=0            //动画模块播放的参数
[2025-12-01 15:52:39.831]  ap0:hw_img:I(587):[_read_jpeg_file][84] read b 0x60026358, 0x600263a0
[2025-12-01 15:52:39.831]  ap0:hw_img:I(597):[_read_jpeg_file][88] read e
[2025-12-01 15:52:39.937]  ap1:hw_dec:E(616):hw_jpeg_decode_in_complete 124 in_complete is NULL
[2025-12-01 15:52:39.937]  ap0:hw_img:I(617):[jpeg_hw_decode_by_frame_output_yuv422][266] decode /littlefs/anim/anim-0.jpg success, width:480, height:480, length:460800   //解码第一张图片
```

## 常见问题

### spi或qspi屏显示异常

因bk7258硬件在spi或qspi屏刷屏时，不支持格式自动转换，所以jpeg图片需要解码为rgb565格式，另外屏一般是接收大端格式数据。

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

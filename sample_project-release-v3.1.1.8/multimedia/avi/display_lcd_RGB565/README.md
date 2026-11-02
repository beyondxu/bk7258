# MULTIMEDIA AVI播放示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  
## 硬件环境

- 核心板：BK7258_QFN88_V3
- 显示屏：st7701s 480X480 RGB屏幕
- 存储设备：SD卡
- 扬声器小板

> **备注** ：如果更换为其它屏幕，需修改dev_lcd.c中的屏幕参数: rgb_config里的屏的lcd_device，相关io管脚：ldo、背光、复位、cs、clk、sda。另外如果lcd分辨率不是480X480，也需按照硬解要求修改。

## 概述

avilib 是一个开源的轻量级 AVI 播放库，可以基于此库文件实现 avi 视频的录制和播放功能。

本示例基于 avi 组件，演示了sd卡中 AVI 视频播放的实现。它具体实现以下功能：

1. 如何挂载文件系统；
2. 如何打开lcd和音频设备；
3. 如何播放avi视频；

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

**文件系统配置：**
| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_VFS          | bk_vfs使能配置，配置为y                                         |
| CONFIG_VFS_MAX_FD_NUM    | bk_vfs同时打开文件的最大个数，如32         |
| CONFIG_FATFS | fatfs使能配置，配置为y |

**SD卡相关配置：**
| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_FATFS_SDCARD | fatfs sd卡使能配置，配置为y |
| CONFIG_SDIO | sdio使能配置，配置为y |
| CONFIG_SDIO_V2P0 | sdio v2.0使能配置，配置为y |
| CONFIG_SDIO_HOST | sdio host使能配置，配置为y |
| CONFIG_SDIO_HOST_DEFAULT_CLOCK_FREQ | sdio host默认时钟频率，配置为7 |
| CONFIG_SDIO_HOST_CLR_WRITE_INT | sdio host清除写中断使能配置，配置为y |
| CONFIG_SDCARD | sd卡使能配置，配置为y |
| CONFIG_SDCARD_DEFAULT_CLOCK_FREQ | sd卡默认时钟频率，配置为14 |
| CONFIG_SDCARD_POWER_GPIO_CTRL | sd卡电源gpio控制使能配置，配置为y |

**硬解码相关配置：**
| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_JPEG_DECODER          | jpeg硬解码使能配置，配置为y                                         |
| CONFIG_JPEG_DECODER_HW    | jpeg硬解码使能配置，配置为y         |
| CONFIG_JPEGENC_HW | jpeg硬编码使能配置，硬解码有接口依赖，配置为y |
| CONFIG_DMA2D | bk7258 jpeg硬解码只能输出yuyv格式，如果需要通过dma2d将yuyv格式数据转换为rgb565格式，需要配置为y |

**LCD相关配置：**
| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_LCD | lcd 使能配置，需要配置为y |
| CONFIG_LCD_ST7701S | lcd st7701s使能配置，测测试demo需要配置为y |

**AUDIO相关配置：**
| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_ADK | 音频功能宏配置使能，配置为y |
| CONFIG_AUDIO | 音频功能使能配置，配置为y |
| CONFIG_AUDIO_ADC | 音频adc使能配置，配置为y |
| CONFIG_AUDIO_DAC | 音频dac使能配置，配置为y |
| CONFIG_AUDIO_RING_BUFF | 音频环形缓冲区使能配置，配置为y |
| CONFIG_ADK_ONBOARD_MIC_STREAM | 板载麦克风使能配置，配置为y |
| CONFIG_ADK_ONBOARD_SPEAKER_STREAM | 板载扬声器使能配置，配置为y |
| CONFIG_ADK_USE_PSRAM | 是否使用psram作为音频缓存，配置为y |
| CONFIG_ADK_AEC_V3_ALGORITHM | 音频aec v3算法使能配置，配置为y |
| CONFIG_AUD_AI_NS_SUPPORT | 音频ai ns支持使能配置，配置为y |
| CONFIG_ADK_RAW_STREAM | 是否使用原始音频流，配置为y |
| CONFIG_ADK_UART_STREAM | 是否使用uart作为音频流，配置为y |
| CONFIG_ADK_UTILS | 音频DEBUG工具使能配置，配置为y |
| CONFIG_ADK_UART_UTIL | uart工具使能配置，配置为y |
| CONFIG_ADK_COUNT_UTIL | 计数器工具使能配置，配置为y |
| CONFIG_ADK_VFS_UTIL | vfs工具使能配置，配置为y |
| CONFIG_ADK_DEBUG_DUMP_UTIL | 调试_dump工具使能配置，配置为y |

## 依赖模块

- sample_project/components/app_jpeg_hw_decode
- sample_project/components/app_avi_player

**根目录下的 CMakeLists.txt** 中需要添加以下内容（放入自己工程需留意自己工程相对components的路径）：

``` cmake
set(EXTRA_COMPONENTS_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../components/app_jpeg_hw_decode
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../components/app_avi_player
)
```
**ap/CMakeLists.txt** PRIV_REQUIRES中需加入app_jpeg_hw_decode和app_avi_player依赖，这样可直接调用这两个模块头文件：

``` cmake
set(priv_req  media_service multimedia avdk_utils bk_camera app_jpeg_hw_decode app_avi_player)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}"
```

## 关键函数

> **备注** ：jpeg图片及解码后的内存申请和释放，是调用frame_buffer_app.c中的函数，默认是从frame_buffer内存分配，如果psram内存调整为所有都作为堆内存(**[多媒体相关的几个PSRAM大小都配置为0](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/developer-guide/peripheral/bk_psram.html?highlight=psram#id8)**)，可以将frame_buffer_app.c里的USE_PSRAM_MALLOC配置为1，即从psram堆分配内存。

``` c

    int avi_player_test()
    {
        //初始化硬解码
        ret = _jpeg_hw_decode_init();
        if(ret != BK_OK)
        {
            LOGE("[%s][%d] jpeg_hw_decode_init failed: %d\r\n", __func__, __LINE__, ret);
            TEST_FAIL();
            return BK_FAIL;
        }
        
        //挂载SD卡
        ret = sdcard_fatfs_mount();
        if(ret != BK_OK)
        {
            LOGE("[%s][%d] sdcard_source_mount failed: %d\r\n", __func__, __LINE__, ret);
            TEST_FAIL();
            return BK_FAIL;
        }

        //初始化AVI播放器
        avi_player_config_t avi_player_config = {0};
        avi_player_config.jpeg_hw_decode_config.jpeg_decode_handle = g_jpeg_decode_handle;
        avi_player_config.jpeg_hw_decode_config.jpeg_frame_malloc = jpeg_frame_malloc;
        avi_player_config.jpeg_hw_decode_config.jpeg_frame_free = jpeg_frame_free;
        avi_player_config.jpeg_hw_decode_config.decode_frame_malloc = decode_frame_malloc;
        avi_player_config.jpeg_hw_decode_config.decode_frame_free = decode_frame_free;
        avi_player_config.jpeg_hw_decode_config.output_width = 0;
        avi_player_config.jpeg_hw_decode_config.output_height = 0;
        avi_player_config.jpeg_hw_decode_config.use_dma2d = false;
        avi_player_config.jpeg_hw_decode_config.byte_swap = false;
        avi_player_config.decode_format = AVI_PLAYER_DECODE_FORMAT_YUV422;
        avi_player_config.sync_time_threshold_ms = 65;
        avi_player_config.lcd_cb = _lcd_cb;

        avi_player_config.voice_malloc_cb = _voice_malloc_cb;
        avi_player_config.voice_free_cb = _voice_free_cb;
        avi_player_config.speaker_cb = _speaker_cb;
        avi_player_config.audio_frame_size = 640;

        avi_player_config.finish_cb = _finish_cb;
        avi_player_config.user_data = NULL;

        ret = avi_player_init(&avi_player_config);
        if(ret != BK_OK)
        {
            LOGE("[%s][%d] avi_player_init failed: %d\r\n", __func__, __LINE__, ret);
            TEST_FAIL();
            return BK_FAIL;
        }

        //打开AVI文件
        char path[255];
        snprintf(path, sizeof(path), "/test.avi");
        ret = avi_player_file_open(path);
        if(ret != BK_OK)
        {
            LOGE("[%s][%d] avi_player_file_open failed: %d\r\n", __func__, __LINE__, ret);
            TEST_FAIL();
            return BK_FAIL;
        }

        //开始播放AVI文件
        ret = avi_player_start();
        if(ret != BK_OK)
        {
            LOGE("[%s][%d] avi_player_start failed: %d\r\n", __func__, __LINE__, ret);
            TEST_FAIL();
            return BK_FAIL;
        }
        //等待播放结束
        while(!_is_play_end)
        {
            rtos_delay_milliseconds(100);
        }
        avi_player_stop();
        avi_player_deinit();
        _jpeg_hw_decode_deinit();
        sdcard_fatfs_umount();
    }

```

## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/multimedia/avi/display_lcd_RGB565
make clean
make bk7258
```

### 操作方法及日志

- 将sample_project/multimedia/avi/display_lcd_RGB565/test.avi视频文件拷贝到sdcard根目录下，并插在开发板上；
- 参考[如何烧录](/BURN.md)，烧录sample_project/multimedia/avi/display_lcd_RGB565/build/bk7258/display_lcd_RGB565/package/all-app.bin；
- 上电后，会将sdcard根目录下的test.avi视频文件播放出来；

以下是上电运行，上面操作后的日志打印：

```
ap0:sd_card:D(1157):sd card support voltage 2.7-3.6 V
ap0:sd_card:D(1166):card capacity SDHC_SDXC
ap0:sd_card:D(1176):sd card class:0x5b5
ap0:sd_card:D(1184):sdio clock freq:7->14
ap0:D(1188):fmt=2
ap0:D(1189):fmt2=1
ap0:file_sys:I(1190):sdcard mount OK!   //文件系统挂载成功

ap0:OS:D(1190):create avi_player_ctrl, tcb=6072b300, stack=[6072a2d8-6072b2d8:4096], prio=3, xCoreID=-1
ap1:avi-play:D(1191):[avi_player_ctrl_thread][514] msg type0 
ap1:avi-play:I(1191):file_path: /test.avi
ap0:D(1278):idx_type = 1
ap0:avi-play:I(1294):video_frame_num: 5088
ap0:avi-play:I(1294):v_width: 480, v_height: 480, fps: 17.693478
ap0:avi-play:I(1295):compresstype: MJPG
ap0:avi-play:I(1295):frame_time: 20
ap0:avi-play:I(1295):audio_total_bytes: 9205760
ap0:avi-play:I(1295):bit_depth: 16, chans: 1, rate: 16000, 
ap0:avi-play:D(1295):[avi_player_open_file_handle][385] open file success   //avi文件打开成功

ap0:hw_img:D(1358):[_jpeg_hw_decode_to_yuv422][252] hw decode init by user  
ap0:rgb_ctlr:D(1372):display start, frame width, height 480, 480
ap0:hw_img:D(1427):[_jpeg_hw_decode_to_yuv422][252] hw decode init by user
ap0:hw_img:D(1474):[_jpeg_hw_decode_to_yuv422][252] hw decode init by user  //开始播放视频
```

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

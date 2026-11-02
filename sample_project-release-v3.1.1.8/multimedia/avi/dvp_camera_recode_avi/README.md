# MULTIMEDIA AVI录制示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  
## 硬件环境

- 核心板：BK7258_QFN88_V3
- 摄像头：gc2145 
- 显示屏：st7701s 480X480 RGB屏幕
- 存储设备：SD卡
- 音频设备：麦克风

> **备注** ：如果更换为其它屏幕，需修改dev_lcd.c中的屏幕参数: rgb_config里的屏的lcd_device，相关io管脚：ldo、背光、复位、cs、clk、sda。另外如果lcd分辨率不是480X480，也需按照硬解要求修改。

## 概述
avilib 是一个开源的轻量级 AVI 播放库，可以基于此库文件实现 avi 视频的录制和播放功能。

本示例基于 avi 组件，演示了 AVI 视频录制到 SD 卡的实现。它具体实现以下功能：

1. 如何挂载文件系统；
2. 如何打开lcd和音频设备；
3. 如何录制avi视频；

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。
>**注意**：
>1. SDK v3.1.1.6 后 CONFIG_DVP_CAMERA_I2C_ID >= 2 识别为模拟i2c，0 1识别为硬件I2C，本示例使用demo板，使用的是模拟i2c0，dvp摄像头i2c id配置为2。
>2. 使用摄像头要把 CONFIG_SIM_I2C_HW_BOARD_V3 配置为 n， 使能CONFIG_SIM_I2C_HW_BOARD_V3会影响到SIM_I2C_IO的配置。CONFIG_SIM_I2C_HW_BOARD_V3 关闭使用的是CONFIG_SIM_I2C0，开启使用的是CONFIG_SIM_I2C1。

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

**DVP摄像头相关配置：**
| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_YUV_BUF | YUV buffer 配置使能，配置为y，摄像头模块依赖于YUV buffer |
| CONFIG_DVP_CAMERA | dvp摄像头使能配置，配置为y |
| CONFIG_DVP_CAMERA_I2C_ID | dvp摄像头i2c id，配置为2 |
| CONFIG_DVP_CTRL_POWER_GPIO_ID| dvp摄像头电源gpio id，配置为28 |
| CONFIG_DVP_THREAD_STACK_SIZE | dvp摄像头线程栈大小，配置为2048 |
| CONFIG_JPEG_FRAME_SIZE | jpeg帧大小，配置为102400 |
| CONFIG_H264_FRAME_SIZE | h264帧大小，配置为102400 |
| CONFIG_SIM_I2C_HW_BOARD_V3 | 模拟i2c硬件板载v3使能配置，配置为n |
| CONFIG_SIM_I2C0_SDA_GPIO | 模拟i2c0 sda gpio，配置为1 |
| CONFIG_SIM_I2C0_SCL_GPIO | 模拟i2c0 scl gpio，配置为0 |
| CONFIG_SIM_I2C1_SDA_GPIO | 模拟i2c1 sda gpio，配置为5 |
| CONFIG_SIM_I2C1_SCL_GPIO | 模拟i2c1 scl gpio，配置为8 |

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

- sample_project/components/app_avi_record

**根目录下的 CMakeLists.txt** 中需要添加以下内容（放入自己工程需留意自己工程相对components的路径）：

``` cmake
set(EXTRA_COMPONENTS_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../components/app_avi_record
)
```
**ap/CMakeLists.txt** PRIV_REQUIRES中需加入app_avi_record依赖，这样可直接调用这两个模块头文件：

``` cmake
set(priv_req  media_service multimedia avdk_utils bk_camera app_avi_record)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}"
```

## 关键函数

``` c
int avi_record_test()
{
    int ret = BK_OK;
    // 注册 MJPEG 消费者
    frame_queue_v2_register_consumer(IMAGE_MJPEG, CONSUMER_STORAGE);
    /*
    * 功能：配置 AVI 录制节点内存池
    * 参数：true  – 一次性预分配 61440 个节点（约 15 分钟 20 fps 视频）
    *       false – 节点用完后动态 malloc（节省PSRAM，但可能碎片）
    * 默认不调用该函数使用动态分配节点
    * 内存开销：61440 × 16 bytes ≈ 960 KB
    * 录制时长：20 fps 视频（20个nodes） + 每 20 ms 一帧音频（50个node） ≈ 70 节点/秒
    *          61440 ÷ 70 ≈ 877 秒 ≈ 14.6 分钟
    */
    bk_AVI_set_node_pool_mode(1, 12280);      // 如需启用节点池模式，可取消此行注释
    // 配置 AVI 录制参数
    avi_record_config_t avi_record_config = {0};
    avi_record_config.video_enable = true;
    avi_record_config.video_width = 480;
    avi_record_config.video_height = 480;
    avi_record_config.video_fps = 20;
    avi_record_config.video_format = AVI_VIDEO_FORMAT_MJPEG;
    avi_record_config.read_video_frame_cb = jpeg_read;
    avi_record_config.complete_video_frame_cb = jpeg_complete;
        
    avi_record_config.audio_enable = true;
    avi_record_config.audio_chan = 1;
    avi_record_config.audio_sample = 16000;
    avi_record_config.audio_bits = 16;
    avi_record_config.audio_format = AVI_AUDIO_FORMAT_PCM;
    avi_record_config.read_audio_frame_cb = audio_read;
    avi_record_config.complete_audio_frame_cb = audio_complete;
    avi_record_config.record_finish_cb = record_finish_cb;

    ret = avi_record_init(&avi_record_config);
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] avi_record_init failed, ret = %d\r\n", __func__, __LINE__, ret);
        return BK_FAIL;
    }
    // 启动 AVI 录制
    char path[255];
    snprintf(path, sizeof(path), "test.avi");
    ret = avi_record_start(path);
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] avi_record_start failed, ret = %d\r\n", __func__, __LINE__, ret);
        return BK_FAIL;
    }
    while(1)
    {
        // 检查是否录制完成
        if(_avi_record_finish)
        {
            avi_record_stop();
            avi_record_deinit();
            frame_queue_v2_unregister_consumer(IMAGE_MJPEG, CONSUMER_STORAGE);
            break;
        }
        rtos_delay_milliseconds(100);
    }
    return BK_OK;
}

```

## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/multimedia/avi/dvp_camera_recode_avi
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/multimedia/avi/dvp_camera_recode_avi/build/bk7258/dvp_camera_recode_avi/package/all-app.bin；
- 上电后，程序将会自动开启录制avi视频，并显示摄像头数据到屏幕；

以下是上电运行，上面操作后的日志打印：

```
ap0:sd_card:D(1297):sd card support voltage 2.7-3.6 V
ap0:sd_card:D(1306):card capacity SDHC_SDXC
ap0:sd_card:D(1316):sd card class:0x5b5
ap0:sd_card:D(1324):sdio clock freq:7->14
ap0:D(1329):fmt=2
ap0:D(1329):fmt2=1
ap0:file_sys:I(1330):sdcard mount OK!   //文件系统挂载成功

ap0:avi-reco:D(1331):[avi_record_ctrl_thread][356]avi_record_ctrl_thread, msg.msg = 0
ap0:avi-reco:I(1331):record_path = test.avi
ap0:avi-reco:I(1337):AVI_set_video video_format = 1 , video_width = 480, video_height = 480, video_fps = 20.000000
ap0:avi-reco:I(1338):AVI_set_audio success, audio_format = 1 , audio_chan = 1, audio_sample = 16000, audio_bits = 16
ap0:OS:D(1338):create video_thread, tcb=6072d0b0, stack=[6072c088-6072d088:4096], prio=3, xCoreID=-1
ap0:OS:D(1338):create audio_thread, tcb=6075e118, stack=[6075d0f0-6075e0f0:4096], prio=3, xCoreID=-1        //开始录制

[16:10:15.935]收←◆ap0:media_se:D(2526):h264:0[0], dec:0[0], lcd:69[138], lcd_fps:11[22], lvgl:0[0]
ap0:media_se:D(2526):wifi:0[0, 0kbps, 0ms, 0-0], jpg:0KB[0Kbps], h264:0KB[0Kbps]
[16:10:17.935]收←◆ap0:media_se:D(4526):h264:0[0], dec:0[0], lcd:107[352], lcd_fps:18[58], lvgl:0[0]
ap0:media_se:D(4526):wifi:0[0, 0kbps, 0ms, 0-0], jpg:0KB[0Kbps], h264:0KB[0Kbps]
[16:10:18.055]收←◆ap0:count_ut:D(4646):[ONBOARD_MIC] data_size: 127360(Bytes), 31KB/s 
ap0:count_ut:D(4646):[RAW_READ] data_size: 126720(Bytes), 30KB/s 
ap0:count_ut:D(4647):[RAW_WRITE] data_size: 0(Bytes), 0KB/s 
ap0:count_ut:D(4648):[ONBOARD_SPK]:0(Bytes),0KB/s | [FILL_SILENCE]:127360(Bytes),31KB/s 
ap0:count_ut:D(4649):[WIFI_TX] data_size: 126720(Bytes), 30KB/s 
ap0:count_ut:D(4649):[WIFI_RX] data_size: 0(Bytes), 0KB/s 
```


## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

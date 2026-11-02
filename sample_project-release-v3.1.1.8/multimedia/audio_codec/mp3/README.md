# mp3播放示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## bk_audio系统
bk_audio系统通过将audio elements组合为audio pipeline, 提供了一套扩展性极强的音频处理方案。
audio element为基本功能块，可分为输入，处理，输出三大类。输入类是数据包的源，如麦克风、文件读取等；处理类对数据包进行相关处理，如编码、解码、AEC、AGC、EQ等；输出类是数据包的目的地，如扬声器、文件写入等。
audio pipeline是audio elements的容器，从总体上进行运行、暂停、恢复、停止等状态的切换。

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_FREERTOS_USE_QUEUE_SETS          | queue set使能配置，配置为y                                         |
| CONFIG_ADK    | 音频框架使能配置，配置为y         |
| CONFIG_AUDIO | 音频驱动使能配置，配置为y |
| CONFIG_AUDIO_ADC | ADC驱动使能配置，配置为y |
| CONFIG_AUDIO_DAC | DAC驱动使能配置，配置为y |
| CONFIG_AUDIO_RING_BUFF | 音频DMA环形缓冲区使能配置，配置为y |
| CONFIG_ADK_ONBOARD_MIC_STREAM | 板载麦克风功能块使能配置，配置为y |
| CONFIG_ADK_ONBOARD_SPEAKER_STREAM | 板载扬声器功能块使能配置，配置为y |
| CONFIG_ADK_RAW_STREAM | 裸读写功能块使能配置，配置为y |
| CONFIG_VFS          | bk_vfs使能配置，配置为y                                         |
| CONFIG_VFS_MAX_FD_NUM    | bk_vfs同时打开文件的最大个数，如32         |
| CONFIG_FATFS | fatfs使能配置，配置为y |
| CONFIG_FATFS_SDCARD | fatfs操作sdcard使能配置，配置为y |
| CONFIG_SDCARD | sdcard读写使能配置，配置为y |
| CONFIG_SDCARD_POWER_GPIO_CTRL | sdcard电源开关使能配置，配置为y |
| CONFIG_LDO3V3_CTRL_GPIO | sdcard电源开关管脚号配置，配置为gpio 13 |
| CONFIG_ADK_VFS_STREAM | BK_VFS文件读写功能块使能配置，配置为y |
| CONFIG_PLAYER_SERVICE_SUPPORT_VFS_STREAM | PLAYER支持VFS源使能配置，配置为y |
| CONFIG_ADK_ARRAY_STREAM | 数组数据读写功能块使能配置，配置为y |
| CONFIG_PLAYER_SERVICE_SUPPORT_ARRAY_STREAM | PLAYER支持数组源使能配置，配置为y |
| CONFIG_PLAYER_SERVICE | PLAYER功能使能配置，配置为y |
| CONFIG_ADK_MP3_DECODER | MP3解码使能配置，配置为y |
| CONFIG_PLAYER_SERVICE_SUPPORT_MP3_DECODER | PLAYER支持MP3解码使能配置，配置为y |
| CONFIG_ADK_WAV_DECODER | WAV解码使能配置，配置为y |
| CONFIG_PLAYER_SERVICE_SUPPORT_WAV_DECODER | PLAYER支持WAV解码使能配置，配置为y |


## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/multimedia/audio_codec/mp3
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录all-app.bin；
- 在SD卡的根目录下放置一个test.mp3文件, 将SD卡插到板子上；
- 上电后，会自动挂载SD卡，然后播放SD卡里的test.mp3文件。

## 常见问题


## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

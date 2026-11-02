# speaker示例

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


## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/peripherals/audio/speaker
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录all-app.bin；
- 上电后，会自动通过板载扬声器播放一段蜂鸣音。

## 常见问题


## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

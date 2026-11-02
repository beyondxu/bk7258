# ASR KWS 关键词识别 示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 概述

本示例基于 ASR 组件，演示了 ASR 关键词识别功能。它具体展示了如何：
1. 配置 ASR 为关键词识别模式，选择识别关键词；
2. 启动 ASR 识别并等待关键词识别结果；
3. 打印识别结果；

## 依赖组件及相关配置

- `bk_kws` - RiseLink_R01_V3.0算法组件，用于关键词识别
- `bk_app_asr` - ASR 组件，用于关键词识别，作为算法组件的适配层
- `bk_voice_service` - 语音服务组件，用于管理ASR组件的初始化、反初始化、识别等操作
- `bk_asr_service` - SDK内部ASR 服务组件，用于管理ASR组件的识别等操作，内部负责将voice模块和asr模块串联
- `media_service` - 媒体服务组件，用于管理音频数据的采集、处理、播放等操作
- `multimedia` - 多媒体组件，用于管理音频数据的采集、处理、播放，psram分配等操作

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

**ASR 配置**
| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_KWS          | kws 组件使能配置，配置为y                                         |
| CONFIG_ASR_SERVICE    | asr 组件使能配置，配置为y         |
| CONFIG_ASR_SERVICE_USE_PSRAM | asr 组件使能配置，使用 psram 堆申请内存，配置为y |
| CONFIG_ASR_SERVICE_NO_MIC | asr 组件使能配置，默认配置为y |

## 相关代码

启动音频模块并注册asr回调
``` C
static void _asr_result_callback(uint32_t param)
{
    char *result = (char *)param;
    if (result) 
    {
        LOGI("[%s][%d] result: %s\r\n", __func__, __LINE__, result);
    } 
    else 
    {
        LOGI("[%s][%d] result is NULL\r\n", __func__, __LINE__);
    }
}

int bk_kws_demo_init(void)
{
    int ret = 0;

    app_audio_config_t cfg = {0};
    cfg.pa_ctrl_gpio = GPIO_44;
    cfg.pa_on_level = 1;

    cfg.dec_type = AUDIO_DEC_TYPE_PCM;
    cfg.spk_sample_rate = SPK_SAMPLE_RATE;
    cfg.spk_frame_duration = SPK_FRAME_DURATION;
    cfg.enc_type = AUDIO_ENC_TYPE_PCM;
    cfg.mic_sample_rate = MIC_SAMPLE_RATE;
    cfg.mic_rx_cb = _mic_data_callback;
#if CONFIG_BEKEN_KWS
    cfg.asr_frame_size = 1280; // Beken KWS asr 一帧 40ms，16000*40/1000*2=1280
#endif
    cfg.asr_result_cb = _asr_result_callback;
    ret = bk_app_audio_init(&cfg);
    if (BK_OK != ret) 
    {
        LOGE("app voice init fail, ret: %d\r\n", ret);
        return ret;
    }

    return ret;
}
```

写入认证信息到内置flash
``` C
    asr_auth_t auth = {0};
    os_strncpy(auth.id, argv[1], sizeof(auth.id));
    os_strncpy(auth.key, argv[2], sizeof(auth.key));
    
    int ret = bk_app_asr_set_auth(&auth);
    if (ret != BK_OK) 
    {
        LOGE("bk_app_asr_set_auth fail, ret: %d\r\n", ret);
    } 
    else 
    {
        LOGI("bk_app_asr_set_auth success\r\n");
    }
```



## 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //如果Makefile中未指定SDK_DIR，或者路径错误，设置smp sdk路径到环境变量
cd sample_project/asr/kws_demo
make clean
make bk7258
```

## 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/asr/kws_demo/build/bk7258/kws_demo/package/all-app.bin；<br>
- 如果是新设备，需要先认证ASR组件，测试步骤：<br>
    - 打开串口，输入`ap_cmd asr_auth <auth_id> <auth_key>`写入到flash中， auth_id 为购买的app_id， auth_key 为购买的app_key；<br>
    - 重启设备，输入`ap_cmd sta <wifi_ssid> <wifi_password>`联网，等待证书认证<br>
    - 认证成功后，重启设备即可使用ASR组件<br>
- 直接对mic说“你好小智”， 日记返回“你好小智”；<br>
- 直接对mic说“再见小智”， 日记返回“再见小智”；<br>

以下是识别结果的串口log 输出：

```
[2026:01:19-17:41:12]ap0:count_ut:D(56637):[ONBOARD_MIC] data_size: 128000(Bytes), 31KB/s 
[2026:01:19-17:41:12]ap0:count_ut:D(56639):[RAW_WRITE] data_size: 0(Bytes), 0KB/s 
[2026:01:19-17:41:12]ap0:count_ut:D(56641):[ONBOARD_SPK]:0(Bytes),0KB/s | [FILL_SILENCE]:126720(Bytes),30KB/s 
[2026:01:19-17:41:12]ap0:count_ut:D(56643):[RAW_READ] data_size: 254720(Bytes), 62KB/s 
[2026:01:19-17:41:12]ap0:count_ut:D(56655):[WIFI_TX] data_size: 0(Bytes), 0KB/s 
[2026:01:19-17:41:12]ap0:count_ut:D(56655):[WIFI_RX] data_size: 0(Bytes), 0KB/s 
[2026:01:19-17:41:13]ap1:asr:I(57361):kws detected, label id: 0, label: 你好小智, cnt: 5
[2026:01:19-17:41:13]ap1:kws_demo:I(57361):[_asr_result_callback][434] [_asr_result_callback][434] result: 你好小智
[2026:01:19-17:41:16]ap0:count_ut:D(60637):[ONBOARD_MIC] data_size: 128000(Bytes), 31KB/s 
[2026:01:19-17:41:16]ap0:count_ut:D(60639):[RAW_WRITE] data_size: 0(Bytes), 0KB/s 
[2026:01:19-17:41:16]ap0:count_ut:D(60641):[ONBOARD_SPK]:0(Bytes),0KB/s | [FILL_SILENCE]:128640(Bytes),31KB/s 
[2026:01:19-17:41:16]ap0:count_ut:D(60643):[RAW_READ] data_size: 256640(Bytes), 62KB/s 
[2026:01:19-17:41:16]ap0:count_ut:D(60655):[WIFI_TX] data_size: 0(Bytes), 0KB/s 
[2026:01:19-17:41:16]ap0:count_ut:D(60655):[WIFI_RX] data_size: 0(Bytes), 0KB/s 
[2026:01:19-17:41:17]ap0:asr:I(61282):kws detected, label id: 1, label: 再见小智, cnt: 6
[2026:01:19-17:41:17]ap0:kws_demo:I(61282):[_asr_result_callback][434] [_asr_result_callback][434] result: 再见小智
```

## 常见问题

- **7258 ASR SERVICE服务打开失败**<br>

    bk_avdk_smp/ap/components/bk_asr_service/Kconfig文件中检测依赖关系是否存在问题，如果存在问题，需要先解决依赖关系，然后重新编译，参考sdk_patch，也可以将sdk_patch目录下的文件替换到sdk目录下。<br>

- **kws组件编译时提示找不到delay_us函数**<br>

    kws组件在编译时需要依赖delay_us函数，该函数需要客户自己在应用层定义：<br>
    ``` C
    extern void bk_delay_us(UINT32 us);

    void delay_us(uint32_t us)
    {
        bk_delay_us(us);
    }
    ```

- **kws组件鉴权失败**<br>

    (1)请确认是否有授权证书：<br>
    ``` C
    #define AUTH_ID "XXXXXXXXXXXXXXXX"
    #define AUTH_KEY "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
    ```

    (2)请检查网络状态是否正常。

- **如何购买授权证书**<br>
    可以去[淘宝链接](https://item.taobao.com/item.htm?ft=t&id=1013455372197&spm=a21dvs.23580594.0.0.4fee2c1bZL34sb)购买
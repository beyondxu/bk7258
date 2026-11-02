# app_asr 组件

[TOC]

## 参考示例

- [asr 示例](../../asr/kws_demo/README.md) 

## 概述

`app_asr` 是一个 ASR 组件，基于 BK 芯片的 ASR 能力，提供高效的语音识别功能。该组件支持从文件系统或内存数据中识别语音，并输出识别结果。组件提供了灵活的内存管理机制和多种识别选项，适用于各种语音识别场景。

## 功能特性

- 支持 ASR 识别加速
- 支持算法注册认证
- 支持返回结果给应用层

## 依赖组件和配置

- `bk_kws` - RiseLink_R01_V3.0 离线语音算法组件

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_KWS          | kws 组件使能配置，配置为y                                         |
| CONFIG_ASR_SERVICE    | asr 组件使能配置，配置为y         |
| CONFIG_ASR_SERVICE_USE_PSRAM | asr 组件使能配置，使用 psram 堆申请内存，配置为y |
| CONFIG_ASR_SERVICE_NO_MIC | asr 组件使能配置，默认配置为y |

## 接口说明

### 1. 证书配置

#### bk_app_asr_set_auth
```c
int bk_app_asr_set_auth(asr_auth_t *auth);
```
**功能**：设置 ASR 组件的认证信息

**参数**：
- `auth` - 输入参数，ASR 组件的认证信息

**返回**：
- 成功返回 `0`，失败返回错误码

**注意**：
- 该函数需要在 ASR 组件初始化之前调用，否则会返回错误。<br>
- 该函数只能设置一次认证信息，重复调用会返回错误。<br>
- 认证信息包括 `app_id`、`app_key` ，具体参考示例。<br>
- 传入的auth参数不能是栈上变量，不能在认证成功前释放。<br>
- 第一次认证需要联网。<br>

### 2. 初始化

#### bk_app_asr_init
```c
int bk_app_asr_init(void);
```
**功能**：初始化 ASR 组件

**参数**：无

**返回**：
- 成功返回 `0`，失败返回错误码

### 3. 识别

#### bk_app_asr_recog
```c
int bk_app_asr_recog(void *data, uint32_t len, void *p1, void *p2);
```
**功能**：识别 ASR 组件的语音数据

**参数**：
- `data` - 输入参数，ASR 组件的语音数据
- `len` - 输入参数，ASR 组件的语音数据长度
- `p1` - 输入参数，ASR 组件的识别配置
- `p2` - 输入参数，ASR 组件的识别配置

**返回**：
- 成功返回 `0`，失败返回错误码

### 4. 反初始化

#### bk_app_asr_deinit
```c
int bk_app_asr_deinit(void);
```
**功能**：反初始化 ASR 组件

**参数**：无

**返回**：
- 成功返回 `0`，失败返回错误码

## 接口使用

给sdk 的asr模块注册用，具体参考示例。

```c
    aud_asr_cfg.aud_asr_init = bk_app_asr_init;
    aud_asr_cfg.aud_asr_deinit = bk_app_asr_deinit;
    aud_asr_cfg.aud_asr_recog = bk_app_asr_recog;
    aud_asr_cfg.aud_asr_result_handle = config->asr_result_cb;

    if (aud_asr_cfg.aud_asr_result_handle == NULL) 
    {
        LOGW("asr result callback is NULL\n");
    }
    
    g_voice_ctx.aud_asr_handle = bk_aud_asr_init(&aud_asr_cfg);
```
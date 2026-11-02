# kws 组件

[TOC]

## 参考示例

- [kws 示例](../../asr/kws_demo/README.md) 

## 概述

`bk_kws` 是一个基于 BK 芯片的关键词识别组件，基于 离线语音识别算法`RiseLink_R01_V3.0`，提供高效的关键词识别功能。该组件支持从文件系统或内存数据中识别关键词，并输出识别结果。组件提供了灵活的内存管理机制和多种识别选项，适用于各种语音识别场景。

## 功能特性

- 关键词识别功能算法

## 依赖组件和配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_KWS          | kws 组件使能配置，配置为y                                         |

## 注意事项

- 关键词识别模型使用需要授权，具体授权方式参考示例，购买进入[授权购买链接](https://item.taobao.com/item.htm?ft=t&id=1013455372197&spm=a21dvs.23580594.0.0.4fee2c1bZL34sb)<br>

- 库中使用了接口 "delay_us"，需要在应用定义：<br>

```c
extern void bk_delay_us(UINT32 us);

void delay_us(UINT32 us)
{
    bk_delay_us(us);
}
```

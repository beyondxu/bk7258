# 芯片内部flash分区读写示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## flash分区读写
本示例操作**内部flash的user_config分区**, 实现以下功能：
1. 如何擦除一个FLASH扇区，并向该扇区写入一段数据
2. 如何从一个扇区读出数据，并与原来的数据进行比较，以确认写入与读出的数据是一致的

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_FLASH          | flash使能配置，配置为y                                         |

## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/storage/internal_flash_operate
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/storage/internal_flash_operate/build/bk7258/internal_flash_operate/package/all-app.bin；
- 上电后，会自动对flash进行写入与读回操作；

## 常见问题

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

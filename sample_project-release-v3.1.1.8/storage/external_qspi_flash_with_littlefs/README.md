# 挂载外部qspi flash作为littlefs示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## bk_vfs文件系统

BK VFS 是 BK 提供的虚拟文件系统组件，全称为 Virtual File System，文件系统接口采用**posix**接口形式，应用程序调用“bk_posix.h”头文件，即可使用mount、open、read、write、close接口进行文件操作。

![bk_vfs](/storage/docs/bk_vfs_overview.png)


本示例是将**外部qspi flash**作为littlefs分区，配合bk_vfs实现以下功能：
1. 如何挂载文件系统；
2. 如何读、写、删除、获取文件，以及获取文件长度；
3. 如何罗列文件系统中的所有文件；

## 硬件环境

1. 核心板 : bk7258 v3核心板
2. qspi flash : 外挂qspi flash存储芯片到qspi 0(gpio22~27)

## 代码可调配置

代码中使用的是**QSPI_0**, 参见FLASH_PART_TYPE宏。
如果使用QSPI_1, 因为BK的开发板使用SD_3.3V供电，需要插上TF卡才能使能到SD_3.3V。

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_VFS          | bk_vfs使能配置，配置为y                                         |
| CONFIG_VFS_MAX_FD_NUM    | bk_vfs同时打开文件的最大个数，如32         |
| CONFIG_LITTLEFS | littlefs使能配置，配置为y |
| CONFIG_QSPI | QSPI驱动使能配置，配置为y |
| CONFIG_QSPI_MST_FLASH | QSPI FLASH读写使能配置，配置为y |


## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/storage/external_qspi_flash_with_littlefs
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录all-app.bin；
- 上电后，会自动挂载文件系统、列出所有文件、写文件、读文件、删除文件；


## 常见问题


## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

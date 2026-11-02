# 挂载SD卡作为fatfs示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## bk_vfs文件系统

BK VFS 是 BK 提供的虚拟文件系统组件，全称为 Virtual File System，文件系统接口采用**posix**接口形式，应用程序调用“bk_posix.h”头文件，即可使用mount、open、read、write、close接口进行文件操作。
BK VFS后端支持fatfs文件系统，可实现挂载SD卡，进行文件操作。

本示例是将SD卡挂载为fatfs分区，配合bk_vfs实现以下功能：
1. 如何挂载文件系统；
2. 如何读、写、删除、获取文件，以及获取文件长度；
3. 如何罗列文件系统中的所有文件；

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_VFS          | bk_vfs使能配置，配置为y                                         |
| CONFIG_VFS_MAX_FD_NUM    | bk_vfs同时打开文件的最大个数，如32         |
| CONFIG_FATFS | fatfs使能配置，配置为y |
| CONFIG_FATFS_SDCARD | fatfs操作sdcard使能配置，配置为y |
| CONFIG_SDCARD | sdcard读写使能配置，配置为y |
| CONFIG_SDCARD_POWER_GPIO_CTRL | sdcard电源开关使能配置，配置为y |
| CONFIG_LDO3V3_CTRL_GPIO | sdcard电源开关管脚号配置，配置为gpio 13 |


## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/storage/sdcard_with_fatfs
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/storage/sdcard_with_fatfs/build/bk7258/sdcard_with_fatfs/package/all-app.bin；
- 上电后，会自动挂载文件系统、写文件、读文件、删除文件；


## 常见问题


## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

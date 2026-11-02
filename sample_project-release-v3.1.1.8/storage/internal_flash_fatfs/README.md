# 芯片内部flash分区作为fatfs示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## bk_vfs文件系统

BK VFS 是 BK 提供的虚拟文件系统组件，全称为 Virtual File System，文件系统接口采用**posix**接口形式，应用程序调用“bk_posix.h”头文件，即可使用mount、open、read、write、close接口进行文件操作。

![bk_vfs](/storage/docs/bk_vfs_overview.png)


本示例是将**内部flash的usr_config分区**作为fatfs分区，配合bk_vfs实现以下功能：
1. 如何挂载文件系统；
2. 如何读文件，以及获取文件长度；
3. 如何罗列文件系统中的所有文件；

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_VFS          | bk_vfs使能配置，配置为y                                         |
| CONFIG_VFS_MAX_FD_NUM    | bk_vfs同时打开文件的最大个数，如32         |
| CONFIG_FATFS | fatfs使能配置，配置为y |

## 文件系统镜像制作及烧录

因基于flash分区的fatfs是只读文件系统，需要将ui相关的图片、开机动画、提示音等，打包到文件系统镜像中去，然后烧录到flash的usr_config分区中。

### 镜像制作

fatfs文件系统镜像打包工具位于sample_project/tools/fatfs/，将要打包的文件夹或文件放到bk文件夹下，然后在windows命令行中执行以下命令，即可生成文件系统镜像bk.bin。

``` Shell
    fatdisk.exe
```
fatdisk.exe工具的配置文件为fatdisk.xml，一般无需修改。

打包完毕的bk.bin文件的大小，如果超出了usr_config分区的大小，则需修改工程下的auto_partitions.csv文件, 

### 如何烧录

打开烧录工具bkfil，选择bk.bin，然后将起始地址改为mount时开始地址start_addr，然后点烧录，下图是以usr_config分区作为fatfs分区的示例：

![bk_vfs](/storage/docs/vfs_burning.png)


## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/storage/internal_flash_fatfs
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/storage/internal_flash_fatfs/build/bk7258/internal_flash_fatfs/package/all-app.bin；
- 参考上面 **如何烧录** 章节，将sample_project/tools/fatfs/bk.bin烧录到usr_config分区，即0x003da000地址；
- 上电后，会自动挂载文件系统、读文件、列出所有文件；


## 常见问题


## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

# 芯片内部flash分区作为littlefs示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## bk_vfs文件系统

BK VFS 是 BK 提供的虚拟文件系统组件，全称为 Virtual File System，文件系统接口采用**posix**接口形式，应用程序调用“bk_posix.h”头文件，即可使用mount、open、read、write、close接口进行文件操作。

![bk_vfs](/storage/docs/bk_vfs_overview.png)

littlefs **空白文件系统** 使用步骤如下：
- 开启littlefs相关配置；
- 代码调用mkfs格式化文件系统镜像（只调用一次）；
- 代码调用mount函数挂载文件系统；
- 代码调用open、read、write、close函数操作文件；

littlefs **非空白文件系统** 使用步骤如下：
- 开启littlefs相关配置；
- linux环境制作文件系统镜像，并用bkfil烧录到代码挂载的flash地址；
- 代码调用mount函数挂载文件系统；
- 代码调用open、read、write、close函数操作文件；

本示例是将**内部flash的user_config分区**作为littlefs分区，配合bk_vfs实现以下功能：
1. 如何格式化文件系统镜像（需将测试代码里的CONFIG_INIT_EMPTY_FS配置为1）；
2. 如何挂载文件系统；
3. 如何读、写、删除、获取文件，以及获取文件长度；
4. 如何罗列文件系统中的所有文件；

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_VFS          | bk_vfs使能配置，配置为y                                         |
| CONFIG_VFS_MAX_FD_NUM    | bk_vfs同时打开文件的最大个数，如32         |
| CONFIG_LITTLEFS | littlefs使能配置，配置为y |

## 文件系统镜像制作及烧录

> **备注** ：如果产品文件系统刚开始不需要放任何文件，是个空文件系统，此章节可忽略，代码调用mkfs格式化制作一个空的文件系统（只调用一次，逻辑可以先mount，如果mount失败，调用mkfs，然后再mount），然后调用mount挂载即可；

在产品出厂时，如果需要文件系统里放文件，如ui相关的图片、开机动画、提示音等，就需要先制作文件系统镜像，然后烧录到flash中。

### 镜像制作

littlefs文件系统镜像打包工具位于sample_project/tools/littlefs/，将要打包的文件夹或文件放到bk文件夹下，然后linux命令行执行以下命令，即可生成文件系统镜像bk.bin。

``` Shell
    ./mklittlefs -c bk/ -b 4096 -p 256 -s flash_size bk.bin  //flash_size改为代码mount时指定的大小

    -b 文件系统block大小，单位字节，十进制格式，一般为4096；
    -p 文件系统page大小，单位字节，十进制格式，一般为256；
    -s 文件系统镜像大小，单位字节，十进制格式，必须和mount时指定的大小一样；
```

### 如何烧录

打开烧录工具bkfil，选择bk.bin，然后将起始地址改为mount时开始地址start_addr，然后点烧录，下图是以user_config分区作为littlefs分区的示例：

![bk_vfs](/storage/docs/vfs_burning.png)

## 关键代码

以下为格式化文件系统镜像的格式化和mount的关键代码，用户只需要关注文件系统的起始地址start_addr、大小flash_size，以及挂载的路径mount_path；

``` c
    //格式化文件系统镜像
    struct bk_little_fs_partition partition;
    char *fs_name = NULL;
    bk_err_t ret = BK_FAIL;

    bk_logic_partition_t *fs_partition = bk_flash_partition_get_info(BK_PARTITION_USR_CONFIG);
    uint32_t flash_start = fs_partition->partition_start_addr;
    uint32_t flash_size = fs_partition->partition_length;
    
    fs_name = "littlefs";
    partition.part_type = LFS_FLASH;
    partition.part_flash.start_addr = flash_start;
    partition.part_flash.size = flash_size;

    ret = mkfs("PART_NONE", fs_name, &partition);

    //挂载文件系统
    struct bk_little_fs_partition partition;
    char *fs_name = NULL;
    bk_err_t ret = BK_FAIL;
    bk_logic_partition_t *fs_partition = bk_flash_partition_get_info(BK_PARTITION_USR_CONFIG);
    uint32_t flash_start = fs_partition->partition_start_addr;
    uint32_t flash_size = fs_partition->partition_length;

    fs_name = "littlefs";
    partition.part_type = LFS_FLASH;
    partition.part_flash.start_addr = flash_start;
    partition.part_flash.size = flash_size;
    partition.mount_path = mount_path;

    ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);
```

## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
export SDK_DIR=~/armino/bk_avdk_smp  //设置smp sdk路径到环境变量
cd sample_project/internal_flash_littlefs
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录sample_project/storage/internal_flash_littlefs/build/bk7258/internal_flash_littlefs/package/all-app.bin；
- 参考上面 **如何烧录** 章节，将sample_project/tools/littlefs/bk.bin烧录到user_config分区，即0x003da000地址；
- 上电后，会自动挂载文件系统、写文件、读文件、删除文件；

以下是上电运行后的关键串口log 输出：

```
ap0:VFS:I(482):[_mount_fs][67] mount ret:0, start_addr:0x3da000, flash_size:0x100000   //文件系统挂载成功
ap0:VFS:I(489):==============================================================================================================
ap0:VFS:I(490):|| [path        ] filename                                                             filesize             || 
ap0:VFS:I(516):|| [/littlefs/mp3] /littlefs/mp3/no_network_config_cn.mp3                            12441 Byte    ||
ap0:VFS:I(530):========================================the sector total size = 12 KB num = 1===============================
ap0:VFS:I(546):[_file_write_test][201] write file success                             //写1.txt文件成功
ap0:VFS:I(554):[_file_read_test][217] /littlefs/1.txt filesize:37
ap0:VFS:I(561):[_file_read_test][237] read:helloworld, this is beken vfs test         //读1.txt文件，并打印文件内容

ap0:VFS:I(561):[main][282] after write list
ap0:VFS:I(574):|| [/littlefs   ] /littlefs/1.txt                                                      37 Byte    ||
ap0:VFS:I(608):|| [/littlefs/mp3] /littlefs/mp3/no_network_config_cn.mp3                            12441 Byte    ||
ap0:VFS:I(622):========================================the sector total size = 12 KB num = 1===============================
ap0:VFS:I(622):========================================the sector total size = 0 KB num = 1===============================
ap0:VFS:I(634):[main][288] after del list
ap0:VFS:I(668):|| [/littlefs/mp3] /littlefs/mp3/no_network_config_cn.mp3                            12441 Byte    ||
ap0:VFS:I(683):========================================the sector total size = 12 KB num = 1===============================
```

## 常见问题

### 项目上到底用fatfs还是littlefs
| flash                   | 文件系统                               | 读写支持                               |
| -------------------- | ------------------------------------------ | ------------------------------------------ |
| 内置flash          | fatfs                                         | 只读
| 内置flash    | littfs         |读写都支持
| SD卡    | fatfs         |读写都支持
| 外置spi/qspi flash | littlefs|读写都支持

fatfs文件系统只有挂载SD卡时，才支持写功能，littfs文件系统读写都支持，所以如果项目只需要对文件系统文件读，用fatfs即可，如果有写的需求，就用littlefs；

### littlefs 挂载失败是什么原因

可确认是否是以下原因引起：
- littlefs相关配置是否开启；
- 制作的文件系统大小和littlefs分区大小不一样导致，可确认下制作littlefs文件系统时，大小是否对；

### littlefs mkfs是干什么的

这个是格式化一个空的文件系统，如果想事先文件系统放一些常用文件，则在PC上做好文件系统，直接mount即可；如果每次出厂，需要一个空的文件系统，则使用mkfs格式化一个空的文件系统；

### 挂载多个文件系统是否支持

这个是支持的，调用时挂载用不同的mount_path名即可，sdk内部会通过mount_path和分区类型part_type来区分是操作哪个flash。

如可以内部flash一个分区挂载fatfs，一个分区挂载littlefs，外部spi/qspi flash挂载一个或多个littlefs；

> **注意** ：芯片内部flash在用来挂载fatfs时，**只能挂载一个，sdk默认使用的是BK_PARTITION_USR_CONFIG分区**（具体可在ap/components/fatfs/disk_io.c搜索，有两处，如果想换为其它分区，修改代码对应位置即可）；

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

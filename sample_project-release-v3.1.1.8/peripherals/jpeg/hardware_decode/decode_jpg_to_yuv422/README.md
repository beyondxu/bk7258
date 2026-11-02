# jpeg整帧硬解码输出yuv422格式示例

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[TOC]  

## 相关章节

 [bk_vfs文件系统](/storage/internal_flash_littlefs/README.md)

## 概述

本示例是将**内部flash的user_config分区**作为littlefs分区，利用jpeg硬解码功能，实现以下功能：
1. 如何挂载文件系统；
2. 如何从文件系统中读取jpeg文件，并调用硬解码接口，输出yuv422格式数据；

> **备注** ：jpeg图片及解码后的内存申请和释放，是调用frame_buffer_app.c中的函数，默认是从frame_buffer内存分配，如果psram内存调整为所有都作为堆内存(**[多媒体相关的几个PSRAM大小都配置为0](https://docs.bekencorp.com/arminodoc/bk_avdk_smp/ap_doc/bk7258/zh_CN/v3.1.1/developer-guide/peripheral/bk_psram.html?highlight=psram#id8)**)，可以将frame_buffer_app.c里的USE_PSRAM_MALLOC配置为1，即从psram堆分配内存。

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_JPEG_DECODER          | jpeg硬解码使能配置，配置为y                                         |
| CONFIG_JPEG_DECODER_HW    | jpeg硬解码使能配置，配置为y         |
| CONFIG_JPEGENC_HW | jpeg硬编码使能配置，硬解码有接口依赖，配置为y |
| CONFIG_DMA2D | bk7258 jpeg硬解码只能输出yuyv格式，如果需要通过dma2d将yuyv格式数据转换为rgb565格式，需要配置为y |

## 依赖模块

- sample_project/components/app_jpeg_hw_decode

**根目录下的 CMakeLists.txt** 中需要添加以下内容（放入自己工程需留意自己工程相对components的路径）：

``` cmake
set(EXTRA_COMPONENTS_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../components/app_jpeg_hw_decode
)
```
**ap/CMakeLists.txt** PRIV_REQUIRES中需加入app_jpeg_hw_decode依赖，这样可直接调用这两个模块头文件：

``` cmake
set(priv_req  media_service multimedia bk_network_provisioning bk_jpeg_decoder app_jpeg_hw_decode)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
```

## 7258硬解码要求

BK7258芯片硬解模块有以下限制，使用前请须知：
- 只能解码yuv422格式，常用图片都是yuv420格式，需要先试用工具转换成yuv422格式；
- 图片的行像素需是32的倍数，列像素必须是8的倍数，如240X240的图片，因行240不是32倍数，所以不能使用硬解码；

yuv420格式的jpg图片可使用下面的工具位置，按照使用方法转换为yuv422格式：
- 工具位置：sample_project/tools/jpgconvert/；
- 使用方法：参考工具下的readme.txt;

## sdk硬解码基础接口

以下sdk硬解码相关的基础接口:
``` c
    //创建jpeg硬解码句柄，配置参数为config，主要包含解码完成的callback
    avdk_err_t bk_hardware_jpeg_decode_new(bk_jpeg_decode_hw_handle_t *handle, bk_jpeg_decode_hw_config_t *config);

    //打开jpeg硬解码句柄
    avdk_err_t bk_jpeg_decode_hw_open(bk_jpeg_decode_hw_handle_t handler);

    //jpeg硬解码函数，in_frame为输入参数，对应jpeg数据，out_frame为解码输出的yuyv数据，
    avdk_err_t bk_jpeg_decode_hw_decode(bk_jpeg_decode_hw_handle_t handler, frame_buffer_t *in_frame, frame_buffer_t *out_frame);

    //关闭jpeg硬解码句柄
    avdk_err_t bk_jpeg_decode_hw_close(bk_jpeg_decode_hw_handle_t handler);

    //删除jpeg硬解码句柄
    avdk_err_t bk_jpeg_decode_hw_delete(bk_jpeg_decode_hw_handle_t handler);
```

## 硬解码二次封装接口

以下是对sdk接口做了封装，代码位于sample_project/components/hw_jpeg_decode/jpeg_hw_decode.c，可直接调用头文件的相关接口使用：

``` c
    /**
    * @brief 解码配置输入参数
    * 
    * JPEG原始图片、解码后所用frame_buffer内存的malloc、free接口，有以下两种示例可借鉴
    * Usage example1: 从系统frame_buffer申请内存，好处是不会因内存碎片引起内存申请失败
    *               //jpeg原始图片malloc
    *               static frame_buffer_t *_jpeg_frame_malloc(uint32 size)
    *               {
    *                   return frame_buffer_encode_malloc(size);
    *               }
    *
    *               //jpeg原始图片free
    *               static void _jpeg_frame_free(frame_buffer_t *frame)
    *               {
    *                   frame_buffer_encode_free(frame);
    *               }
    *
    *               //解码后图片malloc
    *               static frame_buffer_t *_decode_frame_malloc(uint32 size)
    *               {
    *                   return frame_buffer_display_malloc(size);
    *               }
    *
    *               //解码后图片free
    *               static void _decode_frame_free(frame_buffer_t *frame)
    *               {
    *                   frame_buffer_display_free(frame);
    *               }
    *
    *            Usage example2: 从系统psram堆申请内存，好处是可将所有psram当堆内存使用，坏处是如果应用代码使用内存较多，并且没即时释放，可能会导致因内存碎片引起内存申请失败
    *               //jpeg原始图片、解码图片malloc
    *               static frame_buffer_t *_frame_malloc(uint32 size)
    *               {
    *                   frame_buffer_t *frame = psram_malloc(sizeof(frame_buffer_t));
    *                  if(frame)
    *                  {
    *                      os_memset(frame, 0, sizeof(frame_buffer_t));
    *                      frame->frame = psram_malloc(size);
    *                       if(!frame->frame)
    *                      {
    *                          psram_free(frame);
    *                          frame = NULL;
    *                      }
    *                  }
    *
    *                   return frame;
    *                }
    *
    *               //jpeg原始图片、解码图片free
    *               static void _frame_free(frame_buffer_t *frame)
    *               {
    *                   if(frame)
    *                   {
    *                       if(frame->frame)
    *                       {
    *                           psram_free(frame->frame);
    *                       }
    *
    *                       psram_free(frame);
    *                   }
    *               }
    */
    typedef struct{
        bk_jpeg_decode_hw_handle_t jpeg_decode_handle;                  /*!硬解码句柄，传NULL则由内部创建硬解码句柄 */

        frame_buffer_t *(*jpeg_frame_malloc)(uint32 size);              /*!给解码模块用来分配jpeg frame的callback */
        void (*jpeg_frame_free)(frame_buffer_t *frame);                 /*!给解码模块用来释放jpeg frame的callback */

        frame_buffer_t *(*decode_frame_malloc)(uint32 size);            /*!给解码模块用来分配decode frame的callback */
        void (*decode_frame_free)(frame_buffer_t *frame);               /*!给解码模块用来释放decode frame的callback */

    //以下参数仅对转RGB565有效
        /*
        解码输出的宽度、高度，用于图片宽高不满足bk7258硬件要求时使用，填0内部会按实际图片的长宽输出。
        如: 需要解码jpeg图片，显示到240X240的lcd 屏上，宽240不是32倍数，需将图片做成256X240（新建256X240画布，拷贝240X240图片进来按左上角原点对齐），
            这里output_width、out_height填240、240，接口只会解码输出（0,0）到（240,240)区域的解码数据
        */
        uint16 output_width;
        uint16 output_height;

        bool byte_swap;                                                  /*!解码输出大端还是小端格式数据，false为小端， true为大端*/
        bool use_dma2d;                                                  /*!yuv转rgb565用dma2d还是软件算法 false 使用软件算法，true 使用dma2d算法*/
    }jpeg_hw_decode_config_t;

    /**
    * @brief 对文件系统的jpeg文件，利用JPEG硬解并输出YUV422格式
    * @param[in] jpeg_decode_config 输入参数，硬解文件句柄、JPEG原始图片、解码后所用frame_buffer内存的malloc、free接口等
    * @param[in] file_name 输入参数，JPEG文件路径
    * @param[out] decode_frame 输出参数，用于存储解码后的帧缓冲区指针
    * @return bk_err_t 成功返回BK_OK，失败返回错误码
    */
    bk_err_t jpeg_hw_decode_from_file_to_yuv422(const jpeg_hw_decode_config_t *jpeg_decode_config, 
                                                        const char *file_name, 
                                                        frame_buffer_t **decode_frame);

    /**
    * @brief 对文件系统的jpeg文件，利用JPEG硬解并输出RGB565格式
    * @param[in] jpeg_decode_config 输入参数，JPEG原始图片、解码后所用frame_buffer内存的malloc、free接口
    * @param[in] file_name 输入参数，JPEG文件路径
    * @param[out] decode_frame 输出参数，用于存储解码后的帧缓冲区指针
    * @return bk_err_t 成功返回BK_OK，失败返回错误码
    */
    bk_err_t jpeg_hw_decode_from_file_to_rgb565(const jpeg_hw_decode_config_t *jpeg_decode_config, 
                                                        const char *file_name, 
                                                        frame_buffer_t **decode_frame);

    /**
    * @brief 对jpeg内存数据，利用JPEG硬解并输出YUV422格式
    * @param[in] jpeg_decode_config 输入参数，硬解文件句柄、JPEG原始图片、解码后所用frame_buffer内存的malloc、free接口等
    * @param[in] file_name 输入参数，JPEG文件内容数据
    * @param[out] decode_frame 输出参数，用于存储解码后的帧缓冲区指针
    * @return bk_err_t 成功返回BK_OK，失败返回错误码
    */
    bk_err_t jpeg_hw_decode_from_buffer_to_yuv422(const jpeg_hw_decode_config_t *jpeg_decode_config, 
                                                            frame_buffer_t *jpeg_frame, 
                                                            frame_buffer_t **decode_frame);

    /**
    * @brief 对jpeg内存数据，利用JPEG硬解并输出RGB565格式
    * @param[in] jpeg_decode_config 输入参数，JPEG原始图片、解码后所用frame_buffer内存的malloc、free接口
    * @param[in] jpeg_frame 输入参数，JPEG文件内容数据
    * @param[out] decode_frame 输出参数，用于存储解码后的帧缓冲区指针
    * @return bk_err_t 成功返回BK_OK，失败返回错误码
    */
    bk_err_t jpeg_hw_decode_from_buffer_to_rgb565(const jpeg_hw_decode_config_t *jpeg_decode_config, 
                                                            frame_buffer_t *jpeg_frame, 
                                                            frame_buffer_t **decode_frame);
```

## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/peripherals/jpeg/hardware_decode/decode_jpg_to_yuv422
vim Makefile  //修改SDK_DIR为实际smp sdk路径
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录工程build下面的all-app.bin；
- sample_project/storage/tools/littlefs/bk.bin烧录到user_config分区，即0x003da000地址；
- 上电后，会自动挂载文件系统并解码文件系统里的/img/yuv422/example_jpg422.jpg文件；

以下是上电运行后的关键串口log 输出：

```
ap0:VFS:I(472):[_mount_fs][45] mount ret:0, start_addr:0x3da000, flash_size:0x100000   //文件系统挂载成功
ap0:OS:D(473):create jpeg_thread, tcb=60625540, stack=[60624d18-60625518:2048], prio=3, xCoreID=-1
ap1:OS:D(474):create hw_dec_thread, tcb=60625a00, stack=[606255d8-606259d8:1024], prio=3, xCoreID=-1
ap0:jpeg hw_:D(475):bk_jpeg_dec_driver_init complete
ap1:hw_dec:E(682):hw_jpeg_decode_in_complete 124 in_complete is NULL
ap1:hw_img:D(682):[_jpeg_decode_complete][21] jpeg decode success! format_type: 5, out_frame: 0x6062ecf8
ap1:hw_dec:D(683):hw_jpeg_decode_deinit 
ap0:jpeg hw_:D(684):bk_jpeg_dec_driver_deinit complete
ap1:hw_img:I(684):[jpeg_hw_decode_by_frame_output_yuv422][252] decode /littlefs/img/yuv422/example_jpg422.jpg success, width:864, height:480, length:829440   //解码成功
ap1:VFS:I(685):/homeb/honghui.xiao/7258/avdk_smp/sample_project/peripherals/jpeg/hardware_decode_by_frame_output_yuv422/ap/ap_main.c:57:_decode_handle:PASS
```

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

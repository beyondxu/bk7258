# jpeg图片硬解码 组件

[TOC]

## 参考示例

- [jpeg整帧硬解码输出RGB565小端格式示例](/peripherals/jpeg/hardware_decode/decode_jpg_to_rgb565_little_endian/README.md) 

## 概述

`app_jpeg_hw_decode` 是一个 JPEG 硬件解码组件，基于 BK 芯片的硬件 JPEG 解码能力，提供高效的 JPEG 图片解码功能。该组件支持从文件系统或内存数据中解码 JPEG 图片，并输出为 YUV422 或 RGB565 格式。组件提供了灵活的内存管理机制和多种解码选项，适用于各种图像处理场景。

## 功能特性

- 支持 JPEG 硬件解码加速
- 支持从文件系统解码 JPEG 图片
- 支持从内存数据解码 JPEG 图片
- 支持多种输出格式：
  - YUV422 格式
  - RGB565 格式（支持小端和大端）
- 支持 YUV422 转 RGB565，可选择 DMA2D 硬件加速或软件算法
- 提供灵活的内存管理机制：
  - 从 frame_buffer 申请内存（避免内存碎片）
  - 从 psram 堆申请内存（灵活使用内存）
- 支持外部传入硬解码句柄，避免重复创建

## 依赖组件和配置

- `bk_jpeg_decoder` - JPEG 解码器组件
- `bk_vfs` - 虚拟文件系统组件

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_JPEG_DECODER          | jpeg硬解码使能配置，配置为y                                         |
| CONFIG_JPEG_DECODER_HW    | jpeg硬解码使能配置，配置为y         |
| CONFIG_JPEGENC_HW | jpeg硬编码使能配置，硬解码有接口依赖，配置为y |
| CONFIG_DMA2D | bk7258 jpeg硬解码只能输出yuyv格式，如果需要通过dma2d将yuyv格式数据转换为rgb565格式，需要配置为y |
| CONFIG_VFS | 使能虚拟文件系统组件，配置为y |

## 接口说明

### 1. 从文件解码

#### jpeg_hw_decode_from_file_to_yuv422
```c
bk_err_t jpeg_hw_decode_from_file_to_yuv422(
    const jpeg_hw_decode_config_t *jpeg_decode_config,
    const char *file_name,
    frame_buffer_t **decode_frame);
```
**功能**：对文件系统的 JPEG 文件，利用 JPEG 硬解并输出 YUV422 格式

**参数**：
- `jpeg_decode_config` - 输入参数，硬解文件句柄、JPEG 原始图片、解码后所用 frame_buffer 内存的 malloc、free 接口等
- `file_name` - 输入参数，JPEG 文件路径
- `decode_frame` - 输出参数，用于存储解码后的帧缓冲区指针

**返回**：
- 成功返回 `BK_OK`，失败返回错误码

#### jpeg_hw_decode_from_file_to_rgb565
```c
bk_err_t jpeg_hw_decode_from_file_to_rgb565(
    const jpeg_hw_decode_config_t *jpeg_decode_config,
    const char *file_name,
    frame_buffer_t **decode_frame);
```
**功能**：对文件系统的 JPEG 文件，利用 JPEG 硬解并输出 RGB565 格式

**参数**：
- `jpeg_decode_config` - 输入参数，JPEG 原始图片、解码后所用 frame_buffer 内存的 malloc、free 接口
- `file_name` - 输入参数，JPEG 文件路径
- `decode_frame` - 输出参数，用于存储解码后的帧缓冲区指针

**返回**：
- 成功返回 `BK_OK`，失败返回错误码

### 2. 从内存解码

#### jpeg_hw_decode_from_buffer_to_yuv422
```c
bk_err_t jpeg_hw_decode_from_buffer_to_yuv422(
    const jpeg_hw_decode_config_t *jpeg_decode_config,
    frame_buffer_t *jpeg_frame,
    frame_buffer_t **decode_frame);
```
**功能**：对 JPEG 内存数据，利用 JPEG 硬解并输出 YUV422 格式

**参数**：
- `jpeg_decode_config` - 输入参数，硬解文件句柄、JPEG 原始图片、解码后所用 frame_buffer 内存的 malloc、free 接口等
- `jpeg_frame` - 输入参数，JPEG 文件内容数据
- `decode_frame` - 输出参数，用于存储解码后的帧缓冲区指针

**返回**：
- 成功返回 `BK_OK`，失败返回错误码

#### jpeg_hw_decode_from_buffer_to_rgb565
```c
bk_err_t jpeg_hw_decode_from_buffer_to_rgb565(
    const jpeg_hw_decode_config_t *jpeg_decode_config,
    frame_buffer_t *jpeg_frame,
    frame_buffer_t **decode_frame);
```
**功能**：对 JPEG 内存数据，利用 JPEG 硬解并输出 RGB565 格式

**参数**：
- `jpeg_decode_config` - 输入参数，JPEG 原始图片、解码后所用 frame_buffer 内存的 malloc、free 接口
- `jpeg_frame` - 输入参数，JPEG 文件内容数据
- `decode_frame` - 输出参数，用于存储解码后的帧缓冲区指针

**返回**：
- 成功返回 `BK_OK`，失败返回错误码

## 数据结构

### jpeg_hw_decode_config_t
JPEG 硬件解码配置结构体

```c
typedef struct{
    bk_jpeg_decode_hw_handle_t jpeg_decode_handle;      /*!< 硬解码句柄，传 NULL 则由内部创建硬解码句柄 */

    frame_buffer_t *(*jpeg_frame_malloc)(uint32 size);  /*!< 给解码模块用来分配 jpeg frame 的 callback */
    void (*jpeg_frame_free)(frame_buffer_t *frame);     /*!< 给解码模块用来释放 jpeg frame 的 callback */

    frame_buffer_t *(*decode_frame_malloc)(uint32 size); /*!< 给解码模块用来分配 decode frame 的 callback */
    void (*decode_frame_free)(frame_buffer_t *frame);    /*!< 给解码模块用来释放 decode frame 的 callback */

    // 以下参数仅对转 RGB565 有效
    /*
    解码输出的宽度、高度，用于图片宽高不满足 bk7258 硬件要求时使用，填 0 内部会按实际图片的长宽输出。
    如: 需要解码 jpeg 图片，显示到 240X240 的 lcd 屏上，宽 240 不是 32 倍数，需将图片做成 256X240
        （新建 256X240 画布，拷贝 240X240 图片进来按左上角原点对齐），
        这里 output_width、output_height 填 240、240，接口只会解码输出（0,0）到（240,240)区域的解码数据
    */
    uint16 output_width;
    uint16 output_height;

    bool byte_swap;                                      /*!< 解码输出大端还是小端格式数据，false 为小端，true 为大端 */
    bool use_dma2d;                                      /*!< yuv 转 rgb565 用 dma2d 还是软件算法，false 使用软件算法，true 使用 dma2d 算法 */
}jpeg_hw_decode_config_t;
```

## 内存管理

组件提供了两种内存管理方式，可以自己调用SDK预先划分给多媒体的frame_buffer psram内存，也可以调用psram_malloc从psram堆内存申请内存，可参考目录下的frame_buffer.c。

> **备注** ：frame_buffer.c和frame_buffer.h没有参与编译，需要拷贝到自己项目中使用，关键接口如下：
- `frame_buffer_t *jpeg_frame_malloc(uint32 size)`：分配 JPEG 原始图片的内存
- `void jpeg_frame_free(frame_buffer_t *frame)`：释放 JPEG 原始图片的内存
- `frame_buffer_t *decode_frame_malloc(uint32 size)`：分配解码后图片的内存
- `void decode_frame_free(frame_buffer_t *frame)`：释放解码后图片的内存


应用代码可将 `frame_buffer_app.c`的接口赋值给 `jpeg_hw_decode_config_t` 结构体中的接口，在`frame_buffer_app.c` 里由 `USE_PSRAM_MALLOC` 宏定义来切换使用哪种psram内存。

### 方式一：使用 SDK 默认划分的 frame_buffer 内存（推荐）

这种方式通过 SDK 预先划分的内存池进行管理，不会因内存碎片引起内存申请失败，适用于大多数应用场景。

**配置方法**：在 `frame_buffer_app.c` 中设置 `#define USE_PSRAM_MALLOC 0`

**优点**：
- 避免内存碎片问题
- 内存分配和释放效率高
- SDK 自动管理内存池大小

### 方式二：使用系统 PSRAM 堆内存

这种方式在将整个psram作为堆内存时使用，灵活性更高。但如果应用代码使用内存较多且没有及时释放，可能会导致因内存碎片引起内存申请失败。

**配置方法**：在 `frame_buffer_app.c` 中设置 `#define USE_PSRAM_MALLOC 1`

**优点**：
- 内存使用更灵活，不受 SDK 内存池大小限制
- 可以充分利用所有 PSRAM 空间

**缺点**：
- 可能产生内存碎片
- 需要确保内存及时释放
- 长时间运行可能出现内存分配失败


## BK7258 硬解码要求

BK7258 芯片硬解模块有以下限制，使用前请须知：

1. **只能解码 YUV422 格式**：常用图片都是 YUV420 格式，需要先使用工具转换成 YUV422 格式
2. **图片尺寸限制**：
   - 图片的行像素需是 32 的倍数
   - 列像素必须是 8 的倍数
   - 如 240X240 的图片，因行 240 不是 32 倍数，所以不能使用硬解码

### YUV420 转 YUV422 工具

YUV420 格式的 JPG 图片可使用下面的工具位置，按照使用方法转换为 YUV422 格式：

- **工具位置**：`sample_project/tools/jpgconvert/`
- **使用方法**：参考工具下的 `readme.txt`

### 绕过硬件限制的方法

BK7258 上面可以结合 DMA2D 模块，间接绕过上面的硬件限制。如屏是 240X240 大小，宽不满足 32 倍数，可将图片做成 256X240 大小，然后通过 DMA2D 模块的参数只传入 240X240 大小的区域。

可参考封装的 `jpeg_hw_decode_from_file_to_rgb565` 接口的 `output_width` 和 `output_height` 参数。

## 注意事项

1. **内存管理**：内存分配和释放必须成对出现，避免内存泄漏
2. **DMA2D 冲突**：BK7258 只有一路 DMA2D 通道，LVGL 双 buffer 时内部会用到 DMA2D，有以下方法可以解决：
   - 使用时需要与 LVGL 界面互斥，LVGL 界面调用 `lv_vendor_stop` 挂起，使用完后调用 `bk_dma2d_stop_transfer` 停止 DMA2D 传输
   - 使用软件 YUV 转 RGB565 格式，将 `use_dma2d` 参数设为 `false`
3. **硬解码句柄**：如果需要频繁解码，建议在外部创建硬解码句柄并传入，避免重复创建
4. **输出尺寸裁剪**：`output_width` 和 `output_height` 参数仅在解码为 RGB565 格式时有效
5. **文件系统**：确保 JPEG 文件所在的存储设备已正确挂载
6. **图片格式**：确保 JPEG 图片的编码格式符合硬件要求（YUV422，行像素为 32 倍数，列像素为 8 倍数）

## 常见问题

### 1. 解码失败，返回错误码

**可能原因**：
- JPEG 图片格式不符合硬件要求
- 内存分配失败
- 文件路径错误或文件不存在

**解决方法**：
- 使用工具将图片转换为 YUV422 格式
- 检查图片尺寸是否符合硬件要求
- 检查内存是否足够
- 检查文件路径是否正确

### 2. 运行时报 DMA2D 错误

**可能原因**：
- DMA2D 通道被其他模块占用（如 LVGL）

**解决方法**：
- 与 LVGL 界面互斥使用
- 使用软件算法（设置 `use_dma2d = false`）

## 相关文件

- `jpeg_hw_decode.h` - JPEG 硬件解码头文件
- `jpeg_hw_decode.c` - JPEG 硬件解码实现文件
- `CMakeLists.txt` - 组件构建配置文件


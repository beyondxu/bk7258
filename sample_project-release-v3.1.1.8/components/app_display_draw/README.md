# 显示绘制组件（颜色填充、图片数据搬移、透明度图片数据融合、文字融合）

[toc]

## 参考资料
- [字库生成](https://docs.bekencorp.com/arminodoc/bk_avdk/bk7258/zh_CN/v2.0.2/gui/osd/font_generate.html)：文档里的字库工具可自己网上下载
- 图片预解码
    - [png转换为argb8888](https://docs.bekencorp.com/arminodoc/bk_avdk/bk7258/zh_CN/v2.0.2/gui/osd/osd_blend.html#id3)：工具在sample_project/tools/ffmpeg_bat
    - [lvgl 图片解码.c如何保存为bin文件](/tools/lvgl_C_file_to_image_bin/README.md)：参考此章节里的lvgl网址图片解码为rgb565或者argb8888，rgb565对应color_depth为16，argb8888对应color_depth为32

## 概述

`app_display_draw` 是一个统一的显示绘制组件，提供颜色填充、图片数据搬移、透明度图片数据融合、文字融合等功能。该组件支持软件算法和硬件 DMA2D 两种实现方式，用户可以通过配置项选择使用哪种实现。软件算法实现适用于所有场景，硬件 DMA2D 实现利用 BK 芯片的 DMA2D 硬件模块，具有更高的性能和更低的 CPU 占用率。组件采用统一的接口层设计，屏蔽了底层实现差异，为上层应用提供简洁一致的 API。

## 功能特性

- 支持颜色填充功能
  - 支持多种颜色格式：RGB565、RGB888、ARGB8888
  - 支持指定区域的颜色填充
- 支持图片数据搬移功能
  - 支持源图像到目标图像的拷贝
  - 支持指定位置的图像拷贝
  - 支持多种颜色格式的数据拷贝
- 支持透明度图片数据融合功能
  - 支持前景图像（ARGB8888 格式）融合到背景图像
  - 背景图像支持多种格式：PIXEL_FMT_VUYY、PIXEL_FMT_YUYV、PIXEL_FMT_RGB888、PIXEL_FMT_RGB565_LE、PIXEL_FMT_RGB565
  - 支持指定位置的图像融合
- 支持文字融合功能
  - 支持在背景图像上显示文本，背景图像格式支持PIXEL_FMT_VUYY/PIXEL_FMT_YUYV/PIXEL_FMT_RGB888/PIXEL_FMT_RGB565_LE/PIXEL_FMT_RGB565
  - 支持自定义字体颜色
  - 支持指定字体类型和显示位置
- 支持软件算法和硬件 DMA2D 两种实现方式
  - 软件算法：适用于所有场景，无需硬件支持
  - 硬件 DMA2D：利用硬件加速，性能更高，CPU 占用更低
- 支持通过配置项选择实现方式
- 提供统一的接口层，屏蔽底层实现差异
- 支持完整的错误处理和日志输出

## 相关文件

```
app_display_draw/
├── app_display_draw.h          # 显示绘制接口层头文件
├── app_display_draw.c          # 显示绘制接口层实现文件
├── Kconfig                     # 组件配置文件
├── CMakeLists.txt              # 组件构建配置文件
├── sw/                         # 软件算法实现目录
│   ├── app_draw_sw.h           # 软件算法实现头文件
│   └── app_draw_sw.c           # 软件算法实现文件
└── dma2d/                      # 硬件 DMA2D 实现目录
    ├── app_draw_hw_dma2d.h      # 硬件 DMA2D 实现头文件
    └── app_draw_hw_dma2d.c      # 硬件 DMA2D 实现文件
```

## 配置选项

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_APP_DISPLAY_DRAW          | 显示绘制使能配置，需配置为y                                         |
| CONFIG_MEDIA_OSD          | SDK 软件融合组件，CONFIG_APP_DISPLAY_DRAW_SOFTWARE配置为y时需开启 |
| CONFIG_APP_DISPLAY_DRAW_SOFTWARE          | 绘制启用软件算法，默认配置为y                                         |
| CONFIG_DMA2D    | DMA2D 硬件驱动组件使能配置，CONFIG_APP_DISPLAY_DRAW_DMA2D开启时需开启         |
| CONFIG_MEDIA_DAM2D   | DMA2D sdk组件使能配置，CONFIG_APP_DISPLAY_DRAW_DMA2D开启时需开启|
| CONFIG_APP_DISPLAY_DRAW_DMA2D    | 绘制启用硬件 DMA2D 实现，默认配置为n         |


## 接口说明

### 1. 初始化和去初始化

#### app_display_draw_init
```c
int app_display_draw_init(void);
```
**功能**：初始化显示绘制组件

**返回**：
- 成功返回 `BK_OK`，失败返回错误码

#### app_display_draw_deinit
```c
int app_display_draw_deinit(void);
```
**功能**：去初始化显示绘制组件

**返回**：
- 成功返回 `BK_OK`，失败返回错误码

### 2. 颜色填充

#### app_display_draw_fill_color
```c
int app_display_draw_fill_color(
    frame_buffer_t *dst_frame,
    uint32_t color,
    uint16_t dst_x,
    uint16_t dst_y,
    uint16_t fill_width,
    uint16_t fill_height);
```
**功能**：在目标缓冲区的指定区域填充指定颜色

**参数**：
- `dst_frame` - 目标帧缓冲区指针，包含帧缓冲区的所有信息（地址、尺寸、格式等）
- `color` - 填充颜色值（根据帧格式自动解析为 RGB565/RGB888/ARGB8888）
- `dst_x` - 目标区域起始 X 坐标（像素）
- `dst_y` - 目标区域起始 Y 坐标（像素）
- `fill_width` - 填充区域宽度（像素）
- `fill_height` - 填充区域高度（像素）

**返回**：
- 成功返回 `BK_OK`，失败返回错误码

### 3. 图片数据搬移

#### app_display_draw_copy
```c
int app_display_draw_copy(
    frame_buffer_t *src_frame,
    frame_buffer_t *dst_frame,
    uint16_t dst_x,
    uint16_t dst_y);
```
**功能**：将源缓冲区数据拷贝到目标缓冲区的指定区域

**参数**：
- `src_frame` - 源帧缓冲区指针，包含源帧缓冲区的所有信息（地址、尺寸、格式等）
- `dst_frame` - 目标帧缓冲区指针，包含目标帧缓冲区的所有信息（地址、尺寸、格式等）
- `dst_x` - 目标区域起始 X 坐标（像素）
- `dst_y` - 目标区域起始 Y 坐标（像素）

**返回**：
- 成功返回 `BK_OK`，失败返回错误码

### 4. 透明度图片数据融合

#### app_display_draw_blend
```c
int app_display_draw_blend(
    frame_buffer_t *bg_frame,
    frame_buffer_t *fg_frame,
    uint16_t dst_x,
    uint16_t dst_y);
```
**功能**：将前景图像融合到背景图像的指定区域

**参数**：
- `bg_frame` - 背景帧缓冲区指针，包含背景帧缓冲区的所有信息（地址、尺寸、格式等）
- `fg_frame` - 前景帧缓冲区指针，包含前景帧缓冲区的所有信息（地址、尺寸、格式等），必须是 ARGB8888 格式
- `dst_x` - 目标区域起始 X 坐标（像素）
- `dst_y` - 目标区域起始 Y 坐标（像素）

**返回**：
- 成功返回 `BK_OK`，失败返回错误码

### 5. 文字融合

#### app_display_draw_font
```c
int app_display_draw_font(
    frame_buffer_t *bg_frame,
    const char *text,
    unsigned short font_color,
    const gui_font_digit_struct *font_name,
    int font_pos_x,
    int font_pos_y);
```
**功能**：在背景帧缓冲区的指定位置显示文本

**参数**：
- `bg_frame` - 背景帧缓冲区指针，包含背景帧缓冲区的所有信息（地址、尺寸、格式等），支持 PIXEL_FMT_VUYY/PIXEL_FMT_YUYV/PIXEL_FMT_RGB888/PIXEL_FMT_RGB565_LE/PIXEL_FMT_RGB565 格式
- `text` - 要显示的文本字符串
- `font_color` - 字体颜色（RGB565 格式）
- `font_name` - 字体结构体指针，指定使用的字体类型
- `font_pos_x` - 字体显示起始 X 坐标（像素）
- `font_pos_y` - 字体显示起始 Y 坐标（像素）

**返回**：
- 成功返回 `BK_OK`，失败返回错误码

## 数据结构

### frame_buffer_t
帧缓冲区结构体

```c
typedef struct {
    void *frame;           /*!< 帧缓冲区地址 */
    uint32_t size;         /*!< 帧缓冲区大小（字节） */
    uint32_t length;       /*!< 帧缓冲区数据长度（字节） */
    uint16_t width;        /*!< 帧宽度（像素） */
    uint16_t height;       /*!< 帧高度（像素） */
    pixel_fmt_t fmt;       /*!< 帧格式 */
} frame_buffer_t;
```

### pixel_fmt_t
像素格式枚举类型

```c
typedef enum {
    PIXEL_FMT_RGB565,      /*!< RGB565 格式 */
    PIXEL_FMT_RGB565_LE,   /*!< RGB565 小端格式 */
    PIXEL_FMT_RGB888,      /*!< RGB888 格式 */
    PIXEL_FMT_ARGB8888,    /*!< ARGB8888 格式 */
    PIXEL_FMT_VUYY,        /*!< VUYY 格式 */
    PIXEL_FMT_YUYV,        /*!< YUYV 格式 */
} pixel_fmt_t;
```

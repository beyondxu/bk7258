# dma2d 颜色填充、拷贝、融合组件

[toc]

## 概述

`app_dma2d_hw_draw` 是一个基于 DMA2D（Direct Memory Access 2D）硬件加速的绘图组件，提供高效的 2D 图形操作功能。该组件利用 BK 芯片的 DMA2D 硬件模块，实现了内存拷贝、颜色填充和图像融合等功能，相比软件实现具有更高的性能和更低的 CPU 占用率。组件支持多种颜色格式，并提供了灵活的句柄管理机制，适用于各种图形处理场景。

## 功能特性

- 支持 DMA2D 硬件加速的内存区域拷贝
- 支持 DMA2D 硬件加速的颜色填充
- 支持 DMA2D 硬件加速的图像融合
- 支持多种颜色格式：
  - RGB565（2 字节/像素）
  - RGB888（3 字节/像素）
  - ARGB8888（4 字节/像素）
- 支持灵活的句柄管理：
  - 内部自动申请和释放 DMA2D 句柄
  - 支持外部传入 DMA2D 句柄，避免重复申请释放
- 支持同步操作模式
- 支持指定区域的图形操作
- 提供完整的错误处理和日志输出

## 依赖组件和配置

- `bk_dma2d` - DMA2D 硬件驱动组件

## 接口说明

### 1. 内存区域拷贝

#### app_dma2d_copy
```c
int app_dma2d_copy(
    bk_dma2d_ctlr_handle_t dma2d_handle,
    color_bytes_t pix_size,
    void *src_buf,
    uint16_t src_width,
    uint16_t src_height,
    void *dst_buf,
    uint16_t dst_width,
    uint16_t dst_height,
    uint16_t dst_x,
    uint16_t dst_y);
```
**功能**：将源缓冲区数据拷贝到目标缓冲区的指定区域

**参数**：
- `dma2d_handle` - DMA2D 句柄，如果传 NULL 则由接口内部每次申请并释放，如果不想反复申请释放，也可外部传入
- `pix_size` - 每个像素的大小，可以是 RGB565、RGB888、ARGB8888
- `src_buf` - 源缓冲区地址
- `src_width` - 源图像宽度（像素）
- `src_height` - 源图像高度（像素）
- `dst_buf` - 目标缓冲区地址
- `dst_width` - 目标图像宽度（像素）
- `dst_height` - 目标图像高度（像素）
- `dst_x` - 目标区域起始 X 坐标（像素）
- `dst_y` - 目标区域起始 Y 坐标（像素）

**返回**：
- 成功返回 `BK_OK`，失败返回错误码

### 2. 颜色填充

#### app_dma2d_fill_color
```c
int app_dma2d_fill_color(
    bk_dma2d_ctlr_handle_t dma2d_handle,
    color_bytes_t pix_size,
    uint32_t color,
    void *dst_buf,
    uint16_t dst_width,
    uint16_t dst_height,
    uint16_t dst_x,
    uint16_t dst_y,
    uint16_t fill_width,
    uint16_t fill_height);
```
**功能**：在目标缓冲区的指定区域填充指定颜色

**参数**：
- `dma2d_handle` - DMA2D 句柄，如果传 NULL 则由接口内部每次申请并释放，如果不想反复申请释放，也可外部传入
- `pix_size` - 每个像素的大小，可以是 RGB565、RGB888、ARGB8888
- `color` - 填充颜色值
- `dst_buf` - 目标缓冲区地址
- `dst_width` - 目标图像宽度（像素）
- `dst_height` - 目标图像高度（像素）
- `dst_x` - 目标区域起始 X 坐标（像素）
- `dst_y` - 目标区域起始 Y 坐标（像素）
- `fill_width` - 填充区域宽度（像素）
- `fill_height` - 填充区域高度（像素）

**返回**：
- 成功返回 `BK_OK`，失败返回错误码

### 3. 图像融合

#### app_dma2d_blend
```c
int app_dma2d_blend(
    bk_dma2d_ctlr_handle_t dma2d_handle,
    void *fg_buf,
    uint16_t fg_width,
    uint16_t fg_height,
    void *bg_buf,
    uint16_t bg_width,
    uint16_t bg_height,
    color_bytes_t bg_pix_size,
    uint16_t dst_x,
    uint16_t dst_y,
    uint16_t blend_width,
    uint16_t blend_height);
```
**功能**：将前景图像融合到背景图像的指定区域

**参数**：
- `dma2d_handle` - DMA2D 句柄，如果传 NULL 则由接口内部每次申请并释放，如果不想反复申请释放，也可外部传入
- `fg_buf` - 前景缓冲区地址（ARGB8888 格式）
- `fg_width` - 前景图像宽度（像素）
- `fg_height` - 前景图像高度（像素）
- `bg_buf` - 背景缓冲区地址
- `bg_width` - 背景图像宽度（像素）
- `bg_height` - 背景图像高度（像素）
- `bg_pix_size` - 背景像素大小（TWO_BYTES/THREE_BYTES/FOUR_BYTES）
- `dst_x` - 目标区域起始 X 坐标（像素）
- `dst_y` - 目标区域起始 Y 坐标（像素）
- `blend_width` - 融合区域宽度（像素）
- `blend_height` - 融合区域高度（像素）

**返回**：
- 成功返回 `BK_OK`，失败返回错误码

## 数据结构

### color_bytes_t
像素大小枚举类型

```c
typedef enum {
    TWO_BYTES,      /*!< 2 字节/像素（RGB565） */
    THREE_BYTES,    /*!< 3 字节/像素（RGB888） */
    FOUR_BYTES,     /*!< 4 字节/像素（ARGB8888） */
} color_bytes_t;
```

### bk_dma2d_ctlr_handle_t
DMA2D 控制器句柄类型

```c
typedef void *bk_dma2d_ctlr_handle_t;
```

## 相关文件

- `app_dma2d_hw_draw.h` - DMA2D 硬件绘图头文件
- `app_dma2d_hw_draw.c` - DMA2D 硬件绘图实现文件
- `CMakeLists.txt` - 组件构建配置文件

# DMA2D图像融合示例（ARGB8888前景融合到RGB565背景）

| 支持的芯片 | BK7258 |
| --------- |  ------- |

[toc]

## 相关章节
 - [显示绘制组件（颜色填充、图片数据搬移、透明度图片数据融合、文字融合）](/components/app_display_draw/README.md)

## 概述

BK7258集成的DMA2D（Direct Memory Access 2D）硬件加速模块，专门用于图形处理操作，包括：
- 内存拷贝（支持区域拷贝）
- 颜色填充（支持RGB565、RGB888、ARGB8888格式）
- 图像融合（支持Alpha混合）

本示例演示如何使用DMA2D硬件加速进行图像融合操作，实现ARGB8888格式的前景图像融合到RGB565格式的背景图像，具体功能包括：
1. 如何打开LCD显示屏
2. 如何分配背景和前景frame buffer内存
3. 如何使用DMA2D填充背景颜色
4. 如何准备ARGB8888格式的前景数据
5. 如何使用DMA2D进行图像融合（Alpha混合）
6. 如何将融合后的frame buffer刷新到LCD显示

## 硬件环境

- 核心板：BK7258_QFN88_V3
- 显示屏：st7701s 480X480 RGB屏幕

> **备注** ：如果更换为其它屏幕，需修改dev_lcd.c中的屏幕参数: rgb_config里的屏的lcd_device，相关io管脚：ldo、背光、复位、cs、clk、sda；

![bk_vfs](/multimedia/boot_anim/v3_demo_board.jpg)

## 相关配置

以下配置在AP核，可通过 [menuconfig](https://gitlab.bekencorp.com/armino/release_sdk_sample/smp/sample_project#4配置修改) 进行配置。

| 配置名                    | 说明                               |
| -------------------- | ------------------------------------------ |
| CONFIG_DMA2D    | DMA2D 硬件驱动组件使能配置，CONFIG_APP_DISPLAY_DRAW_DMA2D开启时需开启，此用例配置为y         |
| CONFIG_MEDIA_DAM2D   | DMA2D sdk组件使能配置，CONFIG_APP_DISPLAY_DRAW_DMA2D开启时需开启，此用例配置为y|
| CONFIG_APP_DISPLAY_DRAW          | 显示绘制使能配置，需配置为y                                         |
| CONFIG_APP_DISPLAY_DRAW_DMA2D    | 绘制启用硬件 DMA2D 实现，默认配置为y         |
| CONFIG_LCD_ST7701S | st7701s屏幕使能配置，配置为y         |

## 依赖模块

- sample_project/components/app_display_draw

**根目录下的 CMakeLists.txt** 中需要添加以下内容（放入自己工程需留意自己工程相对components的路径）：

``` cmake
set(EXTRA_COMPONENTS_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../components/app_display_draw
)
```

**ap/CMakeLists.txt** PRIV_REQUIRES中需加入app_display_draw依赖，这样可直接调用显示绘制模块头文件：

``` cmake
set(priv_req  media_service multimedia app_display_draw)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
```

## 功能说明

本示例演示了DMA2D图像融合的基本用法：

### 1. 初始化LCD和Frame Buffer

```c
dev_lcd_open();                    // 打开lcd

frame_buffer_t *bg_frame = NULL;
frame_buffer_t *fg_frame = NULL;
int lcd_width = dev_lcd_get_width();
int lcd_height = dev_lcd_get_height();

// 分配背景frame buffer（RGB565格式，每个像素2字节）
bg_frame = decode_frame_malloc(lcd_width * lcd_height * 2);
bg_frame->fmt = PIXEL_FMT_RGB565;
bg_frame->width = lcd_width;
bg_frame->height = lcd_height;

// 分配前景frame buffer（ARGB8888格式，每个像素4字节）
fg_frame = decode_frame_malloc(FG_WIDTH * FG_HEIGHT * 4);
fg_frame->fmt = PIXEL_FMT_ARGB8888;
fg_frame->width = FG_WIDTH;
fg_frame->height = FG_HEIGHT;
```

### 2. 填充背景颜色

```c
// 填充红色背景（RGB565格式，0xf800）
ret = app_display_draw_fill_color(bg_frame, 0xf800, 0, 0, lcd_width, lcd_height);
```

### 3. 准备前景数据（ARGB8888格式）

```c
os_memcpy(fg_frame->frame, icon_map, FG_WIDTH * FG_HEIGHT * 4);
```

> **备注**：用户可以在此处填充自己的ARGB8888格式的前景图像数据。

### 4. 图像融合（Alpha混合）

```c
blend_x_pos = (lcd_width - FG_WIDTH) / 2;   // 水平居中
blend_y_pos = (lcd_height - FG_HEIGHT) / 2; // 垂直居中
app_display_draw_blend(bg_frame, fg_frame, blend_x_pos, blend_y_pos);
```

### 5. 刷屏显示

```c
dev_lcd_flush(bg_frame, display_free_callback);   // 刷屏
dev_lcd_backlight_ctrl(100);                       // 开背光
```

### 6. 反初始化显示绘制组件

```c
app_display_draw_deinit();
```

## 操作说明
### 编译

运行以下命令进行编译构建

``` Shell
cd sample_project/peripherals/dma2d/fg_argb888_blend_bg_rgb565
vim Makefile  // 修改SDK_DIR为实际smp sdk路径
make clean
make bk7258
```

### 操作方法及日志

- 参考[如何烧录](/BURN.md)，烧录工程build下的all-app.bin；
- 上电后，LCD屏上会显示红色背景，在屏幕中央显示透明图片；
<br>
![bk_vfs](/tools/littlefs/bk/img/icon.png)

## 故障排除

如有任何技术疑问，请联系我们FAE支持，或者在博通集成技术论坛上提交一个[问题](https://armino.bekencorp.com/)。我们会尽快回复您。

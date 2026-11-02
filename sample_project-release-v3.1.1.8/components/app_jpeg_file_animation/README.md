# JPEG 图片动画播放组件

[toc]

## 参考示例

- [开机动画](/multimedia/boot_anim/README.md) 

## 概述

`app_jpeg_file_animation` 是一个 JPEG 图片动画播放组件，基于 JPEG 硬件解码能力，提供高效的图片动画播放功能。该组件通过消息队列和线程机制实现异步播放，支持多种解码格式和灵活的回调函数机制。组件提供了完整的动画生命周期管理，包括初始化、开始播放、停止和反初始化等接口，适用于各种动画展示场景。

## 功能特性

- 支持 JPEG 图片序列动画播放
- 支持多种解码格式：
  - YUV422 格式
  - RGB565 小端格式
  - RGB565 大端格式
- 基于消息队列和线程的异步播放机制
- 支持灵活的回调函数：
  - 融合回调函数
  - LCD 显示回调函数
  - 动画完成回调函数
- 支持播放参数配置：
  - 图片文件名前缀
  - 图片张数
  - 播放次数
  - 循环播放
- 支持自定义用户数据传递
- 提供完整的播放状态管理
- 支持播放中途停止和重新开始

## 依赖组件和配置

- `jpeg_hw_decode` - JPEG 硬件解码组件


## 接口说明

### 1. 初始化和反初始化

#### animation_module_init
```c
int animation_module_init(const animation_module_config_t *config);
```
**功能**：初始化动画模块

**参数**：
- `config` - 输入参数，动画模块配置结构体指针

**返回**：
- 成功返回 0，失败返回负数

#### animation_module_deinit
```c
int animation_module_deinit(void);
```
**功能**：停止并反初始化动画模块

**参数**：
- 无

**返回**：
- 成功返回 0，失败返回负数

### 2. 播放控制

#### animation_start_play
```c
int animation_start_play(const char *file_prefix, uint32_t frame_count, uint32_t repeat_count, bool is_loop);
```
**功能**：开始播放动画

**参数**：
- `file_prefix` - 输入参数，图片文件名前缀
- `frame_count` - 输入参数，图片张数
- `repeat_count` - 输入参数，播放次数
- `is_loop` - 输入参数，是否循环播放

**返回**：
- 成功返回 0，失败返回负数

## 数据结构

### animation_module_config_t
动画模块配置结构体

```c
typedef struct {
    jpeg_hw_decode_config_t jpeg_hw_decode_config;   /*!< JPEG 原始图片、解码所用到的内存申请及释放接口 */
    animation_decode_format_e decode_format;           /*!< 解码输出的编码格式 */
    animation_blend_cb_t blend_cb;                     /*!< 融合回调函数 */
    animation_lcd_cb_t lcd_cb;                         /*!< LCD 显示回调函数 */
    animation_finish_cb_t finish_cb;                   /*!< 动画完成回调函数 */
    void *user_data;                                   /*!< 用户自定义数据 */
} animation_module_config_t;
```

### animation_play_param_t
动画播放参数结构体

```c
typedef struct {
    const char *file_prefix;   /*!< 图片文件名前缀 */
    uint32_t frame_count;      /*!< 图片张数 */
    uint32_t repeat_count;     /*!< 播放次数 */
    bool is_loop;              /*!< 是否循环播放 */
} animation_play_param_t;
```

### animation_decode_format_e
解码输出格式枚举

```c
typedef enum {
    ANIMATION_DECODE_FORMAT_YUV422 = 0,       /*!< YUV422 格式 */
    ANIMATION_DECODE_FORMAT_RGB565_LITTLE,    /*!< RGB565 小端格式 */
    ANIMATION_DECODE_FORMAT_RGB565_BIG,       /*!< RGB565 大端格式 */
    ANIMATION_DECODE_FORMAT_MAX               /*!< 格式数量最大值 */
} animation_decode_format_e;
```

### animation_msg_type_e
消息队列消息类型枚举

```c
typedef enum {
    ANIMATION_MSG_IDLE = 0,   /*!< 空闲消息 - 动画播放完成时触发 */
    ANIMATION_MSG_START,      /*!< 开始消息 - 外部调用开始接口时触发 */
    ANIMATION_MSG_EXIT        /*!< 退出消息 - 外部调用反初始化接口时触发 */
} animation_msg_type_e;
```

### 回调函数类型定义

#### animation_blend_cb_t
```c
typedef void (*animation_blend_cb_t)(frame_buffer_t *bg_frame, void *user_data);
```
**功能**：融合回调函数

**参数**：
- `bg_frame` - 解码后的帧数据
- `user_data` - 用户自定义数据

**返回**：
- 无

#### animation_lcd_cb_t
```c
typedef void (*animation_lcd_cb_t)(frame_buffer_t *display_frame, void *user_data);
```
**功能**：LCD 显示回调函数

**参数**：
- `display_frame` - 解码后的帧数据
- `user_data` - 用户自定义数据

**返回**：
- 无

#### animation_finish_cb_t
```c
typedef void (*animation_finish_cb_t)(void *user_data);
```
**功能**：动画播完回调函数

**参数**：
- `user_data` - 用户自定义数据

**返回**：
- 无

## 注意事项

1. **初始化顺序**：必须先调用 `animation_module_init` 初始化动画模块，才能调用其他播放接口
2. **回调函数**：所有回调函数必须在初始化时正确设置，否则动画播放可能无法正常工作
3. **文件命名**：图片文件必须按照 `前缀 + 序号 + .jpg` 的格式命名，序号从 0 开始
4. **播放参数**：
   - `frame_count` 必须与实际图片数量一致
   - `repeat_count` 为 0 表示无限播放
   - `is_loop` 为 true 时会循环播放，为 false 时播放指定次数后停止
5. **内存管理**：回调函数中的帧缓冲区内存由组件内部管理，回调函数不需要释放
6. **播放状态**：
   - 在调用 `animation_start_play` 前应检查模块是否已初始化
   - 可以在播放过程中再次调用 `animation_start_play` 开始新的播放
7. **停止播放**：
   - 调用 `animation_module_deinit` 会停止当前播放并释放所有资源
   - 停止播放后会触发 `finish_cb` 回调
8. **用户数据**：`user_data` 参数会在所有回调函数中传递，可用于保存上下文信息
9. **线程安全**：组件内部使用消息队列和线程机制，接口调用是线程安全的
10. **解码格式**：解码格式必须与实际 JPEG 图片格式匹配，否则可能导致解码失败

## 相关文件

- `jpeg_file_animation.h` - JPEG 文件动画头文件
- `jpeg_file_animation.c` - JPEG 文件动画实现文件
- `CMakeLists.txt` - 组件构建配置文件

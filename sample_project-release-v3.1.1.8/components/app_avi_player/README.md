# app_avi_player

[toc]

## 参考示例

- [MULTIMEDIA AVI播放示例](/multimedia/avi/display_lcd_RGB565/README.md) 

## 概述

`app_avi_player` 是一个基于 AVI 格式的音视频播放器组件，支持硬件 JPEG 解码，可同时播放视频和音频。该组件提供了完整的 AVI 文件播放控制接口，支持播放、暂停、恢复、停止、循环播放等功能。

## 功能特性

- 支持 AVI 格式音视频文件播放
- 支持硬件 JPEG 解码加速
- 支持多种视频解码输出格式：
  - YUV422
  - RGB565 小端格式
  - RGB565 大端格式
- 支持视频和音频同步播放
- 支持播放控制：开始、暂停、恢复、停止
- 支持循环播放模式
- 支持播放位置定位（秒级）
- 提供播放完成回调通知
- 支持自定义 LCD 显示和音频播放回调

## 依赖组件

- `bk_common` - 基础通用组件
- `include` - 头文件组件
- `app_jpeg_hw_decode` - JPEG 硬件解码组件
- `multimedia` - 多媒体组件
- `bk_libavi.a` - AVI 解析静态库

## 接口说明

### 1. 初始化和反初始化

#### avi_player_init
```c
int avi_player_init(avi_player_config_t *config);
```
**功能**：初始化 AVI 播放器

**参数**：
- `config` - AVI 播放器配置结构体

**返回**：
- 成功返回 0，失败返回负数

#### avi_player_deinit
```c
int avi_player_deinit(void);
```
**功能**：反初始化 AVI 播放器

**返回**：
- 成功返回 0，失败返回负数

### 2. 文件操作

#### avi_player_file_open
```c
int avi_player_file_open(const char *file_path);
```
**功能**：打开 AVI 文件

**参数**：
- `file_path` - 文件路径

**返回**：
- 成功返回 0，失败返回负数

#### avi_player_file_close
```c
int avi_player_file_close(void);
```
**功能**：关闭 AVI 文件

**返回**：
- 成功返回 0，失败返回负数

### 3. 播放控制

#### avi_player_start
```c
int avi_player_start(void);
```
**功能**：开始播放 AVI 文件

**返回**：
- 成功返回 0，失败返回负数

#### avi_player_stop
```c
int avi_player_stop(void);
```
**功能**：停止播放 AVI 文件

**返回**：
- 成功返回 0，失败返回负数

#### avi_player_pause
```c
int avi_player_pause(void);
```
**功能**：暂停播放 AVI 文件

**返回**：
- 成功返回 0，失败返回负数

#### avi_player_resume
```c
int avi_player_resume(void);
```
**功能**：恢复播放 AVI 文件

**返回**：
- 成功返回 0，失败返回负数

### 4. 播放设置

#### avi_player_set_loop
```c
int avi_player_set_loop(bool loop);
```
**功能**：设置循环播放模式

**参数**：
- `loop` - 是否循环播放

**返回**：
- 成功返回 0，失败返回负数

#### avi_player_set_pos
```c
int avi_player_set_pos(uint32_t pos);
```
**功能**：设置播放位置

**参数**：
- `pos` - 播放位置（单位：秒）

**返回**：
- 成功返回 0，失败返回负数

### 5. 信息获取

#### avi_player_get_video_info
```c
avi_video_info_t avi_player_get_video_info(void);
```
**功能**：获取 AVI 文件视频信息

**返回**：
- 视频信息结构体

#### avi_player_get_audio_info
```c
avi_audio_info_t avi_player_get_audio_info(void);
```
**功能**：获取 AVI 文件音频信息

**返回**：
- 音频信息结构体

## 数据结构

### avi_player_config_t
AVI 播放器配置结构体

```c
typedef struct{
    jpeg_hw_decode_config_t jpeg_hw_decode_config;  /*!< JPEG硬件解码配置 */
    avi_player_dec_format_e decode_format;          /*!< 解码输出的编码格式 */
    avi_player_lcd_cb_t lcd_cb;                     /*!< LCD显示回调函数 */

    avi_player_voice_malloc_cb_t voice_malloc_cb;   /*!< 音频 malloc 回调函数 */
    avi_player_voice_free_cb_t voice_free_cb;       /*!< 音频 free 回调函数 */
    avi_player_speaker_cb_t speaker_cb;             /*!< 音频播放回调函数 */
    uint32_t audio_frame_size;                      /*!< 音频帧大小（单位：字节）>*/

    avi_player_finish_cb_t finish_cb;               /*!< 播放完成回调函数 */

    uint32_t sync_time_threshold_ms;                /*!< 视频同步线程阈值（单位：ms）>*/
    void *user_data;                                /*!< 用户自定义数据 */   
} avi_player_config_t;
```

### avi_video_info_t
视频信息结构体

```c
typedef struct{
    long width;                 /*!< 视频宽度>*/
    long height;                /*!< 视频高度>*/
    double fps;                 /*!< 视频帧率>*/
    long video_frame_num;       /*!< 视频总帧数>*/
    char compresstype[8];       /*!< 视频格式>*/

    long video_len;             /*!< 帧长度>*/
    long video_cur_pos;         /*!< 当前视频帧位置>*/
    bool video_enable;          /*!< 是否开启视频>*/
    bool video_is_end;          /*!< 视频播放结束标志>*/
} avi_video_info_t;
```

### avi_audio_info_t
音频信息结构体

```c
typedef struct{
    long chans;                 /*!< 音频通道数>*/
    long bits;                  /*!< 音频位深>*/
    long sample_rate;           /*!< 音频采样率>*/
    long audio_total_bytes;     /*!< 音频总大小>*/
    long audio_fmt;             /*!< 音频格式>*/

    long frame_time;            /*!< 音频帧时间（单位：ms）>*/
    long audio_len;             /*!< 帧长度>*/
    long audio_cur_pos;         /*!< 当前音频帧位置>*/  
    bool audio_enable;          /*!< 是否开启音频>*/
    bool audio_is_end;          /*!< 音频播放结束标志>*/    
} avi_audio_info_t;
```

## 注意事项

1. **硬件解码要求**：使用 AVI 播放器前必须先初始化 JPEG 硬件解码器
2. **内存管理**：音频回调函数中的内存分配和释放必须成对出现，避免内存泄漏
3. **同步阈值**：`sync_time_threshold_ms` 参数影响视频同步性能，建议根据实际帧率调整（通常设置为 65ms）
4. **文件系统**：确保 AVI 文件所在的存储设备已正确挂载
5. **LCD 和音频设备**：使用前需要先初始化 LCD 和音频设备
6. **线程安全**：AVI 播放器内部使用多线程，避免在回调函数中执行耗时操作
7. **格式支持**：确保 AVI 文件的视频编码为 JPEG 格式，音频编码为 PCM 格式

## 相关文件

- `bk_avi_player.h` - AVI 播放器头文件
- `bk_avi_player.c` - AVI 播放器实现文件
- `bk_avilib.h` - AVI 解析库头文件
- `bk_libavi.a` - AVI 解析静态库

# AVI 文件录制组件

[toc]

## 参考示例

- [DVP摄像头视频保存为AVI示例](/multimedia/avi/dvp_camera_recode_avi/README.md) 

## 概述

`app_avi_record` 是一个 AVI 文件录制组件，基于 AVI 文件格式标准，提供高效的视频和音频录制功能。该组件支持 MJPEG 视频编码和 PCM 音频编码，通过回调函数机制实现灵活的数据采集和录制流程。组件提供了完整的录制生命周期管理，包括初始化、开始录制、停止录制和反初始化等接口，适用于各种视频录制场景。

## 功能特性

- 支持 AVI 文件格式录制
- 支持 MJPEG 视频编码格式
- 支持 PCM 音频编码格式
- 支持视频和音频同时录制或单独录制
- 提供灵活的回调函数机制：
  - 视频帧读取回调
  - 视频帧完成回调
  - 音频帧读取回调
  - 音频帧完成回调
  - 录制完成回调
- 支持自定义视频参数：
  - 视频宽度和高度
  - 视频帧率
  - 视频编码格式
- 支持自定义音频参数：
  - 音频通道数
  - 音频采样率
  - 音频位深
  - 音频编码格式
- 提供录制状态查询接口
- 支持自定义用户数据传递


## 接口说明

### 1. 初始化和反初始化

#### avi_record_init
```c
int avi_record_init(avi_record_config_t *config);
```
**功能**：初始化 AVI 录制器

**参数**：
- `config` - 输入参数，录制配置结构体指针

**返回**：
- 成功返回 0，失败返回 -1

#### avi_record_deinit
```c
int avi_record_deinit(void);
```
**功能**：反初始化 AVI 录制器

**参数**：
- 无

**返回**：
- 成功返回 0，失败返回 -1

### 2. 录制控制

#### avi_record_start
```c
int avi_record_start(char *record_path);
```
**功能**：开始录制 AVI 文件

**参数**：
- `record_path` - 输入参数，录制文件路径

**返回**：
- 成功返回 0，失败返回 -1

#### avi_record_stop
```c
int avi_record_stop(void);
```
**功能**：停止录制 AVI 文件

**参数**：
- 无

**返回**：
- 成功返回 0，失败返回 -1

### 3. 状态查询

#### avi_record_get_record_status
```c
bool avi_record_get_record_status(void);
```
**功能**：获取录制状态

**参数**：
- 无

**返回**：
- 返回录制状态，true 表示正在录制，false 表示未在录制

## 数据结构

### avi_record_config_t
AVI 录制配置结构体

```c
typedef struct{
    bool video_enable;                                      /*!< 是否开启视频录制 */
    int video_width;                                        /*!< 视频宽度 */
    int video_height;                                       /*!< 视频高度 */
    double video_fps;                                       /*!< 视频帧率 */
    avi_video_format_e video_format;                        /*!< 视频编码类型 */
    read_video_frame_cb_t read_video_frame_cb;              /*!< 读取视频帧回调函数 */
    complete_video_frame_cb_t complete_video_frame_cb;      /*!< 完成视频帧回调函数 */
    
    bool audio_enable;                                      /*!< 是否开启音频录制 */
    int audio_chan;                                         /*!< 音频通道数 */
    long audio_sample;                                      /*!< 音频采样率 */
    int audio_bits;                                         /*!< 音频位深 */
    avi_audio_format_e audio_format;                        /*!< 音频编码类型 */
    read_audio_frame_cb_t read_audio_frame_cb;              /*!< 读取音频帧回调函数 */
    complete_audio_frame_cb_t complete_audio_frame_cb;      /*!< 完成音频帧回调函数 */
    
    record_finish_cb_t record_finish_cb;                    /*!< 录制完成回调函数 */
    void *user_data;                                        /*!< 自定义用户数据 */
}avi_record_config_t;
```

### avi_video_format_e
视频编码格式枚举

```c
typedef enum{
    AVI_VIDEO_FORMAT_NONE = 0,                              /*!< 无视频 */
    AVI_VIDEO_FORMAT_MJPEG,                                 /*!< MJPEG 编码 */
    AVI_VIDEO_FORMAT_MAX,
} avi_video_format_e;
```

### avi_audio_format_e
音频编码格式枚举

```c
typedef enum{
    AVI_AUDIO_FORMAT_NONE = 0,                              /*!< 无音频 */
    AVI_AUDIO_FORMAT_PCM,                                   /*!< PCM 编码 */
    AVI_AUDIO_FORMAT_MAX,
} avi_audio_format_e;
```

### audio_frame_t
音频帧结构体

```c
typedef struct{
    uint8_t *audio_data;                                    /*!< 音频数据指针 */
    uint32_t audio_data_length;                              /*!< 音频数据长度 */
} audio_frame_t;
```

### 回调函数类型定义

#### read_video_frame_cb_t
```c
typedef frame_buffer_t *(*read_video_frame_cb_t)(void);
```
**功能**：读取视频帧回调函数

**返回**：
- 返回视频帧缓冲区指针

#### complete_video_frame_cb_t
```c
typedef bk_err_t (*complete_video_frame_cb_t)(bk_err_t result, frame_buffer_t *out_frame);
```
**功能**：完成视频帧回调函数

**参数**：
- `result` - 操作结果
- `out_frame` - 输出的视频帧缓冲区

**返回**：
- 返回操作结果

#### record_finish_cb_t
```c
typedef void (*record_finish_cb_t)(void* user_data);
```
**功能**：录制完成回调函数

**参数**：
- `user_data` - 用户自定义数据

**返回**：
- 无

#### read_audio_frame_cb_t
```c
typedef audio_frame_t *(*read_audio_frame_cb_t)(void);
```
**功能**：读取音频帧回调函数

**返回**：
- 返回音频帧结构体指针

#### complete_audio_frame_cb_t
```c
typedef bk_err_t (*complete_audio_frame_cb_t)(bk_err_t result, audio_frame_t *out_frame);
```
**功能**：完成音频帧回调函数

**参数**：
- `result` - 操作结果
- `out_frame` - 输出的音频帧结构体

**返回**：
- 返回操作结果

## 注意事项

1. **初始化顺序**：必须先调用 `avi_record_init` 初始化录制器，才能调用其他录制接口
2. **回调函数**：所有回调函数必须在初始化时正确设置，否则录制可能无法正常工作
3. **内存管理**：回调函数返回的帧缓冲区内存由调用方负责管理
4. **录制路径**：确保录制文件路径所在的存储设备已正确挂载且有足够的存储空间
5. **视频参数**：视频宽度和高度必须与实际采集的视频帧尺寸一致
6. **音频参数**：音频采样率和位深必须与实际采集的音频数据参数一致
7. **录制状态**：在调用 `avi_record_start` 前应检查是否已经在录制状态，避免重复调用
8. **停止录制**：在停止录制前，确保所有帧数据已写入完成，避免文件损坏
9. **反初始化**：在调用 `avi_record_deinit` 前，必须先停止录制
10. **用户数据**：`user_data` 参数会在回调函数中传递，可用于保存上下文信息

## 相关文件

- `bk_avi_record.h` - AVI 录制头文件
- `bk_avi_record.c` - AVI 录制实现文件
- `bk_avilib.h` - AVI 库头文件
- `bk_libavi.a` - AVI 库静态库文件
- `CMakeLists.txt` - 组件构建配置文件

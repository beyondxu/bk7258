#pragma once

#include "bk_avilib.h"
#include "frame_buffer.h"

typedef struct{
    uint8_t *audio_data;
    uint32_t audio_data_length;
} audio_frame_t;

typedef frame_buffer_t *(*read_video_frame_cb_t)(void);

typedef bk_err_t (*complete_video_frame_cb_t)(bk_err_t result, frame_buffer_t *out_frame);

typedef void (*record_finish_cb_t)(void* user_data);

typedef audio_frame_t *(*read_audio_frame_cb_t)(void);

typedef bk_err_t (*complete_audio_frame_cb_t)(bk_err_t result, audio_frame_t *out_frame);

typedef enum{
    AVI_VIDEO_FORMAT_NONE = 0,
    AVI_VIDEO_FORMAT_MJPEG,
    AVI_VIDEO_FORMAT_MAX,
} avi_video_format_e;

typedef enum{
    AVI_AUDIO_FORMAT_NONE = 0,
    AVI_AUDIO_FORMAT_PCM,
    AVI_AUDIO_FORMAT_MAX,
} avi_audio_format_e;


typedef struct{
    bool video_enable;                                      // 是否开启视频录制
    int video_width;                                        // 视频宽度
    int video_height;                                       // 视频高度
    double video_fps;                                       // 视频帧率
    avi_video_format_e video_format;                        // 视频编码类型
    read_video_frame_cb_t read_video_frame_cb;              // 读取视频帧回调函数
    complete_video_frame_cb_t complete_video_frame_cb;      // 完成视频帧回调函数
    
    bool audio_enable;                                      // 是否开启音频录制
    int audio_chan;                                         // 音频通道数
    long audio_sample;                                      // 音频采样率
    int audio_bits;                                         // 音频位深 
    avi_audio_format_e audio_format;                        // 音频编码类型
    read_audio_frame_cb_t read_audio_frame_cb;              // 读取音频帧回调函数
    complete_audio_frame_cb_t complete_audio_frame_cb;      // 完成音频帧回调函数
    
    record_finish_cb_t record_finish_cb;                    // 录制完成回调函数
    void *user_data;                                        // 自定义用户数据
}avi_record_config_t;


/**
 * @brief 初始化录制
 * 
 * @return int 成功返回0，失败返回-1
 */
int avi_record_init(avi_record_config_t *config);

/**
 * @brief 反初始化录制
 * 
 * @return int 成功返回0，失败返回-1
 */
int avi_record_deinit(void);


/**
 * @brief 开始录制
 * 
 * @param record_path 录制路径
 * @return int 成功返回0，失败返回-1
 */
int avi_record_start(char *record_path);

/**
 * @brief 停止录制
 * 
 * @return int 成功返回0，失败返回-1
 */
int avi_record_stop(void);

/**
 * @brief 获取录制状态
 * 
 * @return bool 录制状态
 */
bool avi_record_get_record_status(void);

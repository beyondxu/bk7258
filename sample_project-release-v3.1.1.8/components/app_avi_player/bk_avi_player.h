#pragma once

#include "bk_avilib.h"
#include "frame_buffer.h"
#include "jpeg_hw_decode.h"

typedef uint8_t *(*avi_player_voice_malloc_cb_t)(uint32_t size);

typedef void (*avi_player_voice_free_cb_t)(uint8_t *audio_frame);

typedef void (*avi_player_lcd_cb_t)(frame_buffer_t *display_frame, void *user_data);

typedef void (*avi_player_speaker_cb_t)(uint8_t *audio_frame, uint32_t size, void *user_data);

typedef void (*avi_player_finish_cb_t)(void *user_data);

typedef enum{
    AVI_PLAYER_DECODE_FORMAT_YUV422 = 0,        /*!< YUV422格式 */
    AVI_PLAYER_DECODE_FORMAT_RGB565_LB,         /*!< RGB565小端格式 */
    AVI_PLAYER_DECODE_FORMAT_RGB565_BIG,        /*!< RGB565大端格式 */
    AVI_PLAYER_DECODE_FORMAT_MAX                /*!< 格式数量最大值 */
} avi_player_dec_format_e;

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


/**
 * @brief avi播放器初始化
 * @param config avi播放器配置结构体
 * @return 成功返回0，失败返回负数
 */
int avi_player_init(avi_player_config_t *config); 

/**
 * @brief avi播放器反初始化
 * @return 成功返回0，失败返回负数
 */
int avi_player_deinit();

/**
 * @brief avi播放器打开文件
 * @param file_path 文件路径
 * @return 成功返回0，失败返回负数
 */
int avi_player_file_open(const char *file_path); 

/**
 * @brief avi播放器关闭文件
 * @return 成功返回0，失败返回负数
 */
int avi_player_file_close(void);

/**
 * @brief avi播放器开始播放
 * @return 成功返回0，失败返回负数
 */
int avi_player_start(void);

/**
 * @brief avi播放器结束播放
 * @return 成功返回0，失败返回负数
 */
int avi_player_stop(void);

/**
 * @brief avi播放器暂停播放
 * @return 成功返回0，失败返回负数
 */
int avi_player_pause(void);

/**
 * @brief avi播放器恢复播放
 * @return 成功返回0，失败返回负数
 */
int avi_player_resume(void);

/**
 * @brief avi播放器设置循环播放模式
 * @param loop 是否循环播放
 * @return 成功返回0，失败返回负数
 */
int avi_player_set_loop(bool loop);

/**
 * @brief avi播放器设置播放位置
 * @param pos 播放位置（单位：秒）
 * @return 成功返回0，失败返回负数
 */
int avi_player_set_pos(uint32_t pos);

/**
 * @brief avi播放器获取AVI文件视频信息
 * @return 视频信息结构体
 */
avi_video_info_t avi_player_get_video_info(void);

/**
 * @brief avi播放器获取AVI文件音频信息
 * @return 音频信息结构体
 */
avi_audio_info_t avi_player_get_audio_info(void);

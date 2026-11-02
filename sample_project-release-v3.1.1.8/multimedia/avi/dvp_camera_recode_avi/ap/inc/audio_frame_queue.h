#ifndef _AUDIO_QUEUE_H_
#define _AUDIO_QUEUE_H_

#include <os/os.h>
/**
 * @brief 音频格式枚举
 */
typedef enum
{
    AUDIO_PCM,
    AUDIO_G711A,
    AUDIO_G711U,
    AUDIO_UNKNOWN
} audio_format_t;

typedef struct
{
    uint8_t *audio_data;
    uint32_t audio_data_length;
} audio_buffer_t;


/**
 * @brief 音频缓冲区结构体
 */

/**
 * @brief 初始化单个音频队列
 *
 * @param format 音频格式
 * @return bk_err_t 初始化结果
 */
bk_err_t audio_queue_init(audio_format_t format);

/**
 * @brief 初始化所有音频队列
 *
 * @return bk_err_t 初始化结果
 */
bk_err_t audio_queue_init_all(void);

/**
 * @brief 释放单个音频队列
 *
 * @param format 音频格式
 * @return bk_err_t 释放结果
 */
bk_err_t audio_queue_deinit(audio_format_t format);

/**
 * @brief 释放所有音频队列
 *
 * @return bk_err_t 释放结果
 */
bk_err_t audio_queue_deinit_all(void);

/**
 * @brief 从音频队列中申请一个音频缓冲区
 *
 * @param format 音频格式
 * @param size 申请的音频缓冲区大小
 * @return audio_buffer_t* 申请到的音频缓冲区，如果申请失败则返回NULL
 */
audio_buffer_t *audio_queue_malloc(audio_format_t format, uint32_t size);

/**
 * @brief 从音频队列的ready队列中获取一个音频缓冲区
 *
 * @param format 音频格式
 * @param timeout 超时时间（毫秒）
 * @return audio_buffer_t* 获取到的音频缓冲区，如果获取失败则返回NULL
 */
audio_buffer_t *audio_queue_get_frame(audio_format_t format, uint32_t timeout);

/**
 * @brief 将音频缓冲区放回ready队列
 *
 * @param format 音频格式
 * @param audio_frame 要放回队列的音频缓冲区
 * @return bk_err_t 操作结果
 */
bk_err_t audio_queue_complete(audio_format_t format, audio_buffer_t *audio_frame);

/**
 * @brief 根据音频格式释放音频缓冲区，并将消息发送到free队列
 *
 * @param format 音频格式
 * @param audio_frame 要释放的音频缓冲区
 */
void audio_queue_free(audio_format_t format, audio_buffer_t *audio_frame);

#endif
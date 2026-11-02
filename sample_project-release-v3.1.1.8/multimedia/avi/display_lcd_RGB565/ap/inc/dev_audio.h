#ifndef __DEV_AUDIO_H__
#define __DEV_AUDIO_H__

#include <components/bk_audio/audio_pipeline/audio_types.h>

#define SPK_MUTE_IO     GPIO_5                      // 扬声器 mute 脚

typedef void (*speaker_free_cb_t)(void *arg);

/**
 * @brief 打开音频设备
 * @param aec        是否启用回声消除（AEC）
 * @param chan       通道数
 * @param samplerate 采样率（Hz）
 * @param fmt        音频数据格式
 * @return 0 成功；负值 错误码
 */
int dev_audio_open(bool aec, uint32_t chan, uint32_t samplerate, audio_enc_type_t fmt);

/**
 * @brief 关闭音频设备
 * @return 0 成功；负值 错误码
 */
int dev_audio_close(void);

/**
 * @brief 将音频数据发送至扬声器播放
 * @param data   待播放的音频数据缓冲区
 * @param len    数据长度（字节）
 * @param free_cb 数据播放完成后的回调，用于释放缓冲区；可为 NULL
 * @return 0 成功；负值 错误码
 */
int dev_audio_sendto_speaker(uint8_t *data, uint32_t len, speaker_free_cb_t free_cb);

#endif
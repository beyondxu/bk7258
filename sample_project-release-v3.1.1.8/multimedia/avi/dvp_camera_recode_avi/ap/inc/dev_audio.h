#ifndef __DEV_AUDIO_H__
#define __DEV_AUDIO_H__

#include <components/bk_audio/audio_pipeline/audio_types.h>

#define SPK_MUTE_IO         GPIO_5                      // 扬声器 mute 脚

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

#endif
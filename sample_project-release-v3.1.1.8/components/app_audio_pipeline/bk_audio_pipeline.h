#ifndef __BK_AUDIO_PIPELINE_H__
#define __BK_AUDIO_PIPELINE_H__

#include <os/os.h>

typedef enum
{
    CODEC_FORMAT_UNKNOW = 0,
    CODEC_FORMAT_PCM 	= 1,
    CODEC_FORMAT_G711A 	= 2,
    CODEC_FORMAT_G711U 	= 3,
} codec_format_t;

typedef struct
{
	uint8_t uac;	/* 设备选择, 1:使用usb   audio; 0:使用板载麦克风与扬声器 */
	uint8_t aec;	/* 使能AEC算法 */
	uint8_t vad;	/* 使能VAD算法 */
	uint8_t asr;	/* 使能ASR算法 */

	uint8_t rmt_recoder_fmt; 	/* 录音数据输出格式，使用codec_format_t枚举值 */
	uint8_t rmt_player_fmt; 	/* 播放数据输入格式，使用codec_format_t枚举值 */
	uint32_t rmt_recorder_sample_rate;	/* 录音采样率 */
	uint32_t rmt_player_sample_rate;	/* 播放采样率 */
} audio_parameters_t;

typedef enum
{
	ASR_EVT_WAKEUP_IND,
	ASR_EVT_STANDBY_IND,
} asr_evt_type_t;

typedef void (*asr_evt_cb_t)(asr_evt_type_t event, uint32_t param);

/**
 * @brief 注册语音识别结果的回调函数
 * @param[in] cb 用户指定的语音识别结果的回调函数
 * @return void
 */
void bk_audio_register_asr_callback(asr_evt_cb_t cb);

/**
 * @brief 打开语音处理流水线
 * @param[in] parameters 语音处理流水线的配置参数
 * @return bk_err_t 成功返回BK_OK，失败返回错误码
 */
bk_err_t bk_audio_turn_on(audio_parameters_t *parameters);

/**
 * @brief 关闭语音处理流水线
 * @return bk_err_t 成功返回BK_OK，失败返回错误码
 */
bk_err_t bk_audio_turn_off(void);

/**
 * @brief 录音数据环形缓冲区开关控制
 * @param[in] onoff 开关指示, 1:打开, 0:关闭
 * @return void
 */
void bk_audio_mic_ctrl(uint8_t onoff);

/**
 * @brief 读取录音数据
 * @param[out] data 数据缓冲区地址，用于存放所读取的录音数据
 * @param[in] len 数据缓冲区长度，
 * @return int >0:实际读到的录音数据长度；=0:内部录音缓冲区暂时没有数据，上层需要延时一段时间后重试；<0:出错
 */
int bk_audio_read_mic_data(unsigned char *data, unsigned int len);

/**
 * @brief 写入播放数据
 * @param[in] data 数据缓冲区地址，存放将要播放的数据
 * @param[in] len 数据缓冲区长度，
 * @return int >0:实际写入的数据长度；=0:内部播放缓冲区暂时无法容纳新数据，上层需要延时一段时间后重试；<0:出错
 */
int bk_audio_write_spk_data(unsigned char *data, unsigned int len);

/**
 * @brief 设置扬声器的音量
 * @param[in] gain 音量设置值，有效范围为0x0~0x3F，对应于-45db~18db，值0x2D对应于0db
 * @return bk_err_t 成功返回BK_OK，失败返回错误码
 */
bk_err_t bk_audio_set_spk_gain(uint8_t gain);

/**
 * @brief 获取扬声器的音量
 * @param[out] gain 返回的当前扬声器音量值，有效范围为0x0~0x3F，对应于-45db~18db，值0x2D对应于0db
 * @return bk_err_t 成功返回BK_OK，失败返回错误码
 */
bk_err_t bk_audio_get_spk_gain(uint8_t *gain);

#endif

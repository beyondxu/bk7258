#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include "driver/gpio.h"
#include "gpio_driver.h"
#include <components/bk_voice_service.h>
#include <components/bk_voice_service_types.h>
#include <components/bk_voice_read_service.h>
#include <components/bk_voice_read_service_types.h>
#include <components/bk_voice_write_service.h>
#include <components/bk_voice_write_service_types.h>
#include "audio_frame_queue.h"

#include "dev_audio.h"
#define TAG "dev_audio"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static voice_handle_t voice_handle;
static voice_read_handle_t voice_read_handle;
static voice_write_handle_t voice_write_handle;

/**
 * @brief 音频发送回调函数，用于adc采集音频数据发送出去
 * @param data   待播放的音频数据缓冲区
 * @param len    数据长度（字节）
 * @param args   回调参数，此处未使用
 * @return 实际发送的字节数
 */
static int voice_send_callback(unsigned char *data, unsigned int len, void *args)
{
    int ret = 0;
    audio_buffer_t *audio_buf = audio_queue_malloc(AUDIO_PCM, len);
    if (audio_buf == NULL)
    {
        LOGE("[%s][%d] audio_queue_get failed\n", __func__, __LINE__);
        return ret;
    }
    os_memcpy(audio_buf->audio_data, data, len);
    audio_buf->audio_data_length = len;
    audio_queue_complete(AUDIO_PCM, audio_buf);

    return len;
}

/**
 * @brief 音频初始化函数，用于配置音频参数并启动音频服务
 * @param aec      是否开启回声消除
 * @param chan     音频通道数（1或2）
 * @param samplerate 音频采样率（Hz）
 * @param fmt      音频编码格式（0：G711A，1：G711U）
 * @return 0 成功；负值 错误码
 */
int dev_audio_open(bool aec, uint32_t chan, uint32_t samplerate, audio_enc_type_t fmt)
{
    voice_cfg_t voice_cfg = {0};
    uint32_t mic_sample_rate = samplerate;
    uint32_t spk_sample_rate = samplerate;
    
    voice_cfg_t voice_onboard_cfg = VOICE_BY_ONBOARD_MIC_SPK_CFG_DEFAULT();
    voice_cfg = voice_onboard_cfg;
    voice_cfg.mic_type = MIC_TYPE_ONBOARD;
    voice_cfg.mic_cfg.onboard_mic_cfg.adc_cfg.sample_rate = mic_sample_rate;
    voice_cfg.mic_cfg.onboard_mic_cfg.adc_cfg.chl_num = chan;
    voice_cfg.mic_cfg.onboard_mic_cfg.adc_cfg.dig_gain = 0x2D;
    voice_cfg.mic_cfg.onboard_mic_cfg.frame_size = mic_sample_rate * 2 * 20 / 1000; // one frame size(20ms)
    voice_cfg.mic_cfg.onboard_mic_cfg.out_block_size = voice_cfg.mic_cfg.onboard_mic_cfg.frame_size;
    voice_cfg.mic_cfg.onboard_mic_cfg.out_block_num = 2;

    voice_cfg.spk_type = SPK_TYPE_ONBOARD;
    voice_cfg.spk_cfg.onboard_spk_cfg.sample_rate = spk_sample_rate;
    voice_cfg.spk_cfg.onboard_spk_cfg.chl_num = chan;
    voice_cfg.spk_cfg.onboard_spk_cfg.frame_size = spk_sample_rate * 2 * 20 / 1000; // one frame size(20ms)
    voice_cfg.spk_cfg.onboard_spk_cfg.dig_gain = 0x30;
    voice_cfg.spk_cfg.onboard_spk_cfg.pa_ctrl_en = 1;
    voice_cfg.spk_cfg.onboard_spk_cfg.pa_ctrl_gpio = SPK_MUTE_IO;
    voice_cfg.spk_cfg.onboard_spk_cfg.pa_on_level = 1;
    voice_cfg.spk_cfg.onboard_spk_cfg.pa_on_delay = 20;
    voice_cfg.spk_cfg.onboard_spk_cfg.pa_off_delay = 20;

    if (aec == 1)
    {
        voice_cfg.aec_en = true;
        voice_cfg.aec_cfg.aec_alg_cfg.aec_cfg.fs = mic_sample_rate;
    }
    else
    {
        voice_cfg.aec_en = false;
    }

    switch (fmt)
    {
        case AUDIO_ENC_TYPE_G711A:
        case AUDIO_ENC_TYPE_G711U:
        {
            /* g711 encoder config */
            g711_encoder_cfg_t g711_encoder_cfg = DEFAULT_G711_ENCODER_CONFIG();
            voice_cfg.enc_cfg.g711_enc_cfg = g711_encoder_cfg;
            if (fmt == AUDIO_ENC_TYPE_G711A)
            {
                voice_cfg.enc_type = AUDIO_ENC_TYPE_G711A;
                voice_cfg.enc_cfg.g711_enc_cfg.enc_mode = G711_ENC_MODE_A_LOW;
            }
            else
            {
                voice_cfg.enc_type = AUDIO_ENC_TYPE_G711U;
                voice_cfg.enc_cfg.g711_enc_cfg.enc_mode = G711_ENC_MODE_U_LOW;
            }
            voice_cfg.enc_cfg.g711_enc_cfg.buf_sz = mic_sample_rate * 2 * 20 / 1000; // one frame size(20ms)
            voice_cfg.enc_cfg.g711_enc_cfg.out_block_size = voice_cfg.enc_cfg.g711_enc_cfg.buf_sz >> 1;
            /* config raw_read input buffer */
            voice_cfg.read_pool_size = voice_cfg.enc_cfg.g711_enc_cfg.out_block_size;

            /* g711 decoder config */
            g711_decoder_cfg_t g711_decoder_cfg = DEFAULT_G711_DECODER_CONFIG();
            voice_cfg.dec_cfg.g711_dec_cfg = g711_decoder_cfg;
            if (fmt == AUDIO_ENC_TYPE_G711A)
            {
                voice_cfg.dec_type = AUDIO_DEC_TYPE_G711A;
                voice_cfg.dec_cfg.g711_dec_cfg.dec_mode = G711_DEC_MODE_A_LOW;
            }
            else
            {
                voice_cfg.dec_type = AUDIO_DEC_TYPE_G711U;
                voice_cfg.dec_cfg.g711_dec_cfg.dec_mode = G711_DEC_MODE_U_LOW;
            }
            voice_cfg.dec_cfg.g711_dec_cfg.out_block_size = spk_sample_rate * 2 * 20 / 1000; // one frame size(20ms)
            voice_cfg.dec_cfg.g711_dec_cfg.buf_sz = voice_cfg.dec_cfg.g711_dec_cfg.out_block_size >> 1;
            /* config raw_write output buffer */
            voice_cfg.write_pool_size = voice_cfg.dec_cfg.g711_dec_cfg.buf_sz;
        }
        break;

        case AUDIO_ENC_TYPE_PCM:
        {
            /* pcm encoder config */
            voice_cfg.enc_type = AUDIO_ENC_TYPE_PCM;
            voice_cfg.enc_cfg.pcm_enc_cfg = 0; // not used
            voice_cfg.dec_type = AUDIO_DEC_TYPE_PCM;
            voice_cfg.dec_cfg.pcm_dec_cfg = 0; // not used

            /* config raw_read input buffer and raw_write output buffer */
            voice_cfg.read_pool_size = mic_sample_rate * 2 * 20 / 1000;  // one frame size(20ms)
            voice_cfg.write_pool_size = spk_sample_rate * 2 * 20 / 1000; // one frame size(20ms)
        }
        break;

        default:
        {
            LOGE("[%s][%d] not support encoder format\n", __func__, __LINE__);
            goto error;
        }
        break;
    }

    // voice_cfg.event_handle = doorbell_audio_event_handle; /* close audio event, because sram is not enough */
    voice_cfg.event_handle = NULL;
    voice_cfg.args = NULL;

    voice_handle = bk_voice_init(&voice_cfg);
    if (!voice_handle)
    {
        LOGE("[%s][%d] voice init fail\n", __func__, __LINE__);
        goto error;
    }

    voice_read_cfg_t voice_read_cfg = VOICE_READ_CFG_DEFAULT();
    voice_read_cfg.voice_handle = voice_handle;
    // voice_read_cfg.max_read_size = mic_sample_rate * 2 * 20 / 1000; //one frame size(20ms)
    voice_read_cfg.max_read_size = 1280; // mic_sample_rate * 2 * 20 / 1000; //one frame size(20ms)
    voice_read_cfg.voice_read_callback = voice_send_callback;
    voice_read_cfg.args = NULL;
    voice_read_cfg.task_stack = 1024 * 4;
    voice_read_cfg.mem_type = AUDIO_MEM_TYPE_PSRAM;

    voice_read_handle = bk_voice_read_init(&voice_read_cfg);
    if (!voice_read_handle)
    {
        LOGE("[%s][%d] voice read init fail\n", __func__, __LINE__);
        goto error;
    }

    voice_write_cfg_t voice_write_cfg = VOICE_WRITE_CFG_DEFAULT();
    voice_write_cfg.voice_handle = voice_handle;
    voice_write_cfg.mem_type = AUDIO_MEM_TYPE_PSRAM;

    voice_write_handle = bk_voice_write_init(&voice_write_cfg);
    if (!voice_write_handle)
    {
        LOGE("[%s][%d] voice write init fail\n", __func__, __LINE__);
        goto error;
    }

    if (BK_OK != bk_voice_start(voice_handle))
    {
        LOGE("[%s][%d] voice start fail\n", __func__, __LINE__);
        goto error;
    }

    if (BK_OK != bk_voice_read_start(voice_read_handle))
    {
        LOGE("[%s][%d] voice read start fail\n", __func__, __LINE__);
        goto error;
    }

    if (BK_OK != bk_voice_write_start(voice_write_handle))
    {
        LOGE("[%s][%d] voice write start fail\n", __func__, __LINE__);
        goto error;
    }

    return BK_OK;
error:

    if (voice_read_handle)
    {
        bk_voice_read_stop(voice_read_handle);
    }

    if (voice_write_handle)
    {
        bk_voice_write_stop(voice_write_handle);
    }

    if (voice_handle)
    {
        bk_voice_stop(voice_handle);
    }

    if (voice_read_handle)
    {
        bk_voice_read_deinit(voice_read_handle);
    }

    if (voice_write_handle)
    {
        bk_voice_write_deinit(voice_write_handle);
    }

    if (voice_handle)
    {
        bk_voice_deinit(voice_handle);
    }
    voice_read_handle = NULL;
    voice_write_handle = NULL;
    voice_handle = NULL;

    return BK_FAIL;
}

/**
 * @brief 关闭音频设备
 * @return 0 成功；负值 错误码
 */
int dev_audio_close(void)
{
    if (voice_read_handle)
    {
        bk_voice_read_stop(voice_read_handle);
    }

    if (voice_write_handle)
    {
        bk_voice_write_stop(voice_write_handle);
    }

    if (voice_handle)
    {
        bk_voice_stop(voice_handle);
    }

    if (voice_read_handle)
    {
        bk_voice_read_deinit(voice_read_handle);
    }

    if (voice_write_handle)
    {
        bk_voice_write_deinit(voice_write_handle);
    }

    if (voice_handle)
    {
        bk_voice_deinit(voice_handle);
    }
    voice_read_handle = NULL;
    voice_write_handle = NULL;
    voice_handle = NULL;

    LOGD("[%s][%d] close complete\n", __func__, __LINE__);
    return BK_OK;
}

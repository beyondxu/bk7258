#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <bk_private/bk_init.h>
#include <media_service.h>
#include <components/log.h>

#include <bk_audio_pipeline.h>

#define TAG "AUD_PL"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#if CONFIG_VOICE_SERVICE
#include <components/bk_voice_service.h>
#include <components/bk_voice_service_types.h>
#include <components/bk_voice_read_service.h>
#include <components/bk_voice_read_service_types.h>
#include <components/bk_voice_write_service.h>
#include <components/bk_voice_write_service_types.h>
#endif
#if (CONFIG_ASR_SERVICE)
#include <components/bk_audio_asr_service.h>
#include <components/bk_audio_asr_service_types.h>
#include <components/bk_asr_service.h>
#include <components/bk_asr_service_types.h>
#endif
#if (CONFIG_WANSON_ARMINO_ASR)
#include <asr.h>
#endif
#include <driver/audio_ring_buff.h>

#define DB_SAMPLE_RARE_8K (8000)
#define DB_SAMPLE_RARE_16K (16000)

typedef struct mic_object_s {
	int state;
	beken_mutex_t mutex;
	uint8_t *ringbuffer;
	RingBufferContext rb_context;
} mic_object_t;

#define MIC_RINGBUFFER_SIZE         (30*1024)

static int mic_object_init(mic_object_t *mic_obj)
{
	if (!mic_obj)
		return -1;

	mic_obj->state = 0;
	rtos_init_mutex(&mic_obj->mutex);
	mic_obj->ringbuffer =  psram_malloc(MIC_RINGBUFFER_SIZE);
	if (!mic_obj->ringbuffer)
		return -2;

	ring_buffer_init(&mic_obj->rb_context, mic_obj->ringbuffer, MIC_RINGBUFFER_SIZE, DMA_ID_MAX, RB_DMA_TYPE_NULL);

	return 0;
}

static int mic_object_deinit(mic_object_t *mic_obj)
{
	if (!mic_obj)
		return -1;

	rtos_deinit_mutex(&mic_obj->mutex);
	ring_buffer_clear(&mic_obj->rb_context);
	psram_free(mic_obj->ringbuffer);

	return 0;
}

typedef struct device_info_s
{
	uint8_t audio_enable;

#if CONFIG_VOICE_SERVICE
    voice_handle_t voice_handle;
    voice_read_handle_t voice_read_handle;
    voice_write_handle_t voice_write_handle;
#endif
#if (CONFIG_ASR_SERVICE)
	uint32_t asr_enable;
	uint32_t asr_camera;
	asr_handle_t asr_handle;
	aud_asr_handle_t aud_asr_handle;
#endif

	mic_object_t mic_obj;
} device_info_t;

static device_info_t *g_device_info = NULL;

static device_info_t *get_device_info(void)
{
    if (g_device_info == NULL)
    {
        g_device_info = os_malloc(sizeof(device_info_t));
	    os_memset(g_device_info, 0, sizeof(device_info_t));
    }

	return g_device_info;
}

void bk_audio_mic_ctrl(uint8_t onoff)
{
	device_info_t *device_info = get_device_info();

	if (!device_info) {
		bk_printf("no device_info\n");
		return;
	}

    if (device_info->audio_enable)
    {
    	rtos_lock_mutex(&device_info->mic_obj.mutex);
    	ring_buffer_clear(&device_info->mic_obj.rb_context);
		device_info->mic_obj.state = onoff;
		rtos_unlock_mutex(&device_info->mic_obj.mutex);
    }
}

static int _voice_read_callback(unsigned char *data, unsigned int len, void *args)
{
	uint32_t free_size;

	device_info_t *device_info = get_device_info();

	if (!device_info) {
		bk_printf("no device_info\n");
		return BK_FAIL;
	}

    if (device_info->audio_enable)
    {
    	rtos_lock_mutex(&device_info->mic_obj.mutex);
		if (device_info->mic_obj.state) {
			free_size = ring_buffer_get_free_size(&device_info->mic_obj.rb_context);
			if (free_size >= len) {
				ring_buffer_write(&device_info->mic_obj.rb_context, data, len);
			} else {
				LOGE("%s : just %d left, but read %d\n", __func__, free_size, len);
			}
		}
		rtos_unlock_mutex(&device_info->mic_obj.mutex);
    }

	return len;
}

#define MIC_FRAME_SIZE              (320)

int bk_audio_read_mic_data(unsigned char *data, unsigned int len)
{
	uint32_t fill_size;
	int ret = 0;

	device_info_t *device_info = get_device_info();

	if (!device_info) {
		bk_printf("no device_info\n");
		return -1;
	}
	
    if (device_info->audio_enable)
    {
    	rtos_lock_mutex(&device_info->mic_obj.mutex);
		if (device_info->mic_obj.state) {
			fill_size = ring_buffer_get_fill_size(&device_info->mic_obj.rb_context);
			if (fill_size >= MIC_FRAME_SIZE) {
				ret = ring_buffer_read(&device_info->mic_obj.rb_context, data, len);
			}
		}
		rtos_unlock_mutex(&device_info->mic_obj.mutex);
    } else {
    	ret = -2;
    }

	return ret;
}

int bk_audio_write_spk_data(unsigned char *data, unsigned int len)
{
	int ret = 0;
	device_info_t *device_info = get_device_info();

	if (!device_info) {
		bk_printf("no device_info\n");
		return -1;
	}

    if (device_info->audio_enable)
    {
        ret = bk_voice_write_frame_data(device_info->voice_write_handle, (char *)data, len);
        if (ret != len)
        {
            LOGE("write speaker data fail, need_write: %d, ret: %d\n", len, ret);
        }
    } else {
    	ret = -2;
    }

	return ret;
}

static asr_evt_cb_t asr_cb = NULL;

void bk_audio_register_asr_callback(asr_evt_cb_t cb)
{
	asr_cb = cb;
}

static int _aud_asr_init(void)
{
	if (Wanson_ASR_Init() < 0)
	{
		LOGE("Wanson_ASR_Init Failed!\n");
		return -1;
	}
	Wanson_ASR_Reset();
	LOGD("Wanson_ASR_Init OK!\n");

	return 0;
}

static void _aud_asr_deinit(void)
{
	Wanson_ASR_Release();
}

static int _aud_asr_recog(void *read_buf, uint32_t read_size, void *p1, void *p2)	//p1 is char **, p2 is float *
{
	return Wanson_ASR_Recog((short*)read_buf, read_size/2, (const char **)p1, (float *)p2);
}

static void _aud_asr_result_handle(uint32_t param)
{
    uint32_t asr_result = 0;
	char *result = (char *)param;

	bk_printf("asr {%s}\n", result);

	if (!asr_cb)
		return;

    if (os_strcmp(result, "嗨阿米诺") == 0)                 //识别出唤醒词 嗨阿米诺
    {
        LOGI("%s \n", "hi armino, cmd: 0 ");
        asr_result = 1;
    }
    else if (os_strcmp(result, "嘿阿米楼") == 0)
    {
        LOGI("%s \n", "hi armino, cmd: 1 ");
        asr_result = 1;
    }
    else if (os_strcmp(result, "嘿儿米楼") == 0)
    {
        LOGI("%s \n", "hi armino, cmd: 2 ");
        asr_result = 1;
    }
    else if (os_strcmp(result, "嘿鹅迷楼") == 0)
    {
        LOGI("%s \n", "hi armino, cmd: 3 ");
        asr_result = 1;
    }
    else if (os_strcmp(result, "拜拜阿米诺") == 0)     //识别出 拜拜阿米诺
    {
        LOGI("%s \n", "byebye armino, cmd: 0 ");
        asr_result = 2;
    }
    else if (os_strcmp(result, "拜拜阿米楼") == 0)
    {
        LOGI("%s \n", "byebye armino, cmd: 1 ");
        asr_result = 2;
    }
    else
    {
        //nothing
    }

	bk_printf("asr %d\n", asr_result);

	if (asr_result == 1) {
		asr_cb(ASR_EVT_WAKEUP_IND, 0);
	} else if (asr_result == 2) {
		asr_cb(ASR_EVT_STANDBY_IND, 0);
	}

}

/*
void* bk_audio_get_voice_handle(void)
{
	device_info_t *device_info = get_device_info();

	if (!device_info || (device_info->audio_enable == BK_FALSE))
		return NULL;

	return device_info->voice_handle;
}
*/

#define DEFAULT_MIC_GAIN	8
#define DEFAULT_SPK_GAIN	12

bk_err_t bk_audio_turn_on(audio_parameters_t *parameters)
{
    voice_cfg_t *voice_cfg;
	device_info_t *device_info = get_device_info();
	if (!device_info)
		return BK_FAIL;

	if (mic_object_init(&device_info->mic_obj)) {
		LOGD("%s mic_object_init fail\n", __func__);

		return BK_FAIL;
	}

    voice_cfg = os_malloc(sizeof(voice_cfg_t));

    if (!voice_cfg)
    {
        LOGD("%s voice_cfg malloc failure!\n", __func__);

        return BK_FAIL;
    }

    if (device_info->audio_enable == BK_TRUE)
    {
        LOGD("%s already turn on\n", __func__);

        return BK_FAIL;
    }

    LOGD("%s, AEC: %d, UAC: %d, sample rate: %d, %d, fmt: %d, %d\n", __func__,
         parameters->aec, parameters->uac, parameters->rmt_recorder_sample_rate,
         parameters->rmt_player_sample_rate, parameters->rmt_recoder_fmt, parameters->rmt_player_fmt);

    uint32_t mic_sample_rate = 8000;
    uint32_t spk_sample_rate = 8000;
    switch (parameters->rmt_recorder_sample_rate)
    {
        case DB_SAMPLE_RARE_8K:
            mic_sample_rate = 8000;
            break;

        case DB_SAMPLE_RARE_16K:
            mic_sample_rate = 16000;
            break;

        default:
            mic_sample_rate = 8000;
            break;
    }

    switch (parameters->rmt_player_sample_rate)
    {
        case DB_SAMPLE_RARE_8K:
            spk_sample_rate = 8000;
            break;

        case DB_SAMPLE_RARE_16K:
            spk_sample_rate = 16000;
            break;

        default:
            spk_sample_rate = 8000;
            break;
    }


    if (parameters->uac == 1)
    {
        voice_cfg_t voice_uac = VOICE_BY_UAC_MIC_SPK_CFG_DEFAULT();
        *voice_cfg = voice_uac;
        voice_cfg->mic_cfg.uac_mic_cfg.samp_rate = mic_sample_rate;
        voice_cfg->mic_cfg.uac_mic_cfg.frame_size = mic_sample_rate * 2 * 20 / 1000; //one frame size(20ms)
        voice_cfg->mic_cfg.uac_mic_cfg.out_block_size = voice_cfg->mic_cfg.uac_mic_cfg.frame_size;
        voice_cfg->mic_cfg.uac_mic_cfg.out_block_num = 2;

        voice_cfg->spk_cfg.uac_spk_cfg.samp_rate = spk_sample_rate;
        voice_cfg->spk_cfg.uac_spk_cfg.frame_size = spk_sample_rate * 2 * 20 / 1000; //one frame size(20ms)
    }
    else
    {
        voice_cfg_t voice_onboard = VOICE_BY_ONBOARD_MIC_SPK_CFG_DEFAULT();
        *voice_cfg = voice_onboard;
        voice_cfg->mic_cfg.onboard_mic_cfg.adc_cfg.sample_rate = mic_sample_rate;
        voice_cfg->mic_cfg.onboard_mic_cfg.frame_size = mic_sample_rate * 2 * 20 / 1000; //one frame size(20ms)
        //voice_cfg->mic_cfg.onboard_mic_cfg.out_rb_size = voice_cfg->mic_cfg.onboard_mic_cfg.frame_size;
        voice_cfg->mic_cfg.onboard_mic_cfg.out_block_size = voice_cfg->mic_cfg.onboard_mic_cfg.frame_size;
        voice_cfg->mic_cfg.onboard_mic_cfg.out_block_num = 2;
		voice_cfg->mic_cfg.onboard_mic_cfg.adc_cfg.dig_gain = 0x38;	//0x28;	//0x0~0x3f : -45db ~ 18db	//0x2D is 0db
		//voice_cfg->mic_cfg.onboard_mic_cfg.adc_cfg.ana_gain = 0x8;	//0x0~0xf : -15db ~ 0db
		voice_cfg->mic_cfg.onboard_mic_cfg.adc_cfg.ana_gain = DEFAULT_MIC_GAIN;	//0x0~0xf : -15db ~ 0db

        voice_cfg->spk_cfg.onboard_spk_cfg.sample_rate = spk_sample_rate;
        voice_cfg->spk_cfg.onboard_spk_cfg.frame_size = spk_sample_rate * 2 * 20 / 1000; //one frame size(20ms)
        voice_cfg->spk_cfg.onboard_spk_cfg.dig_gain = 0x20;	//0x38;	//0x2D;
        //voice_cfg->spk_cfg.onboard_spk_cfg.ana_gain = 0x7;
        voice_cfg->spk_cfg.onboard_spk_cfg.ana_gain = DEFAULT_SPK_GAIN;

		voice_cfg->spk_cfg.onboard_spk_cfg.multi_in_port_num = 1;	//for play_tone()
    }

    if (parameters->aec == 1)
    {
        voice_cfg->aec_en = true;
        voice_cfg->aec_cfg.aec_alg_cfg.aec_cfg.fs = mic_sample_rate;
    }
    else
    {
        voice_cfg->aec_en = false;
    }

    switch (parameters->rmt_recoder_fmt)
    {
        case CODEC_FORMAT_G711A:
        case CODEC_FORMAT_G711U:
        {
            /* g711 encoder config */
            g711_encoder_cfg_t g711_encoder_cfg = DEFAULT_G711_ENCODER_CONFIG();
            voice_cfg->enc_cfg.g711_enc_cfg = g711_encoder_cfg;
            if (parameters->rmt_recoder_fmt == CODEC_FORMAT_G711A)
            {
                voice_cfg->enc_type = AUDIO_ENC_TYPE_G711A;
                voice_cfg->enc_cfg.g711_enc_cfg.enc_mode = G711_ENC_MODE_A_LOW;
            }
            else
            {
                voice_cfg->enc_type = AUDIO_ENC_TYPE_G711U;
                voice_cfg->enc_cfg.g711_enc_cfg.enc_mode = G711_ENC_MODE_U_LOW;
            }
            voice_cfg->enc_cfg.g711_enc_cfg.buf_sz = mic_sample_rate * 2 * 20 / 1000; //one frame size(20ms)
            voice_cfg->enc_cfg.g711_enc_cfg.out_block_size = voice_cfg->enc_cfg.g711_enc_cfg.buf_sz >> 1;
            /* config raw_read input buffer */
            voice_cfg->read_pool_size = voice_cfg->enc_cfg.g711_enc_cfg.out_block_size;
        }
        break;

        case CODEC_FORMAT_PCM:
        {
            /* pcm encoder config */
            voice_cfg->enc_type = AUDIO_ENC_TYPE_PCM;
            voice_cfg->enc_cfg.pcm_enc_cfg = 0;      // not used

            /* config raw_read input buffer */
            voice_cfg->read_pool_size = mic_sample_rate * 2 * 20 / 1000; //one frame size(20ms)
        }
        break;

        default:
        {
            LOGE("not support encoder format\n");
            goto error;
        }
        break;
    }

    switch (parameters->rmt_player_fmt)
    {
        case CODEC_FORMAT_G711A:
        case CODEC_FORMAT_G711U:
        {
            /* g711 decoder config */
            g711_decoder_cfg_t g711_decoder_cfg = DEFAULT_G711_DECODER_CONFIG();
            voice_cfg->dec_cfg.g711_dec_cfg = g711_decoder_cfg;
            if (parameters->rmt_recoder_fmt == CODEC_FORMAT_G711A)
            {
                voice_cfg->dec_type = AUDIO_DEC_TYPE_G711A;
                voice_cfg->dec_cfg.g711_dec_cfg.dec_mode = G711_DEC_MODE_A_LOW;
            }
            else
            {
                voice_cfg->dec_type = AUDIO_DEC_TYPE_G711U;
                voice_cfg->dec_cfg.g711_dec_cfg.dec_mode = G711_DEC_MODE_U_LOW;
            }
            voice_cfg->dec_cfg.g711_dec_cfg.out_block_size = spk_sample_rate * 2 * 20 / 1000; //one frame size(20ms)
            voice_cfg->dec_cfg.g711_dec_cfg.buf_sz = voice_cfg->dec_cfg.g711_dec_cfg.out_block_size >> 1;
            /* config raw_write output buffer */
            voice_cfg->write_pool_size = voice_cfg->dec_cfg.g711_dec_cfg.buf_sz;
        }
        break;

        case CODEC_FORMAT_PCM:
        {
            /* pcm decoder config */
            voice_cfg->dec_type = AUDIO_DEC_TYPE_PCM;
            voice_cfg->dec_cfg.pcm_dec_cfg = 0;      //not used

            /* config raw_write output buffer */
            voice_cfg->write_pool_size = spk_sample_rate * 2 * 20 / 1000; //one frame size(20ms)
        }
        break;

        default:
        {
            LOGE("not support encoder format\n");
            goto error;
        }
        break;
    }

#if (!CONFIG_ASR_SERVICE_WITH_MIC)
	//parameters->asr = 1;
	asr_cfg_t asr_cfg = {0};
	if (parameters->uac)
	{
		asr_cfg_t t_asr_cfg = ASR_BY_UAC_MIC_CFG_DEFAULT();	//ASR_BY_UAC_MIC_SPK_CFG_DEFAULT();
		asr_cfg = t_asr_cfg;
	} else
	{
		asr_cfg_t t_asr_cfg = ASR_BY_ONBOARD_MIC_CFG_DEFAULT();	//ASR_BY_ONBOARD_MIC_SPK_CFG_DEFAULT();
		asr_cfg = t_asr_cfg;
	}
	if (parameters->asr == 1)
	{
		asr_cfg.asr_en = true;
		if (mic_sample_rate != asr_cfg.asr_sample_rate)
		{
			asr_cfg.asr_rsp_en = true;
			asr_cfg.rsp_cfg.rsp_alg_cfg.rsp_cfg.src_rate = mic_sample_rate;
		} else
		{
			asr_cfg.asr_rsp_en = false;
		}
	} 
	else
	{
		asr_cfg.asr_en     = false;
		asr_cfg.asr_rsp_en = false;
	}
	
		if (voice_cfg->aec_en) {
			voice_cfg->aec_cfg.aec_alg_cfg.multi_out_port_num = 1;
			if (parameters->vad) {
				voice_cfg->aec_cfg.aec_alg_cfg.vad_cfg.vad_enable = 1;
				//voice_cfg->aec_cfg.aec_alg_cfg.vad_cfg.vad_start_threshold = 480;
				//voice_cfg->aec_cfg.aec_alg_cfg.vad_cfg.vad_stop_threshold = 960;
				//voice_cfg->aec_cfg.aec_alg_cfg.vad_cfg.vad_silence_threshold = 320;
				//voice_cfg->aec_cfg.aec_alg_cfg.vad_cfg.vad_eng_threshold = 2000;
			}
		} else
		{
			if (parameters->uac == 1) {
				voice_cfg->mic_cfg.uac_mic_cfg.multi_out_port_num = 1;
			} else {
				voice_cfg->mic_cfg.onboard_mic_cfg.multi_out_port_num = 1;
			}
		}
#endif
    //voice_cfg->event_handle = doorbell_audio_event_handle; /* close audio event, because sram is not enough */
    voice_cfg->event_handle = NULL;
    voice_cfg->args = NULL;
    device_info->voice_handle = bk_voice_init(voice_cfg);
    if (!device_info->voice_handle)
    {
        LOGE("voice init fail\n");
        goto error;
    }
#if (!CONFIG_ASR_SERVICE_WITH_MIC)
	if (asr_cfg.asr_en == true)
	{
	    asr_cfg.event_handle = NULL;
	    asr_cfg.args         = NULL;
		device_info->asr_handle = bk_asr_create(&asr_cfg);
	    if (!device_info->asr_handle)
	    {
	        LOGE("asr init fail\n");
	        goto error;
	    }

	    device_info->asr_handle->mic_str = (audio_element_handle_t)bk_voice_get_mic_str(device_info->voice_handle, voice_cfg);

		if (mic_sample_rate == 16000) {
			asr_cfg.read_pool_size = mic_sample_rate * 2 * 20 / 1000;
		}
		else if (mic_sample_rate == 8000) {
			asr_cfg.read_pool_size = 2 * mic_sample_rate * 2 * 20 / 1000;
		}

		bk_asr_init(&asr_cfg, device_info->asr_handle);

		{
			aud_asr_cfg_t aud_asr_cfg = AUDIO_ASR_CFG_DEFAULT();
			aud_asr_cfg.asr_handle    = device_info->asr_handle;
			aud_asr_cfg.max_read_size = 960 ;//
			aud_asr_cfg.args = NULL;
			aud_asr_cfg.task_stack = 1024 * 2;
			if (parameters->uac == 1) {
				aud_asr_cfg.task_prio = voice_cfg->mic_cfg.uac_mic_cfg.task_prio;
			} else {
				aud_asr_cfg.task_prio = voice_cfg->mic_cfg.onboard_mic_cfg.task_prio;
			}
		//	aud_asr_cfg.task_prio = 4;
			aud_asr_cfg.mem_type = AUDIO_MEM_TYPE_PSRAM;
			aud_asr_cfg.aud_asr_init = _aud_asr_init;
			aud_asr_cfg.aud_asr_recog = _aud_asr_recog;
			aud_asr_cfg.aud_asr_result_handle = _aud_asr_result_handle;
			aud_asr_cfg.aud_asr_deinit = _aud_asr_deinit;
			device_info->aud_asr_handle = bk_aud_asr_init(&aud_asr_cfg);
			if (!device_info->aud_asr_handle)
			{
				LOGE("aud asr init fail\n");
				goto error;
			}
		}
	}
#endif
    voice_read_cfg_t voice_read_cfg = VOICE_READ_CFG_DEFAULT();
    voice_read_cfg.voice_handle = device_info->voice_handle;
    //voice_read_cfg.max_read_size = mic_sample_rate * 2 * 20 / 1000; //one frame size(20ms)
    //voice_read_cfg.max_read_size = 2048;//3200;//mic_sample_rate * 2 * 20 * 10 / 1000; //one frame size(200ms)
    voice_read_cfg.max_read_size = mic_sample_rate * 2 * 20 * 10 / 1000; //one frame size(200ms)
    voice_read_cfg.voice_read_callback = _voice_read_callback;
    voice_read_cfg.args = NULL;
    voice_read_cfg.task_stack = 1024 * 4;
    voice_read_cfg.mem_type = AUDIO_MEM_TYPE_PSRAM;
    device_info->voice_read_handle = bk_voice_read_init(&voice_read_cfg);
    if (!device_info->voice_read_handle)
    {
        LOGE("voice read init fail\n");
        goto error;
    }

    voice_write_cfg_t voice_write_cfg = VOICE_WRITE_CFG_DEFAULT();
    voice_write_cfg.voice_handle = device_info->voice_handle;
    voice_write_cfg.mem_type = AUDIO_MEM_TYPE_PSRAM;
    voice_write_cfg.node_size = 20480;
    device_info->voice_write_handle = bk_voice_write_init(&voice_write_cfg);
    if (!device_info->voice_write_handle)
    {
        LOGE("voice write init fail\n");
        goto error;
    }

    if (BK_OK != bk_voice_start(device_info->voice_handle))
    {
        LOGE("voice start fail\n");
        goto error;
    }

    if (BK_OK != bk_voice_read_start(device_info->voice_read_handle))
    {
        LOGE("voice read start fail\n");
        goto error;
    }

    if (BK_OK != bk_voice_write_start(device_info->voice_write_handle))
    {
        LOGE("voice write start fail\n");
        goto error;
    }
#if (!CONFIG_ASR_SERVICE_WITH_MIC)
	if (asr_cfg.asr_en == true)
	{
		if (BK_OK != bk_asr_start(device_info->asr_handle))
		{
			LOGE("asr start fail\n");
			goto error;
		}
	    if (BK_OK != bk_aud_asr_start(device_info->aud_asr_handle))
	    {
	        LOGE("aud asr start fail\n");
	        goto error;
	    }
	}
#endif
    device_info->audio_enable = BK_TRUE;

    if (voice_cfg)
    {
        os_free(voice_cfg);
        voice_cfg=NULL;
    }

    return BK_OK;


error:

#if (!CONFIG_ASR_SERVICE_WITH_MIC)
	if (device_info->aud_asr_handle)
	{
		bk_aud_asr_stop(device_info->aud_asr_handle);
	}
    if (device_info->asr_handle)
    {
        bk_asr_stop(device_info->asr_handle);
    }
	if (device_info->aud_asr_handle)
	{
		bk_aud_asr_deinit(device_info->aud_asr_handle);
	}
#endif

    if (device_info->voice_read_handle)
    {
        bk_voice_read_stop(device_info->voice_read_handle);
    }

    if (device_info->voice_write_handle)
    {
        bk_voice_write_stop(device_info->voice_write_handle);
    }

    if (device_info->voice_handle)
    {
        bk_voice_stop(device_info->voice_handle);
    }

    if (device_info->voice_read_handle)
    {
        bk_voice_read_deinit(device_info->voice_read_handle);
    }

    if (device_info->voice_write_handle)
    {
        bk_voice_write_deinit(device_info->voice_write_handle);
    }

    if (device_info->voice_handle)
    {
        bk_voice_deinit(device_info->voice_handle);
    }
    device_info->voice_read_handle = NULL;
    device_info->voice_write_handle  = NULL;
    device_info->voice_handle  = NULL;

    if (voice_cfg)
    {
        os_free(voice_cfg);
        voice_cfg=NULL;
    }

    return BK_FAIL;
}

bk_err_t bk_audio_turn_off(void)
{
	device_info_t *device_info = get_device_info();

	if (!device_info)
		return BK_FAIL;

    if (device_info->audio_enable == BK_FALSE)
    {
        LOGD("%s already turn off\n", __func__);

        return BK_FAIL;
    }

    LOGD("%s entry\n", __func__);

    device_info->audio_enable = BK_FALSE;

    if (device_info->voice_read_handle)
    {
        bk_voice_read_stop(device_info->voice_read_handle);
    }

    if (device_info->voice_write_handle)
    {
        bk_voice_write_stop(device_info->voice_write_handle);
    }

#if (!CONFIG_ASR_SERVICE_WITH_MIC)
    if (device_info->asr_handle)
    {
        bk_asr_stop(device_info->asr_handle);
    }

	if (device_info->aud_asr_handle)
	{
		bk_aud_asr_stop(device_info->aud_asr_handle);
	}
#endif

    if (device_info->voice_handle)
    {
        bk_voice_stop(device_info->voice_handle);
    }

    if (device_info->voice_read_handle)
    {
        bk_voice_read_deinit(device_info->voice_read_handle);
    }

    if (device_info->voice_write_handle)
    {
        bk_voice_write_deinit(device_info->voice_write_handle);
    }

#if (!CONFIG_ASR_SERVICE_WITH_MIC)
	if (device_info->aud_asr_handle)
	{
		bk_aud_asr_deinit(device_info->aud_asr_handle);
	}
    if (device_info->asr_handle)
    {
        bk_asr_deinit(device_info->asr_handle);
    }
#endif

    if (device_info->voice_handle)
    {
        bk_voice_deinit(device_info->voice_handle);
    }

    device_info->voice_read_handle = NULL;
    device_info->voice_write_handle = NULL;
    device_info->voice_handle  = NULL;

	if (mic_object_deinit(&device_info->mic_obj)) {
		LOGD("%s mic_object_deinit fail\n", __func__);

		return BK_FAIL;
	}

    LOGD("%s out\n", __func__);

    return BK_OK;
}

bk_err_t bk_audio_set_spk_gain(uint8_t gain)
{
	bk_err_t ret;
	audio_element_handle_t spk_str;
	device_info_t *device_info = get_device_info();

	if (!device_info) {
		bk_printf("no device_info\n");
		return -1;
	}

	if (!device_info->audio_enable)
		return -2;

	ret = bk_voice_get_spkstr(device_info->voice_handle, &spk_str);
	if (ret)
		return -3;

	ret = onboard_speaker_stream_set_digital_gain(spk_str, gain);
	return ret;
}

bk_err_t bk_audio_get_spk_gain(uint8_t *gain)
{
	bk_err_t ret;
	audio_element_handle_t spk_str;
	device_info_t *device_info = get_device_info();

	if (!device_info) {
		bk_printf("no device_info\n");
		return -1;
	}

	if (!device_info->audio_enable)
		return -2;

	ret = bk_voice_get_spkstr(device_info->voice_handle, &spk_str);
	if (ret)
		return -3;

	ret = onboard_speaker_stream_get_digital_gain(spk_str, gain);
	return ret;
}



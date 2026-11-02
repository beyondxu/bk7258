#include <stdbool.h>
#include <stdint.h>

#include <components/bk_voice_service.h>
#include <components/bk_voice_read_service.h>
#include <components/bk_voice_write_service.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream.h>
#include <os/mem.h>
#include <os/os.h>
#include <common/bk_include.h>
#include <components/log.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <driver/hal/hal_gpio_types.h>

#if CONFIG_ASR_SERVICE
#include <components/bk_asr_service.h>
#include <components/bk_audio_asr_service.h>
#include "bk_app_asr.h"
#endif

#include "kws_demo.h"
#include "os/str.h"
#include "cli.h"

#define TAG "kws_demo"

#define LOGE(format, ...) BK_LOGE(TAG, "[%s][%d] " format, __func__, __LINE__, ##__VA_ARGS__)
#define LOGI(format, ...) BK_LOGI(TAG, "[%s][%d] " format, __func__, __LINE__, ##__VA_ARGS__)
#define LOGD(format, ...) BK_LOGD(TAG, "[%s][%d] " format, __func__, __LINE__, ##__VA_ARGS__)
#define LOGW(format, ...) BK_LOGW(TAG, "[%s][%d] " format, __func__, __LINE__, ##__VA_ARGS__)

#define OPUS_DEC_STACK_SIZE   (12 * 1024)
#define OPUS_DEC_FRAME_SIZE   (480)          // 32kbps 码率，120ms 间隔
#define SPK_FRAME_NUM         (16)           // 缓存 16 帧
#define MIC_FRAME_DURATION    (20)           // 帧间隔 20ms

#define SPK_GAIN_MAX          0x2D           // [0x00, 0x3F]
#define MIC_GAIN_MAX          0x2D           // [0x00, 0x3F]

#define VAD_ENG_THRESHOLD     4000

#define DEFAULT_SPK_VOL         45
#define MIC_SAMPLE_RATE         16000
#define SPK_SAMPLE_RATE         16000
#define SPK_FRAME_DURATION      60

typedef struct 
{
    bool                  inited;            // 初始化标志
    voice_handle_t        voice_handle;      // 语音服务句柄
    voice_read_handle_t   voice_read_handle; // 录音句柄
    voice_write_handle_t  voice_write_handle;// 播放句柄
#if CONFIG_ASR_SERVICE
    asr_handle_t          asr_handle;        // ASR服务句柄
    aud_asr_handle_t      aud_asr_handle;    // ASR句柄
#endif
    uint16_t              pa_ctrl_gpio;      // PA控制GPIO
    uint8_t               pa_on_level;       // PA开启电平
} app_voice_ctx_t;

static app_voice_ctx_t g_voice_ctx = {0};

extern void bk_delay_us(UINT32 us);

void delay_us(UINT32 us)
{
    bk_delay_us(us);
}

static void _dump_config(app_audio_config_t *config)
{
    LOGI("=== Voice Configuration Parameters ===\n");
    LOGI("AEC enable: %s\n", config->aec_enable ? "true" : "false");
    LOGI("AEC mode: %s\n", config->aec_mode == AEC_MODE_HARDWARE ? "hardware" : "software");
    LOGI("Decoder type: %d\n", config->dec_type);
    LOGI("Speaker sample rate: %d Hz\n", config->spk_sample_rate);
    LOGI("Speaker frame duration: %d ms\n", config->spk_frame_duration);
    LOGI("PA control GPIO: %d\n", config->pa_ctrl_gpio);
    LOGI("PA on level: %d\n", config->pa_on_level);
    LOGI("Encoder type: %d\n", config->enc_type);
    LOGI("MIC sample rate: %d Hz\n", config->mic_sample_rate);
    LOGI("======================================\n");
}

bk_err_t bk_app_audio_init(app_audio_config_t *config)
{
    uint32_t frame_size = 0;

    if (!config) 
    {
        LOGE("config is NULL\n");
        return BK_ERR_PARAM;
    }

    if (config->mic_sample_rate != 16000 && config->mic_sample_rate != 8000) 
    {
        LOGE("mic sample rate %d not supported\n", config->mic_sample_rate);
        return BK_ERR_PARAM;
    }

    _dump_config(config);

    voice_cfg_t voice_cfg = VOICE_BY_ONBOARD_MIC_SPK_CFG_DEFAULT();

    frame_size = config->mic_sample_rate * 2 * MIC_FRAME_DURATION / 1000;

    // 配置麦克风
    voice_cfg.mic_cfg.onboard_mic_cfg.adc_cfg.sample_rate = config->mic_sample_rate;
    voice_cfg.mic_cfg.onboard_mic_cfg.adc_cfg.dig_gain = MIC_GAIN_MAX;
    voice_cfg.mic_cfg.onboard_mic_cfg.frame_size = frame_size;
    voice_cfg.mic_cfg.onboard_mic_cfg.out_block_size = voice_cfg.mic_cfg.onboard_mic_cfg.frame_size;
    voice_cfg.mic_cfg.onboard_mic_cfg.out_block_num = 2;

    // 配置扬声器
    voice_cfg.spk_cfg.onboard_spk_cfg.sample_rate = config->spk_sample_rate;
    voice_cfg.spk_cfg.onboard_spk_cfg.frame_size = config->spk_sample_rate * 2 * config->spk_frame_duration / 1000;
    voice_cfg.spk_cfg.onboard_spk_cfg.dig_gain = 0x1F;

    if (config->pa_ctrl_gpio < GPIO_NUM) 
    {
        LOGI("enable PA control, GPIO: %d, level: %d\n", config->pa_ctrl_gpio, config->pa_on_level);
        voice_cfg.spk_cfg.onboard_spk_cfg.pa_ctrl_en = true;
        voice_cfg.spk_cfg.onboard_spk_cfg.pa_ctrl_gpio = config->pa_ctrl_gpio;
        voice_cfg.spk_cfg.onboard_spk_cfg.pa_on_level = config->pa_on_level;
        voice_cfg.spk_cfg.onboard_spk_cfg.pa_on_delay = 10;
        voice_cfg.spk_cfg.onboard_spk_cfg.pa_off_delay = 30;
    }

    // 配置AEC
    voice_cfg.aec_en = config->aec_enable;
    if (config->aec_enable) 
    {
        voice_cfg.aec_cfg.aec_alg_cfg.task_stack = 2048;
        voice_cfg.aec_cfg.aec_alg_cfg.aec_cfg.ns_type = NS_TRADITION;
        voice_cfg.aec_cfg.aec_alg_cfg.aec_cfg.fs = config->mic_sample_rate;
        voice_cfg.aec_cfg.aec_alg_cfg.out_block_size = frame_size;
        if (config->aec_mode == AEC_MODE_HARDWARE) 
        { // 硬件回采，mic 配置成双通道，左通道是原始数据，右声道是参考数据，同时 spk 不需要多路输出
            voice_cfg.aec_cfg.aec_alg_cfg.aec_cfg.mode = AEC_MODE_HARDWARE;
            voice_cfg.mic_cfg.onboard_mic_cfg.adc_cfg.chl_num = 2;
            voice_cfg.mic_cfg.onboard_mic_cfg.multi_out_port_num = 1;
            voice_cfg.spk_cfg.onboard_spk_cfg.multi_out_port_num = 0;
        } 
        else 
        { // 软件回采，mic 配置成单通道，spk 配置多路输出
            voice_cfg.aec_cfg.aec_alg_cfg.aec_cfg.mode = AEC_MODE_SOFTWARE;
            voice_cfg.mic_cfg.onboard_mic_cfg.adc_cfg.chl_num = 1;
            voice_cfg.spk_cfg.onboard_spk_cfg.multi_out_port_num = 1;
        }
    }

    // 配置编码器
    switch (config->enc_type) 
    {
        case AUDIO_ENC_TYPE_OPUS:
        {
            opus_enc_cfg_t opus_enc_cfg = DEFAULT_OPUS_ENC_CONFIG();
            opus_enc_cfg.frame_samples_per_channel = config->mic_sample_rate * MIC_FRAME_DURATION / 1000;
            voice_cfg.enc_type = AUDIO_ENC_TYPE_OPUS;
            voice_cfg.enc_cfg.opus_enc_cfg = opus_enc_cfg;

        }
        break;

        case AUDIO_ENC_TYPE_PCM:
        {
            voice_cfg.enc_type = AUDIO_ENC_TYPE_PCM;
            voice_cfg.read_pool_size = frame_size;
        }
        break;

        default:
            LOGE("unsupported encoder type: %d\n", config->enc_type);
            return BK_FAIL;
    }

    // 配置解码器
    switch (config->dec_type) 
    {
        case AUDIO_DEC_TYPE_OPUS:
        {
            opus_dec_cfg_t opus_dec_cfg = DEFAULT_OPUS_DEC_CONFIG();
            opus_dec_cfg.task_stack = OPUS_DEC_STACK_SIZE;
            voice_cfg.dec_type = AUDIO_DEC_TYPE_OPUS;
            voice_cfg.dec_cfg.opus_dec_cfg = opus_dec_cfg;
        }
        break;

        case AUDIO_DEC_TYPE_PCM:
        {
            voice_cfg.dec_type = AUDIO_DEC_TYPE_PCM;
            voice_cfg.write_pool_size = config->spk_sample_rate * 2 * config->spk_frame_duration / 1000;
        }
        break;

        default:
            LOGE("unsupported decoder type: %d\n", config->dec_type);
            return BK_FAIL;
    }

#if CONFIG_ASR_SERVICE
    // 配置ASR
    asr_cfg_t asr_cfg = ASR_BY_ONBOARD_MIC_CFG_DEFAULT();
    asr_cfg.asr_en = true;
    asr_cfg.read_pool_size = asr_cfg.asr_sample_rate * 2 * 20 / 1000; // 20ms帧大小
    
    // 如果麦克风采样率与ASR采样率不同，需要启用重采样
    if (config->mic_sample_rate != asr_cfg.asr_sample_rate) 
    {
        asr_cfg.asr_rsp_en = true;
        asr_cfg.rsp_cfg.rsp_alg_cfg.rsp_cfg.src_rate = config->mic_sample_rate;
        asr_cfg.rsp_cfg.rsp_alg_cfg.rsp_cfg.dest_rate = asr_cfg.asr_sample_rate;
    } 
    else 
    {
        asr_cfg.asr_rsp_en = false;
    }
    
    // 配置多输出端口，若使能 AEC，从 AEC Element 输出，否则从麦克风 Element 输出
    if (config->aec_enable) 
    {
        voice_cfg.aec_cfg.aec_alg_cfg.multi_out_port_num++;
    } 
    else 
    {
        voice_cfg.mic_cfg.onboard_mic_cfg.multi_out_port_num++;
    }
#endif

    // 设置事件回调
    voice_cfg.event_handle = config->event_cb;

    // 初始化语音服务
    g_voice_ctx.voice_handle = bk_voice_init(&voice_cfg);
    if (!g_voice_ctx.voice_handle) 
    {
        LOGE("voice init failed\n");
        goto error;
    }

#if CONFIG_ASR_SERVICE
    g_voice_ctx.asr_handle = bk_asr_create(&asr_cfg);
    if (!g_voice_ctx.asr_handle) 
    {
        LOGE("asr create failed\n");
        goto error;
    }

    // 关联多输出端口
    g_voice_ctx.asr_handle->mic_str = (audio_element_handle_t)bk_voice_get_mic_str(g_voice_ctx.voice_handle, &voice_cfg);

    // 初始化ASR
    if (BK_OK != bk_asr_init(&asr_cfg, g_voice_ctx.asr_handle)) 
    {
        LOGE("asr init failed\n");
        goto error;
    }

    aud_asr_cfg_t aud_asr_cfg = AUDIO_ASR_CFG_DEFAULT();
    aud_asr_cfg.asr_handle = g_voice_ctx.asr_handle;
    if (config->asr_frame_size > 0) 
    {
        LOGI("asr frame size: %d", config->asr_frame_size);
        aud_asr_cfg.max_read_size = config->asr_frame_size;
    }
    aud_asr_cfg.task_prio = voice_cfg.mic_cfg.onboard_mic_cfg.task_prio;
    aud_asr_cfg.aud_asr_init = bk_app_asr_init;
    aud_asr_cfg.aud_asr_deinit = bk_app_asr_deinit;
    aud_asr_cfg.aud_asr_recog = bk_app_asr_recog;
    aud_asr_cfg.aud_asr_result_handle = config->asr_result_cb;

    if (aud_asr_cfg.aud_asr_result_handle == NULL) 
    {
        LOGW("asr result callback is NULL\n");
    }
    
    g_voice_ctx.aud_asr_handle = bk_aud_asr_init(&aud_asr_cfg);
    if (!g_voice_ctx.aud_asr_handle) 
    {
        LOGE("aud asr init failed\n");
        goto error;
    }
#endif

    // 初始化录音
    voice_read_cfg_t voice_read_cfg = VOICE_READ_CFG_DEFAULT();
    voice_read_cfg.voice_handle = g_voice_ctx.voice_handle;
    voice_read_cfg.voice_read_callback = config->mic_rx_cb;

    g_voice_ctx.voice_read_handle = bk_voice_read_init(&voice_read_cfg);
    if (!g_voice_ctx.voice_read_handle) 
    {
        LOGE("voice read init failed\n");
        goto error;
    }

    // 初始化播放
    voice_write_cfg_t voice_write_cfg = VOICE_WRITE_CFG_DEFAULT();
    voice_write_cfg.voice_handle = g_voice_ctx.voice_handle;
    voice_write_cfg.node_num = SPK_FRAME_NUM;
    if (config->dec_type == AUDIO_DEC_TYPE_OPUS) 
    {
        voice_write_cfg.write_buf_type = PORT_TYPE_FB;
        voice_write_cfg.node_size = OPUS_DEC_FRAME_SIZE;
    } 
    else 
    {
        voice_write_cfg.write_buf_type = PORT_TYPE_RB;
        voice_write_cfg.node_size = config->spk_sample_rate * 2 * config->spk_frame_duration / 1000;
    }
    
    g_voice_ctx.voice_write_handle = bk_voice_write_init(&voice_write_cfg);
    if (!g_voice_ctx.voice_write_handle) 
    {
        LOGE("voice write init failed\n");
        goto error;
    }

    // 启动语音服务
    if (BK_OK != bk_voice_start(g_voice_ctx.voice_handle)) 
    {
        LOGE("voice start failed\n");
        goto error;
    }

    // 启动录音
    if (BK_OK != bk_voice_read_start(g_voice_ctx.voice_read_handle)) 
    {
        LOGE("voice read start failed\n");
        goto error;
    }

    // 启动播放
    if (BK_OK != bk_voice_write_start(g_voice_ctx.voice_write_handle)) 
    {
        LOGE("voice write start failed\n");
        goto error;
    }

#if CONFIG_ASR_SERVICE
    // 启动ASR
    if (BK_OK != bk_asr_start(g_voice_ctx.asr_handle)) 
    {
        LOGE("asr start failed\n");
        goto error;
    }

    if (BK_OK != bk_aud_asr_start(g_voice_ctx.aud_asr_handle)) 
    {
        LOGE("aud asr start failed\n");
        goto error;
    }
#endif

    g_voice_ctx.inited = true;
    LOGI("app voice init success\n");
    
    return BK_OK;

error:
    if (g_voice_ctx.voice_write_handle) 
    {
        bk_voice_write_stop(g_voice_ctx.voice_write_handle);
    }
    
    if (g_voice_ctx.voice_read_handle) 
    {
        bk_voice_read_stop(g_voice_ctx.voice_read_handle);
    }

#if CONFIG_ASR_SERVICE
    if (g_voice_ctx.asr_handle) 
    {
        bk_asr_stop(g_voice_ctx.asr_handle);
    }
    
    if (g_voice_ctx.aud_asr_handle) 
    {
        bk_aud_asr_stop(g_voice_ctx.aud_asr_handle);
    }
#endif
    
    if (g_voice_ctx.voice_handle) 
    {
        bk_voice_stop(g_voice_ctx.voice_handle);
    }
    
    if (g_voice_ctx.voice_write_handle) 
    {
        bk_voice_write_deinit(g_voice_ctx.voice_write_handle);
        g_voice_ctx.voice_write_handle = NULL;
    }
    
    if (g_voice_ctx.voice_read_handle) 
    {
        bk_voice_read_deinit(g_voice_ctx.voice_read_handle);
        g_voice_ctx.voice_read_handle = NULL;
    }
    
#if CONFIG_ASR_SERVICE
    if (g_voice_ctx.aud_asr_handle) 
    {
        bk_aud_asr_deinit(g_voice_ctx.aud_asr_handle);
        g_voice_ctx.aud_asr_handle = NULL;
    }
    
    if (g_voice_ctx.asr_handle) 
    {
        bk_asr_deinit(g_voice_ctx.asr_handle);
        g_voice_ctx.asr_handle = NULL;
    }
#endif
    
    if (g_voice_ctx.voice_handle) 
    {
        bk_voice_deinit(g_voice_ctx.voice_handle);
        g_voice_ctx.voice_handle = NULL;
    }
    
    return BK_FAIL;
}

static int _mic_data_callback(unsigned char *data, unsigned int len, void *args)
{
    //LOGD("[%s][%d] len: %d\r\n", __func__, __LINE__, len);
    
    return 0;
}

static void _asr_result_callback(uint32_t param)
{
    char *result = (char *)param;
    if (result) 
    {
        LOGI("[%s][%d] result: %s\r\n", __func__, __LINE__, result);
    } 
    else 
    {
        LOGI("[%s][%d] result is NULL\r\n", __func__, __LINE__);
    }
}

static void cli_asr_auth_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if(argc != 3)
    {
        LOGE("asr_auth: invalid arguments, usage: asr_auth <auth_id> <auth_key>\r\n");
        return;
    }
    
    asr_auth_t auth = {0};
    os_strncpy(auth.id, argv[1], sizeof(auth.id));
    os_strncpy(auth.key, argv[2], sizeof(auth.key));
    
    int ret = bk_app_asr_set_auth(&auth);
    if (ret != BK_OK) 
    {
        LOGE("bk_app_asr_set_auth fail, ret: %d\r\n", ret);
    } 
    else 
    {
        LOGI("bk_app_asr_set_auth success\r\n");
    }
}

static const struct cli_command asr_auth_cmd[] = 
{
    {"asr_auth", "asr authentication commands", cli_asr_auth_cmd},
};

int cli_app_asr_auth_init(void)
{
    return cli_register_commands(asr_auth_cmd, sizeof(asr_auth_cmd) / sizeof(struct cli_command));
}

int bk_kws_demo_init(void)
{
    int ret = 0;

    app_audio_config_t cfg = {0};
    cfg.pa_ctrl_gpio = GPIO_44;
    cfg.pa_on_level = 1;

    cfg.dec_type = AUDIO_DEC_TYPE_PCM;
    cfg.spk_sample_rate = SPK_SAMPLE_RATE;
    cfg.spk_frame_duration = SPK_FRAME_DURATION;
    cfg.enc_type = AUDIO_ENC_TYPE_PCM;
    cfg.mic_sample_rate = MIC_SAMPLE_RATE;
    cfg.mic_rx_cb = _mic_data_callback;
#if CONFIG_BEKEN_KWS
    cfg.asr_frame_size = 1280; // Beken KWS asr 一帧 40ms，16000*40/1000*2=1280
#endif
    cfg.asr_result_cb = _asr_result_callback;
    ret = bk_app_audio_init(&cfg);
    if (BK_OK != ret) 
    {
        LOGE("app voice init fail, ret: %d\r\n", ret);
        return ret;
    }

    ret = cli_app_asr_auth_init();
    if (ret != BK_OK) 
    {
        LOGE("cli_app_asr_auth_init fail, ret: %d\r\n", ret);
        return ret;
    }

    return ret;
}
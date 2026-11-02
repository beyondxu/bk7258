#ifndef __KWS_DEMO_H__
#define __KWS_DEMO_H__

#include <common/bk_include.h>
#include <components/bk_voice_service_types.h>
#include <components/bk_player_service_types.h>

/**
 * @brief MIC data receive callback function type
 *
 * @param data  Received audio data
 * @param len   Data length
 * @param args  User arguments
 *
 * @return 0 for success, other values for error
 */
typedef int (*voice_mic_rx_cb_t)(unsigned char *data, unsigned int len, void *args);

/**
 * @brief ASR result callback function type
 *
 * @param param  ASR result pointer address
 */
typedef void (*voice_asr_result_cb_t)(uint32_t param);

/**
 * @brief Audio configuration structure
 */

typedef struct {
    bool                aec_enable;                     ///< AEC enable flag
    aec_v3_mode_t       aec_mode;                       ///< AEC mode
    bool                vad_enable;                     ///< VAD enable flag, dependent on AEC
    audio_dec_type_t    dec_type;                       ///< Decoder type

    uint32_t            spk_sample_rate;                ///< Speaker sample rate
    uint8_t             spk_frame_duration;             ///< Speaker frame duration in ms

    uint16_t            pa_ctrl_gpio;                   ///< GPIO id of control pa
    uint8_t             pa_on_level;                    ///< GPIO level of turn on pa, 0: low level, 1: high level

    audio_enc_type_t    enc_type;                       ///< Encoder type
    uint32_t            mic_sample_rate;                ///< MIC sample rate
    voice_mic_rx_cb_t   mic_rx_cb;                      ///< MIC data receive callback function

    uint32_t            asr_frame_size;                 ///< ASR frame size in bytes
    voice_asr_result_cb_t asr_result_cb;                ///< ASR result callback function
    voice_event_handle  event_cb;                       ///< Voice event callback function
} app_audio_config_t;

int bk_kws_demo_init(void);

#endif
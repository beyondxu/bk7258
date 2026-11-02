#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <bk_private/bk_init.h>
#include <media_service.h>
#include <components/log.h>

#define TAG "MIC"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_streams/raw_stream.h>
#include <components/bk_audio/audio_streams/onboard_mic_stream.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream.h>

#define TEST_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            BK_LOGE(TAG, "TEST_CHECK_NULL fail at %s:%d\n", __FILE__, __LINE__);\
            return BK_FAIL;\
        }\
    } while(0)

#define TEST_NUM 100
static int read_count = 0;


/* The "onboard mic stream" element is a producer. The "onboard speaker stream" element is a consumer.
   The pipeline "onboard_mic --> raw" record mic data. The pipeline "raw --> onboard_speaker" play audio data.
   The data flow model of this element is as follow:
   +-----------------+               +-------------------+                               +-----------------+               +-------------------+
   |   onboard-mic   |               |        raw        |                               |      raw        |               |  onboard-speaker  |
   |     stream      |               |   stream[read]    |          +----------+         |  stream[write]  |               |    stream[out]    |
   |                 |               |                   |   read   |          |  write  |                 |               |                   |
   |                src - ringbuf - sink                 | -------> | usr task | ------> |                src - ringbuf - sink                 |
   |                 |               |                   |          |          |         |                 |               |                   |
   |                 |               |                   |          +----------+         |                 |               |                   |
   +-----------------+               +-------------------+                               +-----------------+               +-------------------+

   Function: User task read mic data from "onboard_mic --> raw" pipeline, and write audio data to "raw --> onboard_speaker" pipeline.

   The speaker play audio data from mic.
*/

typedef struct record_ctrl_s {
	audio_pipeline_handle_t record_pipeline;
	audio_element_handle_t mic_handle;
	audio_element_handle_t raw_read;
	audio_event_iface_handle_t record_evt;
} record_ctrl_t;

typedef struct play_ctrl_s {
	audio_pipeline_handle_t play_pipeline;
	audio_element_handle_t raw_write;
	audio_element_handle_t spk_handle;
	audio_event_iface_handle_t play_evt;
} play_ctrl_t;

//for 8kHz, 800bytes last 50ms
#define BUFFER_LEN	800

static bk_err_t _record_pipeline_init(record_ctrl_t *record)
{
	audio_pipeline_cfg_t record_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
	record->record_pipeline = audio_pipeline_init(&record_pipeline_cfg);
	TEST_CHECK_NULL(record->record_pipeline);

	onboard_mic_stream_cfg_t onboard_mic_cfg = ONBOARD_MIC_ADC_STREAM_CFG_DEFAULT();
    onboard_mic_cfg.frame_size = BUFFER_LEN;
    onboard_mic_cfg.out_block_size = BUFFER_LEN;
    onboard_mic_cfg.out_block_num = 1;
    record->mic_handle = onboard_mic_stream_init(&onboard_mic_cfg);
    TEST_CHECK_NULL(record->mic_handle);

    raw_stream_cfg_t raw_read_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_read_cfg.type = AUDIO_STREAM_READER;
    record->raw_read = raw_stream_init(&raw_read_cfg);
    TEST_CHECK_NULL(record->raw_read);

    if (BK_OK != audio_pipeline_register(record->record_pipeline, record->mic_handle, "mic"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(record->record_pipeline, record->raw_read, "raw_read"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_link(record->record_pipeline, (const char *[]){"mic", "raw_read"}, 2))
    {
        BK_LOGE(TAG, "record pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    record->record_evt = audio_event_iface_init(&evt_cfg);
    if (BK_OK != audio_pipeline_set_listener(record->record_pipeline, record->record_evt))
    {
        BK_LOGE(TAG, "set record pipeline listener fail, %d \n", __LINE__);
        return BK_FAIL;
    }

	TEST_PASS();
    return BK_OK;
}

static bk_err_t _record_pipeline_deinit(record_ctrl_t *record)
{
	if (!record || !record->record_pipeline)
		return BK_FAIL;

    if (BK_OK != audio_pipeline_unregister(record->record_pipeline, record->mic_handle))
    {
        BK_LOGE(TAG, "record pipeline unregister fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(record->record_pipeline, record->raw_read))
    {
        BK_LOGE(TAG, "record pipeline unregister fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_remove_listener(record->record_pipeline))
    {
        BK_LOGE(TAG, "record pipeline remove listener fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_event_iface_destroy(record->record_evt))
    {
        BK_LOGE(TAG, "record event iface destroy fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_deinit(record->record_pipeline))
    {
        BK_LOGE(TAG, "record pipeline deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_element_deinit(record->mic_handle))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_element_deinit(record->raw_read))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

	TEST_PASS();
    return BK_OK;
}

static bk_err_t _play_pipeline_init(play_ctrl_t *play)
{
    audio_pipeline_cfg_t play_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    play->play_pipeline = audio_pipeline_init(&play_pipeline_cfg);
    TEST_CHECK_NULL(play->play_pipeline);

    raw_stream_cfg_t raw_write_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_write_cfg.type = AUDIO_STREAM_WRITER;
    play->raw_write = raw_stream_init(&raw_write_cfg);
    TEST_CHECK_NULL(play->raw_write);

    onboard_speaker_stream_cfg_t onboard_speaker_cfg = ONBOARD_SPEAKER_STREAM_CFG_DEFAULT();
    play->spk_handle = onboard_speaker_stream_init(&onboard_speaker_cfg);
    TEST_CHECK_NULL(play->spk_handle);

    if (BK_OK != audio_pipeline_register(play->play_pipeline, play->raw_write, "raw_write"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_register(play->play_pipeline, play->spk_handle, "spk"))
    {
        BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_link(play->play_pipeline, (const char *[]){"raw_write", "spk"}, 2))
    {
        BK_LOGE(TAG, "play pipeline link fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    play->play_evt = audio_event_iface_init(&evt_cfg);
    if (BK_OK != audio_pipeline_set_listener(play->play_pipeline, play->play_evt))
    {
        BK_LOGE(TAG, "set play pipeline listener fail, %d \n", __LINE__);
        return BK_FAIL;
    }

	TEST_PASS();
    return BK_OK;
}

static bk_err_t _play_pipeline_deinit(play_ctrl_t *play)
{
	if (!play || !play->play_pipeline)
		return BK_FAIL;

    if (BK_OK != audio_pipeline_unregister(play->play_pipeline, play->spk_handle))
    {
        BK_LOGE(TAG, "play pipeline unregister fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_unregister(play->play_pipeline, play->raw_write))
    {
        BK_LOGE(TAG, "play pipeline unregister fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_remove_listener(play->play_pipeline))
    {
        BK_LOGE(TAG, "play pipeline remove listener fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_event_iface_destroy(play->play_evt))
    {
        BK_LOGE(TAG, "play event iface destroy fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_deinit(play->play_pipeline))
    {
        BK_LOGE(TAG, "play pipeline deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_element_deinit(play->spk_handle))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_element_deinit(play->raw_write))
    {
        BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
        return BK_FAIL;
    }

	TEST_PASS();
    return BK_OK;
}

static bk_err_t _audio_process(record_ctrl_t *record, play_ctrl_t *play)
{
	bk_err_t ret;
	
    char *buf = audio_calloc(1, BUFFER_LEN);
    TEST_CHECK_NULL(buf);

    if (BK_OK != audio_pipeline_run(record->record_pipeline))
    {
        BK_LOGE(TAG, "record_pipeline run fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_run(play->play_pipeline))
    {
        BK_LOGE(TAG, "play_pipeline run fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    while (1)
    {
        audio_event_iface_msg_t msg;
        ret = audio_event_iface_listen(record->record_evt, &msg, 0);
        if (ret == BK_OK)
        {
            if (msg.cmd == AEL_MSG_CMD_REPORT_STATUS
                && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
            {
                BK_LOGI(TAG, "[ * ] Stop event received \n");
                break;
            }
        }
        ret = audio_event_iface_listen(play->play_evt, &msg, 0);
        if (ret == BK_OK)
        {
            if (msg.cmd == AEL_MSG_CMD_REPORT_STATUS
                && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
            {
                BK_LOGI(TAG, "[ * ] Stop event received \n");
                break;
            }
        }

        int size = raw_stream_read(record->raw_read, buf, BUFFER_LEN);
        if (size > 0)
        {        	
            BK_LOGI(TAG, "raw_stream_read size: %d \n", size);
            size = raw_stream_write(play->raw_write, buf, size);
            if (size <= 0)
            {
                BK_LOGE(TAG, "raw_stream_write size: %d \n", size);
                break;
            }
            else
            {
                //BK_LOGD(TAG, "raw_stream_write size: %d \n", size);
                read_count++;
                if (read_count == TEST_NUM)
                {
                    break;
                }
            }
        }
        else
        {
            BK_LOGE(TAG, "raw_stream_read size: %d \n", size);
        }
    }

    audio_free(buf);

    if (BK_OK != audio_pipeline_stop(record->record_pipeline))
    {
        BK_LOGE(TAG, "record_pipeline stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_wait_for_stop(record->record_pipeline))
    {
        BK_LOGE(TAG, "record_pipeline wait stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_terminate(record->record_pipeline))
    {
        BK_LOGE(TAG, "record pipeline terminate fail, %d \n", __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != audio_pipeline_stop(play->play_pipeline))
    {
        BK_LOGE(TAG, "play_pipeline stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_wait_for_stop(play->play_pipeline))
    {
        BK_LOGE(TAG, "play_pipeline wait stop fail, %d \n", __LINE__);
        return BK_FAIL;
    }
    if (BK_OK != audio_pipeline_terminate(play->play_pipeline))
    {
        BK_LOGE(TAG, "play pipeline terminate fail, %d \n", __LINE__);
        return BK_FAIL;
    }

	TEST_PASS();
    return BK_OK;
}

int main(void)
{
    record_ctrl_t record;
    play_ctrl_t play;

    bk_init();
    media_service_init();

   	if (BK_OK != _record_pipeline_init(&record)) {
        LOGE("[%s][%d] record pipeline init fail\r\n", __FUNCTION__, __LINE__);
        return BK_FAIL;
   	}

   	if (BK_OK != _play_pipeline_init(&play)) {
        LOGE("[%s][%d] play pipeline init fail\r\n", __FUNCTION__, __LINE__);
        return BK_FAIL;
   	}

   	if (BK_OK != _audio_process(&record, &play)) {
        LOGE("[%s][%d] play pipeline init fail\r\n", __FUNCTION__, __LINE__);
        return BK_FAIL;
   	}

   	if (BK_OK != _record_pipeline_deinit(&record)) {
        LOGE("[%s][%d] record pipeline deinit fail\r\n", __FUNCTION__, __LINE__);
        return BK_FAIL;
   	}

   	if (BK_OK != _play_pipeline_deinit(&play)) {
        LOGE("[%s][%d] play pipeline deinit fail\r\n", __FUNCTION__, __LINE__);
        return BK_FAIL;
   	}

    return 0;
}

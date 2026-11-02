#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <bk_private/bk_init.h>
#include <media_service.h>
#include <components/log.h>

#include <bk_audio_pipeline.h>

#define TAG "AUDIO"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

#define TEST_AUDIO_FORMAT			CODEC_FORMAT_PCM
#define TEST_AUDIO_SAMPLE_RATE		16000

static bk_err_t _audio_init(void)
{
	bk_err_t ret;
	
	audio_parameters_t params = 
	{
			.uac = 0,
			.aec = 1,
			.vad = 0,
			.asr = 1,
			.rmt_recoder_fmt = TEST_AUDIO_FORMAT,
			.rmt_player_fmt = TEST_AUDIO_FORMAT,
			.rmt_recorder_sample_rate = TEST_AUDIO_SAMPLE_RATE,
			.rmt_player_sample_rate = TEST_AUDIO_SAMPLE_RATE
	};

   	ret = bk_audio_turn_on(&params);
   	if (ret == BK_OK)
   		TEST_PASS();
   	else
   		TEST_FAIL();

   	return ret;
}

static bk_err_t _audio_deinit(void)
{
	return bk_audio_turn_off();
}

static int _get_mic_data(unsigned char *buffer, int length)
{
	int ret;

	while(1) {
		ret = bk_audio_read_mic_data(buffer, length);
		if (ret > 0)
			break;
		else if(ret < 0)	//error
			break;
		rtos_delay_milliseconds(20);
	}

	return ret;
}

static int _write_spk_data(unsigned char *buffer, int length)
{
	int ret;

	while(1) {
		ret = bk_audio_write_spk_data(buffer, length);
		if (ret == length)
			break;
		else if (ret < 0)	//error
			break;
		rtos_delay_milliseconds(20);
	}

	return ret;
}

static bk_err_t _audio_process(void)
{
	int length = TEST_AUDIO_SAMPLE_RATE * 2 * 20 / 1000;	//每20ms读一次
	unsigned char *buffer = psram_malloc(length);
	int ret;
	int ret2;
	int now = rtos_get_time();

	if (!buffer)
		return BK_FAIL;

	bk_audio_mic_ctrl(1);

	while(1)
	{
		ret = _get_mic_data(buffer, length);
		os_printf("mic len=%d\n", ret);
		if (ret < 0)
			break;

		ret2 = _write_spk_data(buffer, ret);
		if (ret2 != ret)
			break;

		int delay_ms = ret / (TEST_AUDIO_SAMPLE_RATE * 2 / 1000);
		if (delay_ms > 4)
			delay_ms -= 4;
		rtos_delay_milliseconds(delay_ms);	//take a breath
		
		if (rtos_get_time() - now >= 20 * 1000) {	//test last for 20s
			break;
		}
	}

	bk_audio_mic_ctrl(0);

	os_free(buffer);

	return BK_OK;
}

int main(void)
{
    bk_init();
    media_service_init();

   	if (BK_OK != _audio_init()) {
        LOGE("[%s][%d] audio init fail\r\n", __FUNCTION__, __LINE__);
        return BK_FAIL;
   	}

   	if (BK_OK != _audio_process()) {
        LOGE("[%s][%d] audio process fail\r\n", __FUNCTION__, __LINE__);
        return BK_FAIL;
   	}

   	if (BK_OK != _audio_deinit()) {
        LOGE("[%s][%d] audio deinit fail\r\n", __FUNCTION__, __LINE__);
        return BK_FAIL;
   	}

    return 0;
}

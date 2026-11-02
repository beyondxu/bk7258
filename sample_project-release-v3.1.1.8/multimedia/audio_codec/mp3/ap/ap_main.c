#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <bk_private/bk_init.h>
#include <media_service.h>
#include <components/log.h>

#define TAG "PLAYER"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

#if CONFIG_PLAYER_SERVICE
#include <components/bk_player_service.h>
#include <components/bk_voice_service.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/rb_port.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream.h>
#endif
#include <bk_posix.h>

typedef enum
{
	MUSIC_STATUS_IDLE,
	MUSIC_STATUS_PLAYING,
} MUSIC_STATUS;

static MUSIC_STATUS music_state = MUSIC_STATUS_IDLE;

#define OUTPUT_RB_SIZE  (4 * 1024)
static audio_port_handle_t g_output_port_handle = NULL;

static bk_player_handle_t player_handle = NULL;

static voice_handle_t get_voice_handle(void)
{
	return NULL;
}

static int player_port_state_notify_handler(int state, void *port_info, void *user_data)
{
    LOGI("%s, %d, ++>>>>>>>> state: %d \n", __func__, __LINE__, state);
	switch(state) {
		case APT_STATE_FINISHED:
			LOGI("%s finish_tone\n", __func__);
			music_state = MUSIC_STATUS_IDLE;
			break;
		default:
			break;	
	}

    return BK_OK;
}

static int player_event_handler(int data, void *params, void *args)
{
    if (data == PLAYER_EVENT_MUSIC_INFO) {
		LOGI("%s got music info\n", __func__);
		
        audio_element_info_t *music_info = (audio_element_info_t *)params;
		voice_handle_t voice_handle = (voice_handle_t)get_voice_handle();
        if (voice_handle == NULL)
        {
            LOGE("%s, %d, doubao_audio_get_voice_handle fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
        audio_element_handle_t spk_element = bk_voice_get_spk_element(voice_handle);
        if (spk_element == NULL)
        {
            LOGE("%s, %d, bk_voice_get_spk_element fail\n", __func__, __LINE__);
            return BK_FAIL;
        }

        audio_port_info_t port_info = {0};
        port_info.chl_num = music_info->channels;
        port_info.sample_rate = music_info->sample_rates;
        port_info.dig_gain = 0x2d;
        port_info.ana_gain = 0x01;
        port_info.bits = music_info->bits;
        port_info.port_id = 1;
        port_info.priority = 1;
        port_info.port = g_output_port_handle;
        port_info.notify_cb = player_port_state_notify_handler;
        port_info.user_data = NULL;
        if (BK_OK != onboard_speaker_stream_set_input_port_info(spk_element, &port_info))
        {
            LOGE("%s, %d, audio_element_set_multi_input_port fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
        LOGI("%s sample_rates: %d, bits: %d, channels: %d\n", __func__, music_info->sample_rates, music_info->bits, music_info->channels);
    } else if (data == PLAYER_EVENT_FINISH) {
    	LOGI("%s got finish\n", __func__);
		music_state = MUSIC_STATUS_IDLE;
    } else {
    	LOGI("%s got unknow event %d\n", __func__, data);
    }

	return BK_OK;
}

#define WITH_PLAYBACK 1

static bk_player_handle_t _play_init(void)
{
	int ret;
	bk_player_handle_t handle;

#if (WITH_PLAYBACK)
	bk_player_cfg_t player_cfg = DEFAULT_PLAYER_WITH_PLAYBACK_CONFIG();
#else
	bk_player_cfg_t player_cfg = DEFAULT_PLAYER_NOT_PLAYBACK_CONFIG();
#endif

	//player_cfg.spk_cfg.onboard_spk_cfg.chl_num = 2;
	player_cfg.event_handle = player_event_handler;
	player_cfg.args = NULL;
	handle = bk_player_create(&player_cfg);

	if (!handle)
		return NULL;

	player_uri_info_t uri_info = {0};
	uri_info.uri_type = PLAYER_URI_TYPE_VFS;
	uri_info.uri = "xxx.mp3";
	ret = bk_player_set_uri(handle, &uri_info);	//???
	if (ret != BK_OK)
	{
		LOGE("%s, %d, bk_player_set_uri fail\n", __func__, __LINE__);
	}

	ringbuf_port_cfg_t cfg = RINGBUF_PORT_CFG_DEFAULT();
	cfg.ringbuf_size = OUTPUT_RB_SIZE;
	g_output_port_handle = ringbuf_port_init(&cfg);
	if (g_output_port_handle == NULL)
	{
		LOGE("%s, %d, ringbuf_port_init fail\n", __func__, __LINE__);
		return NULL;
	}

#if (WITH_PLAYBACK)
	//player has its own output
#else
	ret = bk_player_set_output_port(handle, g_output_port_handle);
	if (ret != BK_OK)
	{
		LOGE("%s, %d, bk_player_set_output_port fail\n", __func__, __LINE__);
		return NULL;
	}
#endif

	return handle;
}

static int _play_deinit(bk_player_handle_t handle)
{
	int ret;

	if (!handle)
		return BK_FAIL;

	ret = bk_player_stop(handle);
	if (ret != BK_OK)
	{
		LOGE("%s, %d, bk_player_stop fail\n", __func__, __LINE__);
	}

#if (WITH_PLAYBACK)
	//player has its own output
#else
	voice_handle_t voice_handle = (voice_handle_t)get_voice_handle();
	if (voice_handle == NULL)
	{
		LOGE("%s, %d, doubao_audio_get_voice_handle fail\n", __func__, __LINE__);
		return BK_FAIL;
	}
	audio_element_handle_t spk_element = bk_voice_get_spk_element(voice_handle);
	if (spk_element == NULL)
	{
		LOGE("%s, %d, bk_voice_get_spk_element fail\n", __func__, __LINE__);
		return BK_FAIL;
	}

	audio_port_info_t port_info = {0};
	port_info.chl_num = 1;
	port_info.sample_rate = 8000;
	port_info.dig_gain = 0x2d;
	port_info.ana_gain = 0x01;
	port_info.bits = 16;
	port_info.port_id = 1;
	port_info.priority = 1;
	port_info.port = NULL;
	if (BK_OK != onboard_speaker_stream_set_input_port_info(spk_element, &port_info))
	{
		LOGE("%s, %d, audio_element_set_multi_input_port fail\n", __func__, __LINE__);
	}

	bk_player_set_output_port(handle, NULL);
#endif

	audio_port_deinit(g_output_port_handle);

	ret = bk_player_destroy(handle);
	if (ret != BK_OK)
	{
		LOGE("%s, %d, bk_player_destroy fail\n", __func__, __LINE__);
		return BK_FAIL;
	}	

	return BK_OK;
}

static int _play_music(char *uri)
{
	int ret;
	player_uri_info_t uri_info = {0};
	bk_player_handle_t handle = player_handle;

	if (!uri)
		return -1;

	if (!handle)
		return -1;

	ret = bk_player_stop(handle);
	if (ret != BK_OK)
	{
		LOGE("%s, %d, bk_player_stop fail, ret=%d\n", __func__, __LINE__, ret);
	}

	uri_info.uri_type = PLAYER_URI_TYPE_VFS;
	uri_info.uri = uri;
	ret = bk_player_set_uri(handle, &uri_info);
	if (ret != BK_OK)
	{
		LOGE("%s, %d, bk_player_set_uri fail, ret=%d\n", __func__, __LINE__, ret);
		return -2;
	}

	ret = bk_player_start(handle);
	if (ret != BK_OK)
	{
		LOGE("%s, %d, bk_player_start fail, ret=%d\n", __func__, __LINE__, ret);
		return -3;
	}

	music_state = MUSIC_STATUS_PLAYING;

	int cnt = 10 * 1000 / 20;
	while(cnt-- > 0) {
		if (music_state == MUSIC_STATUS_IDLE)
			break;
		rtos_delay_milliseconds(20);
	}

	TEST_PASS();
	return BK_OK;
}

void _stop_music(void)
{
	bk_player_handle_t handle = player_handle;

	if (!handle)
		return;

	bk_player_stop(handle);
}

#define VFS_MOUNT_PATH           "/sd0"
#define TEST_FILE_NAME           VFS_MOUNT_PATH"/test.mp3"

static bk_err_t _mount_fs(char *mount_path)
{
    struct bk_fatfs_partition partition;
    char *fs_name = NULL;
    bk_err_t ret = BK_FAIL;

    fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
	partition.part_dev.device_name = FATFS_DEV_SDCARD;
    partition.mount_path = mount_path;

    ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

    LOGI("[%s][%d] mount ret:%d\r\n", __FUNCTION__, __LINE__, ret);
    
    return ret;
}

int main(void)
{
    bk_init();
    media_service_init();

    if(BK_OK != _mount_fs(VFS_MOUNT_PATH))
    {
        LOGE("[%s][%d] fs mount fail\r\n", __FUNCTION__, __LINE__);
        return BK_FAIL;
    }

    player_handle = _play_init();
    if (!player_handle) {
        LOGE("[%s][%d] player init fail\r\n", __FUNCTION__, __LINE__);
        return BK_FAIL;
    }

   	if (BK_OK != _play_music(TEST_FILE_NAME)) {
        LOGE("[%s][%d] play %s fail\r\n", __FUNCTION__, __LINE__, TEST_FILE_NAME);
        return BK_FAIL;
   	}

   	if (BK_OK != _play_deinit(player_handle)) {
        LOGE("[%s][%d] play deinit fail\r\n", __FUNCTION__, __LINE__);
        return BK_FAIL;
   	}

    return 0;
}

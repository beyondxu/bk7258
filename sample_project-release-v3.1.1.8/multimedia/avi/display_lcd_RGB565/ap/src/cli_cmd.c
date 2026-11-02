#include <os/os.h>

#include "dev_lcd.h"
#include "dev_audio.h"
#include "fatfs_fs.h"
#include "jpeg_hw_decode.h"
#include "bk_avi_player.h"

#include "cli.h"

#define TAG "exam_avi_player"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static bool _is_play_end = false;
/**
 * @brief 给解码线程调用的callback，用于分配JPEG原始图片存储的内存
 * @param size jpeg原始图片内容长度
 * @return 成功返回分配的frame_buffer_t指针，失败返回NULL
 */
static frame_buffer_t *_jpeg_frame_malloc(uint32 size)
{
    return frame_buffer_encode_malloc(size);
}

/**
 * @brief 给解码线程调用的callback，用于释放JPEG原始图片存储的内存
 * @return 无
 */
static void _jpeg_frame_free(frame_buffer_t *frame)
{
    frame_buffer_encode_free(frame);
}

/**
 * @brief 给解码线程调用的callback，用于分配JPEG解码内容用来存储的内存
 * @param size 解码内存大小
 * @return 成功返回分配的frame_buffer_t指针，失败返回NULL
 */
static frame_buffer_t *_decode_frame_malloc(uint32 size)
{
    return frame_buffer_display_malloc(size);
}

/**
 * @brief 给解码线程调用的callback，用于释放JPEG解码图片存储的内存
 * @return 无
 */
static void _decode_frame_free(frame_buffer_t *frame)
{
    frame_buffer_display_free(frame);
}

/**
 * @brief 给显示调用的callback，用于释放已显示的图片
 * @return 无
 */
static int display_free_callback(void *frame)
{
    _decode_frame_free((frame_buffer_t *)frame);
    return BK_OK;
}

/**
 * @brief 给显示调用的callback，用于刷新LCD显示
 * @return 无
 */
static void _lcd_cb(frame_buffer_t *display_frame, void *user_data)
{
    dev_lcd_flush(display_frame, display_free_callback);
}

/**
 * @brief 给音频调用的callback，用于分配音频数据内存
 * @param size 音频数据大小
 * @return 成功返回分配的音频数据指针，失败返回NULL
 */
static uint8_t *_voice_malloc_cb(uint32_t size)
{
    return os_malloc(size);
}
/**
 * @brief 给音频调用的callback，用于释放音频数据内存
 * @param audio_frame 音频数据指针
 * @return 无
 */
static void _voice_free_cb(uint8_t *audio_frame)
{
    if(audio_frame)
    {
        os_free(audio_frame);
        audio_frame = NULL;
    }
}
/**
 * @brief 给音频调用的callback，用于释放已播放的音频数据
 * @return 无
 */
static void speaker_free_callback(void *data)
{
    _voice_free_cb(data);
}

/**
 * @brief 给音频调用的callback，用于播放音频数据
 * @return 无
 */
static void _speaker_cb(uint8_t *data, uint32_t size, void *user_data)
{
    dev_audio_sendto_speaker(data, size, speaker_free_callback);
}

static void _finish_cb(void *user_data)
{
    _is_play_end = true;
}


static bk_err_t _jpeg_in_complete(frame_buffer_t *in_frame)
{
    return BK_OK;
}

static bk_err_t _jpeg_decode_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame)
{
    if (result == BK_OK) 
    {
        //LOGD("[%s][%d] jpeg decode success! format_type: %d, out_frame: %p\n", __func__, __LINE__, format_type, out_frame);
    } 
    else 
    {
        LOGE("[%s][%d] jpeg decode failed! format_type: %d, result: %d, out_frame: %p\n", __func__, __LINE__, format_type, result, out_frame);
    }

    return BK_OK;
}

static bk_jpeg_decode_hw_handle_t g_jpeg_decode_handle = NULL;
static bk_jpeg_decode_hw_config_t g_jpeg_decode_config = {
    .decode_cbs = {
        .in_complete = _jpeg_in_complete,
        .out_complete = _jpeg_decode_complete,
    }
};

//硬解码初始化
static bk_err_t _jpeg_hw_decode_init(void)
{
    bk_err_t ret = BK_FAIL;

    do{
        ret = bk_hardware_jpeg_decode_new(&g_jpeg_decode_handle, &g_jpeg_decode_config);
        if(BK_OK != ret)
        {
            LOGE("[%s][%d] jpeg hw new fail\r\n", __FUNCTION__, __LINE__);
            break;
        }

        ret = bk_jpeg_decode_hw_open(g_jpeg_decode_handle);
        if(BK_OK != ret)
        {
            LOGE("[%s][%d] jpeg hw open fail\r\n", __FUNCTION__, __LINE__);
            break;
        }
    }while(0);

    return ret;
}

//硬解码反初始化
static bk_err_t _jpeg_hw_decode_deinit(void)
{
    if(g_jpeg_decode_handle)
    {
        bk_jpeg_decode_hw_close(g_jpeg_decode_handle);
        bk_jpeg_decode_hw_delete(g_jpeg_decode_handle);

        g_jpeg_decode_handle = NULL;
    }
    return BK_OK;
}

void example_avi_player_start(const char *file_name)
{
    int ret = BK_OK;
    ret = dev_lcd_open();
    if(ret != BK_OK)
    {
        LOGE("[%s] [%d] test_display_turn_on failed: %d\r\n", __func__, __LINE__, ret);
        return;
    }
    dev_lcd_backlight_ctrl(100);

    ret = dev_audio_open(0, 1, 16000, AUDIO_ENC_TYPE_PCM);
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] dev_audio_open failed: %d\r\n", __func__, __LINE__, ret);
        return;
    }

    ret = _jpeg_hw_decode_init();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] jpeg_hw_decode_init failed: %d\r\n", __func__, __LINE__, ret);
        return;
    }

    ret = sdcard_fatfs_mount();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] sdcard_source_mount failed: %d\r\n", __func__, __LINE__, ret);
        return;
    }

    avi_player_config_t avi_player_config = {0};
    avi_player_config.jpeg_hw_decode_config.jpeg_decode_handle = g_jpeg_decode_handle;
    avi_player_config.jpeg_hw_decode_config.jpeg_frame_malloc = _jpeg_frame_malloc;
    avi_player_config.jpeg_hw_decode_config.jpeg_frame_free = _jpeg_frame_free;
    avi_player_config.jpeg_hw_decode_config.decode_frame_malloc = _decode_frame_malloc;
    avi_player_config.jpeg_hw_decode_config.decode_frame_free = _decode_frame_free;
    avi_player_config.jpeg_hw_decode_config.output_width = 0;
    avi_player_config.jpeg_hw_decode_config.output_height = 0;
    avi_player_config.jpeg_hw_decode_config.use_dma2d = false;
    avi_player_config.jpeg_hw_decode_config.byte_swap = false;
    avi_player_config.decode_format = AVI_PLAYER_DECODE_FORMAT_YUV422;
    avi_player_config.sync_time_threshold_ms = 65;
    avi_player_config.lcd_cb = _lcd_cb;

    avi_player_config.voice_malloc_cb = _voice_malloc_cb;
    avi_player_config.voice_free_cb = _voice_free_cb;
    avi_player_config.speaker_cb = _speaker_cb;
    avi_player_config.audio_frame_size = 640;

    avi_player_config.user_data = NULL;

    ret = avi_player_init(&avi_player_config);
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] avi_player_init failed: %d\r\n", __func__, __LINE__, ret);
        return;
    }

    char path[255];
    snprintf(path, sizeof(path), file_name);
    ret = avi_player_file_open(path);
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] avi_player_file_open failed: %d\r\n", __func__, __LINE__, ret);
        return;
    }

    ret = avi_player_start();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] avi_player_start failed: %d\r\n", __func__, __LINE__, ret);
        return;
    }
}

void example_avi_player_stop(void)
{
    int ret = 0;
    ret = avi_player_stop();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] avi_player_stop failed: %d\r\n", __func__, __LINE__, ret);
        return;
    }

    ret = avi_player_file_close();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] avi_player_file_close failed: %d\r\n", __func__, __LINE__, ret);
        return;
    }

    ret = avi_player_deinit();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] avi_player_deinit failed: %d\r\n", __func__, __LINE__, ret);
        return;
    }

    ret = _jpeg_hw_decode_deinit();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] jpeg_hw_decode_deinit failed: %d\r\n", __func__, __LINE__, ret);
        return;
    }

    ret = dev_lcd_close();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] dev_lcd_close failed: %d\r\n", __func__, __LINE__, ret);
        return;
    }

    ret = dev_audio_close();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] dev_audio_close failed: %d\r\n", __func__, __LINE__, ret);
        return;
    }
    ret = sdcard_fatfs_umount();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] sdcard_fatfs_umount failed: %d\r\n", __func__, __LINE__, ret);
        return;
    }
}

#define CMDS_COUNT  (sizeof(s_avi_player_cmd) / sizeof(struct cli_command))
void cli_avi_player_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    LOGD("%s %d\r\n", __func__, __LINE__);
    if(strcmp(argv[1], "start") == 0)
    {
        example_avi_player_start(argv[2]);
    }
    if(strcmp(argv[1], "stop") == 0)
    {
        example_avi_player_stop();
    }
    if(strcmp(argv[1], "pause") == 0)
    {
        avi_player_pause();
    }
    if(strcmp(argv[1], "resume") == 0)
    {
        avi_player_resume();
    }
    if(strcmp(argv[1], "set_pos") == 0)
    {
        avi_player_set_pos(atoi(argv[2]));
    }
    if(strcmp(argv[1], "set_loop") == 0)
    {
        avi_player_set_loop(atoi(argv[2]));
    }
}

static const struct cli_command s_avi_player_cmd[] =
{
    {"avi_player", "start filename/pause/resume/stop/set_pos/set_loop", cli_avi_player_cmd},
};

int cli_avi_player_init(void)
{
    return cli_register_commands(s_avi_player_cmd, CMDS_COUNT);
}

#include <os/os.h>

#include "dev_camera.h"
#include "dev_lcd.h"
#include "dev_audio.h"
#include "fatfs_fs.h"
#include "bk_avi_record.h"
#include "video_frame_queue.h"
#include "audio_frame_queue.h"

#include "cli.h"

#define TAG "exam_avi_record"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static frame_buffer_t *jpeg_read(void)
{
    frame_buffer_t *temp = frame_queue_v2_get_frame(IMAGE_MJPEG, CONSUMER_STORAGE, 50);
    if(temp)
    {
        return temp;
    }
    return NULL;
}

static bk_err_t jpeg_complete(bk_err_t result, frame_buffer_t *out_frame)
{
    bk_err_t ret = BK_OK;
    frame_queue_v2_release_frame(IMAGE_MJPEG, CONSUMER_STORAGE, out_frame);
    return ret;
}

static audio_frame_t *audio_read(void)
{
    audio_frame_t *temp = (audio_frame_t*)audio_queue_get_frame(AUDIO_PCM, 50);
    if(temp)
    {
        return temp;
    }
    return NULL;
}

static bk_err_t audio_complete(bk_err_t result, audio_frame_t *out_audio)
{
    bk_err_t ret = BK_OK;
    audio_queue_free(AUDIO_PCM, (audio_buffer_t*)out_audio);
    return ret;
}

void example_avi_record_start(const char *file_name)
{
    int ret = BK_OK;
    ret = frame_queue_v2_init_all();
    if (ret != BK_OK)
    {
        LOGE("[%s][%d] frame_queue_v2_init_all failed\n", __func__, __LINE__);
        return ;
    }
    ret = audio_queue_init_all();
    if (ret != BK_OK)
    {
        LOGE("[%s][%d] audio_queue_init_all failed\n", __func__, __LINE__);
        return ;
    }

    ret = dev_dvp_camera_open(480, 480, FPS20);
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] dev_dvp_camera_open failed, ret = %d\r\n", __func__, __LINE__, ret);
        return;
    }
    ret = dev_audio_open(0, 1, 16000, AUDIO_ENC_TYPE_PCM);
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] dev_audio_open failed, ret = %d\r\n", __func__, __LINE__, ret);
        return;
    }
    ret = dev_lcd_open();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] dev_lcd_open failed, ret = %d\r\n", __func__, __LINE__, ret);
        return;
    }
    dev_lcd_backlight_ctrl(100);

    ret = dev_lcd_display_camera_start();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] dev_lcd_display_camera_start failed, ret = %d\r\n", __func__, __LINE__, ret);
        return;
    }
    frame_queue_v2_register_consumer(IMAGE_MJPEG, CONSUMER_STORAGE);

    ret = sdcard_fatfs_mount();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] sdcard_fatfs_mount failed, ret = %d\r\n", __func__, __LINE__, ret);
        return;
    }
    /*
     * 功能：配置 AVI 录制节点内存池
     * 参数：true  – 一次性预分配 61440 个节点（约 15 分钟 20 fps 视频）
     *       false – 节点用完后动态 malloc（节省PSRAM，但可能碎片）
     * 默认不调用该函数使用动态分配节点
     * 内存开销：61440 × 16 bytes ≈ 960 KB
     * 录制时长：20 fps 视频（20个nodes） + 每 20 ms 一帧音频（50个node） ≈ 70 节点/秒
     *          61440 ÷ 70 ≈ 877 秒 ≈ 14.6 分钟
     */
    bk_AVI_set_node_pool_mode(1, 12280);  // 如需启用节点池模式，可取消此行注释

    // 配置 AVI 录制参数
    avi_record_config_t avi_record_config = {0};
    avi_record_config.video_enable = true;
    avi_record_config.video_width = 480;
    avi_record_config.video_height = 480;
    avi_record_config.video_fps = 20;
    avi_record_config.video_format = AVI_VIDEO_FORMAT_MJPEG;
    avi_record_config.read_video_frame_cb = jpeg_read;
    avi_record_config.complete_video_frame_cb = jpeg_complete;
    
    avi_record_config.audio_enable = true;
    avi_record_config.audio_chan = 1;
    avi_record_config.audio_sample = 16000;
    avi_record_config.audio_bits = 16;
    avi_record_config.audio_format = AVI_AUDIO_FORMAT_PCM;
    avi_record_config.read_audio_frame_cb = audio_read;
    avi_record_config.complete_audio_frame_cb = audio_complete;
    avi_record_config.record_finish_cb = NULL;
    
    ret = avi_record_init(&avi_record_config);
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] avi_record_init failed, ret = %d\r\n", __func__, __LINE__, ret);
        return;
    }
    
    char path[255];
    snprintf(path, sizeof(path), file_name);
    ret = avi_record_start(path);
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] avi_record_start failed, ret = %d\r\n", __func__, __LINE__, ret);
        return;
    }
}

void example_avi_record_stop(void)
{
    int ret = BK_OK;
    ret = avi_record_stop();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] avi_record_stop failed, ret = %d\r\n", __func__, __LINE__, ret);
    }
    ret = avi_record_deinit();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] avi_record_deinit failed, ret = %d\r\n", __func__, __LINE__, ret);
    }
    ret = dev_lcd_display_camera_stop();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] dev_lcd_display_camera_stop failed, ret = %d\r\n", __func__, __LINE__, ret);
    }
    ret = dev_lcd_close();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] dev_lcd_close failed, ret = %d\r\n", __func__, __LINE__, ret);
    }

    ret = dev_audio_close();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] dev_audio_close failed, ret = %d\r\n", __func__, __LINE__, ret);
    }

    ret = dev_dvp_camera_close();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] dev_dvp_camera_close failed, ret = %d\r\n", __func__, __LINE__, ret);
    }
    
    ret = frame_queue_v2_unregister_consumer(IMAGE_MJPEG, CONSUMER_STORAGE);
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] frame_queue_v2_unregister_consumer failed, ret = %d\r\n", __func__, __LINE__, ret);
    }
    audio_queue_deinit_all();
    frame_queue_v2_clear_all();
    frame_queue_v2_deinit_all();
    sdcard_fatfs_umount();
}


void example_avi_single_video_record_start(const char *file_name)
{
    int ret = BK_OK;
    ret = frame_queue_v2_init_all();
    if (ret != BK_OK)
    {
        LOGE("[%s][%d] frame_queue_v2_init_all failed\n", __func__, __LINE__);
        return ;
    }
    ret = dev_dvp_camera_open(480, 480, FPS20);
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] dev_dvp_camera_open failed, ret = %d\r\n", __func__, __LINE__, ret);
        return;
    }

    ret = dev_lcd_open();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] dev_lcd_open failed, ret = %d\r\n", __func__, __LINE__, ret);
        return;
    }
    dev_lcd_backlight_ctrl(100);

    ret = dev_lcd_display_camera_start();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] dev_lcd_display_camera_start failed, ret = %d\r\n", __func__, __LINE__, ret);
        return;
    }

    frame_queue_v2_register_consumer(IMAGE_MJPEG, CONSUMER_STORAGE);
    ret = sdcard_fatfs_mount();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] sdcard_fatfs_mount failed, ret = %d\r\n", __func__, __LINE__, ret);
        return;
    }

    /*
     * 功能：配置 AVI 录制节点内存池
     * 参数：true  – 一次性预分配 61440 个节点（约 15 分钟 20 fps 视频）
     *       false – 节点用完后动态 malloc（节省PSRAM，但可能碎片）
     * 内存开销：61440 × 16 bytes ≈ 960 KB
     * 录制时长：20 fps 视频（20个nodes） + 每 20 ms 一帧音频（50个node） ≈ 70 节点/秒
     *          61440 ÷ 70 ≈ 877 秒 ≈ 14.6 分钟
     */
    
    bk_AVI_set_node_pool_mode(1, 12280);  // 如需启用节点池模式，可取消此行注释

    // 配置 AVI 录制参数
    avi_record_config_t avi_record_config = {0};
    avi_record_config.video_enable = true;
    avi_record_config.video_width = 480;
    avi_record_config.video_height = 480;
    avi_record_config.video_fps = 20;
    avi_record_config.video_format = AVI_VIDEO_FORMAT_MJPEG;
    avi_record_config.read_video_frame_cb = jpeg_read;
    avi_record_config.complete_video_frame_cb = jpeg_complete;
    avi_record_config.audio_enable = false;

    ret = avi_record_init(&avi_record_config);
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] avi_record_init failed, ret = %d\r\n", __func__, __LINE__, ret);
        return;
    }
    // 启动 AVI 录制
    char path[255];
    snprintf(path, sizeof(path), file_name);
    ret = avi_record_start(path);
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] avi_record_start failed, ret = %d\r\n", __func__, __LINE__, ret);
        return;
    }
}

void example_avi_single_video_record_stop(void)
{
    int ret ;
    ret = avi_record_stop();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] avi_record_stop failed, ret = %d\r\n", __func__, __LINE__, ret);
    }
    ret = avi_record_deinit();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] avi_record_deinit failed, ret = %d\r\n", __func__, __LINE__, ret);
    }

    ret = dev_lcd_display_camera_stop();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] dev_lcd_display_camera_stop failed, ret = %d\r\n", __func__, __LINE__, ret);
    }

    ret = dev_lcd_close();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] dev_lcd_close failed, ret = %d\r\n", __func__, __LINE__, ret);
    }

    ret = dev_dvp_camera_close();
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] dev_dvp_camera_close failed, ret = %d\r\n", __func__, __LINE__, ret);
    }
    
    ret = frame_queue_v2_unregister_consumer(IMAGE_MJPEG, CONSUMER_STORAGE);
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] frame_queue_v2_unregister_consumer failed, ret = %d\r\n", __func__, __LINE__, ret);
    }
    frame_queue_v2_clear_all();
    frame_queue_v2_deinit_all();
    sdcard_fatfs_umount();
}

static beken_timer_t avi_record_timer = {0};
static beken_thread_t avi_record_thread = NULL;
static beken_semaphore_t avi_record_sem = NULL;

#define RECORD_INTERVAL_MS    (20 * 60 * 1000)   // 20 mins
#define TOTAL_DURATION_MS     (2 * 60 * 60 * 1000) // 2 hours
static int file_index = 0;
static bool record_start = false;
static bool stress_test_exit = false;

static avi_record_config_t stress_test_config = {
    .video_enable = true,
    .video_width = 480,
    .video_height = 480,
    .video_fps = 20,
    .video_format = AVI_VIDEO_FORMAT_MJPEG,
    .read_video_frame_cb = jpeg_read,
    .complete_video_frame_cb = jpeg_complete,
     
    .audio_enable = true,
    .audio_chan = 1,
    .audio_sample = 16000,
    .audio_bits = 16,
    .audio_format = AVI_AUDIO_FORMAT_PCM,
    .read_audio_frame_cb = audio_read,
    .complete_audio_frame_cb = audio_complete,
    .record_finish_cb = NULL,
};

static void avi_record_timer_handler(void *param)
{
    file_index++;
    rtos_set_semaphore(&avi_record_sem);
}

static void avi_record_Stress_thr_task()
{
    int ret;
    char path[255];

    bk_AVI_set_node_pool_mode(1, 61440); 
    frame_queue_v2_register_consumer(IMAGE_MJPEG, CONSUMER_STORAGE);
    ret = avi_record_init(&stress_test_config);
    if (ret != BK_OK)
    {
        LOGE("avi_record_init failed, ret=%d\r\n", ret);
        return ;
    }

    while (1)
    {
        rtos_get_semaphore(&avi_record_sem, BEKEN_WAIT_FOREVER);
        
        if (file_index * RECORD_INTERVAL_MS >= TOTAL_DURATION_MS)
        {
            stress_test_exit = true;
        }

        if (record_start)
        {
            avi_record_stop();
            record_start = false;
        }
           
        if (stress_test_exit)
        {
            break;
        }

        rtos_delay_milliseconds(100);
        if (!record_start)
        {
            snprintf(path, sizeof(path), "/stress_test_%02d.avi", file_index);
            avi_record_start(path);
            record_start = true;
        }
    }

    avi_record_deinit();
    dev_lcd_display_camera_stop();
    dev_lcd_close();
    dev_audio_close();
    dev_dvp_camera_close();
    frame_queue_v2_unregister_consumer(IMAGE_MJPEG, CONSUMER_STORAGE);
    frame_queue_v2_clear_all();
    frame_queue_v2_deinit_all();
    sdcard_fatfs_umount();
    
    rtos_stop_timer(&avi_record_timer);
    rtos_deinit_timer(&avi_record_timer);
    rtos_deinit_semaphore(&avi_record_sem);
    avi_record_sem = NULL;
    rtos_delete_thread(NULL);
}

void avi_record_Stress_test(void)
{
    int ret = BK_OK;
    ret = frame_queue_v2_init_all();
    if (ret != BK_OK)
    {
        LOGE("[%s][%d] frame_queue_v2_init_all failed, ret=%d\r\n", __func__, __LINE__, ret);
        return ;
    }
    ret = audio_queue_init_all();
    if (ret != BK_OK)
    {
        LOGE("[%s][%d] audio_queue_init_all failed, ret=%d\r\n", __func__, __LINE__, ret);
        return ;
    }

    ret = dev_dvp_camera_open(480, 480, FPS20);
    if (ret != BK_OK)
    {
        LOGE("[%s][%d] dev_dvp_camera_open failed, ret=%d\r\n", __func__, __LINE__, ret);
        return;
    }

    ret = dev_audio_open(0, 1, 16000, AUDIO_ENC_TYPE_PCM);
    if (ret != BK_OK)
    {
        LOGE("[%s][%d] dev_audio_open failed, ret=%d\r\n", __func__, __LINE__, ret);
        return;
    }

    ret = dev_lcd_open();
    if (ret != BK_OK)
    {
        LOGE("[%s][%d] dev_lcd_open failed, ret=%d\r\n", __func__, __LINE__, ret);
        return;
    }
    dev_lcd_backlight_ctrl(100);

    ret = dev_lcd_display_camera_start();
    if (ret != BK_OK)
    {
        LOGE("[%s][%d] dev_lcd_display_camera_start failed, ret=%d\r\n", __func__, __LINE__, ret);
        return;
    }

    ret = sdcard_fatfs_mount();
    if (ret != BK_OK)
    {
        LOGE("[%s][%d] sdcard_fatfs_mount failed, ret=%d\r\n", __func__, __LINE__, ret);
        return;
    }
    
    ret = rtos_init_semaphore(&avi_record_sem, 1);
    if (ret != BK_OK)
    {
        LOGE("[%s][%d] rtos_init_semaphore failed, ret=%d\r\n", __func__, __LINE__, ret);
        return;
    }

    ret = rtos_create_thread(&avi_record_thread,
                               6,
                               "avi_record_thread",
                               avi_record_Stress_thr_task,
                               4096,
                               NULL);
    if (ret != BK_OK)
    {
        LOGE("[%s][%d] rtos_create_thread failed, ret=%d\r\n", __func__, __LINE__, ret);
        return;
    }
    rtos_set_semaphore(&avi_record_sem);

    ret = rtos_init_timer(&avi_record_timer,
                          RECORD_INTERVAL_MS,
                          avi_record_timer_handler,
                          NULL);
    if (ret != BK_OK)
    {
        LOGE("[%s][%d] rtos_init_timer failed, ret=%d\r\n", __func__, __LINE__, ret);
        return;
    }
    rtos_start_timer(&avi_record_timer);
}


#define CMDS_COUNT  (sizeof(s_avi_record_cmd) / sizeof(struct cli_command))
void cli_avi_record_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    LOGD("%s %d\r\n", __func__, __LINE__);
    if(strcmp(argv[1], "start") == 0)
    {
        example_avi_record_start(argv[2]);
    }
    if(strcmp(argv[1], "stop") == 0)
    {
        example_avi_record_stop();
    }
    if(strcmp(argv[1], "start_video_single") == 0)
    {
        example_avi_single_video_record_start(argv[2]);
    }
    if(strcmp(argv[1], "stop_video_single") == 0)
    {
        example_avi_single_video_record_stop();
    }
    if(strcmp(argv[1], "stress_test") == 0)
    {
        avi_record_Stress_test();
    }
}

static const struct cli_command s_avi_record_cmd[] =
{
    {"avi_record", "start/<filename>", cli_avi_record_cmd},
};

int cli_avi_record_init(void)
{
    return cli_register_commands(s_avi_record_cmd, CMDS_COUNT);
}

#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include "cli.h"
#include "media_service.h"

#include "dev_camera.h"
#include "dev_lcd.h"
#include "dev_audio.h"
#include "fatfs_fs.h"
#include "bk_avi_record.h"
#include "video_frame_queue.h"
#include "audio_frame_queue.h"

#include "cli_cmd.h"
#define TAG "ap_main"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

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

static bool _avi_record_finish = false;
static void record_finish_cb(void* user_data)
{
    _avi_record_finish = true;
}

int avi_record_test()
{
    int ret = BK_OK;
    // 注册 MJPEG 消费者
    frame_queue_v2_register_consumer(IMAGE_MJPEG, CONSUMER_STORAGE);
    /*
    * 功能：配置 AVI 录制节点内存池
    * 参数：true  – 一次性预分配 61440 个节点（约 15 分钟 20 fps 视频）
    *       false – 节点用完后动态 malloc（节省PSRAM，但可能碎片）
    * 默认不调用该函数使用动态分配节点
    * 内存开销：61440 × 16 bytes ≈ 960 KB
    * 录制时长：20 fps 视频（20个nodes） + 每 20 ms 一帧音频（50个node） ≈ 70 节点/秒
    *          61440 ÷ 70 ≈ 877 秒 ≈ 14.6 分钟
    */
    bk_AVI_set_node_pool_mode(1, 12280);      // 如需启用节点池模式，可取消此行注释

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
    avi_record_config.record_finish_cb = record_finish_cb;
        
    ret = avi_record_init(&avi_record_config);
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] avi_record_init failed, ret = %d\r\n", __func__, __LINE__, ret);
        return BK_FAIL;
    }

    char path[255];
    snprintf(path, sizeof(path), "test.avi");
    ret = avi_record_start(path);
    if(ret != BK_OK)
    {
        LOGE("[%s][%d] avi_record_start failed, ret = %d\r\n", __func__, __LINE__, ret);
        return BK_FAIL;
    }
    while(1)
    {
        if(_avi_record_finish)
        {
            avi_record_stop();
            avi_record_deinit();
            frame_queue_v2_unregister_consumer(IMAGE_MJPEG, CONSUMER_STORAGE);
            break;
        }
        rtos_delay_milliseconds(100);
    }
    return BK_OK;
}

int main(void)
{
    bk_init();

    media_service_init();

    cli_avi_record_init();
    if(1/*0 == ate_is_enabled()*/)   //非ATE模式才启动应用代码
    {
        int ret = BK_OK;
        ret = frame_queue_v2_init_all();
        if (ret != BK_OK)
        {
            LOGE("[%s][%d] frame_queue_v2_init_all failed, ret=%d\r\n", __func__, __LINE__, ret);
            TEST_FAIL();
            return BK_FAIL;
        }
        ret = audio_queue_init_all();
        if (ret != BK_OK)
        {
            LOGE("[%s][%d] audio_queue_init_all failed, ret=%d\r\n", __func__, __LINE__, ret);
            TEST_FAIL();
            return BK_FAIL;
        }
        // 打开相机
        ret = dev_dvp_camera_open(480, 480, FPS20);
        if(ret != BK_OK)
        {
            LOGE("[%s][%d] test_dvp_camera_turn_on failed, ret = %d\r\n", __func__, __LINE__, ret);
            TEST_FAIL();
            return BK_FAIL;
        }
        // 打开音频
        ret = dev_audio_open(0, 1, 16000, AUDIO_ENC_TYPE_PCM);
        if(ret != BK_OK)
        {
            LOGE("[%s][%d] dev_audio_open failed, ret = %d\r\n", __func__, __LINE__, ret);
            TEST_FAIL();
            return BK_FAIL;
        }
        // 打开LCD
        ret = dev_lcd_open();
        if(ret != BK_OK)
        {
            LOGE("[%s][%d] test_display_turn_on failed, ret = %d\r\n", __func__, __LINE__, ret);
            TEST_FAIL();
            return BK_FAIL;
        }
        dev_lcd_backlight_ctrl(100);
        // 启动相机显示
        ret = dev_lcd_display_camera_start();
        if(ret != BK_OK)
        {
            LOGE("[%s][%d] dev_lcd_display_camera_start failed, ret = %d\r\n", __func__, __LINE__, ret);
            TEST_FAIL();
            return BK_FAIL;
        }
    
        ret = sdcard_fatfs_mount();
        if(ret != BK_OK)
        {
            LOGE("[%s][%d] sdcard_source_mount failed, ret = %d\r\n", __func__, __LINE__, ret);
            TEST_FAIL();
            return BK_FAIL;
        }
        ret = avi_record_test();
        if(ret != BK_OK)
        {
            LOGE("[%s][%d] avi_record_test failed, ret = %d\r\n", __func__, __LINE__, ret);
            TEST_FAIL();
            return BK_FAIL;
        }
        dev_lcd_display_camera_stop();
        dev_lcd_close();
        dev_audio_close();
        dev_dvp_camera_close();
        sdcard_fatfs_umount();
        audio_queue_deinit_all();
        frame_queue_v2_clear_all();
        frame_queue_v2_deinit_all();
        TEST_PASS();
    }
    return 0;
}

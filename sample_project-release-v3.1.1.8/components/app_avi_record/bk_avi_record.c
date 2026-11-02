#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "bk_avi_record.h"

#define TAG "avi-record"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

typedef enum {
    AVI_RECORD_MSG_START = 0,                       // 录制开始消息
    AVI_RECORD_MSG_STOP,                            // 录制停止消息
    AVI_RECORD_MSG_IDLE,                            // 使用节点录制消耗完 或者 录制写数据出问题时退出
    AVI_RECORD_MSG_EXIT,                            // 录制退出消息
} avi_record_msg_e;

typedef struct {
    avi_record_msg_e msg;                           // 录制消息类型
    uint32_t param;                                 // 录制消息参数
} avi_record_msg_t;

typedef struct {
    bool is_inited;                                 // 是否初始化标志
    char record_path[255];                          // 录制存储路径
    bool avi_record_running;                        // 录制运行标志
    avi_t *AVIFile_hdr;                             // AVI 文件头指针
    double num_of_record_fps;                       // 录制帧率
    uint32_t start_time_stamp;                      // 开始时间戳
    uint32_t end_time_stamp;                        // 结束时间戳
    uint32_t total_record_time;                     // 录制总时长

    // video param
    bool video_enable;                                  // 视频录制使能标志
    int video_width;                                    // 视频宽度
    int video_height;                                   // 视频高度
    double video_fps;                                   // 视频帧率
    avi_video_format_e video_format;                    // 视频编码类型
    read_video_frame_cb_t read_video_frame_cb;          // 视频录制回调函数
    complete_video_frame_cb_t complete_video_frame_cb;  // 视频录制回调函数
        
    // audio param
    bool audio_enable;                                  // 音频录制使能标志
    int audio_chan;                                     // 音频通道数
    long audio_sample;                                  // 音频采样率
    int audio_bits;                                     // 音频位深
    avi_audio_format_e audio_format;                    // 音频编码类型
    read_audio_frame_cb_t read_audio_frame_cb;          // 音频录制回调函数
    complete_audio_frame_cb_t complete_audio_frame_cb;  // 音频录制回调函数
    
    record_finish_cb_t record_finish_cb;                // 录制完成回调函数
    void *user_data;                                    // 自定义用户数据

    //OS resource
    beken_thread_t      record_ctrl_thread;             // 录制线程
    beken_queue_t       record_msg_queue;               // 录制消息队列
    beken_mutex_t       record_mutex;                   // 录制互斥锁
    beken_thread_t      video_thread;                   // 视频线程
    beken_thread_t      audio_thread;                   // 音频线程
} avi_record_device_t;

static avi_record_device_t g_record_device ={
    .is_inited = false,
    .start_time_stamp = 0,
    .end_time_stamp = 0,
    .total_record_time = 0,
    .num_of_record_fps = 0,
    .AVIFile_hdr = NULL,
};

int avi_record_msg_send(avi_record_msg_e type, uint32_t param)
{
    int ret = BK_OK;
    avi_record_msg_t msg = {0};
    msg.msg = type;
    msg.param = param;
    if(g_record_device.record_msg_queue == NULL)
    {
        LOGE("[%s][%d] Record msg queue is not initialized\r\n", __func__, __LINE__);
        return -1;
    }
    else
    {
        ret = rtos_push_to_queue(&g_record_device.record_msg_queue, &msg, BEKEN_NO_WAIT);
        if (ret != BK_OK)
        {
            LOGE("[%s][%d] Failed to send message to queue, ret: %d\r\n", __func__, __LINE__, ret);
        }
    }
    return ret;
}

static int avi_record_update_video_param(uint32_t width, uint32_t height, double fps)
{
    if (g_record_device.is_inited == false)
    {
        LOGI("avi_record_update_video_param fail, record_device is NULL\n");
        return BK_FAIL;
    }
    if (g_record_device.video_enable == false)
    {
        return BK_FAIL;
    }
    g_record_device.video_width = width;
    g_record_device.video_height = height;
    g_record_device.video_fps = fps;
    if (g_record_device.video_format == AVI_VIDEO_FORMAT_MJPEG)
    {
        bk_AVI_set_video(g_record_device.AVIFile_hdr, g_record_device.video_width, g_record_device.video_height, g_record_device.video_fps, "MJPG");
    }
    return 0;
}

static void avi_record_close_process()
{
    if(g_record_device.is_inited)
    {
        g_record_device.end_time_stamp = rtos_get_time();
        g_record_device.total_record_time = g_record_device.end_time_stamp - g_record_device.start_time_stamp;
        if (g_record_device.total_record_time > 0 && g_record_device.AVIFile_hdr)
        {
            g_record_device.num_of_record_fps = g_record_device.num_of_record_fps / (g_record_device.total_record_time / 1000.0);
            LOGD("total_record_time = %d ms \n num_of_record_fps = %lf\n", g_record_device.total_record_time, g_record_device.num_of_record_fps);
            avi_record_update_video_param(g_record_device.video_width, g_record_device.video_height, g_record_device.num_of_record_fps);
            bk_AVI_close(g_record_device.AVIFile_hdr);
            g_record_device.AVIFile_hdr = NULL;

            g_record_device.start_time_stamp = 0;
            g_record_device.end_time_stamp = 0;
            g_record_device.total_record_time = 0;
            g_record_device.num_of_record_fps = 0;
            os_memset(g_record_device.record_path, 0, sizeof(g_record_device.record_path));
            g_record_device.audio_enable = false;
            g_record_device.video_enable = false;
        }
    }
}

static void avi_video_process_task()
{
    int ret = 0;

    while (g_record_device.avi_record_running)
    {
        if(g_record_device.read_video_frame_cb != NULL)
        {
            frame_buffer_t *video_frame = g_record_device.read_video_frame_cb();
            if(video_frame != NULL)
            {
                rtos_lock_mutex(&g_record_device.record_mutex);
                ret = bk_AVI_write_frame(g_record_device.AVIFile_hdr, (char*) video_frame->frame, (long)video_frame->length);
                if(ret < 0)
                {
                    LOGE("AVI_write_video fail, ret = %d\n", ret);
                    if(g_record_device.complete_video_frame_cb != NULL)
                        g_record_device.complete_video_frame_cb(BK_OK, video_frame);
                    g_record_device.avi_record_running = false;
                    avi_record_msg_send(AVI_RECORD_MSG_IDLE, 0);
                    rtos_unlock_mutex(&g_record_device.record_mutex);
                    break;
                }
                else
                {
                    g_record_device.num_of_record_fps++;
                    if(g_record_device.complete_video_frame_cb != NULL)
                        g_record_device.complete_video_frame_cb(BK_OK, video_frame);
                }
                rtos_unlock_mutex(&g_record_device.record_mutex);
            }
        }
    }
    g_record_device.video_thread = NULL;
    rtos_delete_thread(NULL);
}

static void avi_audio_process_task()
{
    int ret = 0;

    while (g_record_device.avi_record_running)
    {
        if(g_record_device.read_audio_frame_cb != NULL)
        {
            audio_frame_t *audio_frame = g_record_device.read_audio_frame_cb();
            if(audio_frame != NULL)
            {
                rtos_lock_mutex(&g_record_device.record_mutex); 
                ret = bk_AVI_write_audio(g_record_device.AVIFile_hdr, (char*)audio_frame->audio_data, (long)audio_frame->audio_data_length);
                if(ret < 0)
                {
                    if(g_record_device.complete_audio_frame_cb != NULL)
                        g_record_device.complete_audio_frame_cb(BK_OK, audio_frame);
                    LOGE("AVI write audio fail, ret = %d\n", ret);
                    g_record_device.avi_record_running = false;
                    rtos_unlock_mutex(&g_record_device.record_mutex);
                    break;
                }
                else{
                    if(g_record_device.complete_audio_frame_cb != NULL)
                        g_record_device.complete_audio_frame_cb(BK_OK, audio_frame);
                }
                rtos_unlock_mutex(&g_record_device.record_mutex);
            }
        }
    }
    g_record_device.audio_thread = NULL;
    rtos_delete_thread(NULL);
}

static int avi_record_start_handle(char *record_path)
{
    int ret = BK_OK;
    if(g_record_device.avi_record_running)
    {
        LOGI("device is recording now, please call stop before begin next record\n");
        return BK_FAIL;
    }

    if (record_path == NULL)
    {
        LOGI("record_path is NULL\n");
        return BK_FAIL;
    }

    // 打开AVI文件
    if (g_record_device.AVIFile_hdr == NULL)
    {
        os_strncpy(g_record_device.record_path, record_path, sizeof(g_record_device.record_path) - 1);
        g_record_device.record_path[sizeof(g_record_device.record_path) - 1] = '\0';

        LOGI("record_path = %s\n", record_path);
        bk_AVI_open_output_file(&g_record_device.AVIFile_hdr, record_path);

        if (g_record_device.AVIFile_hdr != NULL)
        {
            // video param set
            if(g_record_device.video_enable)
            {
                if (g_record_device.video_format == AVI_VIDEO_FORMAT_MJPEG)
                {
                    bk_AVI_set_video(g_record_device.AVIFile_hdr, g_record_device.video_width, g_record_device.video_height, g_record_device.video_fps, "MJPG");
                }
                else
                {
                    LOGI("no support video type\n");
                    goto err;
                }
                LOGI("AVI_set_video video_format = %d , video_width = %d, video_height = %d, video_fps = %lf\n",
                     g_record_device.video_format, g_record_device.video_width, g_record_device.video_height, g_record_device.video_fps);
            }

            // audio param set
            if(g_record_device.audio_enable)
            {
                if (g_record_device.audio_format == AVI_AUDIO_FORMAT_PCM)
                {
                    bk_AVI_set_audio(g_record_device.AVIFile_hdr, g_record_device.audio_chan, g_record_device.audio_sample, g_record_device.audio_bits, WAVE_FORMAT_PCM);
                }
                else
                {
                    LOGI("no support audio type\n");
                    goto err;
                }
                LOGI("AVI_set_audio success, audio_format = %d , audio_chan = %d, audio_sample = %d, audio_bits = %d\n",
                     g_record_device.audio_format, g_record_device.audio_chan, g_record_device.audio_sample, g_record_device.audio_bits);
            }
        }
        else
        {
            LOGI("[%s:%d]:AVI_open_output_file fail\n", __FUNCTION__, __LINE__);
            goto err;
        }
    }
    else
    {
        LOGI("AVIFile_hdr is not NULL, please stop record first\n");
        goto err;
    }

    // 打开 record 线程
    g_record_device.avi_record_running = true;
    if(g_record_device.video_enable)
    {
        ret = rtos_create_thread(&g_record_device.video_thread,
                                    6,
                                    "video_thread",
                                    avi_video_process_task,
                                    4096,
                                    NULL);
        if (ret != BK_OK)
        {
            LOGI("avi_record_start fail, rtos_create_thread fail\n");
            goto err;
        }
    }

    if(g_record_device.audio_enable)
    {
        ret = rtos_create_thread(&g_record_device.audio_thread,
                                    6,
                                    "audio_thread",
                                    avi_audio_process_task,
                                    4096,
                                    NULL);
        if (ret != BK_OK)
        {
            LOGI("avi_record_start fail, rtos_create_thread fail\n");
            goto err;
        }
    }
    g_record_device.start_time_stamp = rtos_get_time();
    return BK_OK;

err:
    if(g_record_device.AVIFile_hdr != NULL)
    {
        bk_AVI_close(g_record_device.AVIFile_hdr);
        g_record_device.AVIFile_hdr = NULL;
    }
    if (g_record_device.video_thread)
    {
        rtos_delete_thread(&g_record_device.video_thread);
        g_record_device.video_thread = NULL;
    }
    if (g_record_device.audio_thread)
    {
        rtos_delete_thread(&g_record_device.audio_thread);
        g_record_device.audio_thread = NULL;
    }
    g_record_device.avi_record_running = false;
    return BK_FAIL;
}

static int avi_record_stop_handle()
{
    g_record_device.avi_record_running = false;
    while(g_record_device.video_thread || g_record_device.audio_thread)
    {
        rtos_delay_milliseconds(10);
    }
    avi_record_close_process();
    return BK_OK;
}

static void avi_record_ctrl_thread(void *arg)
{
    int ret = BK_OK;
    avi_record_msg_t msg;
    while(1)
    {
        ret = rtos_pop_from_queue(&g_record_device.record_msg_queue, &msg, BEKEN_NEVER_TIMEOUT);
        if(ret == BK_OK)
        {
            LOGD("[%s][%d]avi_record_ctrl_thread, msg.msg = %d\n", __func__, __LINE__, msg.msg);
            switch(msg.msg)
            {
                case AVI_RECORD_MSG_START:
                    ret = avi_record_start_handle((char *)msg.param);
                    break;
                case AVI_RECORD_MSG_STOP:
                    ret = avi_record_stop_handle();
                    break;
                case AVI_RECORD_MSG_IDLE:
                    ret = avi_record_stop_handle();
                    if(g_record_device.record_finish_cb != NULL)
                        g_record_device.record_finish_cb(g_record_device.user_data);
                    break;
                case AVI_RECORD_MSG_EXIT:
                    while(!rtos_is_queue_empty(&g_record_device.record_msg_queue))
                    {
                        rtos_pop_from_queue(&g_record_device.record_msg_queue, &msg, BEKEN_NEVER_TIMEOUT);
                    }
                    goto exit;
                    break;
                default:
                    break;
            }
        }
    }
exit:
    if(g_record_device.record_mutex != NULL)
    {
        rtos_deinit_mutex(&g_record_device.record_mutex);
        g_record_device.record_mutex = NULL;
    }

    if(g_record_device.record_msg_queue != NULL)
    {
        rtos_deinit_queue(&g_record_device.record_msg_queue);
        g_record_device.record_msg_queue = NULL;
    }
    os_memset(&g_record_device, 0, sizeof(g_record_device));
    rtos_delete_thread(NULL);
} 

int avi_record_init(avi_record_config_t *config)
{
    int ret = BK_OK;
    if(g_record_device.is_inited)
    {
        LOGI("record_device already init\n");
        return BK_FAIL;
    }
    if(config == NULL)
    {
        LOGE("[%s][%d] config param is invalid (NULL)\r\n", __func__, __LINE__);
        return BK_FAIL;
    }

    do{
        g_record_device.video_enable = config->video_enable;
        g_record_device.video_width = config->video_width;
        g_record_device.video_height = config->video_height;
        g_record_device.video_fps = config->video_fps;
        g_record_device.video_format = config->video_format;
        g_record_device.read_video_frame_cb = config->read_video_frame_cb;
        g_record_device.complete_video_frame_cb = config->complete_video_frame_cb;
            
        g_record_device.audio_enable = config->audio_enable;
        g_record_device.audio_chan = config->audio_chan;
        g_record_device.audio_sample = config->audio_sample;
        g_record_device.audio_bits = config->audio_bits;
        g_record_device.audio_format = config->audio_format;
        g_record_device.read_audio_frame_cb = config->read_audio_frame_cb;
        g_record_device.complete_audio_frame_cb = config->complete_audio_frame_cb;
        
        g_record_device.record_finish_cb = config->record_finish_cb;

        ret = rtos_init_mutex(&g_record_device.record_mutex);
        if(ret != BK_OK)
        {
            LOGE("avi_record_init fail, rtos_init_mutex failed, ret = %d\n", ret);
            break;
        }
        ret = rtos_init_queue(&g_record_device.record_msg_queue, "record_msg_queue", sizeof(avi_record_msg_t), 10);
        if(ret != BK_OK)
        {
            LOGE("avi_record_init fail, rtos_init_queue failed, ret = %d\n", ret);
            break;
        }
        ret = rtos_create_thread(&g_record_device.record_ctrl_thread, 6, "record_ctrl_thread", avi_record_ctrl_thread, 4096, NULL);
        if(ret != BK_OK)
        {
            LOGE("avi_record_init fail, rtos_create_thread failed, ret = %d\n", ret);
            break;
        }
        g_record_device.is_inited = true;
    }while(0);

    if(ret != BK_OK)
    {
        LOGE("avi_record_init fail, ret = %d\n", ret);
        if(g_record_device.record_mutex != NULL)
        {
            rtos_deinit_mutex(&g_record_device.record_mutex);
            g_record_device.record_mutex = NULL;
        }
        if(g_record_device.record_msg_queue != NULL)
        {
            rtos_deinit_queue(&g_record_device.record_msg_queue);
            g_record_device.record_msg_queue = NULL;
        }
        if(g_record_device.record_ctrl_thread != NULL)
        {
            rtos_delete_thread(&g_record_device.record_ctrl_thread);
            g_record_device.record_ctrl_thread = NULL;
        }
    }

    return BK_OK;
}

int avi_record_deinit()
{
    int ret = BK_OK;
    if(g_record_device.is_inited == false)
    {
        LOGI("record_device not init\n");
        return BK_FAIL;
    }
    ret = avi_record_msg_send(AVI_RECORD_MSG_EXIT, 0);
    return ret;
}

int avi_record_start(char *record_path)
{
    int ret = BK_OK;
    if (g_record_device.is_inited == false)
    {
        LOGI("avi_record_start fail, record_device is NULL\n");
        return BK_FAIL;
    }
    ret = avi_record_msg_send(AVI_RECORD_MSG_START, (uint32_t)record_path);
    return ret;
}

int avi_record_stop()
{
    int ret = BK_OK;
    if (g_record_device.is_inited == false)
    {
        LOGI("avi_record_start fail, record_device is NULL\n");
        return BK_FAIL;
    }
    ret = avi_record_msg_send(AVI_RECORD_MSG_STOP, 0);
    return ret;
}

bool avi_record_get_record_status()
{
    if (g_record_device.is_inited == false)
    {
        return false;
    }
    return g_record_device.avi_record_running;
}
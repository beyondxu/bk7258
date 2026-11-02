#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "bk_avi_player.h"

#define TAG "avi-player"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

typedef enum{
    AVI_PLAYER_MSG_OPEN_FILE = 0,       /*!< 打开播放文件消息>*/
    AVI_PLAYER_MSG_CLOSE_FILE,          /*!< 关闭播放文件消息>*/
    AVI_PLAYER_MSG_IDLE,                /*!< 播放结束消息>*/
    AVI_PLAYER_MSG_START,               /*!< 开始消息>*/
    AVI_PLAYER_MSG_PAUSE,               /*!< 暂停消息>*/
    AVI_PLAYER_MSG_RESUME,              /*!< 恢复消息>*/
    AVI_PLAYER_MSG_STOP,                /*!< 停止消息>*/
    AVI_PLAYER_MSG_SET_POS,             /*!< 设置播放位置消息>*/
    AVI_PLAYER_MSG_SET_LOOP,            /*!< 设置循环播放消息>*/
    AVI_PLAYER_MSG_EXIT,                /*!< 关闭播放器>*/
} avi_player_msg_e;

typedef struct{
    avi_player_msg_e msg;                               /*!< 消息类型>*/
    uint32_t param;                                    /*!< 参数>*/
} avi_player_msg_t;

typedef struct
{
    bool is_init;                                       /*!< 是否初始化>*/

    jpeg_hw_decode_config_t jpeg_hw_decode_config;      /*!< JPEG硬件解码配置>*/
    avi_player_dec_format_e decode_format;              /*!< 解码输出的编码格式 */
    avi_player_lcd_cb_t lcd_cb;                         /*!< LCD显示回调函数>*/

    avi_player_voice_malloc_cb_t voice_malloc_cb;       /*!< 音频数据内存分配回调函数>*/
    avi_player_voice_free_cb_t voice_free_cb;           /*!< 音频数据释放回调函数>*/
    avi_player_speaker_cb_t speaker_cb;                 /*!< 音频播放回调函数>*/

    avi_player_finish_cb_t finish_cb;                   /*!< 播放完成回调函数 */

    uint32_t sync_time_threshold_ms;                    /*!< 视频同步线程阈值（单位：ms）>*/
    uint32_t audio_frame_size;                          /*!< 音频帧大小（单位：字节）>*/
    void *user_data;                                    /*!< 用户自定义数据>*/

    avi_t *avi;                                         /*!< avi文件句柄>*/
    avi_video_info_t video_info;                        /*!< AVI视频信息>*/
    avi_audio_info_t audio_info;                        /*!< AVI音频信息>*/ 
    bool is_loop_play;                                  /*!< 是否循环播放>*/
    bool is_play_pause;                                 /*!< 是否暂停播放>*/

    beken_thread_t player_ctrl_thread;                  /*!< 播放线程>*/
    beken_queue_t  player_msg_queue;                    /*!< 播放消息队列>*/
    beken_mutex_t  play_mutex;                          /*!< 播放锁>*/
    beken_timer_t  audio_timer;                         /*!< 音频定时器>*/
    beken_timer_t  video_timer;                         /*!< 视频定时器>*/
} avi_player_device_t;

static avi_player_device_t  g_avi_player = {
    .is_init = false,
    .sync_time_threshold_ms = 60,
    .audio_frame_size = 640,
};

static int avi_player_send_msg(avi_player_msg_e type, uint32_t param)
{
    int ret = BK_OK;
    avi_player_msg_t msg = {0};

    msg.msg = type;
    msg.param = param;
    if(g_avi_player.player_msg_queue == NULL)
    {
        LOGE("[%s][%d] Player msg queue is not initialized\r\n", __func__, __LINE__);
        return -1;
    }
    else
    {
        ret = rtos_push_to_queue(&g_avi_player.player_msg_queue, &msg, BEKEN_NO_WAIT);
        if (ret != BK_OK)
        {
            LOGE("[%s][%d] Failed to send message to queue, ret: %d\r\n", __func__, __LINE__, ret);
        }
    }
    return ret;
}

static int avi_decode_jpeg(frame_buffer_t *jpeg_buffer, frame_buffer_t **decode_buffer)
{
    int ret = -1;
    do {
        // 检查参数
        if (jpeg_buffer == NULL) 
        {
            LOGE("[%s][%d] Invalid parameters for JPEG decoding\r\n", __func__, __LINE__);
            ret = -1;
            break;
        }
        
        // 根据解码格式选择不同的解码函数
        switch (g_avi_player.decode_format)
        {
            case AVI_PLAYER_DECODE_FORMAT_YUV422:
                ret = jpeg_hw_decode_from_buffer_to_yuv422(&g_avi_player.jpeg_hw_decode_config, jpeg_buffer, decode_buffer);
                break;

            case AVI_PLAYER_DECODE_FORMAT_RGB565_LB:
                ret = jpeg_hw_decode_from_buffer_to_rgb565(&g_avi_player.jpeg_hw_decode_config, jpeg_buffer, decode_buffer);
                break;

            case AVI_PLAYER_DECODE_FORMAT_RGB565_BIG:
                ret = jpeg_hw_decode_from_buffer_to_rgb565(&g_avi_player.jpeg_hw_decode_config, jpeg_buffer, decode_buffer);
                break;
            
            default:
                LOGE("[%s][%d] Unsupported decode format: %d\r\n", __func__, __LINE__, g_avi_player.decode_format);
                ret = -1;
                break;
        }
    } while (0);
    
    return ret; 
}

static void audio_timer_cb()
{
    if(!g_avi_player.audio_info.audio_is_end)
    {
        // 播放暂停
        if(g_avi_player.is_play_pause)
        {
            return;
        }

        // 文件读取结束
        if (g_avi_player.audio_info.audio_cur_pos >=  g_avi_player.audio_info.audio_total_bytes)
        {
            LOGI("[%s][%d] audio play end\r\n", __func__, __LINE__);
            // 非循环播放则置播放结束
            if(!g_avi_player.is_loop_play)
            {
                g_avi_player.audio_info.audio_is_end = true;
            }
            g_avi_player.audio_info.audio_cur_pos = 0;
            return;
        }

        // 音视频同步操作
        if(g_avi_player.audio_info.audio_enable && g_avi_player.video_info.video_enable)
        {
            if(g_avi_player.audio_info.audio_cur_pos != 0 && g_avi_player.video_info.video_cur_pos != 0)
            {
                uint32_t audio_time = 0; // 音频当前位置（单位：ms）
                uint32_t video_time = 0; // 视频当前位置（单位：ms）
                uint32_t Pre_frame_size = (g_avi_player.avi->a_bits * g_avi_player.avi->a_chans * g_avi_player.avi->a_rate) / 8 /1000 * g_avi_player.audio_info.frame_time;         //32000
                // 音频当前位置/预帧大小*帧时间=音频当前时间（单位：ms）
                audio_time = g_avi_player.audio_info.audio_cur_pos / Pre_frame_size * g_avi_player.audio_info.frame_time ;
                // 视频当前位置*（1000/帧率）=视频当前时间（单位：ms）
                video_time = g_avi_player.video_info.video_cur_pos * (1000 / g_avi_player.avi->fps);
                // 视频当前时间大于音频当前时间+同步时间阈值时，视频快于音频，需要跳过音频数据
                if (video_time > audio_time + g_avi_player.sync_time_threshold_ms)
                {
                    // 视频快于音频，需要跳过音频数据 = 差距时间/帧时间*帧大小
                    uint32_t bytes_to_skip = (video_time - audio_time) / g_avi_player.audio_info.frame_time * Pre_frame_size;
                    // 跳过音频数据后，音频当前位置不能超过音频总字节数
                    if(g_avi_player.audio_info.audio_cur_pos + bytes_to_skip < g_avi_player.audio_info.audio_total_bytes)
                    {    
                        g_avi_player.audio_info.audio_cur_pos += bytes_to_skip;
                        LOGD("video_time %d audio_time %d \n", video_time, audio_time);
                        LOGD("video fast,skip bytes %d \n", bytes_to_skip);
                    }
                }
            }
        }

        // 读写音频帧位置
        rtos_lock_mutex(&g_avi_player.play_mutex);
        bk_AVI_set_audio_position(g_avi_player.avi, g_avi_player.audio_info.audio_cur_pos);
        rtos_unlock_mutex(&g_avi_player.play_mutex);

        uint8_t *audio_frame_temp = NULL;
        uint32_t readlen = 0;
        if(g_avi_player.voice_malloc_cb == NULL || g_avi_player.voice_free_cb == NULL)
        {
            LOGE("[%s][%d] voice_malloc_cb or voice_free_cb is NULL \n", __func__, __LINE__);
            return;
        }

        audio_frame_temp = g_avi_player.voice_malloc_cb(sizeof(uint8_t) * g_avi_player.audio_info.audio_len);
        if(audio_frame_temp == NULL)
        {
            LOGE("[%s][%d]audio_frame_temp malloc failed \n", __func__, __LINE__);
            return;
        }
  
        // 读取音频帧数据
        rtos_lock_mutex(&g_avi_player.play_mutex);
        readlen = bk_AVI_read_audio(g_avi_player.avi, (char*)audio_frame_temp, g_avi_player.audio_info.audio_len);
        rtos_unlock_mutex(&g_avi_player.play_mutex);

        if(g_avi_player.speaker_cb)
        {
            g_avi_player.speaker_cb(audio_frame_temp, readlen, g_avi_player.user_data);
            g_avi_player.audio_info.audio_cur_pos += readlen;
        }
        else
        {
            LOGE("[%s][%d] speaker_cb is NULL \n", __func__, __LINE__);
            g_avi_player.voice_free_cb(audio_frame_temp);
            audio_frame_temp = NULL;
        }
    }
}

static void video_timer_cb()
{
    if(!g_avi_player.video_info.video_is_end)
    {
        // 播放暂停
        if(g_avi_player.is_play_pause)
        {
            return;
        }
        // 文件读取结束
        if (g_avi_player.video_info.video_cur_pos >= g_avi_player.video_info.video_frame_num)
        {
            LOGI("[%s][%d]video play end!\r\n", __func__, __LINE__);
            // 非循环播放
            if(!g_avi_player.is_loop_play)
            {
                g_avi_player.video_info.video_is_end = true;
                avi_player_send_msg(AVI_PLAYER_MSG_IDLE, 0);
            }
            g_avi_player.video_info.video_cur_pos = 0;
            return;
        }

        // 音视频同步操作
        if(g_avi_player.audio_info.audio_enable && g_avi_player.video_info.video_enable)
        {
            if(g_avi_player.audio_info.audio_cur_pos != 0 && g_avi_player.video_info.video_cur_pos != 0)
            {
                uint32_t audio_time = 0; // 音频当前位置（单位：ms）
                uint32_t video_time = 0; // 视频当前位置（单位：ms）
                // 音频预帧大小 = 音频位深*音频通道数*音频采样率/8/1000*音频帧时间（单位：ms）
                uint32_t Pre_frame_size = (g_avi_player.avi->a_bits * g_avi_player.avi->a_chans * g_avi_player.avi->a_rate) /8/1000 *g_avi_player.audio_info.frame_time;
                // 音频当前位置/帧大小*帧时间=音频当前时间（单位：ms）
                audio_time = g_avi_player.audio_info.audio_cur_pos  / Pre_frame_size * g_avi_player.audio_info.frame_time ;
                // 视频当前位置*（1000/帧率）=视频当前时间（单位：ms）
                video_time = g_avi_player.video_info.video_cur_pos * (1000 / g_avi_player.avi->fps);
                // 视频当前时间大于音频当前时间+同步时间阈值时，视频快于音频，需要跳过视频数据
                if (audio_time > video_time + g_avi_player.sync_time_threshold_ms) // 同步阈值（如60ms）
                {
                    // 视频快于音频，需要跳过视频数据 = 差距时间/（1000/帧率）=跳过的视频帧数
                    uint32_t frames_to_skip = (audio_time - video_time) / (1000 / g_avi_player.avi->fps);
                    if(g_avi_player.video_info.video_cur_pos + frames_to_skip < g_avi_player.video_info.video_frame_num)
                    {
                        g_avi_player.video_info.video_cur_pos += frames_to_skip;
                        LOGD("video_time %d audio_time %d \n", video_time, audio_time);
                        LOGD("audio fast,skip frame %d \n", frames_to_skip);
                    }
                }
            }
        }
        // 读写视频帧位置
        rtos_lock_mutex(&g_avi_player.play_mutex);
        bk_AVI_set_video_position(g_avi_player.avi, g_avi_player.video_info.video_cur_pos,&g_avi_player.video_info.video_len);
        if (g_avi_player.video_info.video_len == 0)
        {
            g_avi_player.video_info.video_cur_pos = g_avi_player.video_info.video_cur_pos + 1;
            bk_AVI_set_video_position(g_avi_player.avi, g_avi_player.video_info.video_cur_pos, &g_avi_player.video_info.video_len);
        }
        rtos_unlock_mutex(&g_avi_player.play_mutex);

        if (g_avi_player.jpeg_hw_decode_config.jpeg_frame_malloc == NULL || g_avi_player.jpeg_hw_decode_config.jpeg_frame_free == NULL)
        {
            LOGE("[%s][%d] jpeg_frame_malloc or jpeg_frame_free is NULL \n", __func__, __LINE__);
            return;
        }
        if (g_avi_player.jpeg_hw_decode_config.decode_frame_malloc == NULL || g_avi_player.jpeg_hw_decode_config.decode_frame_free == NULL)
        {
            LOGE("[%s][%d] decode_frame_malloc or decode_frame_free is NULL \n", __func__, __LINE__);
            return;
        }

        frame_buffer_t * decode_frame = NULL;
        frame_buffer_t * display_frame = NULL;
      
        decode_frame = g_avi_player.jpeg_hw_decode_config.jpeg_frame_malloc(g_avi_player.video_info.video_len);
        decode_frame->length = g_avi_player.video_info.video_len;
        if(decode_frame == NULL)
        {
            LOGE("[%s][%d]decode_frame malloc failed \n", __func__, __LINE__);
            return;
        }

        //读取avi jpeg数据
        rtos_lock_mutex(&g_avi_player.play_mutex);
        bk_AVI_read_frame(g_avi_player.avi, (char *)decode_frame->frame, g_avi_player.video_info.video_len);
        rtos_unlock_mutex(&g_avi_player.play_mutex);
        
        int ret = avi_decode_jpeg(decode_frame, &display_frame);
        if(ret == BK_OK)
        {
            if(g_avi_player.lcd_cb)
            {
                g_avi_player.lcd_cb(display_frame, g_avi_player.user_data);
            }
            else
            {
                LOGE("[%s][%d] lcd_cb is NULL \n", __func__, __LINE__);
                g_avi_player.jpeg_hw_decode_config.decode_frame_free(display_frame);
                display_frame = NULL;
            }
        }
        g_avi_player.video_info.video_cur_pos += 1;
        if(g_avi_player.jpeg_hw_decode_config.jpeg_frame_free)
        {
            g_avi_player.jpeg_hw_decode_config.jpeg_frame_free(decode_frame);
        }
    }
}

static int avi_player_open_file_handle(const char *file_path)
{
    if(g_avi_player.avi != NULL)
    {
        LOGE("[%s][%d] An AVI file has already been opened, make sure last one is closed\r\n", __func__, __LINE__);
        return BK_FAIL;
    }
    if(file_path == NULL)
    {
        LOGE("[%s][%d] file_path is NULL \n", __func__, __LINE__);
        return BK_FAIL;
    }

    LOGI("file_path: %s\r\n", file_path);
    g_avi_player.avi = bk_AVI_open_input_file(file_path, 1);
    if (g_avi_player.avi != NULL)
    {
        g_avi_player.video_info.video_frame_num = bk_AVI_video_frames(g_avi_player.avi);
        // 检查是否有视频流
        if(g_avi_player.video_info.video_frame_num > 0)
        {
            g_avi_player.video_info.video_enable = true;
            g_avi_player.video_info.width = g_avi_player.avi->width;
            g_avi_player.video_info.height = g_avi_player.avi->height;
            g_avi_player.video_info.fps = g_avi_player.avi->fps;
            os_strcpy(g_avi_player.video_info.compresstype, g_avi_player.avi->compressor);
            g_avi_player.video_info.video_cur_pos = 0;

            LOGI("video_frame_num: %d\r\n", g_avi_player.video_info.video_frame_num);
            LOGI("v_width: %d, v_height: %d, fps: %f\r\n", g_avi_player.video_info.width, g_avi_player.video_info.height,g_avi_player.video_info.fps);
            LOGI("compresstype: %s\r\n", g_avi_player.video_info.compresstype);
        }
        // 检查是否有音频流
        if(g_avi_player.avi->a_rate > 0)
        {
            g_avi_player.audio_info.audio_enable = true;
            g_avi_player.audio_info.audio_total_bytes = g_avi_player.avi->audio_bytes;
            g_avi_player.audio_info.sample_rate = g_avi_player.avi->a_rate;
            g_avi_player.audio_info.chans = g_avi_player.avi->a_chans;
            g_avi_player.audio_info.bits = g_avi_player.avi->a_bits;
            g_avi_player.audio_info.audio_fmt = g_avi_player.avi->a_fmt;
            g_avi_player.audio_info.audio_cur_pos = 0;
            {
                g_avi_player.audio_info.audio_len = g_avi_player.audio_frame_size;
                g_avi_player.audio_info.frame_time = g_avi_player.audio_frame_size /(g_avi_player.avi->a_bits * g_avi_player.avi->a_chans * g_avi_player.avi->a_rate /8 / 1000);
                LOGI("frame_time: %ld\r\n", g_avi_player.audio_info.frame_time);
            }
            LOGI("audio_total_bytes: %ld\r\n", g_avi_player.audio_info.audio_total_bytes);
            LOGI("bit_depth: %ld, chans: %ld, rate: %ld, \r\n", g_avi_player.audio_info.bits, g_avi_player.audio_info.chans, g_avi_player.audio_info.sample_rate);
        }
        g_avi_player.is_play_pause = false;
    }
    else
    {
        LOGE("[%s][%d] open avi_file %s fail\r\n", __func__, __LINE__, file_path);
        return BK_FAIL;
    }

    LOGD("[%s][%d] open file success\r\n", __func__, __LINE__);
    return BK_OK;
}

static int avi_player_close_file_handle(void)
{
    int ret = BK_OK;

    if (g_avi_player.avi)
    {
        ret = bk_AVI_close(g_avi_player.avi);
        if (ret != BK_OK)
        {
            LOGE("[%s][%d] AVI_close failed\r\n", __func__, __LINE__);
            return BK_FAIL;
        }
        g_avi_player.avi = NULL;
    }
    LOGD("[%s][%d] close file success\r\n", __func__, __LINE__);
    return BK_OK;
}

static int avi_player_start_handle(void)
{
    if(g_avi_player.avi == NULL)
    {
        LOGE("[%s][%d] avi_player->avi is NULL\r\n", __func__, __LINE__);
        return BK_FAIL;
    }

    int fps = (int )g_avi_player.avi->fps;
    if(g_avi_player.video_info.video_enable)
    {
        g_avi_player.video_info.video_is_end = false;
        rtos_init_timer(&g_avi_player.video_timer, (1000 / fps), video_timer_cb, NULL);
        rtos_start_timer(&g_avi_player.video_timer);
    }
    if(g_avi_player.audio_info.audio_enable)
    {
        g_avi_player.audio_info.audio_is_end = false;
        rtos_init_timer(&g_avi_player.audio_timer, g_avi_player.audio_info.frame_time, audio_timer_cb, NULL);
        rtos_start_timer(&g_avi_player.audio_timer);
    }
    return BK_OK;
}

static int avi_player_stop_handle(void)
{
    g_avi_player.is_play_pause = true;
    rtos_delay_milliseconds(100);
    if(g_avi_player.video_info.video_enable)
    {
        rtos_stop_timer(&g_avi_player.video_timer);
        rtos_deinit_timer(&g_avi_player.video_timer);
        g_avi_player.video_info.video_enable = false;
    }
    if(g_avi_player.audio_info.audio_enable)
    {
        rtos_stop_timer(&g_avi_player.audio_timer); 
        rtos_deinit_timer(&g_avi_player.audio_timer);
        g_avi_player.audio_info.audio_enable = false;
    }
    return BK_OK;
}

static int avi_player_pause_handle(void)
{
    g_avi_player.is_play_pause = true;
    return BK_OK;
}

static int avi_player_resume_handle(void)
{
    g_avi_player.is_play_pause = false;
    if(g_avi_player.video_info.video_is_end == true)
        g_avi_player.video_info.video_is_end = false;
    if(g_avi_player.audio_info.audio_is_end == true)
        g_avi_player.audio_info.audio_is_end = false;
    return BK_OK;
}

static int avi_player_set_pos_handle(uint32_t pos)
{
    LOGI("[%s][%d] set pos to %d(s)\r\n", __func__, __LINE__, pos);
    // 设置时间 单位为s，计算视频帧位置
    uint32_t video_set_pos , audio_set_pos;

    video_set_pos= pos * g_avi_player.video_info.fps ;
    if(video_set_pos >= g_avi_player.video_info.video_frame_num || video_set_pos < 0)
    {
        LOGE("[%s][%d] Invalid position: %d\r\n", __func__, __LINE__, pos);
        return BK_FAIL;
    }

    // 如果文件有音频的话，同步设置音频位置
    if(g_avi_player.audio_info.audio_enable)
    {
        uint32_t Pre_frame_size = (g_avi_player.avi->a_bits * g_avi_player.avi->a_chans * g_avi_player.avi->a_rate) / 8 /1000 * g_avi_player.audio_info.frame_time; 
        audio_set_pos = pos * 1000 / g_avi_player.audio_info.frame_time * Pre_frame_size;
    }
    if(g_avi_player.video_info.video_cur_pos != video_set_pos)
    {
        g_avi_player.is_play_pause = true;
        g_avi_player.video_info.video_cur_pos = video_set_pos;
        if(g_avi_player.audio_info.audio_enable)
        {
            g_avi_player.audio_info.audio_cur_pos = audio_set_pos;
        }
        g_avi_player.is_play_pause = false;
    }

    return BK_OK;
}

static int avi_player_set_loop_handle(bool is_loop)
{
    g_avi_player.is_loop_play = is_loop;
    return BK_OK;
}

static void avi_player_ctrl_thread(void *arg)
{
    int ret = BK_OK;
    avi_player_msg_t msg ={0};
    while(1)
    {
        ret = rtos_pop_from_queue(&g_avi_player.player_msg_queue, &msg, BEKEN_WAIT_FOREVER);
        if(ret == BK_OK)
        {  
            LOGD("[%s][%d] msg type%d \n", __func__, __LINE__, msg.msg);
            switch(msg.msg)
            {
                case AVI_PLAYER_MSG_OPEN_FILE:
                    ret = avi_player_open_file_handle((const char *)msg.param);
                    if(ret!=BK_OK)
                    {
                        LOGE("[%s][%d] open file failed \n", __func__, __LINE__);
                    }
                    break;
                case AVI_PLAYER_MSG_CLOSE_FILE:
                    ret = avi_player_close_file_handle();
                    if(ret!=BK_OK)
                    {
                        LOGE("[%s][%d] close file failed \n", __func__, __LINE__);
                    }
                    break;
                case AVI_PLAYER_MSG_IDLE:
                    if(g_avi_player.finish_cb)
                    {
                        g_avi_player.finish_cb(g_avi_player.user_data);
                    }
                    break;
                case AVI_PLAYER_MSG_START:
                    ret = avi_player_start_handle();
                    if(ret!=BK_OK)
                    {
                        LOGE("[%s][%d] play start failed \n", __func__, __LINE__);
                    }
                    break;
                case AVI_PLAYER_MSG_PAUSE:
                    ret = avi_player_pause_handle();
                    if(ret!=BK_OK)
                    {
                        LOGE("[%s][%d] play pause failed \n", __func__, __LINE__);
                    }
                    break;
                case AVI_PLAYER_MSG_RESUME:
                    ret = avi_player_resume_handle();
                    if(ret!=BK_OK)
                    {
                        LOGE("[%s][%d] play resume failed \n", __func__, __LINE__); 
                    }
                    break;
                case AVI_PLAYER_MSG_STOP:
                    ret = avi_player_stop_handle();
                    if(ret!=BK_OK)
                    {
                        LOGE("[%s][%d] play stop failed \n", __func__, __LINE__);
                    }
                    ret = avi_player_close_file_handle();
                    if(ret!=BK_OK)
                    {
                        LOGE("[%s][%d] close file failed \n", __func__, __LINE__);
                    }
                    break;
                case AVI_PLAYER_MSG_SET_POS:
                    ret = avi_player_set_pos_handle(msg.param);
                    if(ret!=BK_OK)
                    {
                        LOGE("[%s][%d] set pos failed \n", __func__, __LINE__);
                    }
                    break;
                case AVI_PLAYER_MSG_SET_LOOP:
                    ret = avi_player_set_loop_handle(msg.param);
                    if(ret!=BK_OK)
                    {
                        LOGE("[%s][%d] set loop failed \n", __func__, __LINE__);
                    }
                    break;

                case AVI_PLAYER_MSG_EXIT:
                    while(!rtos_is_queue_empty(&g_avi_player.player_msg_queue))
                    {
                        rtos_pop_from_queue(&g_avi_player.player_msg_queue, &msg, BEKEN_NO_WAIT);
                    }
                    goto exit;
                    break;
                default:
                    break;
            }
        }
    }

exit:
    LOGD("[%s][%d] exit thread \n", __func__, __LINE__);
    if(g_avi_player.player_msg_queue != NULL)
    {
        rtos_deinit_queue(&g_avi_player.player_msg_queue);
        g_avi_player.player_msg_queue = NULL;
    }
    if(g_avi_player.play_mutex)
    {
        rtos_deinit_mutex(&g_avi_player.play_mutex);
        g_avi_player.play_mutex = NULL;
    }
    os_memset(&g_avi_player, 0, sizeof(g_avi_player));
    rtos_delete_thread(NULL);
}

int avi_player_init(avi_player_config_t *config)
{
    int ret = BK_OK;
    if(g_avi_player.is_init)
    {
        LOGE("[%s][%d]AVI player already init\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if(config == NULL)
    {
        LOGE("[%s][%d] config param is invalid (NULL)\r\n", __func__, __LINE__);
        return BK_FAIL;
    }

    do{
        os_memcpy(&g_avi_player.jpeg_hw_decode_config, &config->jpeg_hw_decode_config, sizeof(jpeg_hw_decode_config_t));
        g_avi_player.decode_format = config->decode_format;
        g_avi_player.lcd_cb = config->lcd_cb;
        if(config->sync_time_threshold_ms != 0)
        {
            g_avi_player.sync_time_threshold_ms = config->sync_time_threshold_ms;
        }

        g_avi_player.voice_malloc_cb = config->voice_malloc_cb;
        g_avi_player.voice_free_cb = config->voice_free_cb;
        g_avi_player.speaker_cb = config->speaker_cb;
        if(config->audio_frame_size != 0)
        {
            g_avi_player.audio_frame_size = config->audio_frame_size;
        }
        g_avi_player.finish_cb = config->finish_cb;

        g_avi_player.user_data = config->user_data;

        ret = rtos_init_mutex(&g_avi_player.play_mutex);
        if(ret != BK_OK)
        {
            LOGE("[%s][%d] play mutex init fail\r\n", __func__, __LINE__);
            break;
        }
        ret = rtos_init_queue(&g_avi_player.player_msg_queue, "avi_player_msg_queue", sizeof(avi_player_msg_t), 10);
        if(ret != BK_OK)
        {
            LOGE("[%s][%d] player msg queue init fail\r\n", __func__, __LINE__);
            break;
        }
        ret = rtos_create_thread(&g_avi_player.player_ctrl_thread, 6, "avi_player_ctrl", avi_player_ctrl_thread, 4096, NULL);
        if(ret != BK_OK)
        {
            LOGE("[%s][%d] player ctrl thread init fail\r\n", __func__, __LINE__);
            break;
        }
        g_avi_player.is_init = true;
    }while(0);

    if(ret != BK_OK)
    {
        if(g_avi_player.play_mutex != NULL)
        {
            rtos_deinit_mutex(&g_avi_player.play_mutex);
            g_avi_player.play_mutex = NULL;
        }
        if(g_avi_player.player_msg_queue != NULL)
        {
            rtos_deinit_queue(&g_avi_player.player_msg_queue);
            g_avi_player.player_msg_queue = NULL;
        }
        if(g_avi_player.player_ctrl_thread != NULL)
        {
            rtos_delete_thread(&g_avi_player.player_ctrl_thread);
            g_avi_player.player_ctrl_thread = NULL;
        }
    }
    return BK_OK;
}

int avi_player_deinit()
{
    int ret = BK_OK;
    if (!g_avi_player.is_init)
    {
        LOGE("[%s][%d] AVI player not init\r\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ret = avi_player_send_msg(AVI_PLAYER_MSG_EXIT, 0);
    return ret;
}

int avi_player_file_open(const char *file_path)
{
    int ret = BK_OK;
    if (!g_avi_player.is_init)
    {
        LOGE("[%s][%d] AVI player not init\r\n", __func__, __LINE__);
        return BK_FAIL;
    }
    if(file_path == NULL)
    {
        LOGE("[%s][%d] file_path is NULL \n", __func__, __LINE__);
        return BK_FAIL;
    }
    ret = avi_player_send_msg(AVI_PLAYER_MSG_OPEN_FILE, (uint32_t)file_path);
    return ret;
}

int avi_player_file_close(void)
{
    int ret = BK_OK;
    if (!g_avi_player.is_init)
    {
        LOGE("[%s][%d] AVI player not init\r\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ret = avi_player_send_msg(AVI_PLAYER_MSG_CLOSE_FILE, 0);
    return ret;
}

int avi_player_start(void)
{
    int ret = BK_OK;
    if (!g_avi_player.is_init)
    {
        LOGE("[%s][%d] AVI player not init\r\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ret = avi_player_send_msg(AVI_PLAYER_MSG_START, 0);
    return ret;
}

int avi_player_stop(void)
{
    int ret = BK_OK;
    if (!g_avi_player.is_init)
    {
        LOGE("[%s][%d] AVI player not init\r\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ret = avi_player_send_msg(AVI_PLAYER_MSG_STOP, 0);
    return ret;
}

int avi_player_pause(void)
{
    int ret = BK_OK;
    if (!g_avi_player.is_init)
    {
        LOGE("[%s][%d] AVI player not init\r\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ret = avi_player_send_msg(AVI_PLAYER_MSG_PAUSE, 0);
    return ret;
}

int avi_player_resume(void)
{
    int ret = BK_OK;
    if (!g_avi_player.is_init)
    {
        LOGE("[%s][%d] AVI player not init\r\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ret = avi_player_send_msg(AVI_PLAYER_MSG_RESUME, 0);
    return ret;
}

int avi_player_set_loop(bool is_loop)
{
    int ret = BK_OK;
    if (!g_avi_player.is_init)
    {
        LOGE("[%s][%d] AVI player not init\r\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ret = avi_player_send_msg(AVI_PLAYER_MSG_SET_LOOP, is_loop);
    return ret;
}

int avi_player_set_pos(uint32_t pos)
{
    int ret = BK_OK;
    if (!g_avi_player.is_init)
    {
        LOGE("[%s][%d] AVI player not init\r\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ret = avi_player_send_msg(AVI_PLAYER_MSG_SET_POS, pos);
    return ret;
}

avi_audio_info_t avi_player_get_audio_info(void)
{
    if (g_avi_player.is_init == false)
    {
        LOGE("[%s][%d] avi_player is not init\r\n", __func__, __LINE__);
        return (avi_audio_info_t){0};
    }
    return g_avi_player.audio_info;
}

avi_video_info_t avi_player_get_video_info(void)
{
    if (g_avi_player.is_init == false)
    {
        LOGE("[%s][%d] avi_player is not init\r\n", __func__, __LINE__);    
        return (avi_video_info_t){0};
    }
    return g_avi_player.video_info;
}


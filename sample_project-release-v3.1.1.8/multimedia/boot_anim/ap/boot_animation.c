#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"
#include "frame_buffer.h"

#include "dev_lcd.h"
#include "common.h"
#include "jpeg_file_animation.h"
#include "frame_buffer_app.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_types_hw.h"

#define TAG "anim"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

/**
 * @brief 给解码线程调用的callback，用于解码完后显示lcd
 * @param display_frame 用来lcd的显示的frame
 * @return 无
 */
static void _app_lcd_display(frame_buffer_t *display_frame, void *user_data)
{
    dev_lcd_flush(display_frame, display_free_callback);
}

static bk_err_t _jpeg_decode_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame)
{
    if (result == BK_OK) 
    {
        LOGD("[%s][%d] jpeg decode success! format_type: %d, out_frame: %p\n", __func__, __LINE__, format_type, out_frame);
    } 
    else 
    {
        LOGE("[%s][%d] jpeg decode failed! format_type: %d, result: %d, out_frame: %p\n", __func__, __LINE__, format_type, result, out_frame);
    }

    return BK_OK;
}

static bk_jpeg_decode_hw_handle_t g_jpeg_decode_handle = NULL;
static bk_jpeg_decode_hw_config_t g_jpeg_decode_config = {
    .decode_cbs = {.out_complete = _jpeg_decode_complete,}
};
static bool g_play_over = false;

//硬解初始化
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

//硬解反初始化
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

static void _animation_finish_cb(void *user_data)
{
    g_play_over = true;
}

/**
 * @brief 启动开机动画播放
 * @return BK_OK成功，其它失败
 */
s32 boot_animation(void)
{
    bk_err_t ret = BK_FAIL;
    animation_module_config_t animation_config = {0};

    do{
        //jpeg 硬解初始化
        ret = _jpeg_hw_decode_init();
        if(BK_OK != ret)
        {
            LOGE("[%s][%d] decode init fail\r\n", __FUNCTION__, __LINE__);
            break;
        }

        //动画图片解码器配置参数
        animation_config.jpeg_hw_decode_config.jpeg_decode_handle = g_jpeg_decode_handle;
        animation_config.jpeg_hw_decode_config.jpeg_frame_malloc = jpeg_frame_malloc;
        animation_config.jpeg_hw_decode_config.jpeg_frame_free = jpeg_frame_free;
        animation_config.jpeg_hw_decode_config.decode_frame_malloc = decode_frame_malloc;
        animation_config.jpeg_hw_decode_config.decode_frame_free = decode_frame_free;
        animation_config.jpeg_hw_decode_config.output_width = 0;
        animation_config.jpeg_hw_decode_config.output_height = 0;
        animation_config.jpeg_hw_decode_config.use_dma2d = false;
        animation_config.jpeg_hw_decode_config.byte_swap = false;
        
        animation_config.decode_format = ANIMATION_DECODE_FORMAT_YUV422;  //rgb屏，BK7258硬件支持输入源是yuv格式数据
        animation_config.lcd_cb = _app_lcd_display;                        //刷屏函数
        animation_config.finish_cb = _animation_finish_cb;                                    //动画完成回调

        // 动画模块初始化
        ret = animation_module_init(&animation_config);
        if(BK_OK != ret)
        {
            LOGE("[%s][%d] init fail\r\n", __FUNCTION__, __LINE__);
            break;
        }

        // 播放动画
        LOGI("[%s][%d] boot animation play start\r\n", __FUNCTION__, __LINE__);
        ret = animation_start_play(VFS_MOUNT_PATH"/anim/anim-", 30, 1, false);
        if(BK_OK != ret)
        {
            LOGE("[%s][%d] boot animation fail\r\n", __FUNCTION__, __LINE__);
            break;
        }

        //等播放完成
        g_play_over = false;
        while(!g_play_over)
        {
            rtos_delay_milliseconds(100);
        }
    }while(0);

    _jpeg_hw_decode_deinit();    //jpeg 硬解反初始化
    animation_module_deinit();   //动画模块反初始化

    LOGI("[%s][%d] boot animation play over\r\n", __FUNCTION__, __LINE__);

    return ret;
}


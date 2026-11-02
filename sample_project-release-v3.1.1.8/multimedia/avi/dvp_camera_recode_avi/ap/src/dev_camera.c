#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include "driver/gpio.h"
#include "gpio_driver.h"
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include "video_frame_queue.h"
#include <components/bk_camera_ctlr.h>

#define TAG "dev_camera"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#ifdef DVP_POWER_PIN
#define DVP_POWER_GPIO_ID DVP_POWER_PIN
#else
#define DVP_POWER_GPIO_ID CONFIG_DVP_CTRL_POWER_GPIO_ID
#endif

static bk_camera_ctlr_handle_t camera_handle;

static frame_buffer_t *dvp_camera_frame_malloc(image_format_t format, uint32_t size)
{
    return frame_queue_v2_malloc(format, size);
}

static void dvp_camera_frame_complete(image_format_t format, frame_buffer_t *frame, int result)
{
    if (result != BK_OK)
    {
        frame_queue_v2_cancel(format, frame);
    }
    else
    {
        frame_queue_v2_complete(format, frame);
    }
}

static const bk_dvp_callback_t dvp_frame_cb =
{
    .malloc = dvp_camera_frame_malloc,
    .complete = dvp_camera_frame_complete,
};

/**
 * @brief 打开 DVP 摄像头
 * @param width 图像宽度
 * @param height 图像高度
 * @param fps 图像帧率
 * @return 成功返回0，失败返回负数
 */
int dev_dvp_camera_open(uint32_t width, uint32_t height, frame_fps_t fps)
{
    int ret = BK_OK;
    bk_dvp_config_t dvp_config = BK_DVP_864X480_30FPS_MJPEG_CONFIG();

    dvp_config.img_format = IMAGE_YUV | IMAGE_MJPEG;
    dvp_config.width = width;
    dvp_config.height = height;
    dvp_config.fps = fps;
    LOGI("[%s][%d] width: %d, height: %d, fps: %d \r\n", __func__, __LINE__, dvp_config.width, dvp_config.height, dvp_config.fps);

    bk_dvp_ctlr_config_t dvp_ctlr_config =
    {
        .config = dvp_config,
        .cbs = &dvp_frame_cb,
    };

    bk_gpio_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_DVP, DVP_POWER_GPIO_ID, GPIO_OUTPUT_STATE_HIGH);

    ret = bk_camera_dvp_ctlr_new((bk_camera_ctlr_handle_t *)&camera_handle, &dvp_ctlr_config);
    if (ret == BK_OK)
    {
        ret = bk_camera_open((bk_camera_ctlr_handle_t)camera_handle);
        if (ret != BK_OK)
        {
            LOGE("[%s][%d] bk_camera_dvp_ctlr_open failed\n", __func__, __LINE__);
            bk_camera_delete((bk_camera_ctlr_handle_t)camera_handle);
            camera_handle = NULL;
            bk_gpio_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_DVP, DVP_POWER_GPIO_ID, GPIO_OUTPUT_STATE_LOW);
        }
    }
    else
    {
        LOGI("[%s][%d] bk_camera_dvp_ctlr_new failed\n", __func__, __LINE__);
    }
    return ret;
}

/**
 * @brief 关闭 DVP 摄像头
 * @return 成功返回0，失败返回负数
 */
int dev_dvp_camera_close(void)
{
    int ret = BK_OK;

    if (camera_handle == NULL)
    {
        LOGE("[%s][%d] camera already close\n", __func__, __LINE__);
        return BK_FAIL;
    }

    ret = bk_camera_close(camera_handle);
    if (ret != BK_OK)
    {
        LOGE("[%s][%d] bk_camera_close failed\n", __func__, __LINE__);
        return ret;
    }
    ret = bk_camera_delete(camera_handle);
    if (ret != BK_OK)
    {
        LOGE("[%s][%d] bk_camera_delete failed\n", __func__, __LINE__);
        return ret;
    }
    camera_handle = NULL;
    bk_gpio_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_DVP, DVP_POWER_GPIO_ID, GPIO_OUTPUT_STATE_LOW);

    return ret;
}
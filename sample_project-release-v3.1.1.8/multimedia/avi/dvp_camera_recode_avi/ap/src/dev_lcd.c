
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include "driver/gpio.h"
#include "gpio_driver.h"
#include <modules/pm.h>
#include <driver/pwr_clk.h>

#include "frame_buffer.h"
#include "components/bk_display.h"
#include "video_frame_queue.h"

#include "dev_lcd.h"
#define TAG "dev_lcd"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#ifdef LCD_LDO_IO
#define LDO_GPIO_ID     LCD_LDO_IO
#else
#define LDO_GPIO_ID     CONFIG_LDO3V3_CTRL_GPIO
#endif

extern const lcd_device_t lcd_device_st7701s;
static bk_display_ctlr_handle_t lcd_display_handle = NULL;
static bool lcd_dev_opened = false;

static beken_thread_t lcd_display_camera_thread = NULL;
static bool display_camera_started = false;

bk_display_rgb_ctlr_config_t rgb_config = {
    .lcd_device = &lcd_device_st7701s,
    .clk_pin = GPIO_0,
    .cs_pin = GPIO_12,
    .sda_pin = GPIO_1,
    .rst_pin = GPIO_6,
};

static avdk_err_t frame_free_cb(void *frame)
{
    frame_buffer_display_free((frame_buffer_t *)frame);
    return AVDK_ERR_OK;
}

static avdk_err_t camera_frame_free_cb(void *frame)
{
    frame_queue_v2_release_frame(IMAGE_YUV, CONSUMER_CUSTOM_1, (frame_buffer_t *)frame);
    return AVDK_ERR_OK;
}

static void lcd_display_camera_task(void *arg)
{
    frame_queue_v2_register_consumer(IMAGE_YUV, CONSUMER_CUSTOM_1);
    while(display_camera_started)
    {
        frame_buffer_t *frame = frame_queue_v2_get_frame(IMAGE_YUV, CONSUMER_CUSTOM_1, 50);
        if(frame)
        {
                int ret = bk_display_flush(lcd_display_handle, (void *)frame, camera_frame_free_cb);
                if(ret != AVDK_ERR_OK)
                {
                    LOGE("bk_display_flush failed!\n");
                    camera_frame_free_cb(frame);
                }
        }
    }
    frame_queue_v2_unregister_consumer(IMAGE_YUV, CONSUMER_CUSTOM_1);
    lcd_display_camera_thread = NULL;
    display_camera_started = false;
    rtos_delete_thread(NULL);
}

/**
 * @brief 打开lcd，注意此接口不会开lcd背光
 * @return 成功返回0，失败返回负数
 */
int dev_lcd_open(void)
{
    int ret = BK_OK;
    if(lcd_dev_opened)
    {
        LOGD("[%s][%d] dev_lcd already opened!\n", __func__, __LINE__);
        return ret;
    }

    do{
        //电源管脚初始化
        gpio_dev_unmap(LCD_LDO_IO);
        BK_LOG_ON_ERR(bk_gpio_enable_output(LCD_LDO_IO));
        BK_LOG_ON_ERR(bk_gpio_pull_up(LCD_LDO_IO));
        BK_LOG_ON_ERR(bk_gpio_set_output_low(LCD_LDO_IO));

        //背光管脚初始化
        gpio_dev_unmap(LCD_BL_IO);
        BK_LOG_ON_ERR(bk_gpio_enable_output(LCD_BL_IO));
        BK_LOG_ON_ERR(bk_gpio_pull_up(LCD_BL_IO));
        BK_LOG_ON_ERR(bk_gpio_set_output_low(LCD_BL_IO));

        bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LDO_GPIO_ID, GPIO_OUTPUT_STATE_HIGH);

        ret = bk_display_rgb_new(&lcd_display_handle, &rgb_config);
        if(ret != BK_OK)
        {
            LOGE("[%s][%d] bk_display_rgb_ctlr_new failed!\n", __func__, __LINE__);
            return ret;
        }

        ret = bk_display_open(lcd_display_handle);
        if(ret != BK_OK)
        {
            LOGE("[%s][%d] bk_display_open failed!\n", __func__, __LINE__);
            return ret;
        }
        lcd_dev_opened = true;
    }while(0);

    return ret;
}

/**
 * @brief 调节lcd背光
 * @param percent: 0关闭， 100：开，其它为背光占空比
 * @return 无
 */
void dev_lcd_backlight_ctrl(int percent)
{
    if (100 == percent)
    {
        LOGI("[%s][%d] on\r\n", __func__, __LINE__);
        bk_gpio_set_output_high(LCD_BL_IO);
    }
    else if (0 == percent)
    {
        LOGI("[%s][%d] off\r\n", __func__, __LINE__);
        bk_gpio_set_output_low(LCD_BL_IO);
    }
    else
    {
        //todo 
    }
}

/**
 * @brief 刷屏
 * @param frame: 显示frame
 * @param free_cb: 显示完成frame 释放函数
 * @return 成功返回0，失败返回负数
 */
int dev_lcd_flush(frame_buffer_t * frame, display_free_cb free_cb)
{
    int ret = bk_display_flush(lcd_display_handle, (void *)frame, free_cb);
    if(ret != AVDK_ERR_OK)
    {
        LOGE("bk_display_flush failed!\n");
        if(free_cb)
        {
            free_cb(frame);
        }
    }
    return ret;
}


/**
 * @brief 关闭lcd
 * @return 成功返回0，失败返回负数
 */
int dev_lcd_close(void)
{
    int ret = 0;

    if( lcd_dev_opened == false)
    {
        LOGD("[%s][%d] dev_lcd_close not running, skip!\n", __func__, __LINE__);
        return BK_OK;
    }
    do{
        dev_lcd_backlight_ctrl(false);

        ret = bk_display_close(lcd_display_handle);
        if(ret != AVDK_ERR_OK)
        {
            LOGE("[%s][%d] bk_display_close failed!\n", __func__, __LINE__);
            return ret;
        }

        ret = bk_display_delete(lcd_display_handle);
        if(ret != AVDK_ERR_OK)
        {
            LOGE("[%s][%d] bk_display_delete failed!\n", __func__, __LINE__);
            return ret;
        }
        
        lcd_display_handle = NULL;

        bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LDO_GPIO_ID, GPIO_OUTPUT_STATE_LOW);
        
        lcd_dev_opened = false;
    }while(0);

    return ret;
}

/**
 * @brief 开始显示摄像头图像
 * @return 成功返回0，失败返回负数
 */
int dev_lcd_display_camera_start()
{
    int ret = BK_OK;
    display_camera_started = true;
    ret = rtos_create_thread(&lcd_display_camera_thread, 6, "lcd_display_camera_task", lcd_display_camera_task , 2048, NULL);
    if(ret != BK_OK)
    {
        LOGE("lcd_display_camera_task create failed!\n");
        return ret;
    }
    return ret;
}

/**
 * @brief 停止显示摄像头图像
 * @return 成功返回0，失败返回负数
 */
int dev_lcd_display_camera_stop()
{
    int ret = BK_OK;
    display_camera_started = false;
    while(lcd_display_camera_thread != NULL)
    {
        rtos_delay_milliseconds(10);
    }
    return ret;
}
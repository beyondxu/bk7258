#include <os/os.h>
#include <os/str.h>
#include "media_evt.h"
#include "frame_buffer.h"
#include "yuv_encode.h"
#include "lcd_panel_devices.h"
#include "driver/gpio.h"
#include "gpio_driver.h"
#include "dev_lcd.h"

static bk_display_ctlr_handle_t g_lcd_handle = NULL;
static bool g_is_init = false;
extern const lcd_device_t lcd_device_st7701s;

bk_display_rgb_ctlr_config_t rgb_config = {
    .lcd_device = &lcd_device_st7701s,
    .clk_pin = 0,
    .cs_pin = 12,
    .sda_pin = 1,
    .rst_pin = 6,
};

/**
 * @brief 打开lcd，注意此接口不会开lcd背光
 * @return 成功返回0，失败返回负数
 */
int dev_lcd_open(void)
{
    bk_err_t ret = BK_FAIL;

    do{
        if (g_is_init)
        {
            ret = BK_OK;
            break;
        }

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

        ret = bk_display_rgb_new(&g_lcd_handle, &rgb_config);
        if (BK_OK != ret)
        {
            LCD_LOGE("[%s][%d] rgb new fail\r\n", __func__, __LINE__);
            break;
        }

        BK_LOG_ON_ERR(bk_gpio_set_output_high(LCD_LDO_IO));    //上电
        ret = bk_display_open(g_lcd_handle);
        if (BK_OK != ret)
        {
            LCD_LOGE("[%s][%d] display open fail\r\n", __func__, __LINE__);
            break;
        }

        g_is_init = true;
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
        LCD_LOGI("[%s][%d] on\r\n", __func__, __LINE__);
        bk_gpio_set_output_high(LCD_BL_IO);
    }
    else if (0 == percent)
    {
        LCD_LOGI("[%s][%d] off\r\n", __func__, __LINE__);
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
s32 dev_lcd_flush(frame_buffer_t *frame, display_free_cb free_cb)
{
    bk_err_t ret = BK_FAIL;

    do{
        if (!g_is_init)
        {
            LCD_LOGE("[%s][%d] lcd don't open\r\n", __func__, __LINE__);
            break;
        }

        if (!frame)
        {
            LCD_LOGE("[%s][%d] param is invalid\r\n", __func__, __LINE__);
            break;
        }

        ret = bk_display_flush(g_lcd_handle, frame, free_cb);
    }while(0);

    return ret;
}

/**
 * @brief 关闭lcd
 * @return 成功返回0，失败返回负数
 */
s32 dev_lcd_close(void)
{
    bk_err_t ret = BK_FAIL;

    do{
        if (!g_is_init)
        {
            LCD_LOGE("[%s][%d] lcd don't open\r\n", __func__, __LINE__);
            ret = BK_OK;
            break;
        }

        dev_lcd_backlight_ctrl(false);

        ret = bk_display_close(g_lcd_handle);
        if (BK_OK != ret)
        {
            LCD_LOGE("[%s][%d] display close fail\r\n", __func__, __LINE__);
            break;
        }

        ret = bk_display_delete(g_lcd_handle);
        if (BK_OK != ret)
        {
            LCD_LOGE("[%s][%d] display open fail\r\n", __func__, __LINE__);
            break;
        }

        BK_LOG_ON_ERR(bk_gpio_set_output_low(LCD_LDO_IO));    //下电

        g_is_init = false;
    }while(0);

    return ret;
}

/**
 * @brief 获取lcd宽
 * @return lcd宽
 */
s32 dev_lcd_get_width(void)
{
    return rgb_config.lcd_device->width;
}

/**
 * @brief 获取lcd高
 * @return lcd高
 */
s32 dev_lcd_get_height(void)
{
    return rgb_config.lcd_device->height;
}

/**
 * @brief 获取lcd操作handle
 * @return lcd操作handle
 */
bk_display_ctlr_handle_t dev_lcd_get_device_handle(void)
{
    return g_lcd_handle;
}



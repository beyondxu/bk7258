#include <os/os.h>
#include <os/str.h>
#include <components/media_types.h>
#include <lv_vendor.h>
#include "lvgl_app_ui.h"

/**
 * @brief 初始化LVGL应用程序
 * 
 * 该函数用于初始化LVGL图形库，包括配置LCD参数、分配帧缓冲区内存等
 * 
 * @param[in] display_handle LCD操作handle
 * @param[in] lcd_width LCD屏幕宽度
 * @param[in] lcd_height LCD屏幕高度
 * @param[in] rotation 屏幕旋转方向
 * @return int 成功返回BK_OK，失败返回BK_FAIL
 */
int lvgl_app_init(bk_display_ctlr_handle_t display_handle, int lcd_width, int lcd_height, media_rotate_t rotation)
{
    lv_vnd_config_t lv_vnd_config = {0};

    lv_vnd_config.handle = display_handle;
    lv_vnd_config.width = lcd_width;
    lv_vnd_config.height = lcd_height;
    lv_vnd_config.render_mode = RENDER_PARTIAL_MODE;
    lv_vnd_config.rotation = rotation;

    for (int i = 0; i < CONFIG_LVGL_FRAME_BUFFER_NUM; i++) 
    {
        lv_vnd_config.frame_buffer[i] = frame_buffer_display_malloc(lv_vnd_config.width * lv_vnd_config.height * sizeof(bk_color_t));
        if (lv_vnd_config.frame_buffer[i] == NULL) 
        {
            // LOGE("lv_frame_buffer[%d] malloc failed\r\n", i);
            return BK_FAIL;
        }
    }

    lv_vendor_init(&lv_vnd_config);
    lv_vendor_start();

    LVGL_LOGI("[%s][%d] init success\r\n", __FUNCTION__, __LINE__);
    
    return BK_OK;
}

/**
 * @brief 暂停LVGL显示
 * 
 * 该函数用于暂停LVGL图形库的显示功能
 * 
 * @return bk_err_t 成功返回BK_OK
 */
bk_err_t lvgl_app_suspend_display(void)
{
    lv_vendor_stop();

    return BK_OK;
}

/**
 * @brief 恢复LVGL显示
 * 
 * 该函数用于恢复LVGL图形库的显示功能
 * 
 * @return bk_err_t 成功返回BK_OK
 */
bk_err_t lvgl_app_resume_display(void)
{
    lv_vendor_start();

    return BK_OK;
}

/**
 * @brief  反初始化 LVGL，一般不建议用
 * 
 * 该函数用于反初始化LVGL图形库，释放相关资源
 * 
 * @return bk_err_t 成功返回BK_OK
 */
bk_err_t lvgl_app_deinit(void)
{
    lv_vendor_stop();

    lv_vendor_deinit();

    LVGL_LOGI("[%s][%d] deinit success\r\n", __FUNCTION__, __LINE__);

    return BK_OK;
}

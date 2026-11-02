#pragma once

#include <components/bk_display.h>

#define LVGL_TAG "lvgl"

#define LVGL_LOGI(...) BK_LOGI(LVGL_TAG, ##__VA_ARGS__)
#define LVGL_LOGE(...) BK_LOGE(LVGL_TAG, ##__VA_ARGS__)
#define LVGL_LOGD(...) BK_LOGD(LVGL_TAG, ##__VA_ARGS__)
#define LVGL_LOGV(...) BK_LOGV(LVGL_TAG, ##__VA_ARGS__)

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
int lvgl_app_init(bk_display_ctlr_handle_t display_handle, int lcd_width, int lcd_height, media_rotate_t rotation);

/**
 * @brief 暂停LVGL显示
 * 
 * 该函数用于暂停LVGL图形库的显示功能
 * 
 * @return bk_err_t 成功返回BK_OK
 */
bk_err_t lvgl_app_suspend_display(void);

/**
 * @brief 恢复LVGL显示
 * 
 * 该函数用于恢复LVGL图形库的显示功能
 * 
 * @return bk_err_t 成功返回BK_OK
 */
bk_err_t lvgl_app_resume_display(void);

/**
 * @brief  反初始化 LVGL，一般不建议用
 * 
 * 该函数用于释放LVGL图形库占用的资源
 * 
 * @return bk_err_t 成功返回BK_OK
 */
bk_err_t lvgl_app_deinit(void);


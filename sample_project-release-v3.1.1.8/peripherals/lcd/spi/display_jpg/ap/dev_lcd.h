#ifndef __DEV_LCD_H__
#define __DEV_LCD_H__

#define LCD_TAG "lvgl"

#define LCD_LOGI(...) BK_LOGI(LCD_TAG, ##__VA_ARGS__)
#define LCD_LOGE(...) BK_LOGE(LCD_TAG, ##__VA_ARGS__)
#define LCD_LOGD(...) BK_LOGD(LCD_TAG, ##__VA_ARGS__)
#define LCD_LOGV(...) BK_LOGV(LCD_TAG, ##__VA_ARGS__)

#define LCD_BL_IO       GPIO_9                                      //背光
#define LCD_LDO_IO      GPIO_23                                     //电源脚

typedef bk_err_t (*display_free_cb)(void *args);

/**
 * @brief 打开lcd，注意此接口不会开lcd背光
 * @return 成功返回0，失败返回负数
 */
int dev_lcd_open(void);

/**
 * @brief 调节lcd背光
 * @param percent: 0关闭， 100：开，其它为背光占空比
 * @return 无
 */
void dev_lcd_backlight_ctrl(int percent);

/**
 * @brief 刷屏
 * @param frame: 显示frame
 * @param free_cb: 显示完成frame 释放函数
 * @return 成功返回0，失败返回负数
 */
s32 dev_lcd_flush(frame_buffer_t *frame, display_free_cb free_cb);

/**
 * @brief 关闭lcd
 * @return 成功返回0，失败返回负数
 */
s32 dev_lcd_close(void);

#endif


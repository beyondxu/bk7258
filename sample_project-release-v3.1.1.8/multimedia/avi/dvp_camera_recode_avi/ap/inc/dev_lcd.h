#ifndef __DEV_LCD_H__
#define __DEV_LCD_H__

#include "frame_buffer.h"

#define LCD_BL_IO       GPIO_7                                      //背光
#define LCD_LDO_IO      GPIO_13                                     //电源脚

typedef bk_err_t (*display_free_cb)(void *args);

/**
 * @brief 打开lcd，注意此接口不会开lcd背光
 * @return 成功返回0，失败返回负数
 */
int dev_lcd_open();

/**
 * @brief 关闭lcd
 * @return 成功返回0，失败返回负数
 */
int dev_lcd_close();

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
int dev_lcd_flush(frame_buffer_t * frame, display_free_cb free_cb);

/**
 * @brief 启动显示相机任务
 * @return 成功返回0，失败返回负数
 */
int dev_lcd_display_camera_start();

/**
 * @brief 停止显示相机任务
 * @return 成功返回0，失败返回负数
 */
int dev_lcd_display_camera_stop();

#endif
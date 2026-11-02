#pragma once

#include <components/media_types.h>
#include <modules/lcd_font.h>

/**
 * @brief: 显示绘制模块初始化
 * @param: 无
 * @return: BK_OK - 成功
 * @return: BK_FAIL - 失败
*/
int app_display_draw_init(void);

/**
 * @brief: 显示绘制模块反初始化
 * @param: 无
 * @return: BK_OK - 成功
 * @return: BK_FAIL - 失败
*/
int app_display_draw_deinit(void);

/**
 * @brief: 颜色填充（在目标缓冲区的指定区域填充指定颜色）
 * @param dst_frame - 目标帧缓冲区指针，包含帧缓冲区的所有信息（地址、尺寸、格式等）
 * @param color - 填充颜色值（根据帧格式自动解析为RGB565/RGB888/ARGB8888）
 * @param dst_x - 目标区域起始X坐标（像素）
 * @param dst_y - 目标区域起始Y坐标（像素）
 * @param fill_width - 填充区域宽度（像素）
 * @param fill_height - 填充区域高度（像素）
 * @return: BK_OK - 成功
 * @return: BK_ERR_PARAM - 参数错误
*/
int app_display_draw_fill_color(
                   frame_buffer_t *dst_frame,
                   uint32_t color,
                   uint16_t dst_x,
                   uint16_t dst_y,
                   uint16_t fill_width,
                   uint16_t fill_height);

/**
 * @brief: 内存区域拷贝（将源缓冲区数据拷贝到目标缓冲区的指定区域）
 * @param src_frame - 源帧缓冲区指针，包含源帧缓冲区的所有信息（地址、尺寸、格式等）
 * @param dst_frame - 目标帧缓冲区指针，包含目标帧缓冲区的所有信息（地址、尺寸、格式等）
 * @param dst_x - 目标区域起始X坐标（像素）
 * @param dst_y - 目标区域起始Y坐标（像素）
 * @return: BK_OK - 成功
 * @return: BK_ERR_PARAM - 参数错误
*/
int app_display_draw_copy(
                   frame_buffer_t *src_frame,
                   frame_buffer_t *dst_frame,
                   uint16_t dst_x,
                   uint16_t dst_y);

/**
 * @brief: 图像融合（将前景图像融合到背景图像的指定区域）
 * @param bg_frame - 背景帧缓冲区指针，包含背景帧缓冲区的所有信息（地址、尺寸、格式等）
 * @param fg_frame - 前景帧缓冲区指针，包含前景帧缓冲区的所有信息（地址、尺寸、格式等），必须是ARGB8888格式
 * @param dst_x - 目标区域起始X坐标（像素）
 * @param dst_y - 目标区域起始Y坐标（像素）
 * @return: BK_OK - 成功
 * @return: BK_ERR_PARAM - 参数错误
*/
int app_display_draw_blend(
                    frame_buffer_t *bg_frame,
                    frame_buffer_t *fg_frame,
                    uint16_t dst_x,
                    uint16_t dst_y);

/**
 * @brief: 字体显示（将字体融合到背景图像的指定区域）
 * @param bg_frame - 背景帧缓冲区指针，包含背景帧缓冲区的所有信息（地址、尺寸、格式等）
 * @param text - 要显示的文本字符串
 * @param font_color - 字体颜色
 * @param font_name - 字体结构体指针
 * @param font_pos_x - 字体显示起始X坐标（像素）
 * @param font_pos_y - 字体显示起始Y坐标（像素）
 * @return: BK_OK - 成功
 * @return: BK_ERR_PARAM - 参数错误
*/
int app_display_draw_font(
                    frame_buffer_t *bg_frame,
                    const char *text,
                    unsigned short font_color,
                    const gui_font_digit_struct *font_name,
                    int font_pos_x,
                    int font_pos_y);

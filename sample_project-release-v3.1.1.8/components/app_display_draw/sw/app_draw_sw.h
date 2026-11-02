#pragma once

#include <components/media_types.h>
#include <modules/lcd_font.h>

/**
 * @brief: 软件 绘制初始化
 * @return: BK_OK - 成功
 * @return: BK_FAIL - 失败
*/
bk_err_t app_sw_draw_init(void);

/**
 * @brief: 软件 颜色填充（在目标缓冲区的指定区域填充指定颜色），支持两字节、三字节、四字节的buffer填充颜色
 * @param dst_frame - 目标帧缓冲区指针，包含帧缓冲区的所有信息（地址、尺寸、格式等）
 * @param color - 填充颜色值（根据帧格式自动解析为RGB565/RGB888/ARGB8888）
 * @param dst_x - 目标区域起始X坐标（像素）
 * @param dst_y - 目标区域起始Y坐标（像素）
 * @param fill_width - 填充区域宽度（像素）
 * @param fill_height - 填充区域高度（像素）
 * @return: BK_OK - 成功
 * @return: BK_ERR_PARAM - 参数错误
*/
int app_sw_fill_color(frame_buffer_t *dst_frame,
                            uint32_t color,
                            uint16_t dst_x,
                            uint16_t dst_y,
                            uint16_t fill_width,
                            uint16_t fill_height);

/**
 * @brief: 软件 内存区域拷贝（将源缓冲区数据拷贝到目标缓冲区的指定区域），支持两字节、三字节、四字节的颜色格式数据拷贝
 * @param src_frame - 源帧缓冲区指针，包含源帧缓冲区的所有信息（地址、尺寸、格式等）
 * @param dst_frame - 目标帧缓冲区指针，包含目标帧缓冲区的所有信息（地址、尺寸、格式等）
 * @param pos_x - 目标区域起始X坐标（像素）
 * @param pos_y - 目标区域起始Y坐标（像素）
 * @return: BK_OK - 成功
 * @return: BK_ERR_PARAM - 参数错误
*/
int app_sw_copy(frame_buffer_t *src_frame, const frame_buffer_t *dst_frame, int pos_x, int pos_y);

/**
 * @brief: 软件 图像融合（将前景图像融合到背景图像的指定区域），背景图像可以是PIXEL_FMT_VUYY/PIXEL_FMT_YUYV/PIXEL_FMT_RGB888/PIXEL_FMT_RGB565_LE/PIXEL_FMT_RGB565，前景图像必须是ARGB8888格式
 * @param bg_frame - 背景帧缓冲区指针，包含背景帧缓冲区的所有信息（地址、尺寸、格式等）
 * @param fg_frame - 前景帧缓冲区指针，包含前景帧缓冲区的所有信息（地址、尺寸、格式等），必须是ARGB8888格式
 * @param pos_x - 目标区域起始X坐标（像素）
 * @param pos_y - 目标区域起始Y坐标（像素）
 * @return: BK_OK - 成功
 * @return: BK_ERR_PARAM - 参数错误
*/
int app_sw_blend(frame_buffer_t *bg_frame, const frame_buffer_t *fg_frame, int pos_x, int pos_y);

/**
 * @brief: 软件 字体显示（在背景帧缓冲区的指定位置显示文本）
 * @param bg_frame - 背景帧缓冲区指针，包含背景帧缓冲区的所有信息（地址、尺寸、格式等），支持PIXEL_FMT_VUYY/PIXEL_FMT_YUYV/PIXEL_FMT_RGB888/PIXEL_FMT_RGB565_LE/PIXEL_FMT_RGB565格式
 * @param text - 要显示的文本字符串
 * @param font_color - 字体颜色（RGB565格式）
 * @param font_name - 字体结构体指针，指定使用的字体类型
 * @param font_pos_x - 字体显示起始X坐标（像素）
 * @param font_pos_y - 字体显示起始Y坐标（像素）
 * @return: BK_OK - 成功
 * @return: BK_ERR_PARAM - 参数错误
*/
s32 app_sw_display_font(frame_buffer_t *bg_frame, 
                              const char * text, 
                              unsigned short font_color, 
                              const gui_font_digit_struct *font_name,
                              int font_pos_x, 
                              int font_pos_y);

bk_err_t app_sw_draw_deinit(void);


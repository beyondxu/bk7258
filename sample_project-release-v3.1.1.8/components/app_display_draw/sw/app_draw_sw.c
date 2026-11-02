#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/bk_dma2d.h>
#include <components/bk_draw_osd.h>
#include "app_draw_sw.h"

#define TAG "sw_draw"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)

static bk_draw_osd_ctlr_handle_t g_ui_blend_handle = {0};
static bool g_is_init = false;

static const blend_info_t blend_assets_null[] =
{
    {.addr = NULL},
};

static const blend_info_t blend_info_null[] =
{
    {.addr = NULL},
};

bk_err_t app_sw_draw_init(void)
{
    bk_err_t ret = BK_FAIL;
    
    do{
        if(g_is_init)
        {
            ret = BK_OK;
            break;
        }
        
        osd_ctlr_config_t osd_cfg = {0}; 
        osd_cfg.blend_assets =blend_assets_null;  /**<all assets */
        osd_cfg.blend_info = blend_info_null;     /**<real display */
        osd_cfg.draw_in_psram = false;
        ret = bk_draw_osd_new(&g_ui_blend_handle, &osd_cfg);
        if(BK_OK != ret)
        {
            LOGE("[%s][%d] osd new fail\r\n", __FUNCTION__, __LINE__);
            break;
        }

        g_is_init = true;
        LOGE("[%s][%d] init success\r\n", __FUNCTION__, __LINE__);
    }while(0);

    return ret;
}

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
                            uint16_t fill_height)
{
    int ret = BK_FAIL;
    uint16_t x, y;
    uint32_t dst_offset;

    do{
        if(NULL == dst_frame || NULL == dst_frame->frame)
        {
            LOGE("[%s][%d] dst_frame or dst_frame->frame is NULL\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if(0 == dst_frame->size)
        {
            LOGE("[%s][%d] dst_frame->size is 0\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if(0 == dst_frame->length)
        {
            LOGE("[%s][%d] dst_frame->length is 0\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if((0 == dst_frame->width) || (0 == dst_frame->height))
        {
            LOGE("[%s][%d] invalid dst_width:%d or dst_height:%d\r\n", __FUNCTION__, __LINE__, dst_frame->width, dst_frame->height);
            ret = BK_ERR_PARAM;
            break;
        }

        color_bytes_t pix_size = dst_frame->length/(dst_frame->width*dst_frame->height);

        if((0 == fill_width) || (0 == fill_height))
        {
            LOGE("[%s][%d] invalid fill_width:%d or fill_height:%d\r\n", __FUNCTION__, __LINE__, fill_width, fill_height);
            ret = BK_ERR_PARAM;
            break;
        }

        if(dst_x >= dst_frame->width)
        {
            LOGE("[%s][%d] dst_x:%d >= dst_width:%d\r\n", __FUNCTION__, __LINE__, dst_x, dst_frame->width);
            ret = BK_ERR_PARAM;
            break;
        }

        if(dst_y >= dst_frame->height)
        {
            LOGE("[%s][%d] dst_y:%d >= dst_height:%d\r\n", __FUNCTION__, __LINE__, dst_y, dst_frame->height);
            ret = BK_ERR_PARAM;
            break;
        }

        if((dst_x + fill_width) > dst_frame->width)
        {
            LOGE("[%s][%d] dst_x:%d + fill_width:%d > dst_width:%d\r\n", __FUNCTION__, __LINE__, dst_x, fill_width, dst_frame->width);
            ret = BK_ERR_PARAM;
            break;
        }

        if((dst_y + fill_height) > dst_frame->height)
        {
            LOGE("[%s][%d] dst_y:%d + fill_height:%d > dst_height:%d\r\n", __FUNCTION__, __LINE__, dst_y, fill_height, dst_frame->height);
            ret = BK_ERR_PARAM;
            break;
        }
        
        if(TWO_BYTES == pix_size)
        {
            uint16_t *dst_ptr16 = (uint16_t *)dst_frame->frame;
            uint16_t color16 = (uint16_t)color;

            for(y = dst_y; y < (dst_y + fill_height); y++)
            {
                dst_offset = y * dst_frame->width + dst_x;
                dst_ptr16 += dst_offset;

                for(x = 0; x < fill_width; x++)
                {
                    dst_ptr16[x] = color16;
                }

                dst_ptr16 = (uint16_t *)dst_frame->frame;
            }
        }
        else if(THREE_BYTES == pix_size)
        {
            uint8_t *dst_ptr8 = (uint8_t *)dst_frame->frame;
            uint8_t color_r = (color >> 16) & 0xFF;
            uint8_t color_g = (color >> 8) & 0xFF;
            uint8_t color_b = color & 0xFF;

            for(y = dst_y; y < (dst_y + fill_height); y++)
            {
                for(x = dst_x; x < (dst_x + fill_width); x++)
                {
                    dst_offset = (y * dst_frame->width + x) * 3;
                    dst_ptr8[dst_offset] = color_b;
                    dst_ptr8[dst_offset + 1] = color_g;
                    dst_ptr8[dst_offset + 2] = color_r;
                }
            }
        }
        else if(FOUR_BYTES == pix_size)
        {
            uint32_t *dst_ptr32 = (uint32_t *)dst_frame->frame;

            for(y = dst_y; y < (dst_y + fill_height); y++)
            {
                dst_offset = y * dst_frame->width + dst_x;
                dst_ptr32 += dst_offset;

                for(x = 0; x < fill_width; x++)
                {
                    dst_ptr32[x] = color;
                }

                dst_ptr32 = (uint32_t *)dst_frame->frame;
            }
        }
        else
        {
            LOGE("[%s][%d] invalid pix_size:%d\r\n", __FUNCTION__, __LINE__, pix_size);
            ret = BK_ERR_PARAM;
            break;
        }

        ret = BK_OK;
    }while(0);

    return ret;
}

/**
 * @brief: 软件 内存区域拷贝（将源缓冲区数据拷贝到目标缓冲区的指定区域），支持两字节、三字节、四字节的颜色格式数据拷贝
 * @param src_frame - 源帧缓冲区指针，包含源帧缓冲区的所有信息（地址、尺寸、格式等）
 * @param dst_frame - 目标帧缓冲区指针，包含目标帧缓冲区的所有信息（地址、尺寸、格式等）
 * @param pos_x - 目标区域起始X坐标（像素）
 * @param pos_y - 目标区域起始Y坐标（像素）
 * @return: BK_OK - 成功
 * @return: BK_ERR_PARAM - 参数错误
*/
int app_sw_copy(frame_buffer_t *src_frame, const frame_buffer_t *dst_frame, int pos_x, int pos_y)
{
    int ret = BK_FAIL;
    uint16_t x, y;
    uint32_t src_offset, dst_offset;

    do{
        if((NULL == src_frame) || (NULL == dst_frame))
        {
            LOGE("[%s][%d] src_frame or dst_frame is NULL\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if((NULL == src_frame->frame) || (NULL == dst_frame->frame))
        {
            LOGE("[%s][%d] src_frame->frame or dst_frame->frame is NULL\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if(0 == src_frame->size)
        {
            LOGE("[%s][%d] src_frame->size is 0\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if(0 == dst_frame->size)
        {
            LOGE("[%s][%d] dst_frame->size is 0\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if(0 == src_frame->length)
        {
            LOGE("[%s][%d] src_frame->length is 0\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if(0 == dst_frame->length)
        {
            LOGE("[%s][%d] dst_frame->length is 0\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if((0 == src_frame->width) || (0 == src_frame->height))
        {
            LOGE("[%s][%d] invalid src_width:%d or src_height:%d\r\n", __FUNCTION__, __LINE__, src_frame->width, src_frame->height);
            ret = BK_ERR_PARAM;
            break;
        }

        if((0 == dst_frame->width) || (0 == dst_frame->height))
        {
            LOGE("[%s][%d] invalid dst_width:%d or dst_height:%d\r\n", __FUNCTION__, __LINE__, dst_frame->width, dst_frame->height);
            ret = BK_ERR_PARAM;
            break;
        }

        if(src_frame->fmt != dst_frame->fmt)
        {
            LOGE("[%s][%d] fmt isn't same:%d, %d\r\n", __FUNCTION__, __LINE__, src_frame->fmt, dst_frame->fmt);
            ret = BK_ERR_PARAM;
            break;
        }

        color_bytes_t pix_size = src_frame->length/(src_frame->width*src_frame->height);

        if(pos_x >= dst_frame->width)
        {
            LOGE("[%s][%d] pos_x:%d >= dst_width:%d\r\n", __FUNCTION__, __LINE__, pos_x, dst_frame->width);
            ret = BK_ERR_PARAM;
            break;
        }

        if(pos_y >= dst_frame->height)
        {
            LOGE("[%s][%d] pos_y:%d >= dst_height:%d\r\n", __FUNCTION__, __LINE__, pos_y, dst_frame->height);
            ret = BK_ERR_PARAM;
            break;
        }

        if((pos_x + src_frame->width) > dst_frame->width)
        {
            LOGE("[%s][%d] pos_x:%d + src_width:%d > dst_width:%d\r\n", __FUNCTION__, __LINE__, pos_x, src_frame->width, dst_frame->width);
            ret = BK_ERR_PARAM;
            break;
        }

        if((pos_y + src_frame->height) > dst_frame->height)
        {
            LOGE("[%s][%d] pos_y:%d + src_height:%d > dst_height:%d\r\n", __FUNCTION__, __LINE__, pos_y, src_frame->height, dst_frame->height);
            ret = BK_ERR_PARAM;
            break;
        }
        
        if(TWO_BYTES == pix_size)
        {
            uint16_t *src_ptr16 = (uint16_t *)src_frame->frame;
            uint16_t *dst_ptr16 = (uint16_t *)dst_frame->frame;

            for(y = 0; y < src_frame->height; y++)
            {
                src_offset = y * src_frame->width;
                dst_offset = (pos_y + y) * dst_frame->width + pos_x;

                for(x = 0; x < src_frame->width; x++)
                {
                    dst_ptr16[dst_offset + x] = src_ptr16[src_offset + x];
                }
            }
        }
        else if(THREE_BYTES == pix_size)
        {
            uint8_t *src_ptr8 = (uint8_t *)src_frame->frame;
            uint8_t *dst_ptr8 = (uint8_t *)dst_frame->frame;

            for(y = 0; y < src_frame->height; y++)
            {
                for(x = 0; x < src_frame->width; x++)
                {
                    src_offset = (y * src_frame->width + x) * 3;
                    dst_offset = ((pos_y + y) * dst_frame->width + pos_x + x) * 3;

                    dst_ptr8[dst_offset] = src_ptr8[src_offset];
                    dst_ptr8[dst_offset + 1] = src_ptr8[src_offset + 1];
                    dst_ptr8[dst_offset + 2] = src_ptr8[src_offset + 2];
                }
            }
        }
        else if(FOUR_BYTES == pix_size)
        {
            uint32_t *src_ptr32 = (uint32_t *)src_frame->frame;
            uint32_t *dst_ptr32 = (uint32_t *)dst_frame->frame;

            for(y = 0; y < src_frame->height; y++)
            {
                src_offset = y * src_frame->width;
                dst_offset = (pos_y + y) * dst_frame->width + pos_x;

                for(x = 0; x < src_frame->width; x++)
                {
                    dst_ptr32[dst_offset + x] = src_ptr32[src_offset + x];
                }
            }
        }
        else
        {
            LOGE("[%s][%d] invalid pix_size:%d\r\n", __FUNCTION__, __LINE__, pix_size);
            ret = BK_ERR_PARAM;
            break;
        }

        ret = BK_OK;
    }while(0);

    return ret;
}

/**
 * @brief: 软件 图像融合（将前景图像融合到背景图像的指定区域），背景图像可以是PIXEL_FMT_VUYY/PIXEL_FMT_YUYV/PIXEL_FMT_RGB888/PIXEL_FMT_RGB565_LE/PIXEL_FMT_RGB565，前景图像必须是ARGB8888格式
 * @param bg_frame - 背景帧缓冲区指针，包含背景帧缓冲区的所有信息（地址、尺寸、格式等）
 * @param fg_frame - 前景帧缓冲区指针，包含前景帧缓冲区的所有信息（地址、尺寸、格式等），必须是ARGB8888格式
 * @param pos_x - 目标区域起始X坐标（像素）
 * @param pos_y - 目标区域起始Y坐标（像素）
 * @return: BK_OK - 成功
 * @return: BK_ERR_PARAM - 参数错误
*/
int app_sw_blend(frame_buffer_t *bg_frame, const frame_buffer_t *fg_frame, int pos_x, int pos_y)
{
    bk_err_t ret = BK_FAIL;

    do{
        if(!g_is_init)
        {
            LOGE("[%s][%d] ui blend don't init\r\n", __FUNCTION__, __LINE__);
            break;
        }

        if((NULL == bg_frame) || (NULL == fg_frame))
        {
            LOGE("[%s][%d] bg_frame or fg_frame is NULL\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if((NULL == bg_frame->frame) || (NULL == fg_frame->frame))
        {
            LOGE("[%s][%d] bg_frame->frame or fg_frame->frame is NULL\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if(0 == bg_frame->size)
        {
            LOGE("[%s][%d] bg_frame->size is 0\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if(0 == fg_frame->size)
        {
            LOGE("[%s][%d] fg_frame->size is 0\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if(0 == bg_frame->length)
        {
            LOGE("[%s][%d] bg_frame->length is 0\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if(0 == fg_frame->length)
        {
            LOGE("[%s][%d] fg_frame->length is 0\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if((0 == fg_frame->width) || (0 == fg_frame->height))
        {
            LOGE("[%s][%d] invalid fg_width:%d or fg_height:%d\r\n", __FUNCTION__, __LINE__, fg_frame->width, fg_frame->height);
            ret = BK_ERR_PARAM;
            break;
        }

        if((0 == bg_frame->width) || (0 == bg_frame->height))
        {
            LOGE("[%s][%d] invalid bg_width:%d or bg_height:%d\r\n", __FUNCTION__, __LINE__, bg_frame->width, bg_frame->height);
            ret = BK_ERR_PARAM;
            break;
        }

        if(PIXEL_FMT_ARGB8888 != fg_frame->fmt)
        {
            LOGE("[%s][%d] fg_frame format must be ARGB8888:%d\r\n", __FUNCTION__, __LINE__, fg_frame->fmt);
            ret = BK_ERR_PARAM;
            break;
        }

        if(PIXEL_FMT_VUYY != bg_frame->fmt && PIXEL_FMT_YUYV != bg_frame->fmt && 
           PIXEL_FMT_RGB888 != bg_frame->fmt && PIXEL_FMT_RGB565_LE != bg_frame->fmt && 
           PIXEL_FMT_RGB565 != bg_frame->fmt)
        {
            LOGE("[%s][%d] bg_frame format invalid:%d\r\n", __FUNCTION__, __LINE__, bg_frame->fmt);
            ret = BK_ERR_PARAM;
            break;
        }

        if(pos_x >=  bg_frame->width)
        {
            LOGE("[%s][%d] dst_x:%d >= bg_width:%d\r\n", __FUNCTION__, __LINE__, pos_x, bg_frame->width);
            ret = BK_ERR_PARAM;
            break;
        }

        if(pos_y >=  bg_frame->height)
        {
            LOGE("[%s][%d] dst_y:%d >= bg_height:%d\r\n", __FUNCTION__, __LINE__, pos_y, bg_frame->height);
            ret = BK_ERR_PARAM;
            break;
        }
        
        osd_bg_info_t bg_info = {0};

        bg_info.frame = bg_frame;
        bg_info.width = bg_frame->width;
        bg_info.height = bg_frame->height;

        bk_blend_t img_info;
        img_info.blend_type = BLEND_TYPE_IMAGE;
        img_info.width = fg_frame->width;
        img_info.height = fg_frame->height;
        img_info.xpos = pos_x;
        img_info.ypos = pos_y;
        img_info.image.format = ARGB8888;
        img_info.image.data_len = fg_frame->length;
        img_info.image.data = fg_frame->frame;

        blend_info_t blend_info = {.name = "img", .addr = &img_info, .content = ""};
        ret = bk_draw_osd_image(g_ui_blend_handle, &bg_info, &blend_info);
    }while(0);

    return ret;
}

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
                              int font_pos_y)
{
    bk_err_t ret = BK_FAIL;

    do{
        if(!g_is_init)
        {
            LOGE("[%s][%d] ui blend don't init\r\n", __FUNCTION__, __LINE__);
            break;
        }

        if(NULL == bg_frame || NULL == text)
        {
            LOGE("[%s][%d] bg_frame or text is NULL\r\n", __FUNCTION__, __LINE__);
            break;
        }

        if(NULL == bg_frame->frame)
        {
            LOGE("[%s][%d] bg_frame->frame is NULL\r\n", __FUNCTION__, __LINE__);
            break;
        }

        if(0 == bg_frame->size)
        {
            LOGE("[%s][%d] bg_frame->size is 0\r\n", __FUNCTION__, __LINE__);
            break;
        }

        if(0 == bg_frame->length)
        {
            LOGE("[%s][%d] bg_frame->length is 0\r\n", __FUNCTION__, __LINE__);
            break;
        }

        if((0 == bg_frame->width) || (0 == bg_frame->height))
        {
            LOGE("[%s][%d] invalid bg_width:%d or bg_height:%d\r\n", __FUNCTION__, __LINE__, bg_frame->width, bg_frame->height);
            break;
        }

        if(PIXEL_FMT_VUYY != bg_frame->fmt && PIXEL_FMT_YUYV != bg_frame->fmt && 
           PIXEL_FMT_RGB888 != bg_frame->fmt && PIXEL_FMT_RGB565_LE != bg_frame->fmt && 
           PIXEL_FMT_RGB565 != bg_frame->fmt)
        {
            LOGE("[%s][%d] bg format only support PIXEL_FMT_VUYY/PIXEL_FMT_YUYV/PIXEL_FMT_RGB888/PIXEL_FMT_RGB565_LE/PIXEL_FMT_RGB565:%d\r\n", __FUNCTION__, __LINE__, bg_frame->fmt);
            break;
        }

        LOGI("[%s][%d] fg_x:%d, fg_y:%d\r\n", 
                                                                    __FUNCTION__, __LINE__, 
                                                                    font_pos_x,
                                                                    font_pos_y);

        LOGI("[%s][%d] bg_w:%d, bg_h:%d, bg_format:%d\r\n", 
                                                                    __FUNCTION__, __LINE__, 
                                                                    bg_frame->width,
                                                                    bg_frame->height,
                                                                    bg_frame->fmt);

        osd_bg_info_t bg_info = {0};
        bg_info.frame = bg_frame;
        bg_info.width = bg_frame->width;
        bg_info.height = bg_frame->height;

        bk_blend_t font_info;
        font_info.blend_type = BLEND_TYPE_FONT;
        font_info.width = bg_frame->width - font_pos_x;
        font_info.height = bg_frame->width - font_pos_y;
        font_info.xpos = font_pos_x;
        font_info.ypos = font_pos_y;
        font_info.font.font_digit_type = font_name;
        font_info.font.color = font_color;
        
        blend_info_t blend_info = {.name = "font", .addr = &font_info, .content = ""};
        os_snprintf(blend_info.content, sizeof(blend_info.content), "%s", text);
        
        ret = bk_draw_osd_font(g_ui_blend_handle, &bg_info, &blend_info);
    }while(0);

    return ret;
}


bk_err_t app_sw_draw_deinit(void)
{
    bk_err_t ret = BK_FAIL;
    
    do{
        if(!g_is_init)
        {
            ret = BK_OK;
            break;
        }
        
        ret = bk_draw_osd_delete(g_ui_blend_handle);
        if(BK_OK != ret)
        {
            LOGE("[%s][%d] osd delete fail\r\n", __FUNCTION__, __LINE__);
            break;
        }

        g_is_init = false;
        LOGE("[%s][%d] deinit success\r\n", __FUNCTION__, __LINE__);
    }while(0);

    return ret;
}


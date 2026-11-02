#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/bk_dma2d.h>
#include "app_draw_hw_dma2d.h"

#define TAG "dma2d"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)

static bk_dma2d_ctlr_handle_t g_dma2d_handle = NULL;

/**
 * @brief: DMA2D 初始化
 * @param: 无
 * @return: BK_OK - 成功
*/
int app_hw_dma2d_init(void)
{
    int ret = BK_OK;
    if (NULL != g_dma2d_handle)
    {
        LOGE("[%s][%d] DMA2D controller already initialized\r\n", __FUNCTION__, __LINE__);
        return BK_FAIL;
    }

    ret = bk_dma2d_new(&g_dma2d_handle);
    if (BK_OK != ret)
    {
        LOGE("[%s][%d] bk_dma2d_new failed:%d\r\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    ret = bk_dma2d_open(g_dma2d_handle);
    if (BK_OK != ret)
    {
        LOGE("[%s][%d] bk_dma2d_open failed:%d\r\n", __FUNCTION__, __LINE__, ret);
        bk_dma2d_delete(g_dma2d_handle);
        return ret;
    }

    LOGD("[%s][%d] init\r\n", __FUNCTION__, __LINE__);
    return ret;
}

/**
 * @brief: DMA2D 内存区域拷贝（将源缓冲区数据拷贝到目标缓冲区的指定区域）
 * @param src_frame - 源帧缓冲区指针，包含源帧缓冲区的所有信息（地址、尺寸、格式等）
 * @param dst_frame - 目标帧缓冲区指针，包含目标帧缓冲区的所有信息（地址、尺寸、格式等）
 * @param dst_x - 目标区域起始X坐标（像素）
 * @param dst_y - 目标区域起始Y坐标（像素）
 * @return: BK_OK - 成功
 * @return: BK_ERR_PARAM - 参数错误
*/
int app_hw_dma2d_copy(
                   frame_buffer_t *src_frame,
                   frame_buffer_t *dst_frame,
                   uint16_t dst_x,
                   uint16_t dst_y)
{
    int ret = BK_FAIL;
    dma2d_memcpy_config_t dma2d_memcpy_cfg = {0};
    input_color_mode_t input_color_mode;
    out_color_mode_t output_color_mode;
    color_bytes_t pix_size;

    do{
        if (NULL == g_dma2d_handle)
        {
            LOGW("[%s][%d] DMA2D don't initialized\r\n", __FUNCTION__, __LINE__);
            return BK_FAIL;
        }

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

        pix_size = src_frame->length/(src_frame->width*src_frame->height);

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

        if((dst_x + src_frame->width) > dst_frame->width)
        {
            LOGE("[%s][%d] dst_x:%d + src_width:%d > dst_width:%d\r\n", __FUNCTION__, __LINE__, dst_x, src_frame->width, dst_frame->width);
            ret = BK_ERR_PARAM;
            break;
        }

        if((dst_y + src_frame->height) > dst_frame->height)
        {
            LOGE("[%s][%d] dst_y:%d + src_height:%d > dst_height:%d\r\n", __FUNCTION__, __LINE__, dst_y, src_frame->height, dst_frame->height);
            ret = BK_ERR_PARAM;
            break;
        }

        if(TWO_BYTES == pix_size)
        {
            input_color_mode = DMA2D_INPUT_RGB565;
            output_color_mode = DMA2D_OUTPUT_RGB565;
        }
        else if(THREE_BYTES == pix_size)
        {
            input_color_mode = DMA2D_INPUT_RGB888;
            output_color_mode = DMA2D_OUTPUT_RGB888;
        }
        else if(FOUR_BYTES == pix_size)
        {
            input_color_mode = DMA2D_INPUT_ARGB8888;
            output_color_mode = DMA2D_OUTPUT_ARGB8888;
        }
        else
        {
            LOGE("[%s][%d] invalid pix_size:%d\r\n", __FUNCTION__, __LINE__, pix_size);
            ret = BK_ERR_PARAM;
            break;
        }

        dma2d_memcpy_cfg.memcpy.mode = DMA2D_M2M;
        dma2d_memcpy_cfg.memcpy.dma2d_width = src_frame->width;
        dma2d_memcpy_cfg.memcpy.dma2d_height = src_frame->height;

        dma2d_memcpy_cfg.memcpy.input_addr = (char *)src_frame->frame;
        dma2d_memcpy_cfg.memcpy.input_color_mode = input_color_mode;
        dma2d_memcpy_cfg.memcpy.src_pixel_byte = pix_size;
        dma2d_memcpy_cfg.memcpy.src_frame_width = src_frame->width;
        dma2d_memcpy_cfg.memcpy.src_frame_height = src_frame->height;
        dma2d_memcpy_cfg.memcpy.src_frame_xpos = 0;
        dma2d_memcpy_cfg.memcpy.src_frame_ypos = 0;
       
        dma2d_memcpy_cfg.memcpy.output_addr = (char *)dst_frame->frame;
        dma2d_memcpy_cfg.memcpy.output_color_mode = output_color_mode;
        dma2d_memcpy_cfg.memcpy.dst_pixel_byte = pix_size;
        dma2d_memcpy_cfg.memcpy.dst_frame_width = dst_frame->width;
        dma2d_memcpy_cfg.memcpy.dst_frame_height = dst_frame->height;
        dma2d_memcpy_cfg.memcpy.dst_frame_xpos = dst_x;
        dma2d_memcpy_cfg.memcpy.dst_frame_ypos = dst_y;
      
        dma2d_memcpy_cfg.transfer_complete_cb = NULL;
        dma2d_memcpy_cfg.is_sync = true;
        dma2d_memcpy_cfg.user_data = NULL;

        ret = bk_dma2d_memcpy(g_dma2d_handle, &dma2d_memcpy_cfg);
        if (BK_OK != ret)
        {
            LOGE("[%s][%d] memcpy fail:%d\r\n", __FUNCTION__, __LINE__, ret);
            break;
        }
    }while(0);

    return ret;
}

/**
 * @brief: DMA2D 颜色填充（在目标缓冲区的指定区域填充指定颜色）
 * @param dst_frame - 目标帧缓冲区指针，包含帧缓冲区的所有信息（地址、尺寸、格式等）
 * @param color - 填充颜色值（根据帧格式自动解析为RGB565/RGB888/ARGB8888）
 * @param dst_x - 目标区域起始X坐标（像素）
 * @param dst_y - 目标区域起始Y坐标（像素）
 * @param fill_width - 填充区域宽度（像素）
 * @param fill_height - 填充区域高度（像素）
 * @return: BK_OK - 成功
 * @return: BK_ERR_PARAM - 参数错误
*/
int app_hw_dma2d_fill_color(
                   frame_buffer_t *dst_frame,
                   uint32_t color,
                   uint16_t dst_x,
                   uint16_t dst_y,
                   uint16_t fill_width,
                   uint16_t fill_height)
{
    int ret = BK_FAIL;
    dma2d_fill_config_t fill_config = {0};
    out_color_mode_t output_color_mode;
    color_bytes_t pix_size;

    do{
        if (NULL == g_dma2d_handle)
        {
            LOGW("[%s][%d] DMA2D don't initialized\r\n", __FUNCTION__, __LINE__);
            return BK_FAIL;
        }

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

        pix_size = dst_frame->length/(dst_frame->width*dst_frame->height);

        if(TWO_BYTES == pix_size)
        {
            output_color_mode = DMA2D_OUTPUT_RGB565;
        }
        else if(THREE_BYTES == pix_size)
        {
            output_color_mode = DMA2D_OUTPUT_RGB888;
        }
        else if(FOUR_BYTES == pix_size)
        {
            output_color_mode = DMA2D_OUTPUT_ARGB8888;
        }
        else
        {
            LOGE("[%s][%d] invalid pix_size:%d\r\n", __FUNCTION__, __LINE__, pix_size);
            ret = BK_ERR_PARAM;
            break;
        }

        fill_config.fill.frameaddr = (char *)dst_frame->frame;
        fill_config.fill.color = color;
        fill_config.fill.color_format = output_color_mode;
        fill_config.fill.pixel_byte = pix_size;
        fill_config.fill.frame_xsize = dst_frame->width;
        fill_config.fill.frame_ysize = dst_frame->height;
        fill_config.fill.xpos = dst_x;
        fill_config.fill.ypos = dst_y;
        fill_config.fill.width = fill_width;
        fill_config.fill.height = fill_height;
        fill_config.transfer_complete_cb = NULL;
        fill_config.is_sync = true;
        fill_config.user_data = NULL;

        ret = bk_dma2d_fill(g_dma2d_handle, &fill_config);
        if (BK_OK != ret)
        {
            LOGE("[%s][%d] bk_dma2d_fill fail:%d\r\n", __FUNCTION__, __LINE__, ret);
            break;
        }
    }while(0);

    return ret;
}


/**
 * @brief: DMA2D 图像融合（将前景图像融合到背景图像的指定区域）
 * @param bg_frame - 背景帧缓冲区指针，包含背景帧缓冲区的所有信息（地址、尺寸、格式等）
 * @param fg_frame - 前景帧缓冲区指针，包含前景帧缓冲区的所有信息（地址、尺寸、格式等），必须是ARGB8888格式
 * @param dst_x - 目标区域起始X坐标（像素）
 * @param dst_y - 目标区域起始Y坐标（像素）
 * @return: BK_OK - 成功
 * @return: BK_ERR_PARAM - 参数错误
*/
int app_hw_dma2d_blend(
                    frame_buffer_t *bg_frame,
                    frame_buffer_t *fg_frame,
                    uint16_t dst_x,
                    uint16_t dst_y)
{
    int ret = BK_FAIL;
    dma2d_blend_config_t blend_config = {0};
    input_color_mode_t fg_color_mode;
    input_color_mode_t bg_color_mode;
    out_color_mode_t dst_color_mode;
    color_bytes_t bg_pix_size;

    do{
        if (NULL == g_dma2d_handle)
        {
            LOGW("[%s][%d] DMA2D don't initialized\r\n", __FUNCTION__, __LINE__);
            return BK_FAIL;
        }

        if((NULL == fg_frame) || (NULL == bg_frame))
        {
            LOGE("[%s][%d] fg_frame or bg_frame is NULL\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if((NULL == fg_frame->frame) || (NULL == bg_frame->frame))
        {
            LOGE("[%s][%d] fg_frame->frame or bg_frame->frame is NULL\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if(0 == fg_frame->size)
        {
            LOGE("[%s][%d] fg_frame->size is 0\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if(0 == bg_frame->size)
        {
            LOGE("[%s][%d] bg_frame->size is 0\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if(0 == fg_frame->length)
        {
            LOGE("[%s][%d] fg_frame->length is 0\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        if(0 == bg_frame->length)
        {
            LOGE("[%s][%d] bg_frame->length is 0\r\n", __FUNCTION__, __LINE__);
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

        bg_pix_size = bg_frame->length/(bg_frame->width*bg_frame->height);

        if(dst_x >= bg_frame->width)
        {
            LOGE("[%s][%d] dst_x:%d >= bg_width:%d\r\n", __FUNCTION__, __LINE__, dst_x, bg_frame->width);
            ret = BK_ERR_PARAM;
            break;
        }

        if(dst_y >= bg_frame->height)
        {
            LOGE("[%s][%d] dst_y:%d >= bg_height:%d\r\n", __FUNCTION__, __LINE__, dst_y, bg_frame->height);
            ret = BK_ERR_PARAM;
            break;
        }

        if((dst_x + fg_frame->width) > bg_frame->width)
        {
            LOGE("[%s][%d] dst_x:%d + fg_width:%d > bg_width:%d\r\n", __FUNCTION__, __LINE__, dst_x, fg_frame->width, bg_frame->width);
            ret = BK_ERR_PARAM;
            break;
        }

        if((dst_y + fg_frame->height) > bg_frame->height)
        {
            LOGE("[%s][%d] dst_y:%d + fg_height:%d > bg_height:%d\r\n", __FUNCTION__, __LINE__, dst_y, fg_frame->height, bg_frame->height);
            ret = BK_ERR_PARAM;
            break;
        }

        // 前景固定为ARGB8888格式
        fg_color_mode = DMA2D_INPUT_ARGB8888;

        // 根据背景像素大小设置颜色模式
        if(TWO_BYTES == bg_pix_size)
        {
            bg_color_mode = DMA2D_INPUT_RGB565;
            dst_color_mode = DMA2D_OUTPUT_RGB565;
        }
        else if(THREE_BYTES == bg_pix_size)
        {
            bg_color_mode = DMA2D_INPUT_RGB888;
            dst_color_mode = DMA2D_OUTPUT_RGB888;
        }
        else if(FOUR_BYTES == bg_pix_size)
        {
            bg_color_mode = DMA2D_INPUT_ARGB8888;
            dst_color_mode = DMA2D_OUTPUT_ARGB8888;
        }
        else
        {
            LOGE("[%s][%d] invalid bg_pix_size:%d\r\n", __FUNCTION__, __LINE__, bg_pix_size);
            ret = BK_ERR_PARAM;
            break;
        }

        // dma2d blend config
        blend_config.blend.pfg_addr = (char *)fg_frame->frame;
        blend_config.blend.pbg_addr = (char *)bg_frame->frame;
        blend_config.blend.pdst_addr = (char *)bg_frame->frame;

        blend_config.blend.fg_color_mode = fg_color_mode;
        blend_config.blend.bg_color_mode = bg_color_mode;
        blend_config.blend.dst_color_mode = dst_color_mode;

        blend_config.blend.fg_red_blue_swap = DMA2D_RB_REGULAR;
        blend_config.blend.bg_red_blue_swap = DMA2D_RB_REGULAR;
        blend_config.blend.dst_red_blue_swap = DMA2D_RB_REGULAR;

        blend_config.blend.fg_frame_width = fg_frame->width;
        blend_config.blend.fg_frame_height = fg_frame->height;
        blend_config.blend.bg_frame_width = bg_frame->width;
        blend_config.blend.bg_frame_height = bg_frame->height;
        blend_config.blend.dst_frame_width = bg_frame->width;
        blend_config.blend.dst_frame_height = bg_frame->height;

        blend_config.blend.fg_frame_xpos = 0;
        blend_config.blend.fg_frame_ypos = 0;
        blend_config.blend.bg_frame_xpos = dst_x;
        blend_config.blend.bg_frame_ypos = dst_y;
        blend_config.blend.dst_frame_xpos = dst_x;
        blend_config.blend.dst_frame_ypos = dst_y;

        blend_config.blend.fg_pixel_byte = FOUR_BYTES;
        blend_config.blend.bg_pixel_byte = bg_pix_size;
        blend_config.blend.dst_pixel_byte = bg_pix_size;

        blend_config.blend.dma2d_width = fg_frame->width;
        blend_config.blend.dma2d_height = fg_frame->height;
        blend_config.blend.fg_alpha_mode = DMA2D_NO_MODIF_ALPHA;
        blend_config.blend.bg_alpha_mode = DMA2D_NO_MODIF_ALPHA;

        blend_config.transfer_complete_cb = NULL;
        blend_config.is_sync = true;
        blend_config.user_data = NULL;

        ret = bk_dma2d_blend(g_dma2d_handle, &blend_config);
        if (BK_OK != ret)
        {
            LOGE("[%s][%d] bk_dma2d_blend fail:%d\r\n", __FUNCTION__, __LINE__, ret);
            break;
        }
    }while(0);

    return ret;
}

/**
 * @brief: DMA2D 反初始化
 * @param: 无
 * @return: BK_OK - 成功
*/
int app_hw_dma2d_deinit(void)
{
    int ret = BK_OK;
    if (NULL == g_dma2d_handle)
    {
        LOGW("[%s][%d] DMA2D don't initialized\r\n", __FUNCTION__, __LINE__);
        return BK_FAIL;
    }

    ret = bk_dma2d_close(g_dma2d_handle);
    if (BK_OK != ret)
    {
        LOGE("[%s][%d] bk_dma2d_open failed:%d\r\n", __FUNCTION__, __LINE__, ret);
        bk_dma2d_delete(g_dma2d_handle);
        return ret;
    }

    ret = bk_dma2d_delete(g_dma2d_handle);
    if (BK_OK != ret)
    {
        LOGE("[%s][%d] bk_dma2d_delete failed:%d\r\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    g_dma2d_handle = NULL;

    LOGD("[%s][%d] deinit success\r\n", __FUNCTION__, __LINE__);
    return ret;
}


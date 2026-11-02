#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "app_display_draw.h"
#include "app_draw_sw.h"
#include "app_draw_hw_dma2d.h"

#define TAG "display_draw"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)

int app_display_draw_init(void)
{
    int ret = BK_OK;

#ifdef CONFIG_APP_DISPLAY_DRAW_SOFTWARE
    LOGD("[%s][%d] init with software algorithm\r\n", __FUNCTION__, __LINE__);
    ret = app_sw_draw_init();
    if (BK_OK != ret)
    {
        LOGE("[%s][%d] app_sw_draw_init failed:%d\r\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
#elif defined(CONFIG_APP_DISPLAY_DRAW_DMA2D)
    LOGD("[%s][%d] init with hardware DMA2D\r\n", __FUNCTION__, __LINE__);
    ret = app_hw_dma2d_init();
    if (BK_OK != ret)
    {
        LOGE("[%s][%d] app_hw_dma2d_init failed:%d\r\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
#else
    LOGW("[%s][%d] no display draw mode selected\r\n", __FUNCTION__, __LINE__);
#endif

    return ret;
}

int app_display_draw_deinit(void)
{
    int ret = BK_OK;

#ifdef CONFIG_APP_DISPLAY_DRAW_SOFTWARE
    LOGD("[%s][%d] deinit with software algorithm\r\n", __FUNCTION__, __LINE__);
    ret = app_sw_draw_deinit();
    if (BK_OK != ret)
    {
        LOGE("[%s][%d] app_sw_draw_deinit failed:%d\r\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
#elif defined(CONFIG_APP_DISPLAY_DRAW_DMA2D)
    LOGD("[%s][%d] deinit with hardware DMA2D\r\n", __FUNCTION__, __LINE__);
    ret = app_hw_dma2d_deinit();
    if (BK_OK != ret)
    {
        LOGE("[%s][%d] app_hw_dma2d_deinit failed:%d\r\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
#endif

    return ret;
}

int app_display_draw_fill_color(
                   frame_buffer_t *dst_frame,
                   uint32_t color,
                   uint16_t dst_x,
                   uint16_t dst_y,
                   uint16_t fill_width,
                   uint16_t fill_height)
{
    int ret = BK_OK;

#ifdef CONFIG_APP_DISPLAY_DRAW_SOFTWARE
    LOGD("[%s][%d] fill color with software algorithm\r\n", __FUNCTION__, __LINE__);
    ret = app_sw_fill_color(dst_frame, color, dst_x, dst_y, fill_width, fill_height);
    if (BK_OK != ret)
    {
        LOGE("[%s][%d] app_sw_fill_color failed:%d\r\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
#elif defined(CONFIG_APP_DISPLAY_DRAW_DMA2D)
    LOGD("[%s][%d] fill color with hardware DMA2D\r\n", __FUNCTION__, __LINE__);
    ret = app_hw_dma2d_fill_color(dst_frame, color, dst_x, dst_y, fill_width, fill_height);
    if (BK_OK != ret)
    {
        LOGE("[%s][%d] app_dma2d_fill_color failed:%d\r\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
#else
    LOGW("[%s][%d] no display draw mode selected\r\n", __FUNCTION__, __LINE__);
    ret = BK_FAIL;
#endif

    return ret;
}

int app_display_draw_copy(
                   frame_buffer_t *src_frame,
                   frame_buffer_t *dst_frame,
                   uint16_t dst_x,
                   uint16_t dst_y)
{
    int ret = BK_OK;

#ifdef CONFIG_APP_DISPLAY_DRAW_SOFTWARE
    LOGD("[%s][%d] copy with software algorithm\r\n", __FUNCTION__, __LINE__);
    ret = app_sw_copy(src_frame, dst_frame, dst_x, dst_y);
    if (BK_OK != ret)
    {
        LOGE("[%s][%d] app_sw_copy failed:%d\r\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
#elif defined(CONFIG_APP_DISPLAY_DRAW_DMA2D)
    LOGD("[%s][%d] copy with hardware DMA2D\r\n", __FUNCTION__, __LINE__);
    ret = app_hw_dma2d_copy(src_frame, dst_frame, dst_x, dst_y);
    if (BK_OK != ret)
    {
        LOGE("[%s][%d] app_dma2d_copy failed:%d\r\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
#else
    LOGW("[%s][%d] no display draw mode selected\r\n", __FUNCTION__, __LINE__);
    ret = BK_FAIL;
#endif

    return ret;
}

int app_display_draw_blend(
                    frame_buffer_t *bg_frame,
                    frame_buffer_t *fg_frame,
                    uint16_t dst_x,
                    uint16_t dst_y)
{
    int ret = BK_OK;

#ifdef CONFIG_APP_DISPLAY_DRAW_SOFTWARE
    LOGD("[%s][%d] blend with software algorithm\r\n", __FUNCTION__, __LINE__);
    ret = app_sw_blend(bg_frame, fg_frame, dst_x, dst_y);
    if (BK_OK != ret)
    {
        LOGE("[%s][%d] app_sw_blend failed:%d\r\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
#elif defined(CONFIG_APP_DISPLAY_DRAW_DMA2D)
    LOGD("[%s][%d] blend with hardware DMA2D\r\n", __FUNCTION__, __LINE__);
    ret = app_hw_dma2d_blend(bg_frame, fg_frame, dst_x, dst_y);
    if (BK_OK != ret)
    {
        LOGE("[%s][%d] app_dma2d_blend failed:%d\r\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
#else
    LOGW("[%s][%d] no display draw mode selected\r\n", __FUNCTION__, __LINE__);
    ret = BK_FAIL;
#endif

    return ret;
}

int app_display_draw_font(
                    frame_buffer_t *bg_frame,
                    const char *text,
                    unsigned short font_color,
                    const gui_font_digit_struct *font_name,
                    int font_pos_x,
                    int font_pos_y)
{
    int ret = BK_OK;

    LOGD("[%s][%d] display font with software algorithm\r\n", __FUNCTION__, __LINE__);
    ret = app_sw_display_font(bg_frame, text, font_color, font_name, font_pos_x, font_pos_y);
    if (BK_OK != ret)
    {
        LOGE("[%s][%d] app_sw_display_font failed:%d\r\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return ret;
}

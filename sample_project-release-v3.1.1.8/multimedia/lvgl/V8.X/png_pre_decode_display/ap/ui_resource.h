#pragma once

#include <lvgl.h>

extern lv_img_dsc_t g_png_icon_dst;

/**
 * @brief 预加载ui需显示的图片，后续ui用时，不用解码，可提升帧率
 * @return 成功返回BK_OK
 */
s32 ui_resource_preload(void);


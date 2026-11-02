#pragma once

#include <lvgl.h>
#include <components/log.h>
#include <components/media_types.h>

#define RES_TAG "res"

#define RES_LOGI(...) BK_LOGI(RES_TAG, ##__VA_ARGS__)
#define RES_LOGE(...) BK_LOGE(RES_TAG, ##__VA_ARGS__)
#define RES_LOGD(...) BK_LOGD(RES_TAG, ##__VA_ARGS__)
#define RES_LOGW(...) BK_LOGW(RES_TAG, ##__VA_ARGS__)


#define IMG_BACKGROUND_NAME               "1.jpg"
#define IMG_ICON_NAME                     "icon.png"

/**
 * @brief UI资源信息结构体
 * 
 * 该结构体用于存储UI界面中使用的图片资源信息，包括图片名称、格式、尺寸、文件路径以及LVGL图片数据结构。
 * 用于统一管理UI界面中需要显示的图片资源，支持图片的预加载和快速获取。
 */
typedef struct{
    char             *image_name;       /*!< 图片名称，用于唯一标识图片资源 */
    pixel_format_t   fmt;               /*!< 图片像素格式，定义了图片数据的编码方式 */
    uint16           width;             /*!< 图片宽度，单位为像素 */
    uint16           height;            /*!< 图片高度，单位为像素 */
    char             *image_file_path;  /*!< 图片解码bin的路径 */
    lv_img_dsc_t     img_data;          /*!< LVGL图片数据结构，包含图片的像素数据和元信息 */
}ui_resource_infor_t;

/**
 * @brief 根据图片名称获取LVGL图片数据结构
 * 
 * 通过图片名称查找对应的图片资源，并返回LVGL图片数据结构指针，用于在LVGL界面中显示图片。
 * 
 * @param image_name 图片名称，用于标识要获取的图片资源
 * @return 成功返回LVGL图片数据结构指针，失败返回NULL
 */
lv_img_dsc_t *ui_resource_get_image_data(const char *image_name);

/**
 * @brief 根据图片名称释放LVGL图片数据结构占用的内存
 * 
 * 通过图片名称查找对应的图片资源，并释放LVGL图片数据结构占用的内存。
 * 
 * @param image_name 图片名称，用于标识要释放的图片资源
 * @return 成功返回BK_OK，失败返回BK_FAIL
 */
s32 ui_resource_free_image_data(const char *image_name);

/**
 * @brief 预加载ui需显示的图片，后续ui用时，不用再从flash读取装载psram，可提升帧率
 * @return 成功返回BK_OK
 */
s32 ui_resource_preload(void);


#pragma once

#include <components/log.h>

#define FONT_TAG "font"

#define FONT_LOGI(...) BK_LOGI(FONT_TAG, ##__VA_ARGS__)
#define FONT_LOGE(...) BK_LOGE(FONT_TAG, ##__VA_ARGS__)
#define FONT_LOGD(...) BK_LOGD(FONT_TAG, ##__VA_ARGS__)
#define FONT_LOGW(...) BK_LOGW(FONT_TAG, ##__VA_ARGS__)

#define FONT_BUFF_SIZE             (32*1024)

typedef enum{
    FONT_SIMHEI_16,
    FONT_SIMHEI_24
}FONT_NAME_E;

typedef struct{
    char version[4];
    uint32_t file1_size;
    uint32_t file1_offset;
    uint32_t file2_size;
    uint32_t file2_offset;
}font_bin_info_t;

/**
 * @brief 根据字体名称获取字体在Flash中的起始地址
 * 
 * 该函数用于获取指定字体在Flash分区中的实际起始地址，以便进行字体数据的读取和使用。
 * 字体数据存储在BK_PARTITION_USR_FONT分区中，函数会根据字体类型返回对应的偏移地址。
 * 
 * @param font_name 字体名称枚举值
 *        - FONT_SIMHEI_16: 16号字
 *        - FONT_SIMHEI_24: 24号字
 * 
 * @return 字体在Flash中的起始地址
 *         - 成功：返回实际的Flash起始地址
 *         - 失败：返回0
 */
uint32_t app_get_font_startaddr(FONT_NAME_E font_name);

/**
 * @brief 根据字体名称获取LVGL字体对象
 * 
 * 该函数用于获取指定字体的lv_font_t类型对象，供LVGL图形库使用。
 * 
 * @param font_name 字体名称枚举值
 *        - FONT_SIMHEI_16: 16号字
 *        - FONT_SIMHEI_24: 24号字
 * 
 * @return LVGL字体对象指针
 *         - 成功：返回对应的lv_font_t对象指针
 *         - 失败：返回NULL
 */
lv_font_t* app_get_font(FONT_NAME_E font_name);


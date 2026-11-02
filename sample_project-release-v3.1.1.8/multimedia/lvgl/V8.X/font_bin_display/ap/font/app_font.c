#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <lvgl.h>
#include <driver/flash_partition.h>
#include <driver/flash.h>
#include "app_font.h"

static font_bin_info_t g_font_bin = {0};

extern lv_font_t simhei_font16;
extern lv_font_t simhei_font24;

/**
 * @brief 获取字库打包bin的包头信息
 * 
 * 该函数用于从Flash分区中读取字库打包bin的包头信息，包括版本、文件大小和偏移量等。
 * 函数会缓存读取的信息，避免重复读取Flash。
 * 
 * @return 字库打包bin的包头信息指针
 */
static font_bin_info_t *app_get_fontbin_info(void)
{
    bk_logic_partition_t *partition = partition = bk_flash_partition_get_info(BK_PARTITION_USR_FONT);

    if(g_font_bin.file2_offset == 0)
    {
        bk_flash_partition_read(BK_PARTITION_USR_FONT, (uint8_t *)&g_font_bin, 4, sizeof(g_font_bin));
        FONT_LOGI("[%s][%d] version:%s\r\n", __FUNCTION__, __LINE__, g_font_bin.version);
    }

    return &g_font_bin;
}

/**
 * @brief 根据字体名称获取字体在Flash中的起始地址
 * 
 * 该函数用于获取指定字体在Flash分区中的实际起始地址，以便进行字体数据的读取和使用。
 * 字体数据存储在BK_PARTITION_USR_FONT分区中，函数会根据字体类型返回对应的偏移地址。
 * 
 * @param font_name 字体名称枚举值
 *        - FONT_SIMHEI_16: 16号黑体字
 *        - FONT_SIMHEI_24: 24号黑体字
 * 
 * @return 字体在Flash中的起始地址
 *         - 成功：返回实际的Flash起始地址
 *         - 失败：返回0
 */
uint32_t app_get_font_startaddr(FONT_NAME_E font_name)
{
    bk_logic_partition_t *fontPartion = bk_flash_partition_get_info(BK_PARTITION_USR_FONT);
    font_bin_info_t *font_bin_info = app_get_fontbin_info();

    if(FONT_SIMHEI_16 == font_name)
    {
        return fontPartion->partition_start_addr + font_bin_info->file1_offset + 4;
    }
    else if(FONT_SIMHEI_24 == font_name)
    {
        return fontPartion->partition_start_addr + font_bin_info->file2_offset + 4;
    }

    return 0;
}

/**
 * @brief 根据字体名称获取LVGL字体对象
 * 
 * 该函数用于获取指定字体的lv_font_t类型对象，供LVGL图形库使用。
 * 
 * @param font_name 字体名称枚举值
 *        - FONT_SIMHEI_16: 16号黑体字
 *        - FONT_SIMHEI_24: 24号黑体字
 * 
 * @return LVGL字体对象指针
 *         - 成功：返回对应的lv_font_t对象指针
 *         - 失败：返回NULL
 */
lv_font_t* app_get_font(FONT_NAME_E font_name)
{
    if(FONT_SIMHEI_16 == font_name)
    {
        return &simhei_font16;
    }
    else if(FONT_SIMHEI_24 == font_name)
    {
        return &simhei_font24;
    }

    return NULL;
}


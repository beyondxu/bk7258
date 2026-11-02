#include <os/os.h>
#include <os/str.h>
#include "common.h"
#include "ui_resource.h"
#include "lv_img_utility.h"

lv_img_dsc_t g_png_icon_dst = {0};

/**
 * @brief 预加载ui需显示的图片，后续ui用时，不用解码，可提升帧率
 * @return 成功返回BK_OK
 */
s32 ui_resource_preload(void)
{
    lv_png_img_load(VFS_MOUNT_PATH"/img/icon.png", &g_png_icon_dst);

    return BK_OK;
}

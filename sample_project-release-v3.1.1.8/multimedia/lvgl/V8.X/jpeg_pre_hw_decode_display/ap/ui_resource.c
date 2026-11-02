#include <os/os.h>
#include <os/str.h>
#include "common.h"
#include "ui_resource.h"
#include "lv_img_utility.h"

lv_img_dsc_t g_bg_img_dst = {0};

/**
 * @brief 预加载ui需显示的图片，后续ui用时，不用解码，可提升帧率
 * @return 成功返回BK_OK
 */
s32 ui_resource_preload(void)
{
    lv_jpeg_img_load_with_hw_dec(VFS_MOUNT_PATH"/img/yuv422/1_422.jpg", &g_bg_img_dst, false);

    return BK_OK;
}

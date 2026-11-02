#include <os/os.h>
#include <os/str.h>
#include "common.h"
#include "ui_screen1.h"

lv_obj_t *ui_Screen1;

/**
 * @brief ui第一个界面初始化
 * @return 无
 */
void ui_screen1_init(void)
{
    ui_Screen1 = lv_obj_create(NULL);
    
    lv_obj_clear_flag( ui_Screen1, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
    lv_obj_set_style_radius(ui_Screen1, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Screen1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT );
    lv_obj_set_style_bg_opa(ui_Screen1, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
    lv_obj_set_width(ui_Screen1, LV_HOR_RES);
    lv_obj_set_height(ui_Screen1, LV_VER_RES);

    lv_obj_t *ui_img = lv_gif_create(ui_Screen1);
    lv_gif_set_src(ui_img, VFS_MOUNT_PATH"/img/test.gif");
    lv_obj_set_align( ui_img, LV_ALIGN_CENTER);
}

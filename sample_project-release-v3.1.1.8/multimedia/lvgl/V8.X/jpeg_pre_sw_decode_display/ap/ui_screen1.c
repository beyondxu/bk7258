#include <os/os.h>
#include <os/str.h>
#include "common.h"
#include "ui_screen1.h"
#include "ui_resource.h"

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
    lv_obj_set_style_bg_color(ui_Screen1, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT );
    lv_obj_set_style_bg_opa(ui_Screen1, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
    lv_obj_set_width(ui_Screen1, LV_HOR_RES);
    lv_obj_set_height(ui_Screen1, LV_VER_RES);

    lv_obj_t *ui_img = lv_img_create(ui_Screen1);
    lv_img_set_src(ui_img, &g_bg_img_dst);
    lv_obj_set_width( ui_img, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height( ui_img, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_x( ui_img, 0 );
    lv_obj_set_y( ui_img, 0 );
    lv_obj_add_flag( ui_img, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
    lv_obj_clear_flag( ui_img, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
}

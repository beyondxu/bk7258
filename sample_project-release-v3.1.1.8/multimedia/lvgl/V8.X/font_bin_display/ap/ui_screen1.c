#include <os/os.h>
#include <os/str.h>
#include "common.h"
#include "ui_screen1.h"
#include "app_font.h"

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

    lv_obj_t *panel = lv_obj_create(ui_Screen1);
    lv_obj_clear_flag( panel, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
    lv_obj_set_style_radius(panel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0xe9e9e9), LV_PART_MAIN | LV_STATE_DEFAULT );
    lv_obj_set_style_bg_opa(panel, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(panel, 10, 0);
    lv_obj_set_width(panel, LV_HOR_RES-20);
    lv_obj_set_height(panel, LV_VER_RES-20);
    lv_obj_set_align(panel, LV_ALIGN_CENTER);

    lv_obj_t *ui_label = lv_label_create(panel);
    lv_label_set_text(ui_label, "博通集成");
    lv_obj_set_width(ui_label, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_label, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_label, LV_ALIGN_TOP_MID);
    lv_obj_set_style_text_color(ui_label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_label, app_get_font(FONT_SIMHEI_24), LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_label = lv_label_create(panel);
    lv_label_set_long_mode(ui_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(ui_label, "博通集成电路（上海）股份有限公司（股票代码：603068.SH）是全球领先的无线连接芯片设计企业，致力于为物联网应用提供尖端芯片解决方案。公司成立于2004年，始终坚持以技术创新驱动发展，引领物联网时代互联互通的未来。博通集成电路凭借世界一流的RF-CMOS集成电路设计能力和先进的数字信号处理技术，为客户提供高性能、低功耗的系统级芯片（SoC）解决方案，广泛应用于各种物联网领域，实现万物互联的愿景。公司产品组合全面支持多种无线协议和通信标准，赋能客户打造新一代智能互联设备。");
    lv_obj_set_pos(ui_label, 0, 50);
    lv_obj_set_width(ui_label, LV_HOR_RES-20);
    lv_obj_set_height(ui_label, LV_VER_RES-20);
    lv_obj_set_align(ui_label, LV_ALIGN_LEFT_MID);
    lv_obj_set_style_text_color(ui_label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_label, app_get_font(FONT_SIMHEI_16), LV_PART_MAIN | LV_STATE_DEFAULT);
}

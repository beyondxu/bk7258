#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include <modules/wifi.h>
#include <components/log.h>
#include <components/netif.h>
#include <components/event.h>
#include <bk_private/bk_init.h>
#include "driver/flash_partition.h"
#include "components/ate.h"
#include "components/media_types.h"

#include "common.h"
#include "dev_lcd.h"
#include "lvgl_app_ui.h"
#include "lv_vendor.h"
#include "ui_screen1.h"

#define TAG "main"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

int main(void)
{
    bk_init();
    media_service_init();

    if(1/*0 == ate_is_enabled()*/)   //非ATE模式才启动应用代码
    {
        dev_lcd_open();                    //打开lcd

        //lvgl初始化
        lvgl_app_init(dev_lcd_get_device_handle(),
                      dev_lcd_get_width(), 
                      dev_lcd_get_height(), 
                      ROTATE_NONE);

        // 第一个ui界面加载，因为这段代码运行不属于lvgl线程，所以需要加互斥锁
        lv_vendor_disp_lock();
        ui_screen1_init();                 //首界面初始化
        lv_disp_load_scr(ui_Screen1);      //显示首界面
        lv_vendor_disp_unlock();

        rtos_delay_milliseconds(100);      //加点延时，让ui绘制完
        dev_lcd_backlight_ctrl(100);       //延后开背光，规避屏内部初始化花屏等问题

        TEST_PASS();
    }

    return 0;
}

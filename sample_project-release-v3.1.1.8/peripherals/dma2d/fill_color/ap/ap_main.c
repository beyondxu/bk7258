#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include <modules/wifi.h>
#include <components/log.h>
#include <components/netif.h>
#include <components/event.h>
#include <bk_private/bk_init.h>
#include "bk_posix.h"
#include "driver/flash_partition.h"
#include "components/ate.h"
#include "components/media_types.h"
#include "frame_buffer_app.h"
#include "app_display_draw.h"

#include "common.h"
#include "dev_lcd.h"

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

        frame_buffer_t *bg_frame = NULL;
        int lcd_width = dev_lcd_get_width();
        int lcd_height = dev_lcd_get_height();
        int fill_x_pos;
        int fill_y_pos;
        bk_err_t ret = BK_FAIL;
        
        bg_frame = decode_frame_malloc(lcd_width * lcd_height * 2);
        if(NULL == bg_frame)
        {
            LOGE("[%s][%d] malloc fail\r\n", __FUNCTION__, __LINE__);
            TEST_FAIL();
            return BK_FAIL;
        }

        bg_frame->fmt = PIXEL_FMT_RGB565;
        bg_frame->width = lcd_width;
        bg_frame->height = lcd_height;

        app_display_draw_init(); 

        fill_x_pos = 0;
        fill_y_pos = 0;
        ret = app_display_draw_fill_color(bg_frame, 0xf800, fill_x_pos, fill_y_pos, lcd_width, lcd_height);
        if(BK_OK != ret)
        {
            LOGE("[%s][%d] fill fail\r\n", __FUNCTION__, __LINE__);
            TEST_FAIL();
            app_display_draw_deinit();
            return BK_FAIL;
        }

        fill_x_pos = 200;
        fill_y_pos = 200;
        ret = app_display_draw_fill_color(bg_frame, 0xffff, fill_x_pos, fill_y_pos, lcd_width - fill_x_pos, lcd_height - fill_y_pos);
        
        if(BK_OK != ret)
        {
            LOGE("[%s][%d] fill fail\r\n", __FUNCTION__, __LINE__);
            TEST_FAIL();
            app_display_draw_deinit();
            return BK_FAIL;
        }

        dev_lcd_flush(bg_frame, display_free_callback);   //刷屏

        dev_lcd_backlight_ctrl(100);       //开背光

        app_display_draw_deinit();

        TEST_PASS();
    }

    return 0;
}

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

#define FG_WIDTH    200
#define FG_HEIGHT   200

int main(void)
{
    bk_init();
    media_service_init();

    if(1/*0 == ate_is_enabled()*/)   
    {
        dev_lcd_open();                   // 打开LCD设备

        frame_buffer_t *bg_frame = NULL;
        frame_buffer_t *fg_frame = NULL;
        int lcd_width = dev_lcd_get_width();  // 获取LCD宽度
        int lcd_height = dev_lcd_get_height(); // 获取LCD高度
        int copy_x_pos;
        int copy_y_pos;
        bk_err_t ret = BK_FAIL;
        
        // 分配背景帧缓冲区内存
        bg_frame = decode_frame_malloc(lcd_width * lcd_height * 2);
        if(NULL == bg_frame)
        {
            LOGE("[%s][%d] malloc bg_frame fail\r\n", __FUNCTION__, __LINE__);
            TEST_FAIL();
            return BK_FAIL;
        }

        app_display_draw_init();     // 显示绘制组件初始化
		
        bg_frame->fmt = PIXEL_FMT_RGB565;  // 设置背景帧格式为RGB565
        bg_frame->width = lcd_width;
        bg_frame->height = lcd_height;

        // 分配前景帧缓冲区内存
        fg_frame = decode_frame_malloc(FG_WIDTH * FG_HEIGHT * 2);
        if(NULL == fg_frame)
        {
            LOGE("[%s][%d] malloc fg_frame fail\r\n", __FUNCTION__, __LINE__);
            TEST_FAIL();
            return BK_FAIL;
        }

        fg_frame->fmt = PIXEL_FMT_RGB565;  // 设置前景帧格式为RGB565
        fg_frame->width = FG_WIDTH;
        fg_frame->height = FG_HEIGHT;

        // 使用DMA2D填充背景颜色（蓝色 0x001f）
        ret = app_display_draw_fill_color(bg_frame, 0X001f, 0, 0, lcd_width, lcd_height);
        if(BK_OK != ret)
        {
            LOGE("[%s][%d] fill bg fail\r\n", __FUNCTION__, __LINE__);
            app_display_draw_deinit();   // 显示绘制组件反初始化
            TEST_FAIL();
            return BK_FAIL;
        }

        // 使用DMA2D填充前景颜色（白色 0xffff）
        ret = app_display_draw_fill_color(fg_frame, 0Xffff, 0, 0, FG_WIDTH, FG_HEIGHT);
        if(BK_OK != ret)
        {
            LOGE("[%s][%d] fill fg fail\r\n", __FUNCTION__, __LINE__);
            app_display_draw_deinit();   // 显示绘制组件反初始化
            TEST_FAIL();
            return BK_FAIL;
        }

        // 计算前景帧在背景帧中的中心位置
        copy_x_pos = (lcd_width - FG_WIDTH) / 2;
        copy_y_pos = (lcd_height - FG_HEIGHT) / 2;
        // 使用DMA2D将前景帧复制到背景帧中心
        ret = app_display_draw_copy(fg_frame, bg_frame, copy_x_pos, copy_y_pos);
        
        if(BK_OK != ret)
        {
            LOGE("[%s][%d] copy fail\r\n", __FUNCTION__, __LINE__);
            app_display_draw_deinit();   // 显示绘制组件反初始化
            TEST_FAIL();
            return BK_FAIL;
        }

        app_display_draw_deinit();   // 显示绘制组件反初始化

        dev_lcd_flush(bg_frame, display_free_callback);   // 刷新LCD显示

        dev_lcd_backlight_ctrl(100);  // 控制背光亮度为100%       

        TEST_PASS();
    }

    return 0;
}

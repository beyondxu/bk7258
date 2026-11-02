#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include <modules/wifi.h>
#include <components/log.h>
#include <components/netif.h>
#include <components/event.h>
#include <bk_private/bk_init.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include "dev_uart.h"

// 日志标签定义
#define TAG "uart"

// 日志输出宏定义
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// 测试通过和失败的宏定义
#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

/**
 * @brief 主函数
 * @return 0
 * @note 程序入口，完成系统初始化并启动UART测试
 */
int main(void)
{
    // 初始化BK系统
    bk_init();
    // 初始化媒体服务
    media_service_init();

    LOGI("[%s][%d] hello world, this is bk7258\r\n", __FUNCTION__, __LINE__);
    TEST_PASS();

    return 0;
}

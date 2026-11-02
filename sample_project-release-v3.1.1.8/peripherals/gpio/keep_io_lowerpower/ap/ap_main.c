#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include "bk_private/bk_init.h"
#include <components/log.h>

#include "components/bluetooth/bk_dm_bluetooth.h"
#include "driver/gpio.h"
#include "gpio_driver.h"
#include "pm_demo.h"
#define TAG "keep_io_lowerpower"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

/**
 * @brief: 配置GPIO为输出模式, 初始化 GPIO 输出低电平
 * @param: io - GPIO引脚号
 * @return: 无
*/
static void gpio_output_config(gpio_id_t io)
{
    // 取消GPIO映射
    gpio_dev_unmap(io);
    // 配置GPIO为输出模式,
    gpio_config_t io_config = {0};
    io_config.io_mode = GPIO_OUTPUT_ENABLE;
    io_config.func_mode = GPIO_SECOND_FUNC_DISABLE;
    io_config.pull_mode = GPIO_PULL_DISABLE;
    bk_gpio_set_config(io, &io_config);
    bk_gpio_set_output_low(io);
}

int main(void)
{
    bk_init();
    media_service_init();

    int ret = BK_OK;

    // 对比 GPIO_24 和 GPIO_25 的输出电平
    gpio_output_config(GPIO_24);
    gpio_output_config(GPIO_25);

    bk_gpio_set_output_high(GPIO_24);
    bk_gpio_set_output_high(GPIO_25);

    // 初始化PM任务
    ret = pm_task_init();
    if(ret != BK_OK)
    {
        LOGE("pm_task_init failed\r\n");
        TEST_FAIL();
        return ret;
    }
    // 关闭蓝牙
    bk_bluetooth_deinit();
    // psram 没使用的时候默认会下电，使用了psram 后电流会高大约 200 ua

    // 配置 GPIO_25 为低功耗保持状态
    gpio_config_t io_config ;
    io_config.io_mode = GPIO_OUTPUT_ENABLE;
    io_config.pull_mode = GPIO_PULL_DISABLE;
    io_config.func_mode = GPIO_SECOND_FUNC_DISABLE;
    bk_gpio_register_lowpower_keep_status(GPIO_25, &io_config);
    rtos_delay_milliseconds(2000);
    // 进入低功耗模式
    ret = app_enter_lowpower(30000, 1, GPIO_8, GPIO_INT_TYPE_FALLING_EDGE);
    if(ret != BK_OK)
    {
        LOGE("app_enter_lowpower failed\r\n");
        TEST_FAIL();
        return ret;
    }
    TEST_PASS();
    return 0;
}

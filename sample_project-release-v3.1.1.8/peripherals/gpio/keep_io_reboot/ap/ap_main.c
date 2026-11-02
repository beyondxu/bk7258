#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include "bk_private/bk_init.h"
#include <components/log.h>

#include "driver/gpio.h"
#include "gpio_driver.h"

#define TAG "keep_io_reboot"
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
    // 配置GPIO为输入模式, 禁用第二功能, 配置上拉/下拉电阻
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

    // 对比 GPIO_24 和 GPIO_25 的输出电平
    gpio_output_config(GPIO_24);
    gpio_output_config(GPIO_25);

    bk_gpio_set_output_high(GPIO_24);
    bk_gpio_set_output_high(GPIO_25);

    rtos_delay_milliseconds(2000);
    TEST_PASS();
    // 进入软重启
    bk_reboot();
    return 0;
}

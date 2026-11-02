#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include "bk_private/bk_init.h"
#include <components/log.h>

#include "driver/gpio.h"
#include "gpio_driver.h"

#define TAG "gpio_input"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

static beken_thread_t gpio_input_thread = NULL;

/**
 * @brief: GPIO中断服务函数
 * @param: io - GPIO引脚号
 * @return: 无
*/
static void gpio_int_isr(gpio_id_t io)
{
    LOGI("gpio_int_isr: %d\r\n", io);
}

/**
 * @brief: 配置GPIO为输入模式, 禁用第二功能, 配置上拉/下拉电阻, 配置GPIO中断类型
 * @param: io - GPIO引脚号
 * @param: pull_mode - 上拉/下拉电阻模式
 * @param: int_type - GPIO中断类型
 * @return: 无
*/
static void gpio_input_config(gpio_id_t io, gpio_pull_mode_t pull_mode,gpio_int_type_t int_type)
{
    // 取消GPIO映射
    gpio_dev_unmap(io);
    // 配置GPIO为输入模式, 禁用第二功能, 配置上拉/下拉电阻
    gpio_config_t io_config = {0};
    io_config.io_mode = GPIO_INPUT_ENABLE;
    io_config.func_mode = GPIO_SECOND_FUNC_DISABLE;
    io_config.pull_mode = pull_mode;
    bk_gpio_set_config(io, &io_config);
    // 配置GPIO中断类型
    if(int_type != GPIO_INT_TYPE_MAX)
    {
        bk_gpio_set_interrupt_type(io, int_type);
        bk_gpio_register_isr(io, gpio_int_isr);
        bk_gpio_enable_interrupt(io);
    }
}

/**
 * @brief: GPIO输入测试任务
 * @param: arg - 任务参数
 * @return: 无
*/
static void gpio_input_task(void *arg)
{
    // 配置GPIO为输入模式, 配置输入下拉, 配置GPIO中断类型为上升沿触发
    gpio_input_config(GPIO_25, GPIO_PULL_DOWN_EN, GPIO_INT_TYPE_RISING_EDGE);
    // 在任务中间隔 1s 读取一次GPIO输入值
    while(1)
    {
        uint32_t input_val = bk_gpio_get_input(GPIO_25);
        LOGI("gpio input test: input_val: %d \r\n", input_val);
        rtos_delay_milliseconds(1000);
    }
}

int main(void)
{
    bk_init();
    media_service_init();

    int ret = BK_OK;
    ret = rtos_create_thread(&gpio_input_thread, 6, "gpio_input_thread", gpio_input_task,  2048, NULL);
    if(ret != BK_OK)
    {
        TEST_FAIL();
        return -1;
    }
    TEST_PASS();
    return 0;
}

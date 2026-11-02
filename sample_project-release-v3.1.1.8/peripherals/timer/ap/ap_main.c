#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include "bk_private/bk_init.h"
#include <components/log.h>

#include <driver/timer.h>

#define TAG "Timer_example"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

#define TIMER_CHAN_ID       TIMER_ID3
#define TIMER_PRIORD_MS     500

static beken_thread_t timer_example_thread = NULL;
static volatile uint32_t timer_interrupt_count = 0;

/**
 * @brief 定时器中断服务函数
 * @param timer_id 定时器ID
 */
static void timer_example_isr(timer_id_t timer_id)
{
    timer_interrupt_count++;
    LOGI("timer_id %d interrupt count %d \r\n",  timer_id, timer_interrupt_count);
}

/**
 * @brief 定时器基础操作示例
 */
static void timer_base_operate(void)
{
    int ret = 0;
    
    // 初始化定时器驱动
    BK_LOG_ON_ERR(bk_timer_driver_init());

    // 启动定时器, 周期为 TIMER_PRIORD_MS 毫秒, 中断服务函数为 timer_example_isr
    ret = bk_timer_start(TIMER_CHAN_ID, TIMER_PRIORD_MS, timer_example_isr);
    if (ret != BK_OK)
    {
        LOGE("bk_timer_start failed, error code:%x\n", ret);
        TEST_FAIL();
        return;
    }
    LOGI("Starting timer channel %d with period %d ms\r\n", TIMER_CHAN_ID, TIMER_PRIORD_MS);

    int i = 0;
    // 定时器运行测试
    for (i = 0; i < 10; i++)
    {
        rtos_delay_milliseconds(1000);
        // 读取定时器计数值
        if (timer_interrupt_count >= 10)
        {
            LOGI("Received %d interrupts, basic timer example completed\r\n",
                           timer_interrupt_count);
            TEST_PASS();
            break;
        }
    }
    if (i == 10)
    {
        TEST_FAIL();
    }

    // 停止定时器
    bk_timer_stop(TIMER_CHAN_ID);
    LOGI("Basic timer operations example completed\r\n");
}

/**
 * @brief 定时器示例任务
 * @param argv 任务参数
 */
static void timer_example_task(beken_thread_arg_t arg)
{
    (void)arg;

    timer_base_operate();

    timer_example_thread = NULL;

    rtos_delete_thread(NULL);
}

int main(void)
{
    bk_init();
    media_service_init();

    if(1/*0 == ate_is_enabled()*/)   //非ATE模式才启动应用代码
    {
        int ret = rtos_create_thread(&timer_example_thread,
                                    6,
                                    "timer_example_task",
                                    timer_example_task,
                                    2048,
                                    NULL);
        if(ret != BK_OK)
        {
            LOGE("rtos_create_thread failed, ret:%x\r\n", ret);
            TEST_FAIL();
        }
        else
        {
            LOGI("rtos_create_thread success\r\n");
        }
    }
    return 0;
}

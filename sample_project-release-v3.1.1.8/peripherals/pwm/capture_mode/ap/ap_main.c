#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include "bk_private/bk_init.h"
#include <components/log.h>

#include <driver/pwm.h>
#define TAG "pwm_capture"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define TEST_PASS()     BK_LOGI(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "PASS")
#define TEST_FAIL()     BK_LOGE(TAG,"%s:%d:%s:%s\r\n", __FILE__, __LINE__, __FUNCTION__, "FAIL")

//  相邻两通道不能同时使用，如chan(0 1) (2 3) (4 5)
#define CLK_SOURCE      26000000                                //时钟频率
#define PERIOD_CYCLE    100                                     //周期周期数
#define PSC             25                                      //预分频系数
#define DUTY_CNT        40                                      //占空比周期数
#define FREQUENCY       CLK_SOURCE / PERIOD_CYCLE / (PSC + 1)   //PWM频率
#define _PERIOD_2_FREQ(period)   ((period == 0) ? (0) : (CLK_SOURCE / (period)))

static beken_thread_t pwm_capture_thread = NULL;

/**
 * @brief: PWM捕获中断服务函数
 * @param: chan - PWM通道号
 * @return: 无
*/
static void pwm_capture_isr(pwm_chan_t chan)
{
    LOGI("capture chan (%d)\n", chan);
}

/**
 * @brief: PWM捕获模式处理函数
 * @param: cap_chan_id - PWM捕获通道号
 * @return: 无
*/
static void pwm_capture_handle(uint8_t cap_chan_id)
{
    // 初始化并启动PWM捕获通道
    pwm_capture_init_config_t capture_config = {0};

    capture_config.edge = PWM_CAPTURE_NEG;              // 下降沿捕获
    capture_config.isr = NULL;
    bk_pwm_capture_init(cap_chan_id, &capture_config);
    bk_pwm_capture_start(cap_chan_id);

    // 获取 两次下降沿的周期数 计算出频率，
    uint32_t period_cycle = bk_pwm_capture_get_period_duty_cycle(cap_chan_id, 1000);                 // this api cann't be call in callback_isr,it has semphore operation which will coase assert
    LOGI("pwm_capture period_cycle:%d, freq:%dHz\n", period_cycle, _PERIOD_2_FREQ(period_cycle));
    // 停止并反初始化PWM捕获通道
    bk_pwm_capture_stop(cap_chan_id);
    bk_pwm_capture_deinit(cap_chan_id);

    capture_config.edge = PWM_CAPTURE_EDGE;             // 双边沿捕获
    capture_config.isr = NULL;
    bk_pwm_capture_init(cap_chan_id, &capture_config);
    bk_pwm_capture_start(cap_chan_id);

    // 获取到两个边沿的周期数 计算出占空比， 统计的高电平段
    uint32_t duty_cycle = bk_pwm_capture_get_period_duty_cycle(cap_chan_id, 1000);
    float duty_ratio = (period_cycle == 0) ? 0 : (float)duty_cycle / (float)period_cycle;
    LOGI("pwm_capture duty_cycle:%d, duty_ratio:%f\r\n", duty_cycle, duty_ratio);
    bk_pwm_capture_stop(cap_chan_id);
    bk_pwm_capture_deinit(cap_chan_id);
}

/**
 * @brief: PWM捕获任务函数
 * @param: arg - PWM捕获通道号指针
 * @return: 无
*/
static void pwm_capture_task(beken_thread_arg_t arg)
{
    if (arg == NULL)
    {
        LOGE("Invalid argument in pwm_capture_task\n");
        return;
    }
    uint8_t cap_chan_id = *(uint8_t *)arg;
    LOGI("pwm capture task chan_id: %d\n", cap_chan_id);
    while (1)
    {
        pwm_capture_handle(cap_chan_id);
        rtos_delay_milliseconds(1000);
    }
}

/**
 * @brief: PWM周期输出启动函数
 * @param: chan_id - PWM通道号
 * @return: BK_OK - 成功
*/
static int pwm_period_output_start(pwm_chan_t chan_id)
{
    int ret = BK_OK;
    pwm_init_config_t pwm_config = {0};
    pwm_config.period_cycle = PERIOD_CYCLE;
    pwm_config.psc = PSC;
    pwm_config.duty_cycle = DUTY_CNT;
    pwm_config.duty2_cycle = 0;
    pwm_config.duty3_cycle = 0;
    // 初始化PWM通道
    ret = bk_pwm_init(chan_id, &pwm_config);
    if(ret != BK_OK) {
        LOGE("pwm init fail, ret = %d\n", ret);
        return ret;
    }
    // 设置PWM初始信号为低电平
    ret = bk_pwm_set_init_signal_low(chan_id);
    if(ret != BK_OK) {
        LOGE("pwm set init signal low fail, ret = %d\n", ret);
        return ret;
    }
    // 启动PWM输出
    ret = bk_pwm_start(chan_id);
    if(ret != BK_OK) {
        LOGE("pwm start fail, ret = %d\n", ret);
        return ret;
    }
    LOGI("pwm output start success, Channel: %d\n", chan_id);
    LOGI("pwm output frequency: %dHz, duty: %d%%\n", FREQUENCY, DUTY_CNT);
    return ret;
}

/**
 * @brief: PWM捕获模式启动函数
 * @param: cap_chan_id - PWM捕获通道号
 * @return: 无
*/
static int pwm_capture_mode_start(pwm_chan_t cap_chan_id)
{
    uint8_t *chan = os_malloc(sizeof(uint8_t));
    *chan = (uint8_t)cap_chan_id;
    bk_err_t ret = rtos_create_thread(&pwm_capture_thread,
                                        6,
                                        "pwm_capture_task",
                                        (beken_thread_function_t)pwm_capture_task,
                                        2048,
                                        (void *)chan);
    if (ret != kNoErr)
    {
        BK_LOGE(NULL, "Error: Failed to create pwm_capture_task: %d\r\n", ret);
    }
    return ret;
}

int main(void)
{
    bk_init();
    media_service_init();
    if(1/*0 == ate_is_enabled()*/)   //非ATE模式才启动应用代码
    {
        int ret = BK_OK;
        bk_pwm_driver_init();                   // 初始化PWM驱动
        ret = pwm_period_output_start(0);       // 启动PWM周期输出
        if(ret != BK_OK) {
            LOGE("pwm period output start fail, ret = %d\n", ret);
            TEST_FAIL();
            return ret;
        }
        ret = pwm_capture_mode_start(2);        // 启动PWM捕获模式
        if(ret != BK_OK) {
            LOGE("pwm capture mode start fail, ret = %d\n", ret);
            TEST_FAIL();
            return ret;
        }
        TEST_PASS();
    }
    return 0;
}

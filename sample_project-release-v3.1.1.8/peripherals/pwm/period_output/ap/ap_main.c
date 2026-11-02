#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <media_service.h>
#include "bk_private/bk_init.h"
#include <components/log.h>

#include <driver/pwm.h>
#define TAG "pwm_output"
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
        TEST_PASS();
    }
    return 0;
}

#pragma once
/**
 * @brief: 初始化PM任务
 * @param: 无
 * @return: BK_OK - 成功
*/
int pm_task_init(void);
/**
 * @brief: 反初始化PM任务
 * @param: 无
 * @return: BK_OK - 成功
*/
int pm_task_deinit(void);
/**
 * @brief: 进入低功耗模式
 * @param: rtc_time - 低功耗时间, 单位: 毫秒
 * @param: rtc_repeat_cnt - 低功耗重复次数, 0: 无限循环
 * @param: wakeup_io - 唤醒IO引脚号
 * @param: int_type - 唤醒IO中断类型
 * @return: BK_OK - 成功
*/
int app_enter_lowpower(uint32_t rtc_time, uint32_t rtc_repeat_cnt, gpio_id_t wakeup_io, gpio_int_type_t int_type);
/**
 * @brief: 进入深睡模式
 * @param: rtc_time - 深睡时间, 单位: 毫秒
 * @param: rtc_repeat_cnt - 深睡重复次数, 0: 无限循环
 * @param: wakeup_io - 唤醒IO引脚号
 * @param: int_type - 唤醒IO中断类型
 * @return: BK_OK - 成功
*/
int app_enter_deepsleep(uint32_t rtc_time, uint32_t rtc_repeat_cnt, gpio_id_t wakeup_io, gpio_int_type_t int_type);

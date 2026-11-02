#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include "driver/pm_ap_core.h"
#include <modules/pm.h>

#define TAG "pm_demo"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

typedef enum{
    LOWVOL_SLEEP_HANDLE = 0,
    DEEP_SLEEP_HANDLE,
    WAKEUP_HANDLE,
    PM_TASK_EXIT,
} pm_queue_event_e;

typedef struct{
    pm_queue_event_e event;
    int param1;
    int param2;
    int param3;
    int param4;
} pm_queue_msg_t;

static beken_thread_t pm_process_thread = NULL;
static beken_queue_t pm_message_queue = NULL;

/**
 * @brief: 发送消息到 PM 消息队列
 * @param: msg - 指向 pm_queue_msg_t 结构体的指针, 包含要发送的消息
 * @return: BK_OK 表示成功, 其他值表示失败
*/
static int pm_queue_send_msg(pm_queue_msg_t *msg)
{
    int ret = BK_OK;
    if(msg == NULL)
    {
        LOGE("pm msg param is NULL\r\n");
        return BK_FAIL;
    }
    if (pm_message_queue)
    {
        ret = rtos_push_to_queue(&pm_message_queue, msg, BEKEN_NO_WAIT);
        if(ret != BK_OK)
        {
            LOGE("push msg to queue failed\r\n");
        }
    }
    return ret;
}

/**
 * @brief: GPIO 中断回调函数
 * @param: gpio_id - 触发中断的 GPIO 引脚号
 * @return: 无
*/
static void pm_demo_gpio_callback(gpio_id_t gpio_id)
{
    pm_queue_msg_t msg ={0};
    msg.event = WAKEUP_HANDLE;
    msg.param1 = PM_MODE_LOW_VOLTAGE;
    msg.param2 = PM_WAKEUP_SOURCE_INT_GPIO;
    msg.param3 = gpio_id;
    msg.param4 = 0;
    int ret = pm_queue_send_msg(&msg);
    if (ret != BK_OK)
    {
        LOGE("push msg to queue failed\r\n");
    }
}

/**
 * @brief: RTC 中断回调函数
 * @param: sleep_mode - 低功耗模式
 * @param: wake_source - 唤醒源
 * @param: param_p - 指向参数的指针
 * @return: 无
*/
static int pm_demo_rtc_callback(pm_sleep_mode_e sleep_mode,pm_wakeup_source_e wake_source,void* param_p)
{
    pm_queue_msg_t msg ={0};
    msg.event = WAKEUP_HANDLE;
    msg.param1 = PM_MODE_LOW_VOLTAGE;
    msg.param2 = PM_WAKEUP_SOURCE_INT_RTC;
    msg.param3 = 0;
    msg.param4 = 0;
    int ret = pm_queue_send_msg(&msg);
    if (ret != BK_OK)
    {
        LOGE("push msg to queue failed\r\n");
        return BK_FAIL;
    }
    return BK_OK;
}


/**
 * @brief: 进入低功耗模式
 * @param: rtc_time - 低功耗时间，单位：毫秒
 * @param: rtc_repeat_cnt - 低功耗重复次数，0 表示无限次
 * @param: wakeup_io - 触发唤醒的 GPIO 引脚号
 * @param: int_type - GPIO 中断类型
 * @return: 无
*/
int app_enter_lowpower(uint32_t rtc_time, uint32_t rtc_repeat_cnt, gpio_id_t wakeup_io, gpio_int_type_t int_type)
{
    pm_queue_msg_t msg ={0};
    
    msg.event = LOWVOL_SLEEP_HANDLE;
    msg.param1 = rtc_time;
    msg.param2 = rtc_repeat_cnt;
    msg.param3 = wakeup_io;
    msg.param4 = int_type;
    int ret = pm_queue_send_msg(&msg);
    if (ret != BK_OK)
    {
        LOGE("push msg to queue failed\r\n");
        return ret;
    }
    return BK_OK;
}
/**
 * @brief: 进入深度睡眠模式
 * @param: rtc_time - 深度睡眠时间，单位：毫秒
 * @param: rtc_repeat_cnt - 深度睡眠重复次数，0 表示无限次
 * @param: wakeup_io - 触发唤醒的 GPIO 引脚号
 * @param: int_type - GPIO 中断类型
 * @return: 无
*/
int app_enter_deepsleep(uint32_t rtc_time, uint32_t rtc_repeat_cnt, gpio_id_t wakeup_io, gpio_int_type_t int_type)
{
    pm_queue_msg_t msg ={0};
    msg.event = DEEP_SLEEP_HANDLE;
    msg.param1 = rtc_time;
    msg.param2 = rtc_repeat_cnt;
    msg.param3 = wakeup_io;
    msg.param4 = int_type;
    int ret = pm_queue_send_msg(&msg);
    if (ret != BK_OK)
    {
        LOGE("push msg to queue failed\r\n");
        return ret;
    }
    return BK_OK;
}

/**
 * @brief: PM 处理任务
 * @param: arg - 任务参数
 * @return: 无
*/
static void pm_proecess_task(void *arg)
{
    int ret = BK_OK;
    pm_queue_msg_t msg = {0};
    pm_ap_rtc_info_t low_power_info           = {0};
    uint32_t           rtc_sleep_time         = 0;
    uint32_t           rtc_sleep_repeat_count = 0;
    // gpio 低功耗配置
              
    uint32_t           gpio_id                = 0;
    uint32_t           gpio_wakeup_int_type   = 0;
    while (1)
    {
        ret = rtos_pop_from_queue(&pm_message_queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret == BK_OK)
        {
            switch (msg.event)
            {
            case DEEP_SLEEP_HANDLE:
                // rtc 低功耗配置
                rtc_sleep_time         = msg.param1;
                rtc_sleep_repeat_count = msg.param2;
                // gpio 低功耗配置
                #if CONFIG_GPIO_WAKEUP_SUPPORT
                gpio_id                = msg.param3;
                gpio_wakeup_int_type   = msg.param4;
                #endif //CONFIG_GPIO_WAKEUP_SUPPORT
                if(rtc_sleep_time != 0)
                {
                    rtc_sleep_time = 50*1000;//default 50s
                }
                if(rtc_sleep_repeat_count == 0)
                {
                    rtc_sleep_repeat_count = 1;//default 1 time
                }
                low_power_info.period_tick                = rtc_sleep_time;//unit:ms;
                low_power_info.period_cnt                 = rtc_sleep_repeat_count;//total period count == 0Xffffffff means forever period;else period how many times
                low_power_info.callback                   = pm_demo_rtc_callback;
                low_power_info.param_p                    = NULL;
                bk_pm_ap_rtc_regsiter_wakeup(PM_MODE_DEEP_SLEEP,&low_power_info);
         
                #if CONFIG_GPIO_WAKEUP_SUPPORT
                bk_gpio_register_isr(gpio_id, pm_demo_gpio_callback);
                bk_gpio_register_wakeup_source(gpio_id, gpio_wakeup_int_type);
                #endif //CONFIG_GPIO_WAKEUP_SUPPORT

                bk_pm_ap_sleep_mode_set(PM_MODE_DEEP_SLEEP);
                LOGI("[%s][%d]enter deep sleep mode\r\n",__func__,__LINE__);
                break;
                
            case LOWVOL_SLEEP_HANDLE:
                rtc_sleep_time         = msg.param1;
                rtc_sleep_repeat_count = msg.param2;
                #if CONFIG_GPIO_WAKEUP_SUPPORT
                gpio_id                = msg.param3;
                gpio_wakeup_int_type   = msg.param4;
                #endif //CONFIG_GPIO_WAKEUP_SUPPORT
                if(rtc_sleep_time == 0)
                {
                    rtc_sleep_time = 50*1000;//default 50s
                }
                if(rtc_sleep_repeat_count == 0)
                {
                    rtc_sleep_repeat_count = 1;//default 1 time
                }
                low_power_info.period_tick                = rtc_sleep_time;//unit:ms;
                low_power_info.period_cnt                 = rtc_sleep_repeat_count;//total period count == 0Xffffffff means forever period;else period how many times
                low_power_info.callback                   = pm_demo_rtc_callback;
                low_power_info.param_p                    = NULL;
                bk_pm_ap_rtc_regsiter_wakeup(PM_MODE_DEEP_SLEEP,&low_power_info);

                #if CONFIG_GPIO_WAKEUP_SUPPORT
                bk_gpio_register_isr(gpio_id, pm_demo_gpio_callback);
                bk_gpio_register_wakeup_source(gpio_id, gpio_wakeup_int_type);
                #endif //CONFIG_GPIO_WAKEUP_SUPPORT

                bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x1,0x0);
                LOGI("[%s][%d]enter low power mode\r\n",__func__,__LINE__);
                break;

            case WAKEUP_HANDLE:  
                if(msg.param1 == PM_MODE_LOW_VOLTAGE)
                {
                    bk_pm_ap_sleep_mode_set(PM_MODE_DEFAULT);
                    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x0,0x0);
                    LOGI("[%s][%d]wakeup mode %d wakeup source %d\r\n",__func__,__LINE__, msg.param1, msg.param2);
                }
                break;
            case PM_TASK_EXIT:
                LOGI("[%s][%d]exit pm process thread\r\n",__func__,__LINE__);
                break;
            default:
                break;
            }
        }
    }
    rtos_deinit_queue(&pm_message_queue);
    pm_message_queue = NULL;
    pm_process_thread = NULL;
    rtos_delete_thread(NULL);
}

int pm_task_init(void)
{
    int ret = BK_OK;
    ret = rtos_init_queue(&pm_message_queue, "pm_msg_queue", sizeof(pm_queue_msg_t), 10);
    if(ret != BK_OK)
    {
        LOGE("init pm msg queue failed\r\n");
        return -1;
    }
    ret = rtos_create_thread(&pm_process_thread, 6, "pm_process_thread", pm_proecess_task, 2048, NULL);
    if(ret != BK_OK)
    {
        LOGE("create pm process thread failed\r\n");
    
        return -1;
    }
    return ret;
}

int pm_task_deinit(void)
{
    int ret = BK_OK;
    pm_queue_msg_t msg ={0};
    msg.event = PM_TASK_EXIT;
    ret = pm_queue_send_msg(&msg);
    if (ret != BK_OK)
    {
        LOGE("push msg to queue failed\r\n");
    }
    return ret;
}
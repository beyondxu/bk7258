#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>

#define APP_TIMEOUT_VALUE    BEKEN_WAIT_FOREVER

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern int bk_cli_init(void);
extern void bk_set_jtag_mode(uint32_t cpu_id, uint32_t group_id);


#if CONFIG_FREERTOS_SMP
static beken_semaphore_t app_semaphore;

static void cpu1_test_task(void *arg)
{
    BK_LOGD(NULL, "===cpu1_test_task===:\r\n");
    for(;;) {
        rtos_get_semaphore(&app_semaphore, BEKEN_WAIT_FOREVER);
        BK_LOGD(NULL, "cpu1_test_task run core: %d\r\n", rtos_get_core_id());
    }
}

static void cpu2_test_task(void *arg)
{
    BK_LOGD(NULL, "cpu2_test_task run core: %d\r\n", rtos_get_core_id());
  
    for(;;) {
        rtos_set_semaphore(&app_semaphore);
        BK_LOGD(NULL, "cpu2_test_task run core: %d\r\n", rtos_get_core_id());
        rtos_delay_milliseconds(1000);
    }
}

void app_test_smp_core0(void)
{
    int ret;
    beken_thread_t cpu1_thread;

    /* create a semaphore */
    ret = rtos_init_semaphore(&app_semaphore, 5);
    if (ret != kNoErr) {
        BK_LOGD(NULL, "Error: Failed to init app_semaphore: %d\r\n",ret);
    }

    /* create a thread on core 0 */
    ret = rtos_core0_create_thread(&cpu1_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "cpu1_test_task",
                             (beken_thread_function_t)cpu1_test_task,
                             2048,
                             0);
    if (ret != kNoErr) {
        BK_LOGD(NULL, "Error: Failed to create cpu1_test_task: %d\r\n",ret);
    }
}

void app_test_smp_core1(void)
{
    int ret;
    beken_thread_t cpu2_thread;
    
    /* create a thread on core 1 */
    ret = rtos_core1_create_thread(&cpu2_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "cpu2_test_task",
                             (beken_thread_function_t)cpu2_test_task ,
                             2048,
                             0);
    if (ret != kNoErr) {
        BK_LOGD(NULL, "Error: Failed to create cpu1_test_task: %d\r\n",ret);
    }
}
#endif


// 【关键】请查阅 BK7258 数据手册，填入硬件定时器的实际输入频率
// 通常这类定时器挂载在固定的外部晶振（如 26MHz）上，不受 CPU 变频影响
#ifndef HW_TIMER_INPUT_CLK_HZ
#define HW_TIMER_INPUT_CLK_HZ 26000000UL 
#endif

// 选择您系统中空闲的定时器（强烈建议避开 Timer0 和 Timer1，它们常被 RTOS 占用）
// 这里以 Timer3 为例，请根据实际 SDK 定义修改
#define DELAY_TIMER_BASE    SOC_TIMER3_REG_BASE 
#define DELAY_TIMER_EN_BIT  BIT(0)

/**
 * @brief 初始化用于延时的硬件定时器
 * @note  必须在系统启动时（main函数开头）调用一次
 */
void hw_timer_delay_init(void)
{
    // 检查定时器是否已经运行，如果没有则启动它（自由计数模式）
    if ((REG_READ(DELAY_TIMER_BASE + (7 << 2)) & DELAY_TIMER_EN_BIT) == 0) {
        // 配置定时器：不分频，自由计数模式，并使能
        // 注意：这里的宏是博通 SDK 典型的定时器配置宏，请确保您的 SDK 支持
        TIMER0_REG_SET(3, 0, 0, 1); 
    }
}

/**
 * @brief 基于硬件定时器的精确微秒延时 (最终版)
 * @param us 需要延时的微秒数 (1 ~ 10000)
 */
static inline void hw_timer_delay_us(volatile uint32_t us)
{
    // 1. 计算需要等待的硬件时钟周期数
    // 例如：26MHz 下，1us = 26 个周期
    uint32_t delay_cycles = (HW_TIMER_INPUT_CLK_HZ / 1000000UL) * us;
    
    uint32_t begin, end, diff;

    // 2. 【防中断抢占】在极短延时期间，临时关闭全局中断
    // 防止 RTOS 的 Tick 或 Wi-Fi/BT 底层中断打断延时，导致时间被拉长
    uint32_t cpu_sr = bk_irq_disable(); 

    // 3. 获取硬件定时器的初始计数值
    begin = REG_READ(DELAY_TIMER_BASE); 

    // 4. 硬件轮询等待
    do {
        end = REG_READ(DELAY_TIMER_BASE);
        diff = end - begin; // 无符号减法，天然兼容 32位寄存器溢出回绕
    } while (diff < delay_cycles);

    // 5. 恢复中断状态
    bk_irq_enable(cpu_sr);
}
int main(void)
{
	

		bk_init();
		BK_LOGE( NULL, "333 main \n" );
		
	pm_cpu_freq_e current_freq = 0;
	current_freq=bk_pm_module_current_cpu_freq_get(PM_DEV_ID_GPIO);
		BK_LOGE( NULL,"00 current  cpu freq: %d;\r\n",current_freq);
		
		bk_pm_module_vote_cpu_freq(PM_CPU_FRQ_DEFAULT, PM_CPU_FRQ_480M);
		current_freq=bk_pm_module_current_cpu_freq_get(PM_DEV_ID_GPIO); 
		BK_LOGE(NULL, " 11 current	cpu freq: %d;\r\n",current_freq);
#if CONFIG_FREERTOS_SMP_TEST
    app_test_smp_core0();
    app_test_smp_core1();
#endif

	return 0;
}
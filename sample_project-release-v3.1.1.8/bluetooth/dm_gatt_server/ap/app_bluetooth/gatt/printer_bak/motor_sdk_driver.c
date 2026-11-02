#include "motor_sdk_driver.h"
#include "bk_misc.h"
#include <stdio.h>

#define SET_GPIO_HIGH(pin)   GPIO_UP(pin)
#define SET_GPIO_LOW(pin)    GPIO_DOWN(pin)
#define SET_GPIO_LEVEL(pin, level) do { \
    if (level) GPIO_UP(pin); \
    else GPIO_DOWN(pin); \
} while(0)

#define TEST_LOGE(...)   printf("[MOTOR_ERR] " __VA_ARGS__)
#define TEST_LOGI(...)   printf("[MOTOR_INFO] " __VA_ARGS__)

extern bk_err_t gpio_dev_unmap(gpio_id_t gpio_id);
__IRAM_SEC void bk_delay_us(UINT32 us);

// 精确微秒延时
__IRAM_SEC void delay_us(uint32_t us) {
    for (volatile uint32_t i = 0; i < us * 2; i++)
		__asm volatile("nop \n");
//	bk_delay_us(us);
}



static void safe_gpio_config_output(gpio_id_t pin, gpio_pull_mode_t pull_mode) {
    gpio_dev_unmap(pin); 
    gpio_config_t cfg = {
        .io_mode = GPIO_OUTPUT_ENABLE,
        .pull_mode = pull_mode,
        .func_mode = GPIO_SECOND_FUNC_DISABLE
    };
    bk_gpio_set_config(pin, &cfg);
}

static void safe_gpio_config_input_pullup(gpio_id_t pin) {
    gpio_dev_unmap(pin); 
    gpio_config_t cfg = {
        .io_mode = GPIO_INPUT_ENABLE,
        .pull_mode = GPIO_PULL_UP_EN,
        .func_mode = GPIO_SECOND_FUNC_DISABLE
    };
    bk_gpio_set_config(pin, &cfg);
}

bk_err_t bk_motor_gpio_config_init(void) {
    bk_gpio_driver_init();

    // 15V 使能脚
    safe_gpio_config_output(PIN_VDD15V_EN, GPIO_PULL_DISABLE);
    SET_GPIO_LOW(PIN_VDD15V_EN); 

    // 字车电机 (U4)
    safe_gpio_config_output(TPH_MOT_STEP, GPIO_PULL_DISABLE);
    safe_gpio_config_output(TPH_MOT_DIR, GPIO_PULL_DISABLE);
    safe_gpio_config_output(TPH_MOT_EN, GPIO_PULL_DISABLE);
    safe_gpio_config_output(TPH_MOT_MS1, GPIO_PULL_DISABLE);
    safe_gpio_config_output(TPH_MOT_MS2, GPIO_PULL_DISABLE);
    safe_gpio_config_output(TPH_MOT_MS3, GPIO_PULL_DISABLE);
    safe_gpio_config_output(TPH_MOT_RES, GPIO_PULL_UP_EN);   
    safe_gpio_config_output(TPH_MOT_SLEEP, GPIO_PULL_DISABLE); 
    safe_gpio_config_input_pullup(TPH_MOT_nFA);

    // 进纸电机 (U5)
    safe_gpio_config_output(PAPER_MOT_STEP, GPIO_PULL_DISABLE);
    safe_gpio_config_output(PAPER_MOT_DIR, GPIO_PULL_DISABLE);
    safe_gpio_config_output(PAPER_MOT_EN, GPIO_PULL_DISABLE);
    safe_gpio_config_output(PAPER_MOT_MS1, GPIO_PULL_DISABLE);
    safe_gpio_config_output(PAPER_MOT_MS2, GPIO_PULL_DISABLE);
    safe_gpio_config_output(PAPER_MOT_MS3, GPIO_PULL_DISABLE);
    safe_gpio_config_output(PAPER_MOT_RES, GPIO_PULL_UP_EN);
    safe_gpio_config_output(PAPER_MOT_SLEEP, GPIO_PULL_DISABLE);
    safe_gpio_config_input_pullup(PAPER_MOT_nFA);

    // 设置初始休眠状态
    SET_GPIO_HIGH(TPH_MOT_EN);     
    SET_GPIO_LOW(TPH_MOT_SLEEP);   
    SET_GPIO_LOW(TPH_MOT_RES);     

    SET_GPIO_HIGH(PAPER_MOT_EN);   
    SET_GPIO_LOW(PAPER_MOT_SLEEP); 
    SET_GPIO_LOW(PAPER_MOT_RES);   

    return BK_OK;
}

void bk_motor_boost_power(uint8_t enable) {
    if (enable) {
        SET_GPIO_HIGH(PIN_VDD15V_EN); 
        rtos_delay_milliseconds(30); 
    } else {
        SET_GPIO_LOW(PIN_VDD15V_EN);  
    }
}

void bk_motor_set_state(uint8_t is_tph_motor, uint8_t active) {
    gpio_id_t pin_sleep = is_tph_motor ? TPH_MOT_SLEEP : PAPER_MOT_SLEEP;
    gpio_id_t pin_res   = is_tph_motor ? TPH_MOT_RES   : PAPER_MOT_RES;
    gpio_id_t pin_en    = is_tph_motor ? TPH_MOT_EN    : PAPER_MOT_EN;

    if (active) {
        SET_GPIO_HIGH(pin_sleep);       
        rtos_delay_milliseconds(5);    
        SET_GPIO_HIGH(pin_res);         
        SET_GPIO_LOW(pin_en);           
    } else {
        SET_GPIO_HIGH(pin_en);          
        SET_GPIO_LOW(pin_res);         
        SET_GPIO_LOW(pin_sleep);       
    }
}

void bk_motor_set_microstep(uint8_t is_tph_motor, MotorStepMode mode) {
    gpio_id_t pin_ms1 = is_tph_motor ? TPH_MOT_MS1 : PAPER_MOT_MS1;
    gpio_id_t pin_ms2 = is_tph_motor ? TPH_MOT_MS2 : PAPER_MOT_MS2;
    gpio_id_t pin_ms3 = is_tph_motor ? TPH_MOT_MS3 : PAPER_MOT_MS3;

    SET_GPIO_LEVEL(pin_ms1, (mode & 0x01));
    SET_GPIO_LEVEL(pin_ms2, ((mode >> 1) & 0x01));
    SET_GPIO_LEVEL(pin_ms3, ((mode >> 2) & 0x01));
}

// ★ 关键修正：移除脉冲循环内的 rtos_delay_milliseconds ★
bk_err_t bk_motor_move_run(uint8_t is_tph_motor, uint32_t steps, uint8_t direction, uint32_t speed_delay_us) {
    gpio_id_t pin_step = is_tph_motor ? TPH_MOT_STEP : PAPER_MOT_STEP;
    gpio_id_t pin_dir  = is_tph_motor ? TPH_MOT_DIR  : PAPER_MOT_DIR;
    gpio_id_t pin_nfa  = is_tph_motor ? TPH_MOT_nFA  : PAPER_MOT_nFA;
    gpio_id_t pin_en   = is_tph_motor ? TPH_MOT_EN   : PAPER_MOT_EN;

    SET_GPIO_LEVEL(pin_dir, direction);
    bk_delay_us(20); 

    for (uint32_t i = 0; i < steps; i++) {
        SET_GPIO_HIGH(pin_step);
        bk_delay_us(10); 
        SET_GPIO_LOW(pin_step);
        bk_delay_us(speed_delay_us);

        // 硬件故障保护检测（纯软件极速读取，不阻塞调度）
        if (bk_gpio_get_input(pin_nfa) == 0) {
            SET_GPIO_HIGH(pin_en); 
            TEST_LOGE("Motor Fault detected on Pin %d!\r\n", pin_nfa);
            return BK_FAIL; 
        }
    }
    return BK_OK;
}


#define U4_LOGI(...) printf("[U4_TEST_INFO] " __VA_ARGS__)
#define U4_LOGE(...) printf("[U4_TEST_ERR]  " __VA_ARGS__)

// 精确微秒延时 (针对 BK7258 高主频优化)
static void u4_delay_us(uint32_t us) {
    for (volatile uint32_t i = 0; i < us; i++) {
        for (volatile uint32_t j = 0; j < 35; j++) {
            __asm__ volatile ("nop");
        }
    }
}

/**
 * @brief U4 电机带梯形加减速的脉冲驱动函数 (消除异响与回退的核心)
 * 
 * @param steps          总移动微步数
 * @param direction      方向 (1: 正向, 0: 反向)
 * @param start_delay_us 起始延时 (微秒，数值越大速度越慢，例如 1200us)
 * @param target_delay_us 匀速段目标延时 (微秒，数值越小速度越快，例如 250us)
 * @param ramp_steps     加速/减速段所需的步数 (例如 300 步)
 */
bk_err_t u4_motor_move_ramped(uint32_t steps, uint8_t direction, 
                              uint32_t start_delay_us, uint32_t target_delay_us, 
                              uint32_t ramp_steps) 
{
    // 保护：如果总步数不够完成加减速，自动调整加速步数
    if (steps < ramp_steps * 2) {
        ramp_steps = steps / 2;
    }

    // 设置方向 Pin
    SET_GPIO_LEVEL(TPH_MOT_DIR, direction);
    u4_delay_us(20); // 方向建立时间

    uint32_t current_delay_us = start_delay_us;

    for (uint32_t i = 0; i < steps; i++) {
        
        // ----------------------------------------------------
        // 1. 动态计算梯形加减速脉冲间隔 (Delay)
        // ----------------------------------------------------
        if (i < ramp_steps) {
            // A. 加速段 (Ramp-up)：延时逐渐减小，速度越来越快
            current_delay_us = start_delay_us - ((start_delay_us - target_delay_us) * i / ramp_steps);
        } 
        else if (i >= (steps - ramp_steps)) {
            // B. 减速段 (Ramp-down)：延时逐渐增大，速度平稳慢下来
            uint32_t decel_step = steps - 1 - i;
            current_delay_us = start_delay_us - ((start_delay_us - target_delay_us) * decel_step / ramp_steps);
        } 
        else {
            // C. 匀速段 (Cruise)：保持目标最高打印速度
            current_delay_us = target_delay_us;
        }

        // ----------------------------------------------------
        // 2. 发送 STEP 物理脉冲
        // ----------------------------------------------------
        SET_GPIO_HIGH(TPH_MOT_STEP);
        u4_delay_us(5); // 脉冲保持时间 >= 1us
        SET_GPIO_LOW(TPH_MOT_STEP);
        u4_delay_us(current_delay_us);

        // ----------------------------------------------------
        // 3. 硬件过流/故障保护检测
        // ----------------------------------------------------
        if (bk_gpio_get_input(TPH_MOT_nFA) == 0) {
            SET_GPIO_HIGH(TPH_MOT_EN); // 立即关闭输出
            U4_LOGE("ATD5984 U4 Fault Triggered on Step %d!\r\n", i);
            return BK_FAIL;
        }
    }

    return BK_OK;
}

/**
 * @brief U4 字车电机独立模拟打印扫掠测试
 */
void u4_carriage_motor_standalone_test(void) {
    U4_LOGI("===========================================\r\n");
    U4_LOGI("Starting U4 Carriage Motor Precision Test...\r\n");
    U4_LOGI("===========================================\r\n");

    // 1. 初始化电机 GPIO
    bk_motor_gpio_config_init();

    // 2. 开启 15V 主升压供电
    U4_LOGI("Step 1: Boosting 15V Power...\r\n");
    bk_motor_boost_power(1);
    rtos_delay_milliseconds(50); // 必须等待 15V 彻底稳定

    // 3. 配置 ATD5984 (U4) 为 1/16 高精度微步细分模式 (MS1=1, MS2=1, MS3=1)
    // 细分越高，电机越平滑，噪音越小！
    U4_LOGI("Step 2: Configuring 1/16 Microstepping...\r\n");
    SET_GPIO_HIGH(TPH_MOT_MS1);
    SET_GPIO_HIGH(TPH_MOT_MS2);
    SET_GPIO_HIGH(TPH_MOT_MS3);

    // 4. 唤醒 U4 驱动芯片
    U4_LOGI("Step 3: Waking up U4 Driver...\r\n");
    SET_GPIO_HIGH(TPH_MOT_SLEEP); // 退出休眠
    rtos_delay_milliseconds(5);
    SET_GPIO_HIGH(TPH_MOT_RES);   // 退出复位
    SET_GPIO_LOW(TPH_MOT_EN);     // 开启 H 桥输出使能

    rtos_delay_milliseconds(10);

    // ----------------------------------------------------
    // 5. 模拟喷墨打印机的真实扫掠动作（往复移动 10 次）
    // ----------------------------------------------------
    // 参数匹配实际打印：
    // - 总步数: 3200 步 (在 1/16 细分下约对应 5~8 cm 移动距离)
    // - 起始速度延时: 1000us (慢速启动)
    // - 打印最高速度延时: 220us (高速扫掠)
    // - 加减速步数: 400 步
    
    for (int cycle = 1; cycle <= 10; cycle++) {
        U4_LOGI("Cycle %d/10: Moving FORWARD (Printing Left to Right)...\r\n", cycle);
        bk_err_t err = u4_motor_move_ramped(3200, 1, 1000, 220, 400);
        if (err != BK_OK) goto test_failed;

        rtos_delay_milliseconds(100); // 扫掠到右端，停顿 100ms

      //  U4_LOGI("Cycle %d/10: Moving BACKWARD (Returning Right to Left)...\r\n", cycle);
      //  err = u4_motor_move_ramped(3200, 0, 1000, 220, 400);
    //    if (err != BK_OK) goto test_failed;

        rtos_delay_milliseconds(200); // 退回左端原点，停顿 200ms
    }

    U4_LOGI("U4 Carriage Motor Test PASSED PERFECTLY!\r\n");

test_failed:
    // 安全下电：让电机进入低功耗休眠
    SET_GPIO_HIGH(TPH_MOT_EN);   // 关断输出
    SET_GPIO_LOW(TPH_MOT_SLEEP); // 休眠
    SET_GPIO_LOW(TPH_MOT_RES);
    bk_motor_boost_power(0);     // 关断 15V
}



static void u4_simple_delay_us(uint32_t us) {
    for (volatile uint32_t i = 0; i < us; i++) {
        for (volatile uint32_t j = 0; j < 35; j++) {
            __asm__ volatile ("nop");
        }
    }
}

// 纯粹发送脉冲的极简函数（不做任何电源和 Pin 配置）
static void u4_send_pulses_only(uint32_t total_steps, uint32_t speed_delay_us) {
    uint32_t int_mask = rtos_disable_int();

    for (uint32_t i = 0; i < total_steps; i++) {
        SET_GPIO_HIGH(TPH_MOT_STEP);
        u4_simple_delay_us(4);
        SET_GPIO_LOW(TPH_MOT_STEP);
        u4_simple_delay_us(speed_delay_us);
    }

    rtos_enable_int(int_mask);
}

// 正确的测试入口
void u4_correct_while_loop_test(void) {
    printf("\r\n===========================================\r\n");
    printf("U4 Motor Continuous Forward Test (Correct Architecture)\r\n");
    printf("===========================================\r\n");

    // ====================================================
    // 1. 初始化与上电（在 while(1) 外面！只做一次！）
    // ====================================================
    bk_motor_gpio_config_init();
    bk_motor_boost_power(1); // 开启 15V 主电源
    rtos_delay_milliseconds(50); // 必须等待 15V 彻底稳定

    // 配置 1/16 细分
    SET_GPIO_HIGH(TPH_MOT_MS1);
    SET_GPIO_HIGH(TPH_MOT_MS2);
    SET_GPIO_HIGH(TPH_MOT_MS3);

    // 唤醒 ATD5984 并保持常开
    SET_GPIO_HIGH(TPH_MOT_SLEEP); // 退出休眠
    rtos_delay_milliseconds(2);   // 等待电荷泵建立 VCP 高压（重要！）
    SET_GPIO_HIGH(TPH_MOT_RES);   // 退出复位
    
    // 锁定方向为 1 (正转)
    SET_GPIO_HIGH(TPH_MOT_DIR);   
    u4_simple_delay_us(20);

    // 开启输出使能
    SET_GPIO_LOW(TPH_MOT_EN);     
    rtos_delay_milliseconds(10);  // 建立静态锁紧磁场

    printf("Setup Complete. Entering Continuous Forward Loop...\r\n");

    // ====================================================
    // 2. while(1) 循环体（内部只发脉冲，不开关电源）
    // ====================================================
    while (1) {
        printf("Moving Forward 3200 steps...\r\n");

        // 匀速发送 3200 脉冲（正转 1 圈）
        u4_send_pulses_only(3200, 1200);

        // 转完 1 圈后停顿 1 秒（此时电机保持通电静态锁紧，相序不会乱）
        rtos_delay_milliseconds(1000); 
    }
}



#define U5_LOGI(...) printf("[U5_PAPER_INFO] " __VA_ARGS__)
#define U5_LOGE(...) printf("[U5_PAPER_ERR]  " __VA_ARGS__)

// U5 专属 NOP 微秒延时
static void u5_simple_delay_us(uint32_t us) {
    for (volatile uint32_t i = 0; i < us; i++) {
        for (volatile uint32_t j = 0; j < 35; j++) {
            __asm__ volatile ("nop");
        }
    }
}

/**
 * @brief U5 纯脉冲发送函数（关中断保护，不做任何电源/GPIO重新配置）
 */
static void u5_send_pulses_only(uint32_t total_steps, uint32_t speed_delay_us) {
    // 关中断保护，确保走纸脉冲周期绝对均匀
    uint32_t int_mask = rtos_disable_int();

    for (uint32_t i = 0; i < total_steps; i++) {
        SET_GPIO_HIGH(PAPER_MOT_STEP);
        u5_simple_delay_us(4); // 高电平保持 4us
        SET_GPIO_LOW(PAPER_MOT_STEP);
        u5_simple_delay_us(speed_delay_us); // 进纸脉冲间隔
    }

    rtos_enable_int(int_mask); // 恢复中断
}

/**
 * @brief 测试 1：U5 基础匀速连续进纸测试（验证相线、方向、基本运转）
 */
void u5_paper_motor_constant_speed_test(void) {
    U5_LOGI("===========================================\r\n");
    U5_LOGI("Starting U5 Paper Motor Fixed-Speed Test...\r\n");
    U5_LOGI("===========================================\r\n");

    // ----------------------------------------------------
    // 1. 初始化与硬件唤醒（在 while(1) 外面，只做一次！）
    // ----------------------------------------------------
    bk_motor_gpio_config_init();
    bk_motor_boost_power(1);      // 开启 15V 主升压供电
    rtos_delay_milliseconds(50);  // 等待 15V 电源稳定

    // 配置 U5 为 1/16 细分模式 (MS1=1, MS2=1, MS3=1)
    SET_GPIO_HIGH(PAPER_MOT_MS1);
    SET_GPIO_HIGH(PAPER_MOT_MS2);
    SET_GPIO_HIGH(PAPER_MOT_MS3);

    // 唤醒 ATD5984 (U5) 驱动芯片
    SET_GPIO_HIGH(PAPER_MOT_SLEEP); // 退出休眠
    rtos_delay_milliseconds(2);    // 必须等待 2ms 建立 VCP 电荷泵高压
    SET_GPIO_HIGH(PAPER_MOT_RES);    // 退出复位
    
    // 锁定正向进纸方向 (GPIO_55)
    SET_GPIO_LOW(PAPER_MOT_DIR);   
    u5_simple_delay_us(20);

    // 开启 U5 H桥输出使能 (GPIO_44)
    SET_GPIO_LOW(PAPER_MOT_EN);     
    rtos_delay_milliseconds(10);   // 建立初始锁紧磁场

    U5_LOGI("U5 Power & Hardware Ready. Entering Loop...\r\n");

    // ----------------------------------------------------
    // 2. 连续进纸循环（内强参数：2500us 安全慢速，防失步）
    // ----------------------------------------------------
    while (1) {
        U5_LOGI("Advancing Paper Forward 3200 steps...\r\n");

        // 安全慢速进纸：2500us/步（约 100Hz 步进频率，对 15BY25 极其安全）
        u5_send_pulses_only(3200, 500);

        // 走纸 1 圈后停顿 1 秒，保持 EN=0（锁定力矩，防止纸张退滑）
        rtos_delay_milliseconds(1000); 
    }
}

/**
 * @brief 测试 2：U5 模拟真实打印逐行进纸测试（Line Feed Mode）
 */
void u5_paper_motor_line_feed_test(void) {
    U5_LOGI("===========================================\r\n");
    U5_LOGI("Starting U5 Simulated Line-Feed Test...\r\n");
    U5_LOGI("===========================================\r\n");

    // 初始化硬件 (仅一次)
    bk_motor_gpio_config_init();
    bk_motor_boost_power(1);
    rtos_delay_milliseconds(50);

    SET_GPIO_HIGH(PAPER_MOT_MS1);
    SET_GPIO_HIGH(PAPER_MOT_MS2);
    SET_GPIO_HIGH(PAPER_MOT_MS3);

    SET_GPIO_HIGH(PAPER_MOT_SLEEP);
    rtos_delay_milliseconds(2);
    SET_GPIO_HIGH(PAPER_MOT_RES);
    
    SET_GPIO_LOW(PAPER_MOT_DIR); // 向走纸
    u5_simple_delay_us(20);

    SET_GPIO_LOW(PAPER_MOT_EN);   // 开启输出
    rtos_delay_milliseconds(10);

    // 模拟连续打印 10 行纸张递进
    for (int line = 1; line <= 1000; line++) {
        U5_LOGI("[Line %d/1000] Advancing 160 microsteps...\r\n", line);

        // 模拟打印一行后进纸 160 步，速度 1800us/步
        u5_send_pulses_only(1600, 100);

        // 停顿 200ms 模拟喷头打火过程
        rtos_delay_milliseconds(20);
    }

    // 走纸测试完成，安全关闭使能防发热
    SET_GPIO_HIGH(PAPER_MOT_EN);   // 关断 H 桥
    SET_GPIO_LOW(PAPER_MOT_SLEEP); // 休眠
    bk_motor_boost_power(0);       // 关断 15V
    U5_LOGI("Line-Feed Simulation Completed Successfully.\r\n");
}



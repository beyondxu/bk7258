#ifndef MOTOR_SDK_DRIVER_H
#define MOTOR_SDK_DRIVER_H

#include <common/bk_include.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>





// ==========================================
// 1. 电源与电机驱动 GPIO 物理引脚定义 (采用标准枚举)
// ==========================================



// 喷墨头移动电机 (U4) 引脚
// 15V 升压电路使能引脚 (U3)
#define PIN_VDD15V_EN          GPIO_13

// 喷墨头移动电机 (U4) 引脚
#define TPH_MOT_EN             GPIO_49
#define TPH_MOT_STEP           GPIO_19
#define TPH_MOT_DIR            GPIO_50
#define TPH_MOT_MS1            GPIO_23
#define TPH_MOT_MS2            GPIO_22
#define TPH_MOT_MS3            GPIO_15
#define TPH_MOT_RES            GPIO_21
#define TPH_MOT_SLEEP          GPIO_26
#define TPH_MOT_nFA            GPIO_48


// 进纸电机 (U5) 引脚
#define PAPER_MOT_EN           GPIO_44
#define PAPER_MOT_STEP         GPIO_18
#define PAPER_MOT_DIR          GPIO_55
#define PAPER_MOT_MS1          GPIO_46
#define PAPER_MOT_MS2          GPIO_47
#define PAPER_MOT_MS3          GPIO_17
#define PAPER_MOT_RES          GPIO_53
#define PAPER_MOT_SLEEP        GPIO_54
#define PAPER_MOT_nFA          GPIO_45

// 细分模式配置
typedef enum {
    MODE_FULL_STEP     = 0, 
    MODE_HALF_STEP     = 1, 
    MODE_1_32_STEP     = 2, 
    MODE_1_16_STEP     = 3, 
    MODE_QUARTER_STEP  = 5, 
    MODE_1_8_STEP      = 6  
} MotorStepMode;

extern void delay_us(uint32_t us);




// 延时函数（通常直接调用 Beken SDK 内置的 rtos_delay_milliseconds 等）

void bk_motor_boost_power(uint8_t enable);
bk_err_t bk_motor_gpio_config_init(void);
void bk_motor_set_state(uint8_t is_tph_motor, uint8_t active);
void bk_motor_set_microstep(uint8_t is_tph_motor, MotorStepMode mode);
bk_err_t bk_motor_move_run(uint8_t is_tph_motor, uint32_t steps, uint8_t direction, uint32_t speed_delay_us);
// ==========================================
// 打印流水线统一电机驱动接口 (单测验证通过的稳定版本)
// ==========================================

// 1. 双电机统一上电唤醒 (15V升压稳定 + 1/16细分 + 退出休眠使能)
void motor_all_power_up_and_enable(void);
bk_err_t motor_page_feed(uint32_t steps, uint8_t direction, uint32_t *sent);
/* Caller owns an already enabled boost; never toggles it. */
bk_err_t motor_page_feed_keep_power(uint32_t steps, uint8_t direction, uint32_t *sent);
void motor_carriage_test_power_up(void);
bk_err_t motor_carriage_test_leg(uint8_t direction, uint32_t steps, uint32_t *sent);

// 2. 进纸电机控制 (forward: 1=正向进纸, 0=反向退纸)
void motor_feed_paper(uint32_t steps, uint8_t forward);

// 3. 字车电机步进 (打印扫掠时每列移动指定微步)
void motor_carriage_step(uint32_t steps, uint32_t speed_delay_us);

// 4. 字车电机快速归位 (高速返回原点)
void motor_carriage_return_home(uint32_t steps);

// 5. 打印完成安全下电休眠 (关断15V防发热)
void motor_all_power_down(void);


void motor_u5_boot_scope_test(void);

#endif // MOTOR_SDK_DRIVER_H


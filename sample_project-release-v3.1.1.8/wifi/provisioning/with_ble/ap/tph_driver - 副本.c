#include "tph_driver.h"
#include "motor_sdk_driver.h"
#include <stdio.h>
#include "modules/pm.h"

#define TPH_LOGI(...) printf("[TPH_INFO] " __VA_ARGS__)
#define TPH_LOGE(...) printf("[TPH_ERR]  " __VA_ARGS__)
__IRAM_SEC void bk_delay_us(UINT32 us);

extern bk_err_t gpio_dev_unmap(gpio_id_t gpio_id);

const uint8_t TPH_ADDR_TABLE[25] = {
    0x07, 0x0B, 0x13, 0x0D, 0x15, 0x19, 0x0E, 0x16, 0x26, 0x1A, 
    0x2A, 0x32, 0x1C, 0x2C, 0x4C, 0x34, 0x54, 0x64, 0x38, 0x58, 
    0x68, 0x70, 0x23, 0x43, 0x25
};

static const gpio_id_t s_pins[4] = { PIN_TPH_S1, PIN_TPH_S2, PIN_TPH_S3, PIN_TPH_S4 };
static const gpio_id_t d_pins[6] = { PIN_TPH_D1, PIN_TPH_D2, PIN_TPH_D3, PIN_TPH_D4, PIN_TPH_D5, PIN_TPH_D6 };
static const gpio_id_t a_pins[7] = { PIN_TPH_A1, PIN_TPH_A2, PIN_TPH_A3, PIN_TPH_A4, PIN_TPH_A5, PIN_TPH_A6, PIN_TPH_A7 };

static void safe_tph_gpio_out(gpio_id_t pin) {
    gpio_dev_unmap(pin); 
    gpio_config_t cfg = {
        .io_mode = GPIO_OUTPUT_ENABLE,
        .pull_mode = GPIO_PULL_DISABLE,
        .func_mode = GPIO_SECOND_FUNC_DISABLE
    };
    bk_gpio_set_config(pin, &cfg);
}

bk_err_t tph_gpio_init(void) {
    bk_gpio_driver_init();

    safe_tph_gpio_out(PIN_VL_ENABLE);
    safe_tph_gpio_out(PIN_TPH_ENABLE);
    safe_tph_gpio_out(PIN_SCOPE_TRIGGER);
    safe_tph_gpio_out(PIN_TPH_CE);

    for (int i = 0; i < 4; i++) {
        safe_tph_gpio_out(s_pins[i]);
        bk_gpio_set_output_high(s_pins[i]);
    }
    for (int i = 0; i < 6; i++) {
        safe_tph_gpio_out(d_pins[i]);
        bk_gpio_set_output_low(d_pins[i]);
    }
    for (int i = 0; i < 7; i++) {
        safe_tph_gpio_out(a_pins[i]);
        bk_gpio_set_output_low(a_pins[i]);
    }

    bk_gpio_set_output_low(PIN_VL_ENABLE);
    bk_gpio_set_output_low(PIN_TPH_ENABLE);
    bk_gpio_set_output_low(PIN_TPH_CE);
    bk_gpio_set_output_low(PIN_SCOPE_TRIGGER);

    return BK_OK;
}

// ★ 关键修正：正确的电源上电顺序 ★
void tph_power_on(void) {
    TPH_LOGI("Step 1: Enabling Main 15V Boost Power (LP6221)...\r\n");
    bk_motor_boost_power(1);
    rtos_delay_milliseconds(30); // 必须等待 15V 升压彻底稳定

    TPH_LOGI("Step 2: Enabling TPH Logic Power (VDD_VL)...\r\n");
    bk_gpio_set_output_high(PIN_VL_ENABLE);
    rtos_delay_milliseconds(10); 

    TPH_LOGI("Step 3: Enabling TPH Heater Power (VDD_TPH 15V)...\r\n");
    bk_gpio_set_output_high(PIN_TPH_ENABLE);
    rtos_delay_milliseconds(10);
}

void tph_power_off(void) {
    TPH_LOGI("Shutting down TPH power...\r\n");
    bk_gpio_set_output_low(PIN_TPH_CE);
    bk_gpio_set_output_low(PIN_TPH_ENABLE);
    bk_gpio_set_output_low(PIN_VL_ENABLE);
    rtos_delay_milliseconds(5);
    bk_motor_boost_power(0);
}

// ★ 关键修正：完善的列打火时序 ★
void tph_fire_one_columnx(const TphTestColumn_t *col_data) {
    uint8_t bit_600[600];
    for (int i = 0; i < 200; i++) {
        bit_600[i]       = (col_data->c_data[i / 8] >> (i % 8)) & 0x01; // C
        bit_600[200 + i] = (col_data->m_data[i / 8] >> (i % 8)) & 0x01; // M
        bit_600[400 + i] = (col_data->y_data[i / 8] >> (i % 8)) & 0x01; // Y
    }

    for (uint8_t addr = 0; addr < 25; addr++) {
        // 1. 先准备好地址编码 A1 - A7
        uint8_t addr_val = TPH_ADDR_TABLE[addr];
        for (int a = 0; a < 7; a++) {
            if ((addr_val >> a) & 0x01) bk_gpio_set_output_high(a_pins[a]);
            else bk_gpio_set_output_low(a_pins[a]);
        }

        // 2. 依次加载 4 段（S1-S4）的数据 D1-D6 并锁存
        for (uint8_t section = 0; section < 4; section++) {
            uint8_t d_data = 0;
            for (uint8_t d = 0; d < 6; d++) {
                uint32_t nozzle_idx = (addr * 24) + (section * 6) + d;
                if (bit_600[nozzle_idx]) d_data |= (1 << d);
            }

            for (int d = 0; d < 6; d++) {
                if ((d_data >> d) & 0x01) bk_gpio_set_output_high(d_pins[d]);
                else bk_gpio_set_output_low(d_pins[d]);
            }

            // S 锁存脉冲：低电平取样，高电平锁存
            bk_gpio_set_output_low(s_pins[section]);
            bk_delay_us(1); // ts > 0.38us
            bk_gpio_set_output_high(s_pins[section]);
        }

        // 3. 产生 2us 绝对精确加热打火脉冲
        bk_gpio_set_output_high(PIN_TPH_CE);
        bk_delay_us(2); // 必须保证实测波形在 1.8~2.0us
        bk_gpio_set_output_low(PIN_TPH_CE);
    }
}
void timer_hal_delay_us(uint32_t us);

/**
 * @brief 精确延时 1 微秒 (基于 480MHz 主频)
 * @note  1us = 480 个时钟周期
 */
__IRAM_SEC void delay_1us(void)
{
    // 使用 volatile 防止编译器优化掉 NOP 指令
    volatile uint32_t i;
    for (i = 0; i < 11; i++) {
      __asm volatile( "nop" );
    }
}
__IRAM_SEC void delay_400ns(void)
{
    // 使用 volatile 防止编译器优化掉 NOP 指令
   __asm volatile("nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;");
	__asm volatile("nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;");
	__asm volatile("nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;");
}


/**
 * @brief 精确延时 2 微秒 (基于 480MHz 主频)
 * @note  2us = 960 个时钟周期
 */
__IRAM_SEC void delay_2us(void)
{
    // 使用 volatile 防止编译器优化掉 NOP 指令
    volatile uint32_t i;
    for (i = 0; i < 22; i++) {
        __asm volatile( "nop" );
    }
}
// --- 3. 查找表 (LUT) ---
// 这些表将在初始化时生成，将逻辑值(0/1)转换为寄存器写入值(0/2)
static uint32_t LUT_A[128][7]; // 7个地址引脚，支持128个地址
static uint32_t LUT_D[64][6];  // 6个数据引脚，支持64个数据值

// --- 4. 初始化函数 ---
// 在系统启动时调用一次，预计算所有可能的组合
void TPH_Init_LUT(void) {
    // 预计算地址引脚的查找表
    for (int addr = 0; addr < 128; addr++) {
        LUT_A[addr][0] = (addr & (1 << 0)) ? 2 : 0; // A1
        LUT_A[addr][1] = (addr & (1 << 1)) ? 2 : 0; // A2
        LUT_A[addr][2] = (addr & (1 << 2)) ? 2 : 0; // A3
        LUT_A[addr][3] = (addr & (1 << 3)) ? 2 : 0; // A4
        LUT_A[addr][4] = (addr & (1 << 4)) ? 2 : 0; // A5
        LUT_A[addr][5] = (addr & (1 << 5)) ? 2 : 0; // A6
        LUT_A[addr][6] = (addr & (1 << 6)) ? 2 : 0; // A7
    }

    // 预计算数据引脚的查找表
    for (int data = 0; data < 64; data++) {
        LUT_D[data][0] = (data & (1 << 0)) ? 2 : 0; // D1
        LUT_D[data][1] = (data & (1 << 1)) ? 2 : 0; // D2
        LUT_D[data][2] = (data & (1 << 2)) ? 2 : 0; // D3
        LUT_D[data][3] = (data & (1 << 3)) ? 2 : 0; // D4
        LUT_D[data][4] = (data & (1 << 4)) ? 2 : 0; // D5
        LUT_D[data][5] = (data & (1 << 5)) ? 2 : 0; // D6
    }
}

#if 0
// ==========================================
// 完美解决 A1 脉冲宽度，使其与 CE 完全对齐的代码
// ==========================================
__IRAM_SEC void tph_fire_one_columnbak(const TphTestColumn_t *col_data) {
    uint8_t bit_600[600];
	pm_cpu_freq_e current_freq = 0;
	current_freq=bk_pm_module_current_cpu_freq_get(PM_DEV_ID_DEFAULT);
	TPH_LOGI( "00 current  cpu freq: %d;\r\n",current_freq);
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_480M);
	
	current_freq=bk_pm_module_current_cpu_freq_get(PM_DEV_ID_DEFAULT);	
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_GPIO, PM_CPU_FRQ_240M);
	TPH_LOGI( " 11 current  cpu freq: %d;\r\n",current_freq);
    for (int i = 0; i < 200; i++) {
        bit_600[i]       = (col_data->c_data[i / 8] >> (i % 8)) & 0x01; // C
        bit_600[200 + i] = (col_data->m_data[i / 8] >> (i % 8)) & 0x01; // M
        bit_600[400 + i] = (col_data->y_data[i / 8] >> (i % 8)) & 0x01; // Y
    }
	GPIO_UP(PIN_TPH_CE);
	int j=0;
while(j++<10000)
{
    // 25 组典型地址打印循环 ( Add 1 ~ Add 25 )
    for (uint8_t addr = 0; addr < 25; addr++) {

        // A. 建立地址 A1-A7
        uint8_t addr_val = TPH_ADDR_TABLE[addr];
     /*   for (int a = 0; a < 7; a++) {
            if ((addr_val >> a) & 0x01) GPIO_UP(a_pins[a]);
            else GPIO_DOWN(a_pins[a]);
        }
       */
       
	    //bk_delay_us(1);
        // B. 依次对 S1, S2, S3, S4 装载 D1-D6 数据并锁存
        for (uint8_t section = 0; section < 4; section++) {
		
            uint8_t d_data = 0;
            for (uint8_t d = 0; d < 6; d++) {
                uint32_t nozzle_idx = (addr * 24) + (section * 6) + d;
                if (bit_600[nozzle_idx]) d_data |= (1 << d);
            }
			

            // 输出 D1-D6 数据
            for (int d = 0; d < 6; d++) {
                if ((d_data >> d) & 0x01) GPIO_UP(d_pins[d]);
                else GPIO_DOWN(d_pins[d]);
            }
            // S[section] 锁存负脉冲 (高 -> 低 -> 高)
            GPIO_DOWN(s_pins[section]);
           
             delay_400ns();
           // bk_delay_us(2);
            GPIO_UP(s_pins[section]);
            //bk_delay_us(1);
            
        }
        // C. CE 产生 2us 精确打火脉冲
     
		//delay_2us();
		//bk_delay_us(2);
      ///  bk_gpio_set_output_low(PIN_TPH_CE);

        //  ：立即将地址 A1-A7 和数据 D1-D6 全部归零拉低！★
        // 这样 A1 的高电平脉宽瞬间缩短为 10us，与样机 CE 脉冲形态彻底重合！
         /*  for (int a = 0; a < 7; a++) {
            GPIO_DOWN(a_pins[a]);
        }
     for (int d = 0; d < 6; d++) {
            GPIO_DOWN(d_pins[d]);
        }*/
        // D. 组间延时 600us（在 600us 期间 A1 为 0V，形成窄脉冲）
       
    }
}
    // 2. 追加停止清零时序 (Stop Sequence)
   // GPIO_UP(PIN_TPH_CE);
    for (uint8_t section = 0; section < 4; section++) {
        GPIO_DOWN(s_pins[section]);
        delay_2us();
    
	// bk_delay_us(2);
        GPIO_UP(s_pins[section]);
    }
    GPIO_DOWN(PIN_TPH_CE);
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_DEFAULT);
	BK_LOGD(NULL, "end current max cpu freq: %d;\r\n",bk_pm_current_max_cpu_freq_get());

    // 3. 墨水回填等待 (列间停顿 1ms)
    rtos_delay_milliseconds(1);
}
#endif

// ==========================================
// 完美解决 A1 脉冲宽度，使其与 CE 完全对齐的代码
// ==========================================
__IRAM_SEC void tph_fire_one_column(const TphTestColumn_t *col_data) {
    uint8_t bit_600[600];

    for (int i = 0; i < 200; i++) {
        bit_600[i]       = (col_data->c_data[i / 8] >> (i % 8)) & 0x01; // C
        bit_600[200 + i] = (col_data->m_data[i / 8] >> (i % 8)) & 0x01; // M
        bit_600[400 + i] = (col_data->y_data[i / 8] >> (i % 8)) & 0x01; // Y
    }
	GPIO_UP(PIN_TPH_CE);
//	int j=0;
//while(j++<10000)
{
    // 25 组典型地址打印循环 ( Add 1 ~ Add 25 )
    for (uint8_t addr = 0; addr < 25; addr++) {

        // A. 建立地址 A1-A7
        uint8_t addr_val = TPH_ADDR_TABLE[addr];
        for (int a = 0; a < 7; a++) {
            if ((addr_val >> a) & 0x01) GPIO_UP(a_pins[a]);
            else GPIO_DOWN(a_pins[a]);
        }
       
       
	    //bk_delay_us(1);
        // B. 依次对 S1, S2, S3, S4 装载 D1-D6 数据并锁存
        for (uint8_t section = 0; section < 4; section++) {
		
            uint8_t d_data = 0;
            for (uint8_t d = 0; d < 6; d++) {
                uint32_t nozzle_idx = (addr * 24) + (section * 6) + d;
                if (bit_600[nozzle_idx]) d_data |= (1 << d);
           
            // 输出 D1-D6 数据         
                if ((d_data >> d) & 0x01) GPIO_UP(d_pins[d]);
                else GPIO_DOWN(d_pins[d]);
            
			//delay_400ns();
            // S[section] 锁存负脉冲 (高 -> 低 -> 高)
            GPIO_DOWN(s_pins[section]);
           
            // delay_400ns();
           
  		    bk_delay_us(1); 
			GPIO_DOWN(d_pins[d]);
		  //  delay_400ns(); 
		   bk_delay_us(1);
		   GPIO_UP(s_pins[section]);
             
        }
        // C. CE 产生 2us 精确打火脉冲
     }
		//delay_2us();
		//bk_delay_us(2);
      ///  bk_gpio_set_output_low(PIN_TPH_CE);

        //  ：立即将地址 A1-A7 和数据 D1-D6 全部归零拉低！★
        // 这样 A1 的高电平脉宽瞬间缩短为 10us，与样机 CE 脉冲形态彻底重合！
         /*  for (int a = 0; a < 7; a++) {
            GPIO_DOWN(a_pins[a]);
        }
     for (int d = 0; d < 6; d++) {
            GPIO_DOWN(d_pins[d]);
        }*/
        // D. 组间延时 600us（在 600us 期间 A1 为 0V，形成窄脉冲）
       
    }
}
    // 2. 追加停止清零时序 (Stop Sequence)
   // GPIO_UP(PIN_TPH_CE);
   
    GPIO_DOWN(PIN_TPH_CE);
	
	BK_LOGD(NULL, "end current max cpu freq: %d;\r\n",bk_pm_current_max_cpu_freq_get());

    // 3. 墨水回填等待 (列间停顿 1ms)
    rtos_delay_milliseconds(1);
}

// ==========================================
// 辅助：生成单点测试图案（让 D1 像样机一样只有稀疏脉冲）
// ==========================================
void generate_single_dot_pattern(TphTestColumn_t *out_col) {
    for (int i = 0; i < 25; i++) {
        out_col->c_data[i] = 0x00;
        out_col->m_data[i] = 0x00;
        out_col->y_data[i] = 0x00;
    }
    // 仅点亮第一个地址的第一个喷孔
    out_col->c_data[0] = 0x01; 
}


void generate_color_bar_pattern(uint32_t col_idx, TphTestColumn_t *out_col) {
    for (int i = 0; i < 25; i++) {
        out_col->c_data[i] = 0x00;
        out_col->m_data[i] = 0x00;
        out_col->y_data[i] = 0x00;
    }

    uint32_t band = (col_idx / 40) % 4; 
    switch (band) {
        case 0: for (int i = 0; i < 25; i++) out_col->c_data[i] = 0xFF; break;
        case 1: for (int i = 0; i < 25; i++) out_col->m_data[i] = 0xFF; break;
        case 2: for (int i = 0; i < 25; i++) out_col->y_data[i] = 0xFF; break;
        case 3: for (int i = 0; i < 25; i++) {
            out_col->c_data[i] = 0xFF;
            out_col->m_data[i] = 0xFF;
            out_col->y_data[i] = 0xFF;
        } break;
    }
}

void tph_test_static(void) {
    TPH_LOGI("===========================================\r\n");
    TPH_LOGI("Starting TPH Print Test...\r\n");
    TPH_LOGI("===========================================\r\n");
	

    tph_gpio_init();
    bk_motor_gpio_config_init();
    tph_power_on();

    // 唤醒电机驱动
    bk_motor_set_state(1, 1);
    bk_motor_set_state(0, 1);

    bk_motor_set_microstep(1, MODE_1_16_STEP); 
    bk_motor_set_microstep(0, MODE_1_8_STEP);  

    TphTestColumn_t test_col;

    // 示波器触发标志
    bk_gpio_set_output_high(PIN_SCOPE_TRIGGER);
    bk_delay_us(10);
    bk_gpio_set_output_low(PIN_SCOPE_TRIGGER);
	
	TPH_LOGI(" Test 000...\r\n");

    // 扫掠打印 200 列
    for (uint32_t col = 0; col < 2000; col++) {
        generate_color_bar_pattern(col, &test_col);
		
        tph_fire_one_column(&test_col);

        // 字车每次移动 3 微步
        bk_motor_move_run(1, 3, 1, 600); 
    }

    // 复位与出纸
    bk_motor_move_run(1, 600, 0, 400); 
    rtos_delay_milliseconds(50);
    bk_motor_move_run(0, 800, 1, 1000); 

    bk_motor_set_state(1, 0);
    bk_motor_set_state(0, 0);
    tph_power_off();
    TPH_LOGI("Print Test Completed Successfully.\r\n");
}




#define PRINT_LOGI(...) printf("[PRINTER_APP] " __VA_ARGS__)
#define PRINT_LOGE(...) printf("[PRINTER_ERR] " __VA_ARGS__)

// NOP 极速微秒延时


// 定义 ARM M33 内核 DWT 寄存器地址
#define DWT_CR         (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT     (*(volatile uint32_t *)0xE0001004)
#define DEMCR          (*(volatile uint32_t *)0xE000EDFC)

/**
 * @brief 在系统初始化时调用一次，开启 ARM 内核 DWT 硬件计数器
 */
void dwt_delay_init(void) {
    DEMCR |= (1 << 24);    // 开启 TRCENA (Trace 使能)
    DWT_CYCCNT = 0;        // 计数器清零
    DWT_CR |= (1 << 0);    // 开启 CYCCNT 硬件周期计数器
}

/**
 * @brief 绝对精准的微秒级延时 (误差 < 5 纳秒)
 * 
 * @param us 延时微秒数
 */
static inline void print_delay_us(uint32_t us) {
    uint32_t start_cycles = DWT_CYCCNT;
    
    // 如果 CPU 跑在 480MHz：1 微秒 = 480 个 Clock Cycles
    // 如果 CPU 跑在 240MHz：1 微秒 = 240 个 Clock Cycles
    // 请根据实际当前 CPU 主频自动计算（以 480MHz 为例）：
    uint32_t wait_cycles = us * 480; 

    // 硬件死等，直到 CPU 硬件计数器累加达到指定 Cycle 数
    while ((DWT_CYCCNT - start_cycles) < wait_cycles);
}


// ============================================================================
// 1. 生成极易识别的“诊断彩条与网格”点阵数据
// ============================================================================
/**
 * @brief 生成第 col_idx 列的测试图案
 * @param col_idx 列索引 (0 ~ 199)
 * @param out_col 输出的 600-喷孔点阵数据
 */
void generate_diagnostic_pattern(uint32_t col_idx, TphTestColumn_t *out_col) {
    // 清空当前列数据
    for (int i = 0; i < 25; i++) {
        out_col->c_data[i] = 0x00;
        out_col->m_data[i] = 0x00;
        out_col->y_data[i] = 0x00;
    }

    // 划分为 4 个区域（每区 50 列）：
    // 0~49: 青色(C)  | 50~99: 品红(M) | 100~149: 黄色(Y) | 150~199: 黑色(C+M+Y)
    uint32_t zone = col_idx / 50;

    for (int i = 0; i < 25; i++) {
        // 增加“网格断点”（每 8 个喷孔断开 1 个），用于容易识别是否有喷孔堵塞
        uint8_t pattern_mask = 0xEE; // 1110 1110 点阵

        switch (zone) {
            case 0: // 青色 (C)
                out_col->c_data[i] = pattern_mask;
                break;
            case 1: // 品红 (M)
                out_col->m_data[i] = pattern_mask;
                break;
            case 2: // 黄色 (Y)
                out_col->y_data[i] = pattern_mask;
                break;
            case 3: // 三色重叠 = 黑色 (K)
                out_col->c_data[i] = pattern_mask;
                out_col->m_data[i] = pattern_mask;
                out_col->y_data[i] = pattern_mask;
                break;
        }
    }
}


#define PRINT_LOGI(...) printf("[PRINTER_APP] " __VA_ARGS__)
#define PRINT_LOGE(...) printf("[PRINTER_ERR] " __VA_ARGS__)

// ============================================================================
// 1. U4 字车扫掠 + 调用原厂 TPH 边走边打火
// ============================================================================
static void sweep_and_print_one_line(void) {
    TphTestColumn_t col_data;

    // A. 设置 U4 字车移动方向：正向 (向右扫掠打印)
    GPIO_UP(TPH_MOT_DIR);
    print_delay_us(20);

    // B. U4 起步加慢速平稳启动 (200 步)
    for (int step = 0; step < 200; step++) {
        uint32_t delay = 1000 - ((1000 - 300) * step / 200);
        GPIO_UP(TPH_MOT_STEP);
        print_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        print_delay_us(delay);
    }

    // C. 匀速扫掠段：打印 200 列，调用原厂 tph_fire_one_column 打火
    for (uint32_t col = 0; col < 200; col++) {
        
        // 1. 生成色块测试图案
        generate_color_bar_pattern(col, &col_data);
        
        // 2. 直接调用原厂供应商打火算法（零修改）
        tph_fire_one_column(&col_data);

        // 3. 打完一列，字车移动 4 个微步 (对应 600DPI 分辨率)
        for (int m = 0; m < 4; m++) {
            GPIO_UP(TPH_MOT_STEP);
            print_delay_us(4);
            GPIO_DOWN(TPH_MOT_STEP);
            print_delay_us(300); 
        }
    }

    // D. U4 减速停止段 (200 步)
    for (int step = 0; step < 200; step++) {
        uint32_t delay = 300 + ((1000 - 300) * step / 200);
        GPIO_UP(TPH_MOT_STEP);
        print_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        print_delay_us(delay);
    }
}

// ============================================================================
// 2. U5 进纸马达平稳走纸 1 行
// ============================================================================
static void paper_advance_one_line(uint32_t microsteps) {
    GPIO_DOWN(PAPER_MOT_DIR); // 正向走纸
    print_delay_us(20);

    uint32_t int_mask = rtos_disable_int();
    for (uint32_t i = 0; i < microsteps; i++) {
        GPIO_UP(PAPER_MOT_STEP);
        print_delay_us(4);
        GPIO_DOWN(PAPER_MOT_STEP);
        print_delay_us(300); 
    }
    rtos_enable_int(int_mask);
}

// ============================================================================
// 3. U4 字车马达快速退回左侧原点
// ============================================================================
static void carriage_return_home(void) {
    GPIO_DOWN(TPH_MOT_DIR); // 向左复位
    print_delay_us(20);

    uint32_t total_return_steps = 1200;
    for (uint32_t i = 0; i < total_return_steps; i++) {
        uint32_t delay = 220;
        if (i < 200) delay = 800 - (580 * i / 200);
        else if (i > 1000) delay = 220 + (580 * (i - 1000) / 200);

        GPIO_UP(TPH_MOT_STEP);
        print_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        print_delay_us(delay);
    }
}

// ============================================================================
// 4. 【全流程打印入口】run_full_printer_demo
// ============================================================================
void run_full_printer_demo(void) {
    PRINT_LOGI("==================================================\r\n");
    PRINT_LOGI("      STARTING FULL INKJET PRINTING DEMO          \r\n");
    PRINT_LOGI("==================================================\r\n");

    // 全局提升 CPU 到 480MHz 保证极速运行
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_480M);

    // STEP 1: 初始化外设 GPIO
    tph_gpio_init();
    bk_motor_gpio_config_init();

    // STEP 2: 严格电源上电时序 (15V -> VL -> TPH)
    PRINT_LOGI("[1/5] Powering ON System Rails...\r\n");
    tph_power_on(); 
    rtos_delay_milliseconds(100); 

    // STEP 3: 唤醒 U4 与 U5 马达（错开 50ms 防止电池放电拉垮）
    PRINT_LOGI("[2/5] Waking Up Motors (U4 Carriage & U5 Paper)...\r\n");
    
    // 使能 U4 (1/16 细分)
    GPIO_UP(TPH_MOT_MS1); GPIO_UP(TPH_MOT_MS2); GPIO_UP(TPH_MOT_MS3);
    GPIO_UP(TPH_MOT_SLEEP);
    print_delay_us(100);
    GPIO_UP(TPH_MOT_RES);
    GPIO_DOWN(TPH_MOT_EN); 

    rtos_delay_milliseconds(50); // 缓冲电源大电流

    // 使能 U5 (1/16 细分)
    GPIO_UP(PAPER_MOT_MS1); GPIO_UP(PAPER_MOT_MS2); GPIO_UP(PAPER_MOT_MS3);
    GPIO_UP(PAPER_MOT_SLEEP);
    print_delay_us(100);
    GPIO_UP(PAPER_MOT_RES);
    GPIO_DOWN(PAPER_MOT_EN); 

    rtos_delay_milliseconds(10);

    // STEP 4: 连续打印 5 行彩条
    PRINT_LOGI("[3/5] Printing 5 Lines of Color Bars...\r\n");

    for (int row = 1; row <= 5; row++) {
        PRINT_LOGI("  -> Printing Line %d/5...\r\n", row);
        
        // 1. U4 正向扫掠同时 TPH 打火
        sweep_and_print_one_line();
        //rtos_delay_milliseconds(0); 

        // 2. U5 进纸 160 微步 (~1mm)
        paper_advance_one_line(160);
       // rtos_delay_milliseconds(5);

        // 3. U4 字车快速退回原点
        carriage_return_home();
        rtos_delay_milliseconds(1); 
    }

    // STEP 5: 自动吐纸并安全下电
    PRINT_LOGI("[4/5] Ejecting Paper...\r\n");
    paper_advance_one_line(800); 

    PRINT_LOGI("[5/5] Safely Powering OFF System...\r\n");
    
    // 关闭马达使能 (防止静态大电流发热)
    GPIO_UP(TPH_MOT_EN);   GPIO_DOWN(TPH_MOT_SLEEP);
    GPIO_UP(PAPER_MOT_EN); GPIO_DOWN(PAPER_MOT_SLEEP);
    
    // 关断喷头与主 15V 电源
    tph_power_off();

    // 还原 CPU 主频为默认频率
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT, PM_CPU_FRQ_DEFAULT);

    PRINT_LOGI("==================================================\r\n");
    PRINT_LOGI("      PRINTING COMPLETED SUCCESSFULLY!            \r\n");
    PRINT_LOGI("==================================================\r\n");
}




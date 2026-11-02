#include "tph_driver.h"
#include "motor_sdk_driver.h"
#include <stdio.h>
#include "modules/pm.h"

#include <stdlib.h>

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



// ==========================================
// DWT 硬件绝对精准微秒延时
// ==========================================
#define DWT_CR         (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT     (*(volatile uint32_t *)0xE0001004)
#define DEMCR          (*(volatile uint32_t *)0xE000EDFC)
#define PRINT_LOGI(...) printf("[PRINTER_APP] " __VA_ARGS__)
#define PRINT_LOGE(...) printf("[PRINTER_ERR] " __VA_ARGS__)

void dwt_delay_init(void) {
    DEMCR |= (1 << 24);    
    DWT_CYCCNT = 0;        
    DWT_CR |= (1 << 0);    
}

static inline void print_delay_us(uint32_t us) {
    uint32_t start_cycles = DWT_CYCCNT;
    uint32_t wait_cycles = us * 480; // 480MHz
    while ((DWT_CYCCNT - start_cycles) < wait_cycles);
}

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
        GPIO_UP(s_pins[i]);
    }
    for (int i = 0; i < 6; i++) {
        safe_tph_gpio_out(d_pins[i]);
        GPIO_DOWN(d_pins[i]);
    }
    for (int i = 0; i < 7; i++) {
        safe_tph_gpio_out(a_pins[i]);
        GPIO_DOWN(a_pins[i]);
    }

    GPIO_DOWN(PIN_VL_ENABLE);
    GPIO_DOWN(PIN_TPH_ENABLE);
    GPIO_DOWN(PIN_TPH_CE);
    GPIO_DOWN(PIN_SCOPE_TRIGGER);

    return BK_OK;
}

void tph_power_on(void) {
    TPH_LOGI("Enabling Power Rails...\r\n");
    bk_motor_boost_power(1);
    rtos_delay_milliseconds(30); 

    GPIO_UP(PIN_VL_ENABLE);
    rtos_delay_milliseconds(10); 

    GPIO_UP(PIN_TPH_ENABLE);
    rtos_delay_milliseconds(10);
}

void tph_power_off(void) {
    GPIO_DOWN(PIN_TPH_CE);
    GPIO_DOWN(PIN_TPH_ENABLE);
    GPIO_DOWN(PIN_VL_ENABLE);
    rtos_delay_milliseconds(5);
    bk_motor_boost_power(0);
}
/**
 * @brief 【生产级/普适性】M0333N 喷头单列打火驱动函数
 * @note  直接从 col_data 中读取任意图像点阵，零临时内存占用，速度极快
 */
__IRAM_SEC void tph_fire_one_columnbak2(const TphTestColumn_t *col_data) {

	GPIO_UP(PIN_TPH_CE);

    // 25 组典型地址打印循环 ( Add 1 ~ Add 25 )
    for (uint8_t addr = 0; addr < 25; addr++) {
		

        // A. 建立地址 A1-A7
        uint8_t addr_val = TPH_ADDR_TABLE[addr];
        for (int a = 0; a < 7; a++) {
            if ((addr_val >> a) & 0x01) GPIO_UP(a_pins[a]);
            else GPIO_DOWN(a_pins[a]);
        }
      //  print_delay_us(1); // 地址建立时间

        // B. 依次对 S1, S2, S3, S4 装载数据
        for (uint8_t section = 0; section < 4; section++) {
            
            // 计算当前 Section 下各 Domain 的 0~99 垂直喷孔索引
            uint32_t idx = (section * 25) + addr;
            uint32_t even_idx = 2 * idx;
            uint32_t odd_idx  = even_idx + 1;

            // ★ 生产级优化：直接从源结构体中提取位，省去 600 字节的临时数组分配与移位开销 ★
            
            // 1. 青色通道 (c_data) -> D1 (偶), D2 (奇)
            uint8_t c_even = (col_data->c_data[even_idx / 8] >> (even_idx % 8)) & 0x01;
            uint8_t c_odd  = (col_data->c_data[odd_idx / 8] >> (odd_idx % 8)) & 0x01;

            // 2. 品红通道 (m_data) -> D3 (偶), D4 (奇)
            uint8_t m_even = (col_data->m_data[even_idx / 8] >> (even_idx % 8)) & 0x01;
            uint8_t m_odd  = (col_data->m_data[odd_idx / 8] >> (odd_idx % 8)) & 0x01;

            // 3. 黄色通道 (y_data) -> D5 (偶), D6 (奇)
            uint8_t y_even = (col_data->y_data[even_idx / 8] >> (even_idx % 8)) & 0x01;
            uint8_t y_odd  = (col_data->y_data[odd_idx / 8] >> (odd_idx % 8)) & 0x01;

             
            // 4. 精准输出到对应的 D1~D6 数据引脚
            if (c_even) 
            {
               GPIO_UP(PIN_TPH_D1);
               GPIO_DOWN(s_pins[section]);
			   print_delay_us(1); 
			   GPIO_DOWN(PIN_TPH_D1);
			   print_delay_us(1);
		       GPIO_UP(s_pins[section]);
            }
			else
				 GPIO_DOWN(PIN_TPH_D1); // D1: Y 偶数孔
            if (c_odd) 
			{
			   GPIO_UP(PIN_TPH_D2);
               GPIO_DOWN(s_pins[section]);
			   print_delay_us(2); 
			   GPIO_DOWN(PIN_TPH_D2);
			   print_delay_us(1);
		       GPIO_UP(s_pins[section]);
			}
			else
			  GPIO_DOWN(PIN_TPH_D2); // D2: Y 奇数孔
            if (m_even)
			{
			   GPIO_UP(PIN_TPH_D3);
               GPIO_DOWN(s_pins[section]);
			   print_delay_us(1); 
			   GPIO_DOWN(PIN_TPH_D3);
			   print_delay_us(1);
		       GPIO_UP(s_pins[section]);
            }
			else GPIO_DOWN(PIN_TPH_D3); // D3: M 偶数孔
            if (m_odd)
			{
			   GPIO_UP(PIN_TPH_D4);
               GPIO_DOWN(s_pins[section]);
			   print_delay_us(1); 
			   GPIO_DOWN(PIN_TPH_D4);
			   print_delay_us(1);
		       GPIO_UP(s_pins[section]);
			}
			  else GPIO_DOWN(PIN_TPH_D4); // D4: M 奇数孔
            if (y_even)
			{  
			   GPIO_UP(PIN_TPH_D5);
               GPIO_DOWN(s_pins[section]);
			   print_delay_us(1); 
			   GPIO_DOWN(PIN_TPH_D5);
			   print_delay_us(1);
		       GPIO_UP(s_pins[section]);
			} 
			 else GPIO_DOWN(PIN_TPH_D5); // D5: C 偶数孔
            if (y_odd)
			{
			   GPIO_UP(PIN_TPH_D6);
               GPIO_DOWN(s_pins[section]);
			   print_delay_us(1); 
			   GPIO_DOWN(PIN_TPH_D6);
			   print_delay_us(1);
		       GPIO_UP(s_pins[section]);
			}
			else GPIO_DOWN(PIN_TPH_D6); // D6: C 奇数孔
       
        }

      

      
        // 组间回填延时 20us
        print_delay_us(20); 
    }
	  GPIO_DOWN(PIN_TPH_CE);
}


// ==========================================
// 诊断彩条与网格图案生成
// ==========================================
void generate_diagnostic_pattern(uint32_t col_idx, TphTestColumn_t *out_col) {
    for (int i = 0; i < 25; i++) {
        out_col->c_data[i] = 0x00;
        out_col->m_data[i] = 0x00;
        out_col->y_data[i] = 0x00;
    }

    // 划分为 4 个色块区域
    uint32_t zone = col_idx / 50;
    for (int i = 0; i < 25; i++) {
        uint8_t pattern_mask = 0xee; // 带诊断网格的线条

        switch (zone) {
            case 0: out_col->c_data[i] = pattern_mask; break; // 青色 (C)
            case 1: out_col->m_data[i] = pattern_mask; break; // 品红 (M)
            case 2: out_col->y_data[i] = pattern_mask; break; // 黄色 (Y)
            case 3: // 黑色 (K)
                out_col->c_data[i] = pattern_mask;
                out_col->m_data[i] = pattern_mask;
                out_col->y_data[i] = pattern_mask;
                break;
        }
    }
}

static void sweep_and_print_one_line(void) {
    TphTestColumn_t col_data;

    GPIO_UP(TPH_MOT_DIR);
    print_delay_us(20);

    // U4 加速段
    for (int step = 0; step < 200; step++) {
        uint32_t delay = 1000 - ((1000 - 300) * step / 200);
        GPIO_UP(TPH_MOT_STEP);
        print_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        print_delay_us(delay);
    }

    // 匀速打火段 (200 列)
    for (uint32_t col = 0; col < 200; col++) {
        generate_diagnostic_pattern(col, &col_data);
        tph_fire_one_column(&col_data);

        for (int m = 0; m < 4; m++) {
            GPIO_UP(TPH_MOT_STEP);
            print_delay_us(4);
            GPIO_DOWN(TPH_MOT_STEP);
            print_delay_us(300); 
        }
    }

    // U4 减速段
    for (int step = 0; step < 200; step++) {
        uint32_t delay = 300 + ((1000 - 300) * step / 200);
        GPIO_UP(TPH_MOT_STEP);
        print_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        print_delay_us(delay);
    }
}

static void paper_advance_one_line(uint32_t microsteps) {
    GPIO_DOWN(PAPER_MOT_DIR);
    print_delay_us(20);

    uint32_t int_mask = rtos_disable_int();
    for (uint32_t i = 0; i < microsteps; i++) {
        GPIO_UP(PAPER_MOT_STEP);
        print_delay_us(4);
        GPIO_DOWN(PAPER_MOT_STEP);
        print_delay_us(250); 
    }
    rtos_enable_int(int_mask);
}

static void carriage_return_home(void) {
    GPIO_DOWN(TPH_MOT_DIR);
    print_delay_us(20);

    for (uint32_t i = 0; i < 1200; i++) {
        uint32_t delay = 200;
        if (i < 200) delay = 800 - (600 * i / 200);
        else if (i > 1000) delay = 200 + (600 * (i - 1000) / 200);

        GPIO_UP(TPH_MOT_STEP);
        print_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        print_delay_us(delay);
    }
}

// ============================================================================
// ★ 调校参数：修改此处的进纸微步数，彻底消除行间漏白 ★
// ============================================================================
#define ROW_FEED_STEPS    150  // 如果依然有漏白，请尝试调小至 130 或 120

void run_full_printer_demo(void) {
    PRINT_LOGI("==================================================\r\n");
    PRINT_LOGI("      STARTING FULL INKJET PRINTING DEMO          \r\n");
    PRINT_LOGI("==================================================\r\n");

    dwt_delay_init(); 
    tph_gpio_init();
    bk_motor_gpio_config_init();

    PRINT_LOGI("[1/5] Powering ON System Rails...\r\n");
    tph_power_on(); 
    rtos_delay_milliseconds(100); 

    PRINT_LOGI("[2/5] Waking Up Motors...\r\n");
    
    // U4 Carriage
    GPIO_UP(TPH_MOT_MS1); GPIO_UP(TPH_MOT_MS2); GPIO_UP(TPH_MOT_MS3);
    GPIO_UP(TPH_MOT_SLEEP);
    print_delay_us(100);
    GPIO_UP(TPH_MOT_RES);
    GPIO_DOWN(TPH_MOT_EN); 

    rtos_delay_milliseconds(50); 

    // U5 Paper
    GPIO_UP(PAPER_MOT_MS1); GPIO_UP(PAPER_MOT_MS2); GPIO_UP(PAPER_MOT_MS3);
    GPIO_UP(PAPER_MOT_SLEEP);
    print_delay_us(100);
    GPIO_UP(PAPER_MOT_RES);
    GPIO_DOWN(PAPER_MOT_EN); 

    rtos_delay_milliseconds(10);

    PRINT_LOGI("[3/5] Ultra-Fast Seamless Printing...\r\n");

    for (int row = 1; row <= 6; row++) {
        // 1. 字车扫掠打火
        sweep_and_print_one_line();

        // 2. 进纸（使用可调校的微步数）
        paper_advance_one_line(ROW_FEED_STEPS);

        // 3. 字车复位归位
        carriage_return_home();
    }

    PRINT_LOGI("[4/5] Ejecting Paper...\r\n");
    paper_advance_one_line(800); 

    PRINT_LOGI("[5/5] Safely Powering OFF System...\r\n");
    GPIO_UP(TPH_MOT_EN);   GPIO_DOWN(TPH_MOT_SLEEP);
    GPIO_UP(PAPER_MOT_EN); GPIO_DOWN(PAPER_MOT_SLEEP);
    tph_power_off();

    PRINT_LOGI("==================================================\r\n");
    PRINT_LOGI("      PRINTING COMPLETED SUCCESSFULLY!            \r\n");
    PRINT_LOGI("==================================================\r\n");
}







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



// ==========================================
// 完美解决 A1 脉冲宽度，使其与 CE 完全对齐的代码
// ==========================================
__IRAM_SEC void tph_fire_one_column_relbak(const TphTestColumn_t *col_data) {
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


//50*50 mm 测试


#define TEST_LOGI(...) printf("[50x50_TEST] " __VA_ARGS__)



// ============================================================================
// 1. 配合 6 行无缝拼接的 50x50mm 图案生成器 (1180 x 1200 像素坐标系)
// ============================================================================
void generate_50x50_pattern_calibrated(uint32_t row_idx, uint32_t col_idx, TphTestColumn_t *out_col) {
    for (int i = 0; i < 25; i++) {
        out_col->c_data[i] = 0x00;
        out_col->m_data[i] = 0x00;
        out_col->y_data[i] = 0x00;
    }

    uint32_t x = col_idx; // 全局 X 坐标 (0 ~ 1179)

    for (uint32_t nozzle = 0; nozzle < 200; nozzle++) {
        uint32_t y = row_idx * 200 + nozzle; // 全局 Y 坐标 (0 ~ 1199，完美对应 50mm 高度)

        // A. 50mm 正方形外边框 (四周细线)
        bool is_border = (x < 6 || x > 1170 || y < 6 || y > 1190);
        
        // B. 网格线 (每 200 像素画一条网格)
        bool is_grid   = ((x % 200 == 0) || (y % 200 == 0));

        // C. 1:1 标准大 X 交叉对角斜线
        bool is_cross  = (abs((int)x - (int)y) < 4) || (abs((int)(1180 - x) - (int)y) < 4);

        bool is_line = is_border || is_grid || is_cross;

        // D. 50% 棋盘格降密网格掩码 (防止喷墨量过大)
        uint8_t dither_mask = ((nozzle + col_idx) % 2 == 0) ? 0x55 : 0xAA;

        uint32_t zone = x / 295; // 4 色块区
        uint32_t byte_idx = nozzle / 8;
        uint8_t  bit_mask = (1 << (nozzle % 8));

        if (is_line) {
            // 线条部分打印黑色 (C + M)
            out_col->c_data[byte_idx] |= bit_mask;
            out_col->m_data[byte_idx] |= bit_mask;
        } else {
            // 色块区域降密喷墨
            if (bit_mask & dither_mask) {
                switch (zone) {
                    case 0: out_col->c_data[byte_idx] |= bit_mask; break; // 青色 (C)
                    case 1: out_col->m_data[byte_idx] |= bit_mask; break; // 品红 (M)
                    case 2: out_col->y_data[byte_idx] |= bit_mask; break; // 黄色 (Y)
                    case 3: // 黑色区 (C + M)
                        out_col->c_data[byte_idx] |= bit_mask;
                        out_col->m_data[byte_idx] |= bit_mask;
                        break;
                }
            }
        }
    }
}

// ============================================================================
// 2. 扫掠与归位子函数
// ============================================================================
static void sweep_50mm_line_calibrated(uint32_t row_idx) {
    TphTestColumn_t col_data;

    GPIO_UP(TPH_MOT_DIR);
    print_delay_us(20);

    // 加速段
    for (int step = 0; step < 200; step++) {
        uint32_t delay = 1000 - ((1000 - 300) * step / 200);
        GPIO_UP(TPH_MOT_STEP);
        print_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        print_delay_us(delay);
    }

    // 匀速打火段：扫掠 1180 列
    for (uint32_t col = 0; col < 1180; col++) {
        generate_50x50_pattern_calibrated(row_idx, col, &col_data);
        tph_fire_one_column(&col_data);

        // 每列移动 6 微步 (宽度覆盖 45mm~50mm)
        for (int m = 0; m < 6; m++) {
            GPIO_UP(TPH_MOT_STEP);
            print_delay_us(4);
            GPIO_DOWN(TPH_MOT_STEP);
            print_delay_us(300); 
        }
    }

    // 减速段
    for (int step = 0; step < 200; step++) {
        uint32_t delay = 300 + ((1000 - 300) * step / 200);
        GPIO_UP(TPH_MOT_STEP);
        print_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        print_delay_us(delay);
    }
}

static void carriage_return_50mm_home_calibrated(void) {
    GPIO_DOWN(TPH_MOT_DIR);
    print_delay_us(20);

    uint32_t total_return_steps = 7480;
    for (uint32_t i = 0; i < total_return_steps; i++) {
        uint32_t delay = 200; 
        if (i < 400) delay = 800 - (600 * i / 400);
        else if (i > 7080) delay = 200 + (600 * (i - 7080) / 400);

        GPIO_UP(TPH_MOT_STEP);
        print_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        print_delay_us(delay);
    }
}

// ============================================================================
// 3. 【精准 50mm x 50mm 打印入口】
// ============================================================================
void print_50x50mm_test_pattern(void) {
    TEST_LOGI("==================================================\r\n");
    TEST_LOGI("   STARTING ACCURATE 50mm x 50mm PRINT TEST       \r\n");
    TEST_LOGI("==================================================\r\n");

    dwt_delay_init(); 
    tph_gpio_init();
    bk_motor_gpio_config_init();

    tph_power_on(); 
    rtos_delay_milliseconds(100); 

    // 马达使能 (1/16 细分)
    GPIO_UP(TPH_MOT_MS1); GPIO_UP(TPH_MOT_MS2); GPIO_UP(TPH_MOT_MS3);
    GPIO_UP(TPH_MOT_SLEEP);
    print_delay_us(100);
    GPIO_UP(TPH_MOT_RES);
    GPIO_DOWN(TPH_MOT_EN); 

    rtos_delay_milliseconds(50); 

    GPIO_UP(PAPER_MOT_MS1); GPIO_UP(PAPER_MOT_MS2); GPIO_UP(PAPER_MOT_MS3);
    GPIO_UP(PAPER_MOT_SLEEP);
    print_delay_us(100);
    GPIO_UP(PAPER_MOT_RES);
    GPIO_DOWN(PAPER_MOT_EN); 

    rtos_delay_milliseconds(10);

    // ★ 关键修改：无缝拼接！只需 6 行，每行走纸 2820 微步 (精准 8.46mm) ★
    for (uint32_t row = 0; row < 6; row++) {
        TEST_LOGI("Printing Row %d/6 (50mm Sweep)...\r\n", row + 1);

        sweep_50mm_line_calibrated(row);

        // ★ 核心改变：走纸 2820 微步 (8.46mm)，消除重叠，精准拉长高度到 50.8mm ★
        paper_advance_one_line(3150); 

        carriage_return_50mm_home_calibrated();
    }

    // 打印完成出纸 50mm
    paper_advance_one_line(5000); 

    // 下电
    GPIO_UP(TPH_MOT_EN);   GPIO_DOWN(TPH_MOT_SLEEP);
    GPIO_UP(PAPER_MOT_EN); GPIO_DOWN(PAPER_MOT_SLEEP);
    tph_power_off();

    TEST_LOGI("==================================================\r\n");
    TEST_LOGI("     50x50mm PRINT COMPLETED PERFECTLY!           \r\n");
    TEST_LOGI("==================================================\r\n");
}



#define PHOTO_ARRAY_LOGI(...) printf("[ICON_PRINT] " __VA_ARGS__)

#define SRC_IMG_WIDTH   128
#define SRC_IMG_HEIGHT  128

// 动态三通道图像内存 (128x128 / 8 = 2048 字节)
static uint8_t g_icon_cyan[2048];
static uint8_t g_icon_magenta[2048];
static uint8_t g_icon_yellow[2048];

// ============================================================================
// 1. 构建真实可识别的具象图案 (彩色大爱心 + 边框) 
// 注：已用纯乘法替换 pow()，100% 规避 Armino SDK 编译错误
// ============================================================================
void build_real_icon_image(void) {
    for (int i = 0; i < 2048; i++) {
        g_icon_cyan[i] = 0x00;
        g_icon_magenta[i] = 0x00;
        g_icon_yellow[i] = 0x00;
    }

    for (int y = 0; y < 128; y++) {
        for (int x = 0; x < 128; x++) {
            uint32_t idx = y * 128 + x;
            uint32_t byte_idx = idx / 8;
            uint8_t  bit_mask = (1 << (idx % 8));

            // A. 外边框 (黑色 = C + M + Y)
            bool is_border = (x < 4 || x > 123 || y < 4 || y > 123);

            // B. 爱心隐式曲线方程 (用纯乘法替代 pow，零数学库依赖)
            double nx = (x - 64.0) / 38.0;
            double ny = (60.0 - y) / 38.0 + 0.3; // 居中与坐标系翻转

            double val_a  = nx * nx + ny * ny - 1.0;
            double val_a3 = val_a * val_a * val_a;     // 替代 pow(val_a, 3)
            double val_b3 = nx * nx * (ny * ny * ny);  // 替代 nx^2 * pow(ny, 3)
            
            bool is_heart = ((val_a3 - val_b3) <= 0.0);

            if (is_border) {
                // 外框打印黑色
                g_icon_cyan[byte_idx] |= bit_mask;
                g_icon_magenta[byte_idx] |= bit_mask;
                g_icon_yellow[byte_idx] |= bit_mask;
            } 
            else if (is_heart) {
                // 爱心内部打印鲜艳的大红色/品红 (M + Y)
                g_icon_magenta[byte_idx] |= bit_mask;
                g_icon_yellow[byte_idx]  |= bit_mask;
            } 
            else {
                // 背景打印浅青色 (C)
                if ((x + y) % 2 == 0) {
                    g_icon_cyan[byte_idx] |= bit_mask;
                }
            }
        }
    }
}


// ============================================================================
// 2. 图像读取与 21 像素物理错位对齐
// ============================================================================
static inline uint8_t get_icon_pixel(const uint8_t *img_buf, int x, int y) {
    if (x < 0 || x >= SRC_IMG_WIDTH || y < 0 || y >= SRC_IMG_HEIGHT) return 0;
    uint32_t idx = y * SRC_IMG_WIDTH + x;
    return (img_buf[idx / 8] & (1 << (idx % 8))) ? 1 : 0;
}
void generate_icon_from_memory(uint32_t row_idx, uint32_t col_idx, TphTestColumn_t *out_col) {
    for (int i = 0; i < 25; i++) {
        out_col->c_data[i] = 0x00;
        out_col->m_data[i] = 0x00;
        out_col->y_data[i] = 0x00;
    }

    uint32_t curr_x = col_idx; 

    for (uint32_t nozzle = 0; nozzle < 200; nozzle++) {
        uint32_t global_y = row_idx * 200 + nozzle; 
        int src_y = (int)(global_y * SRC_IMG_HEIGHT / 1200);

        uint32_t byte_idx = nozzle / 8;
        uint8_t  bit_mask = (1 << (nozzle % 8));

        // 1. 青色 C (最右侧，X 物理偏移 = 0)
        int src_x_c = (int)(curr_x * SRC_IMG_WIDTH / 1180);
        if (get_icon_pixel(g_icon_cyan, src_x_c, src_y)) {
            out_col->c_data[byte_idx] |= bit_mask;
        }

        // 2. 品红 M (物理落后 889um，精准校准为偏移 12 列)
        if (curr_x >= 12) {
            int src_x_m = (int)((curr_x - 12) * SRC_IMG_WIDTH / 1180);
            if (get_icon_pixel(g_icon_magenta, src_x_m, src_y)) {
                out_col->m_data[byte_idx] |= bit_mask;
            }
        }

        // 3. 黄色 Y (物理落后 1778um，精准校准为偏移 24 列)
        if (curr_x >= 24) {
            int src_x_y = (int)((curr_x - 24) * SRC_IMG_WIDTH / 1180);
            if (get_icon_pixel(g_icon_yellow, src_x_y, src_y)) {
                out_col->y_data[byte_idx] |= bit_mask;
            }
        }
    }
}

void old_generate_icon_from_memoryx(uint32_t row_idx, uint32_t col_idx, TphTestColumn_t *out_col) {
    for (int i = 0; i < 25; i++) {
        out_col->c_data[i] = 0x00;
        out_col->m_data[i] = 0x00;
        out_col->y_data[i] = 0x00;
    }

    uint32_t curr_x = col_idx; 

    for (uint32_t nozzle = 0; nozzle < 200; nozzle++) {
        uint32_t global_y = row_idx * 200 + nozzle; 

        // 缩放映射 (1180x1200 -> 128x128)
        int src_y = (int)(global_y * SRC_IMG_HEIGHT / 1200);

        uint32_t byte_idx = nozzle / 8;
        uint8_t  bit_mask = (1 << (nozzle % 8));

        // 1. 青色 C (最右侧，X 偏移 = 0)
        int src_x_c = (int)(curr_x * SRC_IMG_WIDTH / 1180);
        if (get_icon_pixel(g_icon_cyan, src_x_c, src_y)) {
            out_col->c_data[byte_idx] |= bit_mask;
        }

        // 2. 品红 M (落后 21 像素，X 偏移 = -21)
        if (curr_x >= 21) {
            int src_x_m = (int)((curr_x - 21) * SRC_IMG_WIDTH / 1180);
            if (get_icon_pixel(g_icon_magenta, src_x_m, src_y)) {
                out_col->m_data[byte_idx] |= bit_mask;
            }
        }

        // 3. 黄色 Y (落后 42 像素，X 偏移 = -42)
        if (curr_x >= 42) {
            int src_x_y = (int)((curr_x - 42) * SRC_IMG_WIDTH / 1180);
            if (get_icon_pixel(g_icon_yellow, src_x_y, src_y)) {
                out_col->y_data[byte_idx] |= bit_mask;
            }
        }
    }
}

// ============================================================================
// 3. 扫掠与归位
// ============================================================================
static void sweep_50mm_icon_line(uint32_t row_idx) {
    TphTestColumn_t col_data;

    GPIO_UP(TPH_MOT_DIR);
    print_delay_us(20);

    for (int step = 0; step < 200; step++) {
        uint32_t delay = 1000 - ((1000 - 300) * step / 200);
        GPIO_UP(TPH_MOT_STEP);
        print_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        print_delay_us(delay);
    }

    for (uint32_t col = 0; col < 1222; col++) {
        generate_icon_from_memory(row_idx, col, &col_data);
        tph_fire_one_column(&col_data);

        for (int m = 0; m < 6; m++) {
            GPIO_UP(TPH_MOT_STEP);
            print_delay_us(4);
            GPIO_DOWN(TPH_MOT_STEP);
            print_delay_us(300); 
        }
    }

    for (int step = 0; step < 200; step++) {
        uint32_t delay = 300 + ((1000 - 300) * step / 200);
        GPIO_UP(TPH_MOT_STEP);
        print_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        print_delay_us(delay);
    }
}

static void carriage_return_50mm_home_icon(void) {
    GPIO_DOWN(TPH_MOT_DIR);
    print_delay_us(20);

    uint32_t total_return_steps = 7732;
    for (uint32_t i = 0; i < total_return_steps; i++) {
        uint32_t delay = 200; 
        if (i < 400) delay = 800 - (600 * i / 400);
        else if (i > 7332) delay = 200 + (600 * (i - 7332) / 400);

        GPIO_UP(TPH_MOT_STEP);
        print_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        print_delay_us(delay);
    }
}

// ============================================================================
// 4. 【彩色爱心图案打印总入口】print_real_icon_demo
// ============================================================================
void print_real_icon_demo(void) {
    PHOTO_ARRAY_LOGI("==================================================\r\n");
    PHOTO_ARRAY_LOGI("     STARTING PRINTING REAL COLOR HEART ICON      \r\n");
    PHOTO_ARRAY_LOGI("==================================================\r\n");

    dwt_delay_init(); 
    tph_gpio_init();
    bk_motor_gpio_config_init();

    // 构建爱心图案
    build_real_icon_image();

    tph_power_on(); 
    rtos_delay_milliseconds(100); 

    GPIO_UP(TPH_MOT_MS1); GPIO_UP(TPH_MOT_MS2); GPIO_UP(TPH_MOT_MS3);
    GPIO_UP(TPH_MOT_SLEEP);
    print_delay_us(100);
    GPIO_UP(TPH_MOT_RES);
    GPIO_DOWN(TPH_MOT_EN); 

    rtos_delay_milliseconds(50); 

    GPIO_UP(PAPER_MOT_MS1); GPIO_UP(PAPER_MOT_MS2); GPIO_UP(PAPER_MOT_MS3);
    GPIO_UP(PAPER_MOT_SLEEP);
    print_delay_us(100);
    GPIO_UP(PAPER_MOT_RES);
    GPIO_DOWN(PAPER_MOT_EN); 

    rtos_delay_milliseconds(10);

    // 扫掠 6 行，拼接 50mm 高度
    for (uint32_t row = 0; row < 6; row++) {
        PHOTO_ARRAY_LOGI("Printing Heart Pass %d/6...\r\n", row + 1);

        sweep_50mm_icon_line(row);

        paper_advance_one_line(3150); 

        carriage_return_50mm_home_icon();
    }

    paper_advance_one_line(5000); 

    GPIO_UP(TPH_MOT_EN);   GPIO_DOWN(TPH_MOT_SLEEP);
    GPIO_UP(PAPER_MOT_EN); GPIO_DOWN(PAPER_MOT_SLEEP);
    tph_power_off();

    PHOTO_ARRAY_LOGI("==================================================\r\n");
    PHOTO_ARRAY_LOGI("     HEART ICON PRINT COMPLETED PERFECTLY!        \r\n");
    PHOTO_ARRAY_LOGI("==================================================\r\n");
}


#define STEP1_LOGI(...) printf("[STEP1_PUR_M] " __VA_ARGS__)

#define SRC_IMG_WIDTH   128
#define SRC_IMG_HEIGHT  128

static uint8_t g_pure_magenta_heart[2048]; // 仅保留品红单通道内存

// ============================================================================
// 1. 构建纯品红爱心 (无背景、无外框、无其它颜色)
// ============================================================================
void build_pure_magenta_heart(void) {
    for (int i = 0; i < 2048; i++) {
        g_pure_magenta_heart[i] = 0x00;
    }

    for (int y = 0; y < 128; y++) {
        for (int x = 0; x < 128; x++) {
            uint32_t idx = y * 128 + x;
            uint32_t byte_idx = idx / 8;
            uint8_t  bit_mask = (1 << (idx % 8));

            // 爱心隐式方程
            double nx = (x - 64.0) / 38.0;
            double ny = (60.0 - y) / 38.0 + 0.3;

            double val_a  = nx * nx + ny * ny - 1.0;
            double val_a3 = val_a * val_a * val_a;
            double val_b3 = nx * nx * (ny * ny * ny);
            
            bool is_heart = ((val_a3 - val_b3) <= 0.0);

            if (is_heart) {
                // ★ 只向品红通道写入 100% 实心点阵 ★
                g_pure_magenta_heart[byte_idx] |= bit_mask;
            }
        }
    }
}

// 提取单色像素
static inline uint8_t get_pure_m_pixel(int x, int y) {
    if (x < 0 || x >= SRC_IMG_WIDTH || y < 0 || y >= SRC_IMG_HEIGHT) return 0;
    uint32_t idx = y * SRC_IMG_WIDTH + x;
    return (g_pure_magenta_heart[idx / 8] & (1 << (idx % 8))) ? 1 : 0;
}

// 生成纯品红单列点阵 (单色无需做 X 轴跨通道偏移)
void generate_pure_m_column(uint32_t row_idx, uint32_t col_idx, TphTestColumn_t *out_col) {
    for (int i = 0; i < 25; i++) {
        out_col->c_data[i] = 0x00;
        out_col->m_data[i] = 0x00;
        out_col->y_data[i] = 0x00;
    }

    for (uint32_t nozzle = 0; nozzle < 200; nozzle++) {
        uint32_t global_y = row_idx * 200 + nozzle; 
        int src_y = (int)(global_y * SRC_IMG_HEIGHT / 1200);
        int src_x = (int)(col_idx * SRC_IMG_WIDTH / 1180);

        uint32_t byte_idx = nozzle / 8;
        uint8_t  bit_mask = (1 << (nozzle % 8));

        // 仅将数据塞品质品红 m_data
        if (get_pure_m_pixel(src_x, src_y)) {
            out_col->m_data[byte_idx] |= bit_mask;
        }
    }
}

// 扫掠单行
static void sweep_pure_m_line(uint32_t row_idx) {
    TphTestColumn_t col_data;

    GPIO_UP(TPH_MOT_DIR);
    print_delay_us(20);

    for (int step = 0; step < 200; step++) {
        uint32_t delay = 1000 - ((1000 - 300) * step / 200);
        GPIO_UP(TPH_MOT_STEP);
        print_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        print_delay_us(delay);
    }

    for (uint32_t col = 0; col < 1180; col++) {
        generate_pure_m_column(row_idx, col, &col_data);
        tph_fire_one_column(&col_data);

        for (int m = 0; m < 6; m++) {
            GPIO_UP(TPH_MOT_STEP);
            print_delay_us(4);
            GPIO_DOWN(TPH_MOT_STEP);
            print_delay_us(300); 
        }
    }

    for (int step = 0; step < 200; step++) {
        uint32_t delay = 300 + ((1000 - 300) * step / 200);
        GPIO_UP(TPH_MOT_STEP);
        print_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        print_delay_us(delay);
    }
}

// ============================================================================
// 【步骤一入口】只打印白纸上的纯品红爱心
// ============================================================================
void test_step1_pure_magenta_heart(void) {
    STEP1_LOGI("==================================================\r\n");
    STEP1_LOGI("  STEP 1: PRINTING PURE MAGENTA HEART ON WHITE    \r\n");
    STEP1_LOGI("==================================================\r\n");

    dwt_delay_init(); 
    tph_gpio_init();
    bk_motor_gpio_config_init();

    // 构建纯品红爱心
    build_pure_magenta_heart();

    tph_power_on(); 
    rtos_delay_milliseconds(100); 

    // 使能马达
    GPIO_UP(TPH_MOT_MS1); GPIO_UP(TPH_MOT_MS2); GPIO_UP(TPH_MOT_MS3);
    GPIO_UP(TPH_MOT_SLEEP);
    print_delay_us(100);
    GPIO_UP(TPH_MOT_RES);
    GPIO_DOWN(TPH_MOT_EN); 

    rtos_delay_milliseconds(50); 

    GPIO_UP(PAPER_MOT_MS1); GPIO_UP(PAPER_MOT_MS2); GPIO_UP(PAPER_MOT_MS3);
    GPIO_UP(PAPER_MOT_SLEEP);
    print_delay_us(100);
    GPIO_UP(PAPER_MOT_RES);
    GPIO_DOWN(PAPER_MOT_EN); 

    rtos_delay_milliseconds(10);

    // 扫掠 6 行，拼接爱心
    for (uint32_t row = 0; row < 6; row++) {
        STEP1_LOGI("Printing Pass %d/6...\r\n", row + 1);

        sweep_pure_m_line(row);

        paper_advance_one_line(2920); 

        carriage_return_50mm_home_icon();
    }

    paper_advance_one_line(5000); 

    GPIO_UP(TPH_MOT_EN);   GPIO_DOWN(TPH_MOT_SLEEP);
    GPIO_UP(PAPER_MOT_EN); GPIO_DOWN(PAPER_MOT_SLEEP);
    tph_power_off();

    STEP1_LOGI("==================================================\r\n");
    STEP1_LOGI("  STEP 1 TEST COMPLETED!                          \r\n");
    STEP1_LOGI("==================================================\r\n");
}


#define BLOCK_LOGI(...) printf("[BLOCK_TEST] " __VA_ARGS__)

// ============================================================================
// ★ 100% 原厂逐点打火算法 (锁存顺序完全保持原样) ★
// ============================================================================
__IRAM_SEC void tph_fire_one_column(const TphTestColumn_t *col_data) {

    GPIO_UP(PIN_TPH_CE);

    for (uint8_t addr = 0; addr < 25; addr++) {
		GPIO_UP(PIN_TPH_CE);

        // A. 建立地址 A1-A7
        uint8_t addr_val = TPH_ADDR_TABLE[addr];
        for (int a = 0; a < 7; a++) {
            if ((addr_val >> a) & 0x01) GPIO_UP(a_pins[a]);
            else GPIO_DOWN(a_pins[a]);
        }

        // B. 依次对 S1, S2, S3, S4 逐点装载数据
        for (uint8_t section = 0; section < 4; section++) {
            
            uint32_t nozzle_base = (section * 25) + addr;

            uint32_t y_idx_e = 2 * nozzle_base;
            uint32_t y_idx_o = 2 * nozzle_base + 1;
            uint32_t m_idx_e = 2 * nozzle_base;
            uint32_t m_idx_o = 2 * nozzle_base + 1;
            uint32_t c_idx_e = 2 * nozzle_base;
            uint32_t c_idx_o = 2 * nozzle_base + 1;

            uint8_t c_e = (col_data->c_data[c_idx_e / 8] >> (c_idx_e % 8)) & 0x01;
            uint8_t c_o = (col_data->c_data[c_idx_o / 8] >> (c_idx_o % 8)) & 0x01;
            uint8_t m_e = (col_data->m_data[m_idx_e / 8] >> (m_idx_e % 8)) & 0x01;
            uint8_t m_o = (col_data->m_data[m_idx_o / 8] >> (m_idx_o % 8)) & 0x01;
            uint8_t y_e = (col_data->y_data[y_idx_e / 8] >> (y_idx_e % 8)) & 0x01;
            uint8_t y_o = (col_data->y_data[y_idx_o / 8] >> (y_idx_o % 8)) & 0x01;

            // 原厂锁存顺序：D1 -> D2 -> D3 -> D4 -> D5 -> D6
            if (c_e) {
               GPIO_UP(PIN_TPH_D1);
			 
               GPIO_DOWN(s_pins[section]); print_delay_us(2); 
               GPIO_DOWN(PIN_TPH_D1);     print_delay_us(1);
               GPIO_UP(s_pins[section]);
            } else GPIO_DOWN(PIN_TPH_D1);

            if (c_o) {
               GPIO_UP(PIN_TPH_D2);
			  
               GPIO_DOWN(s_pins[section]); print_delay_us(2); 
               GPIO_DOWN(PIN_TPH_D2);     print_delay_us(1);
               GPIO_UP(s_pins[section]);
            } else GPIO_DOWN(PIN_TPH_D2);

            if (m_e) {
               GPIO_UP(PIN_TPH_D3);
			   
               GPIO_DOWN(s_pins[section]); print_delay_us(2); 
               GPIO_DOWN(PIN_TPH_D3);     print_delay_us(1);
               GPIO_UP(s_pins[section]);
            } else GPIO_DOWN(PIN_TPH_D3);

            if (m_o) {
               GPIO_UP(PIN_TPH_D4);
			  
               GPIO_DOWN(s_pins[section]); print_delay_us(2); 
               GPIO_DOWN(PIN_TPH_D4);     print_delay_us(1);
               GPIO_UP(s_pins[section]);
            } else GPIO_DOWN(PIN_TPH_D4);

            if (y_e) {
               GPIO_UP(PIN_TPH_D5);
			   
               GPIO_DOWN(s_pins[section]); print_delay_us(2); 
               GPIO_DOWN(PIN_TPH_D5);     print_delay_us(1);
               GPIO_UP(s_pins[section]);
            } else GPIO_DOWN(PIN_TPH_D5);

            if (y_o) {
               GPIO_UP(PIN_TPH_D6);
			  
               GPIO_DOWN(s_pins[section]); print_delay_us(2); 
               GPIO_DOWN(PIN_TPH_D6);     print_delay_us(1);
               GPIO_UP(s_pins[section]);
            } else GPIO_DOWN(PIN_TPH_D6);
			
        }
		GPIO_DOWN(PIN_TPH_CE);

        print_delay_us(20); 
    }

    GPIO_DOWN(PIN_TPH_CE);
}

// 左品红 右黄色 纯色块点阵生成
void generate_magenta_vs_yellow_blocks(uint32_t col_idx, TphTestColumn_t *out_col) {
    for (int i = 0; i < 25; i++) {
        out_col->c_data[i] = 0x00;
        out_col->m_data[i] = 0x00;
        out_col->y_data[i] = 0x00;
    }

    if (col_idx < 590) {
        // 左半边：纯品红 100% 全开
        for (int i = 0; i < 25; i++) out_col->m_data[i] = 0xFF;
    } else {
        // 右半边：纯黄色 100% 全开
        for (int i = 0; i < 25; i++) out_col->y_data[i] = 0xFF;
    }
}



// 扫掠单行
static void sweep_color_blocks_line(uint32_t row_idx) {
    TphTestColumn_t col_data;

    GPIO_UP(TPH_MOT_DIR);
    print_delay_us(20);

    for (int step = 0; step < 200; step++) {
        uint32_t delay = 1000 - ((1000 - 300) * step / 200);
        GPIO_UP(TPH_MOT_STEP);
        print_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        print_delay_us(delay);
    }

    for (uint32_t col = 0; col < 1180; col++) {
        generate_magenta_vs_yellow_blocks(col, &col_data);
        tph_fire_one_column(&col_data);

        for (int m = 0; m < 6; m++) {
            GPIO_UP(TPH_MOT_STEP);
            print_delay_us(4);
            GPIO_DOWN(TPH_MOT_STEP);
            print_delay_us(300); 
        }
    }

    for (int step = 0; step < 200; step++) {
        uint32_t delay = 300 + ((1000 - 300) * step / 200);
        GPIO_UP(TPH_MOT_STEP);
        print_delay_us(4);
        GPIO_DOWN(TPH_MOT_STEP);
        print_delay_us(delay);
    }
}

// ============================================================================
// 【测试入口】左品红 右黄色 纯色块测试
// ============================================================================
void print_magenta_vs_yellow_demo(void) {
    BLOCK_LOGI("==================================================\r\n");
    BLOCK_LOGI("   STARTING MAGENTA VS YELLOW COLOR BLOCK TEST    \r\n");
    BLOCK_LOGI("==================================================\r\n");

    dwt_delay_init(); 
    tph_gpio_init();
    bk_motor_gpio_config_init();

    tph_power_on(); 
    rtos_delay_milliseconds(100); 

    GPIO_UP(TPH_MOT_MS1); GPIO_UP(TPH_MOT_MS2); GPIO_UP(TPH_MOT_MS3);
    GPIO_UP(TPH_MOT_SLEEP);
    print_delay_us(100);
    GPIO_UP(TPH_MOT_RES);
    GPIO_DOWN(TPH_MOT_EN); 

    rtos_delay_milliseconds(50); 

    GPIO_UP(PAPER_MOT_MS1); GPIO_UP(PAPER_MOT_MS2); GPIO_UP(PAPER_MOT_MS3);
    GPIO_UP(PAPER_MOT_SLEEP);
    print_delay_us(100);
    GPIO_UP(PAPER_MOT_RES);
    GPIO_DOWN(PAPER_MOT_EN); 

    rtos_delay_milliseconds(10);

    // 扫掠 4 行，打出两块矩形色块
    for (uint32_t row = 0; row < 1; row++) {
        BLOCK_LOGI("Printing Block Pass %d/4...\r\n", row + 1);

        sweep_color_blocks_line(row);

        paper_advance_one_line(2920); 

        carriage_return_50mm_home_icon();
    }

    paper_advance_one_line(5000); 

    GPIO_UP(TPH_MOT_EN);   GPIO_DOWN(TPH_MOT_SLEEP);
    GPIO_UP(PAPER_MOT_EN); GPIO_DOWN(PAPER_MOT_SLEEP);
    tph_power_off();

    BLOCK_LOGI("==================================================\r\n");
    BLOCK_LOGI("      COLOR BLOCK TEST PRINTED COMPLETED!         \r\n");
    BLOCK_LOGI("==================================================\r\n");
}


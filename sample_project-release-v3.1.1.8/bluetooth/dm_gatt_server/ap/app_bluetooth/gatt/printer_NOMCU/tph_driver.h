// Copyright 2024 Beken
// TPH (M0333N Thermal Print Head) Driver Header for BK7258 SDK

#ifndef TPH_DRIVER_H
#define TPH_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <common/bk_include.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

// ==========================================
// 1. TPH 电源与打火控制引脚定义 (映射至 BK7258)
// ==========================================

// 系统与 TPH 电源使能引脚
#define PIN_VDD15V_EN          GPIO_13  // 主系统 15V 升压使能 (U3 LP6221)
#define PIN_VL_ENABLE          GPIO_52  // TPH 逻辑 3.3V 使能 (VDD_VL)
#define PIN_TPH_ENABLE         GPIO_51  // TPH 加热 15V 使能 (VDD_TPH)

// TPH 芯片打火使能端
#define PIN_TPH_CE             GPIO_32  // 加热使能端 CE (高电平有效，最大2us)

// 4 个分段数据锁存引脚 (S1 - S4)
#define PIN_TPH_S1             GPIO_38  // S1 (P38)
#define PIN_TPH_S2             GPIO_12  // S2 (P12)
#define PIN_TPH_S3             GPIO_3   // S3 (P3)
#define PIN_TPH_S4             GPIO_36  // S4 (P36)

// 6 个区域数据传输引脚 (D1 - D6)
#define PIN_TPH_D1             GPIO_7   // D1 (P7)
#define PIN_TPH_D2             GPIO_5   // D2 (P5)
#define PIN_TPH_D3             GPIO_6   // D3 (P6)
#define PIN_TPH_D4             GPIO_30  // D4 (P30)
#define PIN_TPH_D5             GPIO_37  // D5 (P37)
#define PIN_TPH_D6             GPIO_28  // D6 (P28)

// 7 个内部地址译码引脚 (A1 - A7)
#define PIN_TPH_A1             GPIO_34  // A1 (P34)
#define PIN_TPH_A2             GPIO_35  // A2 (P35)
#define PIN_TPH_A3             GPIO_33  // A3 (P33)
#define PIN_TPH_A4             GPIO_2   // A4 (P2)
#define PIN_TPH_A5             GPIO_39  // A5 (P39)
#define PIN_TPH_A6             GPIO_4   // A6 (P4)
#define PIN_TPH_A7             GPIO_29  // A7 (P29)

// TPH 热敏电阻检测引脚 (TSR)
#define PIN_TPH_TSR            GPIO_1   // 温度采样端 (P1)

// 示波器调试同步触发引脚 (Scope Trigger)
#define PIN_SCOPE_TRIGGER      GPIO_0   // 抓脉冲用的同步脚 (P0)

// ==========================================
// 2. 硬件常量与数据结构定义
// ==========================================

#define TPH_NOZZLES_PER_COLOR  200  // 单色喷孔数 (青/品红/黄各200个)
#define TPH_TOTAL_NOZZLES      600  // 总喷孔数 (200 * 3 = 600)
#define TPH_ADDR_COUNT         25   // 内部 3D 逻辑寻址总组数 (600 / 24 = 25)

/**
 * @brief 单列打印点阵数据结构体
 * 包含 C、M、Y 三种颜色通道，每通道 200 个喷孔 (200 bits = 25 字节)
 */
typedef struct {
    uint8_t c_data[25]; // 青色通道点阵 (200 bits)
    uint8_t m_data[25]; // 品红通道点阵 (200 bits)
    uint8_t y_data[25]; // 黄色通道点阵 (200 bits)
} TphTestColumn_t;

// 地址译码表
extern const uint8_t TPH_ADDR_TABLE[25];

// ==========================================
// 3. API 函数声明
// ==========================================

bk_err_t tph_gpio_init(void);
void tph_power_on(void);
void tph_power_off(void);

void tph_fire_one_column(const TphTestColumn_t *col_data);
void generate_color_bar_pattern(uint32_t col_idx, TphTestColumn_t *out_col);

void tph_test_static(void);                 // 测试 1：不移动进纸马达（单行测试/示波器抓波形）
void tph_test_with_paper_feed(uint32_t lines); // 测试 2：带进纸马达（连续送纸打印）
void dwt_delay_init(void);
void TPH_Init_LUT(void);
	// 接收蓝牙 APP 点阵数据的真实打印函数
void tph_print_photo_buffer(const uint8_t *bin_data, uint32_t total_len);


#ifdef __cplusplus
}
#endif

#endif // TPH_DRIVER_H


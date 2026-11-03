#ifndef TPH_DRIVER_H
#define TPH_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <common/bk_include.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

// ==========================================
// 1. 与小华 HC32F460 通信握手与电源控制引脚 (依据原理图)
// ==========================================
#define PIN_VDD15V_EN          GPIO_13  // 主系统 15V 升压使能 (U3 LP6221)
#define PIN_BK_SYNC_OUT        GPIO_1   // BK_1: 通知 HC32 打火脉冲 (Output)
#define PIN_BK_READY_IN        GPIO_0   // BK_2: HC32 就绪信号检测 (Input)

// 硬件 SPI 引脚 (BK7258 SPI Master)
#define PIN_SPI_CS             GPIO_3   // P3
#define PIN_SPI_SCK            GPIO_2   // P2
#define PIN_SPI_MOSI           GPIO_4   // P4
#define PIN_SPI_MISO           GPIO_5   // P5

// ==========================================
// 2. 喷头与数据结构定义
// ==========================================
#define TPH_COLUMN_BYTES       (100U)   // 25 Address * 4 Section = 100 字节

typedef struct {
    uint8_t c_data[25]; // 青色通道点阵 (200 bits)
    uint8_t m_data[25]; // 品红通道点阵 (200 bits)
    uint8_t y_data[25]; // 黄色通道点阵 (200 bits)
} TphTestColumn_t;

// 喷头颜色掩码定义
#define COLOR_MASK_CYAN         (0x03)  // 青色 (D1, D2)
#define COLOR_MASK_MAGENTA      (0x0C)  // 品红 (D3, D4)
#define COLOR_MASK_YELLOW       (0x30)  // 黄色 (D5, D6)
#define COLOR_MASK_GREEN        (0x33)  // 绿色 (青色+黄色)
#define COLOR_MASK_ALL_BLACK    (0x3F)  // 三色全开 (黑色)

// ==========================================
// 3. 协处理器通信 API
// ==========================================
bk_err_t tph_mcu_comm_init(void);
void tph_power_on(void);
void tph_power_off(void);

// 将 C/M/Y 点阵流通过 SPI 发送并触发 HC32 打火一列
bk_err_t tph_fire_one_column_via_mcu(const TphTestColumn_t *col_data);
bk_err_t tph_fire_raw_column_bytes(const uint8_t *p_column_100b);
#ifdef __cplusplus
}
#endif

#endif // TPH_DRIVER_H


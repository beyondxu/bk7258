#include "tph_driver.h"
#include "motor_sdk_driver.h"
#include <driver/spi.h>
#include <stdio.h>
#include <string.h>

#define TPH_LOGI(...) printf("[TPH_BRIDGE_INFO] " __VA_ARGS__)
#define TPH_LOGE(...) printf("[TPH_BRIDGE_ERR]  " __VA_ARGS__)

extern bk_err_t gpio_dev_unmap(gpio_id_t gpio_id);
__IRAM_SEC void bk_delay_us(UINT32 us);

// SPI 传输单列缓存 (100 字节)
static uint8_t s_spi_tx_column[TPH_COLUMN_BYTES];

static void safe_gpio_output(gpio_id_t pin) {
    gpio_dev_unmap(pin);
    gpio_config_t cfg = {
        .io_mode = GPIO_OUTPUT_ENABLE,
        .pull_mode = GPIO_PULL_DISABLE,
        .func_mode = GPIO_SECOND_FUNC_DISABLE
    };
    bk_gpio_set_config(pin, &cfg);
}

static void safe_gpio_input(gpio_id_t pin) {
    gpio_dev_unmap(pin);
    gpio_config_t cfg = {
        .io_mode = GPIO_INPUT_ENABLE,
        .pull_mode = GPIO_PULL_UP_EN,
        .func_mode = GPIO_SECOND_FUNC_DISABLE
    };
    bk_gpio_set_config(pin, &cfg);
}

/**
 * @brief 初始化与 HC32F460 互联的 GPIO 与 SPI Master
 */

bk_err_t tph_mcu_comm_init(void) {
    // 1. 初始化握手 GPIO
    safe_gpio_output(PIN_BK_SYNC_OUT);
    GPIO_UP(PIN_BK_SYNC_OUT); // BK_1 默认低

    safe_gpio_input(PIN_BK_READY_IN);  // BK_2 默认输入检测

    // 2. 初始化硬件 SPI1 Master (匹配小华从机)
    spi_config_t spi_cfg;
    memset(&spi_cfg, 0, sizeof(spi_cfg));
    spi_cfg.role      = SPI_ROLE_MASTER;
    spi_cfg.bit_width = SPI_BIT_WIDTH_8BITS;
    spi_cfg.polarity  = SPI_POLARITY_HIGH;   // ★ 关键：Mode 3 (CPOL=1) ★
    spi_cfg.phase     = SPI_PHASE_2ND_EDGE; // ★ 关键：Mode 3 (CPHA=1) ★
    spi_cfg.baud_rate = 10000000;            // 10MHz 时钟 (100字节传输仅需 80us)
    spi_cfg.wire_mode = SPI_4WIRE_MODE;
    spi_cfg.bit_order = SPI_MSB_FIRST;
    spi_cfg.dma_mode  = SPI_DMA_MODE_DISABLE; // 100字节极速单工直接发，无需频繁启停DMA

    // ★ 关键：原理图为 SPI1 (P2~P5)，严禁写成 SPI_ID_0 ★
    bk_spi_init(SPI_ID_1, &spi_cfg);

    TPH_LOGI("BK7258 <-> HC32F460 SPI1 (10MHz Mode 3) Ready.\r\n");
    return BK_OK;
}

void tph_power_on(void) {
    TPH_LOGI("Powering ON 15V Boost...\r\n");
    bk_motor_boost_power(1);
    rtos_delay_milliseconds(50); // 稳定 15V

    // 等待 HC32 报告就绪 (BK_2 为高)
    uint32_t timeout = 500;
    while ((bk_gpio_get_input(PIN_BK_READY_IN) == 0) && timeout--) {
        rtos_delay_milliseconds(2);
    }
    if (timeout == 0) {
        TPH_LOGE("Wait HC32 Ready Timeout!\r\n");
    } else {
        TPH_LOGI("HC32 Coprocessor is ONLINE and READY.\r\n");
    }
}

void tph_power_off(void) {
    GPIO_UP(PIN_BK_SYNC_OUT);
    bk_motor_boost_power(0);
}


static void pack_column_data(const TphTestColumn_t *col_data, uint8_t *out_100b) {
    uint32_t out_idx = 0;

    for (uint8_t addr = 0; addr < 25; addr++) {
        for (uint8_t section = 0; section < 4; section++) {
            // 当前喷嘴在 100 个偶/奇喷孔中的基础编号 (0 ~ 99)
            uint32_t nozzle_idx = (section * 25) + addr;

            // 对应 200 喷嘴中的连续线性位置：
            // 偶列喷孔对应 2 * nozzle_idx
            // 奇列喷孔对应 2 * nozzle_idx + 1
            uint32_t idx_e = 2 * nozzle_idx;     // 偶列
            uint32_t idx_o = 2 * nozzle_idx + 1; // 奇列

            // 从 25 字节数组中提取该喷孔的 1-bit 开关状态
            uint8_t c_e = (col_data->c_data[idx_e / 8] >> (idx_e % 8)) & 0x01;
            uint8_t c_o = (col_data->c_data[idx_o / 8] >> (idx_o % 8)) & 0x01;
            uint8_t m_e = (col_data->m_data[idx_e / 8] >> (idx_e % 8)) & 0x01;
            uint8_t m_o = (col_data->m_data[idx_o / 8] >> (idx_o % 8)) & 0x01;
            uint8_t y_e = (col_data->y_data[idx_e / 8] >> (idx_e % 8)) & 0x01;
            uint8_t y_o = (col_data->y_data[idx_o / 8] >> (idx_o % 8)) & 0x01;

            // 打包进 M0333N 对应的 6 个 Domain (D1 ~ D6)
            out_100b[out_idx++] = (c_e) | (c_o << 1) | (m_e << 2) | (m_o << 3) | (y_e << 4) | (y_o << 5);
        }
    }
}


/**
 * @brief 【高速协处理器模式】通过 SPI1 下发一列点阵并产生同步脉冲
 */
__IRAM_SEC void tph_fire_one_column_via_mcu(const TphTestColumn_t *col_data) {
    // 1. 本地快速位重组打包 (耗时 < 3us)
    pack_column_data(col_data, s_spi_tx_column);

    // 2. 通过 SPI1 Master 发送 100 字节至 HC32 双缓冲 DMA (10MHz 耗时仅 80us)
    bk_spi_write_bytes(SPI_ID_1, s_spi_tx_column, TPH_COLUMN_BYTES);

    // 3. 产生 BK_1 (GPIO_1) 正脉冲，通知 HC32 启动本列物理微秒打火
    GPIO_DOWN(PIN_BK_SYNC_OUT);
   
    bk_delay_us(5); // 脉冲保持 5us，确保小华中断/电平检测 100% 捕获
    GPIO_UP(PIN_BK_SYNC_OUT);
}
/**
 * @brief 通过 SPI 下发 100 字节点阵，并产生 BK_1 (P1) 低电平有效打火脉冲
 */
__IRAM_SEC void tph_fire_raw_column_bytes(const uint8_t *p_column_100b) {
    // 1. SPI1 极速下发 100 字节 (10MHz 耗时仅 80us)
    //bk_spi_dma_write_bytes(SPI_ID_1, (uint8_t *)p_column_100b, TPH_COLUMN_BYTES);
	// 这样能 100% 保证 HC32 端的 DMA 已经把数据完完整整地放进了内存！

	// 加一个静态计数器，每 200 列打印一次 BK7258 真正送进 SPI 寄存器的第 1 个字节
		static uint32_t s_send_cnt = 0;
		if (s_send_cnt++ % 200 == 0) {
			os_printf("[BK TX CHECK] Sending Byte[0]=0x%02X, Byte[1]=0x%02X\r\n", 
					  p_column_100b[0], p_column_100b[1]);
		}

	bk_spi_write_bytes(SPI_ID_1, (uint8_t *)p_column_100b, TPH_COLUMN_BYTES);

    // 2. ★ 产生低电平有效打火脉冲：平时为高，拉低 5us 后恢复为高 ★
    bk_gpio_set_output_low(PIN_BK_SYNC_OUT);   // 拉低 (0V) 触发小华打火！
    bk_delay_us(5);                            // 保持 5us 脉宽
    bk_gpio_set_output_high(PIN_BK_SYNC_OUT);  // 恢复高电平 (3.3V)
}



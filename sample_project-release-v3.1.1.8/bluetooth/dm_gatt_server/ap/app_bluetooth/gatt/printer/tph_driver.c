#include "tph_driver.h"
#include "printer_control.h"
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
    bk_err_t result = bk_spi_init(SPI_ID_1, &spi_cfg);
    if (result != BK_OK) return result;

    TPH_LOGI("BK7258 <-> HC32F460 SPI1 (10MHz Mode 3) Ready.\r\n");
    return BK_OK;
}

void tph_power_on(void) {
    TPH_LOGI("Powering ON 15V Boost...\r\n");
    bk_motor_boost_power(1);
    rtos_delay_milliseconds(50); // 稳定 15V

    // 等待 HC32 报告就绪 (BK_2 为高)
    uint32_t timeout = 500;
    while (timeout && (bk_gpio_get_input(PIN_BK_READY_IN) == 0)) {
        --timeout;
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
/* BK_2 is HIGH only while HC32 has armed a complete 100-byte receive.
 * BK_1 stays LOW until HC32 acknowledges the completed DMA with BK_2 LOW.
 * The next HIGH on BK_2 means firing has finished and the next DMA is armed.
 * Both MCUs must be updated together; a 5us pulse is not a reliable handshake.
 */
static bk_err_t wait_hc32_level(uint32_t level)
{
    for (uint32_t elapsed = 0; elapsed < 100000; elapsed += 5) {
        if (bk_gpio_get_input(PIN_BK_READY_IN) == level) return BK_OK;
        bk_delay_us(5);
    }
    return BK_FAIL;
}

bk_err_t tph_fire_one_column_via_mcu(const TphTestColumn_t *col_data)
{
    if (col_data == NULL) return BK_FAIL;
    pack_column_data(col_data, s_spi_tx_column);
    return tph_fire_raw_column_bytes(s_spi_tx_column);
}

bk_err_t tph_fire_raw_column_bytes(const uint8_t *column)
{
    if (column == NULL || wait_hc32_level(1) != BK_OK) return BK_FAIL;
    if (bk_spi_write_bytes(SPI_ID_1, column, TPH_COLUMN_BYTES) != BK_OK) {
        return BK_FAIL;
    }
    bk_gpio_set_output_low(PIN_BK_SYNC_OUT);
    bk_err_t result = wait_hc32_level(0);
    bk_gpio_set_output_high(PIN_BK_SYNC_OUT);
    if (result != BK_OK) {
        TPH_LOGE("Column DMA acknowledgement timed out; abort job.\r\n");
        return result;
    }
    return wait_hc32_level(1);
}

bk_err_t tph_mcu_set_head_power(uint8_t enable)
{
    uint8_t command[TPH_COLUMN_BYTES] = {0};
    command[0] = PRINTER_CONTROL_TAG;
    command[1] = enable ? PRINTER_CONTROL_ON : PRINTER_CONTROL_OFF;
    memcpy(command + 2, PRINTER_CONTROL_MAGIC, PRINTER_CONTROL_MAGIC_LEN);
    return tph_fire_raw_column_bytes(command);
}

/* LBUFv1: all packet handshakes finish BEFORE any motion. During a cached
 * traversal only the existing SYNC wire changes at the image origin. */
static bk_err_t tph_line_read_status(uint8_t status[PL_BYTES]) {
    if(wait_hc32_level(1)!=BK_OK)return BK_FAIL;
    if(bk_spi_read_bytes(SPI_ID_1,status,PL_BYTES)!=BK_OK)return BK_FAIL;
    bk_gpio_set_output_low(PIN_BK_SYNC_OUT);
    bk_err_t rc=wait_hc32_level(0);
    bk_gpio_set_output_high(PIN_BK_SYNC_OUT);
    if(rc!=BK_OK||wait_hc32_level(1)!=BK_OK)return BK_FAIL;
    return pl_status_valid(status)?BK_OK:BK_FAIL;
}
bk_err_t tph_line_probe(void) {
    uint8_t status[PL_BYTES];
    if(tph_line_read_status(status)!=BK_OK){TPH_LOGE("[LINE_PEER] LBUFv1 missing_or_bad_reply no_motion_no_fire\r\n");return BK_FAIL;}
    return BK_OK;
}
bk_err_t tph_line_cancel(const tph_line_ticket_t *ticket) {
    uint8_t command[PL_BYTES];
    if(!ticket)return BK_FAIL;
    pl_command(command,PL_CANCEL,ticket->token,ticket->period_us,ticket->crc);
    return tph_fire_raw_column_bytes(command);
}
static bk_err_t tph_line_prepare_profile(const uint8_t *columns,uint32_t period_us,tph_line_ticket_t *ticket,unsigned profile) {
    static uint32_t token;
    uint8_t command[PL_BYTES],status[PL_BYTES];
    if(profile>PL_FIRE_PRESPRAY||!columns||!ticket||period_us<(profile==PL_FIRE_PRESPRAY?PL_PRESPRAY_PERIOD_US:PL_MIN_PERIOD_US)||period_us>PL_MAX_PERIOD_US)return BK_FAIL;
    if(tph_mcu_set_head_power(0)!=BK_OK||tph_line_probe()!=BK_OK)return BK_FAIL;
    if(profile==PL_FIRE_PRESPRAY){
        if(tph_line_read_status(status)!=BK_OK || (status[10]&(PL_CAP_PRESPRAY_FIRE|PL_CAP_PRESPRAY_FAST))!=(PL_CAP_PRESPRAY_FIRE|PL_CAP_PRESPRAY_FAST) || status[32]!=PL_PRESPRAY_PAD_NOPS || pl_get16(status+33)!=PL_PRESPRAY_PERIOD_US){
            TPH_LOGE("[LINE_FIRE_PROFILE_REJECT] HC32_prespray_profile_missing_or_mismatched update_HC32_first no_ARM_no_fire\r\n");
            return BK_FAIL;
        }
    }
    if(++token==0)++token;
    ticket->token=token;ticket->period_us=period_us;ticket->crc=pl_crc(columns,PL_ROW_BYTES);
    pl_command(command,profile==PL_FIRE_PRESPRAY?PL_BEGIN_PRESPRAY:PL_BEGIN,ticket->token,period_us,ticket->crc);
    if(tph_fire_raw_column_bytes(command)!=BK_OK)return BK_FAIL;
    for(unsigned i=0;i<PL_COLUMNS;i++)if(tph_fire_raw_column_bytes(columns+i*PL_BYTES)!=BK_OK)return BK_FAIL;
    if(tph_line_read_status(status)!=BK_OK){TPH_LOGE("[LINE_BUFFER_REJECT] invalid_status no_motion_no_fire\r\n");return BK_FAIL;}
    if(status[8]!=PL_READY||status[9]!=PL_OK||status[11]!=profile||pl_get16(status+12)!=PL_COLUMNS||pl_get32(status+16)!=ticket->token||pl_get32(status+20)!=period_us||pl_get32(status+24)!=ticket->crc){
        TPH_LOGE("[LINE_BUFFER_REJECT] state=%u error=%u received=%u token=%lu/%lu crc32=%08lX/%08lX profile=%u/%u no_motion_no_fire\r\n",status[8],status[9],(unsigned)pl_get16(status+12),(unsigned long)pl_get32(status+16),(unsigned long)ticket->token,(unsigned long)pl_get32(status+24),(unsigned long)ticket->crc,status[11],profile);
        return BK_FAIL;
    }
    pl_command(command,PL_ARM,ticket->token,period_us,ticket->crc);
    if(tph_fire_raw_column_bytes(command)!=BK_OK)return BK_FAIL;
    TPH_LOGI("[LINE_BUFFER_READY] token=%lu columns=1180 period_us=%lu crc32=%08lX fire_profile=%u extra_pad_nops=%u start_SYNC_only\r\n",(unsigned long)ticket->token,(unsigned long)period_us,(unsigned long)ticket->crc,profile,profile?PL_PRESPRAY_PAD_NOPS:0U);
    return BK_OK;
}
bk_err_t tph_line_prepare(const uint8_t *columns,uint32_t period_us,tph_line_ticket_t *ticket){
    return tph_line_prepare_profile(columns,period_us,ticket,PL_FIRE_NORMAL);
}
bk_err_t tph_line_prepare_prespray(const uint8_t *columns,uint32_t period_us,tph_line_ticket_t *ticket){
    return tph_line_prepare_profile(columns,period_us,ticket,PL_FIRE_PRESPRAY);
}
bk_err_t tph_line_finish(const tph_line_ticket_t *ticket) {
    uint8_t status[PL_BYTES];
    if(!ticket||tph_line_read_status(status)!=BK_OK)return BK_FAIL;
    unsigned ok=status[8]==PL_DONE&&status[9]==PL_OK&&pl_get16(status+14)==PL_COLUMNS&&pl_get32(status+16)==ticket->token&&pl_get32(status+20)==ticket->period_us&&pl_get32(status+24)==ticket->crc;
    TPH_LOGI("[LINE_BUFFER_RESULT] token=%lu state=%u error=%u fired=%u/1180 crc32=%08lX match=%u ink_unverified\r\n",(unsigned long)ticket->token,status[8],status[9],(unsigned)pl_get16(status+14),(unsigned long)pl_get32(status+24),ok);
    return ok?BK_OK:BK_FAIL;
}

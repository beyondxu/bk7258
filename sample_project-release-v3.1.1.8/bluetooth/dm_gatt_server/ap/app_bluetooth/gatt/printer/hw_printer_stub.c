#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <components/log.h>
#include <os/os.h>
#include <os/mem.h>

#include "tph_driver.h"
#include "motor_sdk_driver.h"
#include "printer_protocol.h"
#include "printer_diagnostics.h"
#include "ble_printer_service.h"
#include "gpio_driver.h"

#define TAG "HW_PRINTER"

#define LINE_FEED_STEPS        2340
#define PAPER_EJECT_STEPS      4500

#define TOTAL_PRINT_COLS       1180
#define SWEEP_COLS_TOTAL       1180
#define CARRIAGE_RUN_STEPS     7480 // 严格 7480 步防撞

#define SWATH_BYTES_PER_COL    24   // 1180 * 24 = 28320 字节

static uint8_t *s_print_buffer = NULL;
static uint32_t s_max_buffer_size = 0;
static uint32_t s_buffer_len = 0;

static beken_thread_t s_print_task_handle = NULL;
static beken_semaphore_t s_print_sem = NULL;

static uint8_t (*s_final_swath_cols)[TPH_COLUMN_BYTES] = NULL;
static uint8_t *s_unpacked_swath = NULL;

#define SET_GPIO_HIGH(pin)   GPIO_UP(pin)
#define SET_GPIO_LOW(pin)    GPIO_DOWN(pin)

__IRAM_SEC void bk_delay_us(UINT32 us);
/*
// 规格书附页 B 偶列喷孔真实物理地址表
static const uint8_t s_EvenNozzleToAddr[25] = {
    0,  7, 13, 19,  1,  8, 14, 20,  2,  9,
   15, 21,  3, 10, 16, 22,  4, 11, 17, 23,
    5, 12, 18, 24,  6
};
*/
void hw_printer_init(void) {
    tph_mcu_comm_init();
    bk_motor_gpio_config_init();
}

bool hw_printer_prepare_buffer(uint32_t total_len) {
    if (s_print_buffer) {
        psram_free(s_print_buffer);
        s_print_buffer = NULL;
    }
    uint32_t alloc_size = total_len + 4096;
    s_print_buffer = (uint8_t *)psram_malloc(alloc_size);
    if (!s_print_buffer) return false;

    s_max_buffer_size = alloc_size;
    s_buffer_len = 0;
    return true;
}

void hw_printer_append_data(const uint8_t *data, uint16_t len) {
    if (s_print_buffer && (s_buffer_len + len <= s_max_buffer_size)) {
        memcpy(s_print_buffer + s_buffer_len, data, len);
        s_buffer_len += len;
    }
}

void hw_printer_clear_buffer(void) {
    if (s_print_buffer) {
        psram_free(s_print_buffer);
        s_print_buffer = NULL;
    }
    s_buffer_len = 0;
    s_max_buffer_size = 0;
}

static int decode_packbits_full(const uint8_t *src, int src_len, uint8_t *dst, int max_dst_len) {
    int src_idx = 0;
    int dst_idx = 0;
    while (src_idx < src_len && dst_idx < max_dst_len) {
        uint8_t flag = src[src_idx++];
        if (flag == 0x80) continue;
        else if (flag > 0x80) {
            int count = 256 - flag + 1;
            if (src_idx >= src_len) break;
            uint8_t val = src[src_idx++];
            for (int i = 0; i < count && dst_idx < max_dst_len; i++) dst[dst_idx++] = val;
        } else {
            int count = flag + 1;
            for (int i = 0; i < count && src_idx < src_len && dst_idx < max_dst_len; i++) {
                dst[dst_idx++] = src[src_idx++];
            }
        }
    }
    return dst_idx;
}

static bool sweep_and_fire_line_direct(void) {
    uint32_t accel_steps = 200;
    uint32_t decel_steps = 200;

    SET_GPIO_HIGH(TPH_MOT_DIR);
    bk_delay_us(50);

    for (uint32_t s = 0; s < accel_steps; s++) {
        uint32_t d = 1000 - ((1000 - 300) * s / accel_steps);
        SET_GPIO_HIGH(TPH_MOT_STEP); bk_delay_us(4);
        SET_GPIO_LOW(TPH_MOT_STEP);  bk_delay_us(d);
    }

    for (uint32_t col = 0; col < SWEEP_COLS_TOTAL; col++) {
        if (tph_fire_raw_column_bytes(s_final_swath_cols[col]) != BK_OK) {
            motor_all_power_down();
            return false;
        }

        for (int m = 0; m < 6; m++) {
            SET_GPIO_HIGH(TPH_MOT_STEP); bk_delay_us(4);
            SET_GPIO_LOW(TPH_MOT_STEP);  bk_delay_us(300);
        }
    }

    for (uint32_t s = 0; s < decel_steps; s++) {
        uint32_t d = 300 + ((1000 - 300) * s / decel_steps);
        SET_GPIO_HIGH(TPH_MOT_STEP); bk_delay_us(4);
        SET_GPIO_LOW(TPH_MOT_STEP);  bk_delay_us(d);
    }
    return true;
}

// =========================================================================
// ★★★ 任务 A：BK7258 内部自主手绘单像素纯黑线直打（完全脱离手机 APP！）★★★
// =========================================================================
void test_print_standalone_single_black_line(void) {
    os_printf("\r\n========================================================\r\n");
    os_printf(">>> [NOZZLE DIAGNOSTIC] Printing Selected Domain Mask/Slot Line <<<\r\n");
    os_printf("========================================================\r\n");

    s_final_swath_cols = (uint8_t (*)[TPH_COLUMN_BYTES])psram_malloc(SWEEP_COLS_TOTAL * TPH_COLUMN_BYTES);
    if (!s_final_swath_cols) return;

    memset(s_final_swath_cols, 0, SWEEP_COLS_TOTAL * TPH_COLUMN_BYTES);
    hw_printer_init();
    motor_all_power_up_and_enable();

    // 在相纸中间：第 200 列到第 900 列，只点亮【唯一的一个物理喷嘴】！
    // Address 10, Section 0 -> 对应的 byte_idx = 10 * 4 + 0 = 40
    // A diagnostic mask may select multiple physical nozzles.
    uint32_t fixed_byte_idx = PRINTER_DIAG_SLOT;

    for (uint32_t col = 200; col < 900; col++) {
        s_final_swath_cols[col][fixed_byte_idx] = (uint8_t)PRINTER_DIAG_MASK;
    }

    os_printf(">>> Firing Standalone Sweep (ByteIdx=%lu, mask=0x%02X) <<<\r\n",
              (unsigned long)fixed_byte_idx, (unsigned)PRINTER_DIAG_MASK);

    // 扫掠打火一趟
    if (!sweep_and_fire_line_direct()) {
        psram_free(s_final_swath_cols);
        s_final_swath_cols = NULL;
        return;
    }
    rtos_delay_milliseconds(40);
    motor_carriage_return_home(CARRIAGE_RUN_STEPS);
    rtos_delay_milliseconds(100);

    motor_all_power_down();

    psram_free(s_final_swath_cols);
    s_final_swath_cols = NULL;
    os_printf("[DIAG] single-slot line complete; paper feed unchanged\r\n");
}

// 映射函数 (无环形折叠，严格单线映射)
static void map_channel_pixels_to_tph(const uint8_t *raw_28320, uint8_t channel) {
    uint8_t even_bit = (channel == 1) ? 0 : (channel == 2 ? 2 : 4);
    uint8_t odd_bit  = (channel == 1) ? 1 : (channel == 2 ? 3 : 5);

    for (uint32_t col = 0; col < TOTAL_PRINT_COLS; col++) {
        uint32_t col_src_base = col * SWATH_BYTES_PER_COL;

        // 直接一对一映射，不加任何环形折叠！
        for (int b = 0; b < 24; b++) {
            uint8_t byte_val = raw_28320[col_src_base + b];
            if (byte_val == 0) continue;

            // 映射到固定物理槽位
            int byte_idx = b * 4; // 只写 Section 0，绝不跨区
            s_final_swath_cols[col][byte_idx] |= (1 << even_bit) | (1 << odd_bit);
        }
    }
}

/**
 * @brief ★★★ 任务 B：物理打火前门闸 LOG 审计 ★★★
 * 只有当且仅当数据 100% 是一条线时，才允许打火！
 */
static void parse_and_execute_stream_perfect(const uint8_t *buf, uint32_t len) {
    os_printf("\r\n========================================================\r\n");
    os_printf(">>> [GATEWAY AUDIT & PRINT ENGINE] Len = %lu Bytes <<<\r\n", (unsigned long)len);
    os_printf("========================================================\r\n");

    s_final_swath_cols = (uint8_t (*)[TPH_COLUMN_BYTES])psram_malloc(SWEEP_COLS_TOTAL * TPH_COLUMN_BYTES);
    s_unpacked_swath   = (uint8_t *)psram_malloc(36000);
    if (!s_final_swath_cols || !s_unpacked_swath) return;

    memset(s_final_swath_cols, 0, SWEEP_COLS_TOTAL * TPH_COLUMN_BYTES);
    hw_printer_init();

    uint32_t idx = 0;
    uint8_t current_channel = 0;
    uint32_t completed_swaths = 0;

    while (idx < len - 4) {
        if (buf[idx] == 0x1B && buf[idx+1] == 0x56) {
            current_channel = buf[idx+2];
            idx += 3;
            continue;
        }

        if (buf[idx] == 0x1B && buf[idx+1] == 0x25) {
            idx += 2;
            uint32_t data_start = idx + 3;
            if (data_start >= len) break;

            uint32_t search_pos = data_start;
            while (search_pos < (len - 1)) {
                if (buf[search_pos] == 0x1B && (buf[search_pos+1] == 0x56 || buf[search_pos+1] == 0x25)) {
                    break;
                }
                search_pos++;
            }
            uint32_t block_size = search_pos - data_start;
            idx = search_pos;

            memset(s_unpacked_swath, 0, 36000);
            int unpacked_len = decode_packbits_full(&buf[data_start], block_size, s_unpacked_swath, 36000);

            if (unpacked_len < 100) continue;

            map_channel_pixels_to_tph(s_unpacked_swath, current_channel);

            // 当 Channel 3 到齐，在真正打火前执行【门闸审查】！
            if (current_channel == 3) {
                completed_swaths++;

                // 扫描硬件点阵中被激活的物理槽位数
                int active_slots = 0;
             //   int active_byte_mark = -1;
                for (int b = 0; b < 100; b++) {
                    int count = 0;
                    for (int c = 0; c < SWEEP_COLS_TOTAL; c++) {
                        if (s_final_swath_cols[c][b] != 0) count++;
                    }
                    if (count > 50) {
                        active_slots++;
                       // active_byte_mark = b;
                        os_printf("   -> Hardware ByteIdx[%d] is Active (%d dots)\r\n", b, count);
                    }
                }

                os_printf(">>> [SWATH %lu GATEWAY] Active Heights = %d <<<\r\n", (unsigned long)completed_swaths, active_slots);

                // ★★★ 核心把关：如果只激活了 1 个物理高度，100% 安全，放行真实打印！★★★
                if (active_slots == 1) {
                    os_printf("🌟 [AUDIT PASSED! EXECUTING REAL HARDWARE PRINT!]\r\n");
                    motor_all_power_up_and_enable();
                    if (!sweep_and_fire_line_direct()) goto cleanup;
                    motor_feed_paper(LINE_FEED_STEPS, 1);
                    motor_carriage_return_home(CARRIAGE_RUN_STEPS);
                } else if (active_slots > 1) {
                    // 如果超过 1 个高度，说明数据有鬼，直接拦截报警，不打火！
                    os_printf("❌ [AUDIT REJECTED!] Multi-line detected (%d lines), blocked fire to protect paper!\r\n", active_slots);
                } else {
                    os_printf("-> [BLANK SWATH] Skipping motor fire.\r\n");
                }

                memset(s_final_swath_cols, 0, SWEEP_COLS_TOTAL * TPH_COLUMN_BYTES);
            }
            continue;
        }
        idx++;
    }

    motor_feed_paper(PAPER_EJECT_STEPS, 1);
cleanup:
    motor_all_power_down();

    psram_free(s_final_swath_cols);
    psram_free(s_unpacked_swath);
    s_final_swath_cols = NULL;
    s_unpacked_swath = NULL;
}

static void printer_worker_thread(void *arg) {
    while (1) {
        rtos_get_semaphore(&s_print_sem, BEKEN_WAIT_FOREVER);
        if (s_print_buffer && s_buffer_len > 0) {
            parse_and_execute_stream_perfect(s_print_buffer, s_buffer_len);
        }
        hw_printer_clear_buffer();
    }
}

void hw_printer_start_job_async(void) {
    if (s_print_sem) rtos_set_semaphore(&s_print_sem);
}

void hw_printer_create_task(void) {
    if (s_print_sem == NULL) {
        rtos_init_semaphore(&s_print_sem, 1);
        rtos_create_thread(&s_print_task_handle, 4, "prt_worker", printer_worker_thread, 4096, NULL);
    }
}


/* Called before BLE starts, so it cannot overlap an incoming print job. */
void printer_run_startup_diagnostic(void)
{
    uint8_t column[TPH_COLUMN_BYTES] = {0};
    if (PRINTER_STARTUP_DIAGNOSTIC == 0) return;
    if (tph_mcu_comm_init() != BK_OK) {
        os_printf("[DIAG] SPI init failed\r\n");
        return;
    }
    if (PRINTER_STARTUP_DIAGNOSTIC == 3) {
        test_print_standalone_single_black_line();
        return;
    }
    if (PRINTER_STARTUP_DIAGNOSTIC == 2) {
        column[PRINTER_DIAG_SLOT] = (uint8_t)PRINTER_DIAG_MASK;
    }
    tph_power_on();
    if (PRINTER_STARTUP_DIAGNOSTIC == 4) {
        os_printf("[DIAG POWER] 30-second measurement window: no columns, no motion. Measure VDD_15V, VDD_VL and VDD_TPH against GND.\r\n");
        rtos_delay_milliseconds(30000);
        tph_power_off();
        os_printf("[DIAG POWER] window closed; boost disabled.\r\n");
        return;
    }
    unsigned count = PRINTER_STARTUP_DIAGNOSTIC == 1 ? 100U : 1U;
    unsigned sent = 0;
    while (sent < count) {
        if (tph_fire_raw_column_bytes(column) != BK_OK) break;
        ++sent;
    }
    tph_power_off();
    os_printf("[DIAG] mode=%u slot=%u mask=0x%02X columns=%u/%u %s\r\n",
              PRINTER_STARTUP_DIAGNOSTIC, PRINTER_DIAG_SLOT,
              (unsigned)PRINTER_DIAG_MASK, sent, count, sent == count ? "PASS" : "FAIL");
}

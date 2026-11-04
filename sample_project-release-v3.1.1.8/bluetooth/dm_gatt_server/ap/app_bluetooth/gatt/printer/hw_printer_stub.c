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
#include "printer_audit_utils.h"
#include "printer_mapper.h"
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
    if (!total_len || total_len > 4U * 1024U * 1024U) return false;
    uint32_t alloc_size = total_len;
    s_print_buffer = (uint8_t *)psram_malloc(alloc_size);
    if (!s_print_buffer) return false;

    s_max_buffer_size = alloc_size;
    s_buffer_len = 0;
    return true;
}

bool hw_printer_append_data(const uint8_t *data, uint32_t len) {
    if (!s_print_buffer || !data || len > s_max_buffer_size - s_buffer_len) return false;
    memcpy(s_print_buffer + s_buffer_len, data, len);
    s_buffer_len += len;
    return true;
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

/* Geometry candidates are logged so a fingerprint never hides calibration. */
static void *printer_map_psram_alloc(size_t bytes)
{
    if (bytes > 4U * 1024U * 1024U) return NULL;
    return psram_malloc(bytes);
}
static void printer_map_psram_free(void *ptr)
{
    psram_free(ptr);
}
static void printer_map_yield_cpu(void)
{
    rtos_delay_milliseconds(1);
}
static void printer_run_mapping_audit(const uint8_t *bin, uint32_t length)
{
    printer_map_config_t cfg;
    printer_map_stats_t stats;
    printer_map_default_config(&cfg);
    cfg.x_pitch_centi_um = PRINTER_MAP_X_PITCH_CENTI_UM;
    cfg.x_direction = PRINTER_MAP_X_DIRECTION;
    cfg.first_nozzle = PRINTER_MAP_FIRST_NOZZLE;
    cfg.reverse_y = PRINTER_MAP_REVERSE_Y;
    os_printf("[MAP_CONFIG] pitch_centi_um=%lu direction=%ld first_nozzle=%lu reverse_y=%lu CANDIDATE_ONLY\r\n",
              (unsigned long)cfg.x_pitch_centi_um, (long)cfg.x_direction,
              (unsigned long)cfg.first_nozzle, (unsigned long)cfg.reverse_y);
    int result = printer_map_bin_dry_run(bin, length, &cfg, &stats,
                                        printer_map_psram_alloc, printer_map_psram_free, printer_map_yield_cpu);
    if (result) {
        os_printf("[MAP_AUDIT] result=%d NOT_MAPPED no_physical_print\r\n", result);
        return;
    }
    os_printf("[MAP_AUDIT] swaths=%lu width=%lu max_delay=%lu columns=%lu crc32=%08lX\r\n",
              (unsigned long)stats.swaths, (unsigned long)stats.max_width,
              (unsigned long)stats.max_delay, (unsigned long)stats.output_columns,
              (unsigned long)stats.columns_crc32);
    os_printf("[MAP_DOTS] input_CMY=%lu,%lu,%lu output_CMY=%lu,%lu,%lu no_physical_print\r\n",
              (unsigned long)stats.input_dots[0], (unsigned long)stats.input_dots[1], (unsigned long)stats.input_dots[2],
              (unsigned long)stats.output_dots[0], (unsigned long)stats.output_dots[1], (unsigned long)stats.output_dots[2]);
}

extern bool printer_link_is_ready(void);
/* One explicitly selected APP strip; no page feed, no trajectory extension. */
static int printer_run_bounded_line(const uint8_t *bin, uint32_t length)
{
    static bool attempted = false;
    uint8_t *columns = printer_map_psram_alloc(SWEEP_COLS_TOTAL * TPH_COLUMN_BYTES);
    if (!columns) return -2;
    printer_map_config_t cfg;
    printer_map_stats_t stats;
    printer_map_capture_t capture;
    printer_map_default_config(&cfg);
    cfg.x_pitch_centi_um = PRINTER_MAP_X_PITCH_CENTI_UM;
    cfg.x_direction = PRINTER_MAP_X_DIRECTION;
    cfg.first_nozzle = PRINTER_MAP_FIRST_NOZZLE;
    cfg.reverse_y = PRINTER_MAP_REVERSE_Y;
    memset(&capture, 0, sizeof(capture));
    capture.columns = columns;
    capture.buffer_bytes = SWEEP_COLS_TOTAL * TPH_COLUMN_BYTES;
    capture.column_limit = SWEEP_COLS_TOTAL;
    capture.target_y = 576;
    capture.first_row = 175;
    capture.last_row = 175;
    capture.channel_mask = PRINTER_BOUNDED_CHANNEL_MASK;
    capture.min_column = 200;
    capture.max_column = 999;
    int result = printer_map_bin_capture(bin, length, &cfg, &stats, &capture,
                                        printer_map_psram_alloc, printer_map_psram_free, printer_map_yield_cpu);
    unsigned dots = capture.selected_dots[0] + capture.selected_dots[1] + capture.selected_dots[2];
    if (!result && dots < 64) result = -6;
    os_printf("[LINE_PLAN] result=%d y=576 row=175 mask=%u dots_CMY=%lu,%lu,%lu first=%lu last=%lu columns=1180 steps=7480 feed=0\r\n",
              result, (unsigned)capture.channel_mask, (unsigned long)capture.selected_dots[0],
              (unsigned long)capture.selected_dots[1], (unsigned long)capture.selected_dots[2],
              (unsigned long)capture.first_active, (unsigned long)capture.last_active);
    if (result) goto done;
    if (!PRINTER_BOUNDED_PRINT) {
        os_printf("[LINE_PLAN] PLAN_ONLY no_SPI_no_motion_no_ink\r\n");
        goto done;
    }
    if (attempted || !printer_link_is_ready()) { result = -7; goto done; }
    attempted = true; /* Never automatically repeat a potentially fired job. */
    if (tph_mcu_comm_init() != BK_OK || tph_mcu_set_head_power(0) != BK_OK) { result = -8; goto done; }
    motor_all_power_up_and_enable();
    bk_motor_set_state(0, 0); /* paper motor stays asleep throughout */
    if (tph_mcu_set_head_power(1) != BK_OK) { result = -8; goto stop; }
    s_final_swath_cols = (uint8_t (*)[TPH_COLUMN_BYTES])columns;
    if (!sweep_and_fire_line_direct()) { result = -9; goto stop; }
    if (tph_mcu_set_head_power(0) != BK_OK) { result = -8; goto stop; }
    motor_carriage_return_home(CARRIAGE_RUN_STEPS);
    os_printf("[LINE_DONE] sweep_and_return=1 feed=0 ink_unverified\r\n");
stop:
    if (result) {
        (void)tph_mcu_set_head_power(0);
        os_printf("[LINE_STOP] code=%d no_blind_return_after_failure\r\n", result);
    }
    motor_all_power_down();
    s_final_swath_cols = NULL;
done:
    printer_map_psram_free(columns);
    return result;
}

static void printer_worker_thread(void *arg) {
    while (1) {
        rtos_get_semaphore(&s_print_sem, BEKEN_WAIT_FOREVER);
        if (PRINTER_DATA_AUDIT) {
            uint32_t length = s_buffer_len;
            uint32_t crc = printer_crc32(s_print_buffer, length);
            uint32_t dump_len = length < PRINTER_AUDIT_DUMP_MAX_BYTES ? length : PRINTER_AUDIT_DUMP_MAX_BYTES;
            os_printf("[AUDIT_BEGIN] len=%lu crc32=%08lX dump_len=%lu\r\n",
                      (unsigned long)length, (unsigned long)crc, (unsigned long)dump_len);
            for (uint32_t offset = 0; offset < dump_len; offset += 32) {
                char line[112];
                unsigned used = (unsigned)snprintf(line, sizeof(line), "[AUDIT_HEX] %08lX ", (unsigned long)offset);
                uint32_t count = dump_len - offset;
                if (count > 32) count = 32;
                for (uint32_t i = 0; i < count; ++i) {
                    used += (unsigned)snprintf(line + used, sizeof(line) - used, "%02X", s_print_buffer[offset + i]);
                }
                os_printf("%s\r\n", line);
                if ((offset & 511U) == 0) rtos_delay_milliseconds(1);
            }
            os_printf("[AUDIT_END] len=%lu dump_len=%lu %s\r\n", (unsigned long)length,
                      (unsigned long)dump_len, dump_len == length ? "COMPLETE" : "TRUNCATED");
            if (PRINTER_MAP_AUDIT) printer_run_mapping_audit(s_print_buffer, length);
            int line_result = 0;
            if (PRINTER_BOUNDED_LINE) line_result = printer_run_bounded_line(s_print_buffer, length);
            hw_printer_clear_buffer();
            if (PRINTER_BOUNDED_PRINT) printer_protocol_bounded_complete(length, crc, line_result);
            else printer_protocol_audit_complete(length, crc);
            continue;
        }
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
    if (PRINTER_DATA_AUDIT || PRINTER_STARTUP_DIAGNOSTIC == 0) return;
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

/* Paired HC32 stays powered off: only PCTL OFF is sent, never a firing column. */
void printer_run_carriage_diagnostic(void)
{
    uint32_t forward = 0, reverse = 0;
    int result = -1;
    if (tph_mcu_set_head_power(0) != BK_OK) {
        os_printf("[CARRIAGE_TEST] head_OFF handshake failed; NO_MOTION\r\n");
        return;
    }
    motor_carriage_test_power_up();
    os_printf("[CARRIAGE_TEST] BEGIN steps_each=7480 timing=USER_RAMP_SDK_TIMER delay=bk_delay_us ramp=250 start=1000 cruise=260 high=4 nFAULT=IGNORED feed=0 ink=OFF MS=111 no_position_feedback\r\n");
    result = motor_carriage_test_leg(1, CARRIAGE_RUN_STEPS, &forward);
    os_printf("[CARRIAGE_TEST] FORWARD result=%d pulses=%lu\r\n", result, (unsigned long)forward);
    if (result == BK_OK) {
        rtos_delay_milliseconds(200);
        result = motor_carriage_test_leg(0, CARRIAGE_RUN_STEPS, &reverse);
        os_printf("[CARRIAGE_TEST] REVERSE result=%d pulses=%lu\r\n", result, (unsigned long)reverse);
    }
    rtos_delay_milliseconds(300);
    bk_motor_set_state(1, 0);
    motor_all_power_down();
    os_printf("[CARRIAGE_TEST] END result=%d forward=%lu reverse=%lu pulse_counts_only inspect_actual_motion\r\n",
              result, (unsigned long)forward, (unsigned long)reverse);
}
/**
 * @brief ★ 字车电机单向平稳移动 (带纯净梯形加减速) ★
 * @param direction 1: 向右 (前行), 0: 向左 (归位)
 * @param total_steps 总步数 (例如 7480)
 */
 
#define SHUTTLE_TOTAL_STEPS    7480 // 经过实测验证的安全往返行程
#define ACCEL_RAMP_STEPS       250  // 平滑加减速步数
static void carriage_shuttle_move_ramped(uint8_t direction, uint32_t total_steps) {
    // 1. 设置物理方向
    if (direction) {
        GPIO_UP(TPH_MOT_DIR);   // 向右
    } else {
        GPIO_DOWN(TPH_MOT_DIR); // 向左
    }
    delay_us(50); // 方向建立时间

    // 2. 梯形速度曲线参数
    uint32_t start_delay_us  = 3000; // 起步慢速 (1000us) 稳健力矩起动
    uint32_t cruise_delay_us = 260;  // 巡航高速 (260us) 平稳匀速段
    uint32_t current_delay   = start_delay_us;

    for (uint32_t i = 0; i < total_steps; i++) {
        // A. 加速段
        if (i < ACCEL_RAMP_STEPS) {
            current_delay = start_delay_us - ((start_delay_us - cruise_delay_us) * i / ACCEL_RAMP_STEPS);
        }
        // B. 减速段
        else if (i >= (total_steps - ACCEL_RAMP_STEPS)) {
            uint32_t decel_i = total_steps - 1 - i;
            current_delay = start_delay_us - ((start_delay_us - cruise_delay_us) * decel_i / ACCEL_RAMP_STEPS);
        }
        // C. 匀速段
        else {
            current_delay = cruise_delay_us;
        }

        // 发送 STEP 物理脉冲
        GPIO_UP(TPH_MOT_STEP);
        delay_us(4); // 脉冲保持时间
        GPIO_DOWN(TPH_MOT_STEP);
        delay_us(current_delay);
    }
}

/**
 * @brief ★ 字车往复移动独立测试函数 (可独立在 app_main 中死循环运行) ★
 * @param test_cycles 往返循环次数 (传 0 为无限死循环测试)
 */
void standalone_carriage_shuttle_test(uint32_t test_cycles) {
    os_printf("\r\n========================================================\r\n");
    os_printf(">>> [STANDALONE] Carriage Motor Shuttle Test Starting... <<<\r\n");
    os_printf("Stroke: %d steps, Speed: Ramp(1000us->260us)\r\n", SHUTTLE_TOTAL_STEPS);
    os_printf("========================================================\r\n");

    // 1. 初始化电机 GPIO
    bk_motor_gpio_config_init();

    // 2. 开启 15V 主升压电源并延时等待稳定
    os_printf("Step 1: 15V Power Boosting...\r\n");
    bk_motor_boost_power(1);
    rtos_delay_milliseconds(60);

    // 3. 配置 ATD5984 (U4) 为 1/16 高精度微步细分 (MS1=1, MS2=1, MS3=1)
    GPIO_UP(TPH_MOT_MS1);
    GPIO_UP(TPH_MOT_MS2);
    GPIO_UP(TPH_MOT_MS3);

    // 4. 唤醒驱动芯片 (退出休眠与复位)
    GPIO_UP(TPH_MOT_SLEEP);
    rtos_delay_milliseconds(5);
    GPIO_UP(TPH_MOT_RES);
    
    // 5. 开启 H 桥使能输出 (低电平使能)
    GPIO_DOWN(TPH_MOT_EN);
    rtos_delay_milliseconds(10); // 建立初始锁紧力矩

    uint32_t current_cycle = 0;
    while (1) {
        current_cycle++;
        os_printf("[SHUTTLE] Cycle #%lu: Moving RIGHT >>>\r\n", (unsigned long)current_cycle);
        
        // ---- 向右前行 7480 步 ----
        carriage_shuttle_move_ramped(1, SHUTTLE_TOTAL_STEPS);

        // 扫掠到右端停顿 200ms
        rtos_delay_milliseconds(200);

        os_printf("[SHUTTLE] Cycle #%lu: Moving LEFT <<<\r\n", (unsigned long)current_cycle);

        // ---- 向左退回 7480 步 (严丝合缝回到原点) ----
        carriage_shuttle_move_ramped(0, SHUTTLE_TOTAL_STEPS);

        // 退回左端停顿 300ms (喂看门狗并让机械彻底稳定)
        rtos_delay_milliseconds(300);

        // 如果达到指定测试次数，退出循环
        if (test_cycles > 0 && current_cycle >= test_cycles) {
            break;
        }
    }

    // 测试完毕，安全下电休眠
    os_printf(">>> Shuttle Test Finished! Entering Safe Sleep. <<<\r\n");
    GPIO_UP(TPH_MOT_EN);     // 关断输出
    GPIO_DOWN(TPH_MOT_SLEEP);// 休眠
    GPIO_DOWN(TPH_MOT_RES);
    bk_motor_boost_power(0); // 关断 15V
}


/* Fixed S2 single-D or color-pair inspection, ordinary dose and symmetric line order.
 * Included after the shared scan/feed/power helpers; no new build unit.
 */
#include "printer_fixed_s2_check_plan.h"
static int printer_run_fixed_s2_check(const uint8_t *bin, uint32_t length) {
    static bool failure_locked = false;
    static uint32_t completed;
    static int locked_code;
    static const char *locked_stage = "NONE";
    static unsigned locked_line;
    uint8_t *frames[PRINTER_S2_CHECK_PARTS] = {NULL};
    uint32_t first[PRINTER_S2_CHECK_PARTS], last[PRINTER_S2_CHECK_PARTS];
    uint32_t job_crc = 0xFFFFFFFFU;
    int result;
    const char *stage = "FRAME_PRECHECK";
    unsigned active_line = 0;
    bool link_ready = printer_link_is_ready();
    if (failure_locked || !link_ready) {
        os_printf("[" PRINTER_S2_CHECK_TAG "_BLOCKED] code=-7 reason=%s link_ready=%u failure_locked=%u first_failure_code=%d first_failure_stage=%s first_failure_line=%u completed_runs=%lu no_motion_no_fire\r\n",
                  link_ready ? "PREVIOUS_FAILURE_LOCK" : "BOOT_LINK_UNAVAILABLE",
                  (unsigned)link_ready, (unsigned)failure_locked, locked_code,
                  locked_stage, locked_line, (unsigned long)completed);
        return -7;
    }

    /* Keep the existing native-F2 format validation, but ignore APP pixels.
     * Release parser memory before allocating the sealed inspection frames.
     */
    printer_map_config_t cfg;
    printer_map_default_config(&cfg);
    cfg.x_pitch_centi_um = PRINTER_MAP_X_PITCH_CENTI_UM;
    cfg.x_direction = PRINTER_MAP_X_DIRECTION;
    cfg.first_nozzle = PRINTER_MAP_FIRST_NOZZLE;
    cfg.reverse_y = PRINTER_MAP_REVERSE_Y;
    printer_page_t page;
    result = printer_page_prepare(bin, length, &cfg, &page,
        printer_map_psram_alloc, printer_map_psram_free, printer_map_yield_cpu);
    if (result) return result;
    printer_page_release(&page, printer_map_psram_free);

    for (unsigned part = 0; part < PRINTER_S2_CHECK_PARTS; ++part) {
        frames[part] = printer_map_psram_alloc(PRINTER_IDLE_AB_BYTES);
        if (!frames[part]) { result = -2; goto done; }
        if (printer_fixed_s2_build(frames[part], PRINTER_IDLE_AB_BYTES)) {
            result = -4; goto done;
        }
        first[part] = PRINTER_PAGE_COLUMNS;
        last[part] = 0;
        for (unsigned x = 0; x < PRINTER_PAGE_COLUMNS; ++x) {
            unsigned any = 0;
            for (unsigned slot = 0; slot < 100U; ++slot) {
                uint8_t *byte = frames[part] + x * 100U + slot;
                *byte &= s_s2_check_masks[part];
                any |= *byte;
            }
            if (any) {
                if (first[part] == PRINTER_PAGE_COLUMNS) first[part] = x;
                last[part] = x;
            }
            if ((x & 63U) == 63U) printer_map_yield_cpu();
        }
        if (first[part] == PRINTER_PAGE_COLUMNS ||
            printer_crc32(frames[part], PRINTER_IDLE_AB_BYTES) != s_s2_check_crc[part]) {
            failure_locked = true; result = -11; goto done;
        }
    }
    stage = "JOB_PRECHECK";
    for (unsigned line = 0; line < PRINTER_S2_CHECK_LINES; ++line)
        job_crc = printer_recovery_crc_update(job_crc, frames[s_s2_check_order[line]], 1U);
    job_crc ^= 0xFFFFFFFFU;
    if (job_crc != PRINTER_S2_CHECK_JOB_CRC) { failure_locked = true; result = -11; goto done; }
    os_printf("[" PRINTER_S2_CHECK_TAG "_BEGIN] run=%lu section=" PRINTER_FIXED_SECTION_NAME " order=" PRINTER_S2_CHECK_ORDER_TEXT " scans=%u repeats=1 head_sessions=1 APP_pixels_ignored no_priming\r\n",
              (unsigned long)(completed + 1U), PRINTER_S2_CHECK_LINES);
    os_printf("[" PRINTER_S2_CHECK_TAG "_RX_EXPECT] columns=%lu bytes=%lu crc32=%08lX A=004A2912 S=%02X D=3F dots_D1_D6=5688,5688,5688,5688,5688,5688\r\n",
              (unsigned long)(PRINTER_S2_CHECK_LINES * PRINTER_PAGE_COLUMNS),
              (unsigned long)(PRINTER_S2_CHECK_LINES * PRINTER_IDLE_AB_BYTES), (unsigned long)job_crc, (unsigned)PRINTER_FIXED_SECTION_MASK);

    failure_locked = true;
    s_recovery_repeats = 1U;
    stage = "INITIAL_HEAD_OFF";
    if (tph_mcu_set_head_power(0) != BK_OK) { result = -8; goto stop; }
    carriage_verified_power_up();
    stage = "HEAD_ON";
    if (tph_mcu_set_head_power(1) != BK_OK) { result = -8; goto stop; }
    rtos_delay_milliseconds(PRINTER_PAGE_HEAD_SETTLE_MS);
    for (unsigned line = 0; line < PRINTER_S2_CHECK_LINES; ++line) {
        unsigned part = s_s2_check_order[line];
        uint64_t lead_us = 0;
        s_idle_ab_first_col = first[part];
        s_idle_ab_last_col = last[part];
        s_recovery_line_crc[0] = s_s2_check_crc[part];
        os_printf("[" PRINTER_S2_CHECK_TAG "_LINE] line=%u/%u expected=%s D_mask=%02X frame_crc32=%08lX first=%lu last=%lu selected_nozzles=%u per_nozzle_commands=711 repeats=1\r\n",
                  line + 1U, PRINTER_S2_CHECK_LINES, s_s2_check_names[part], (unsigned)s_s2_check_masks[part],
                  (unsigned long)s_s2_check_crc[part],
                  (unsigned long)first[part], (unsigned long)last[part], PRINTER_S2_CHECK_NOZZLES);
        active_line = line + 1U;
        stage = "SCAN";
        result = printer_idle_ab_scan(frames[part], s_s2_check_crc[part],
                    PRINTER_S2_CHECK_TAG, s_s2_check_names[part], line + 1U, 0, &lead_us);
        if (result) goto stop;
        stage = "FEED";
        result = printer_idle_ab_feed(line + 1U == PRINTER_S2_CHECK_LINES ? PRINTER_IDLE_AB_GROUP_FEED : PRINTER_IDLE_AB_LINE_FEED,
                                     PRINTER_S2_CHECK_TAG, line + 1U);
        if (result) goto stop;
    }
    result = 0;
stop:
    s_recovery_repeats = 1U;
    if (tph_mcu_set_head_power(0) != BK_OK && !result) {
        result = -8; stage = "FINAL_HEAD_OFF";
    }
    bk_motor_set_state(0, 0);
    carriage_verified_power_down();
    if (!result) { failure_locked = false; ++completed; }
    os_printf("[" PRINTER_S2_CHECK_TAG "_END] code=%d completed_runs=%lu failure_locked=%u planned_per_nozzle_job_commands=1422 head_OFF_requested boost_OFF ink_unverified\r\n",
              result, (unsigned long)completed, (unsigned)failure_locked);
done:
    if (result && failure_locked && locked_code == 0) {
        locked_code = result; locked_stage = stage; locked_line = active_line;
        os_printf("[" PRINTER_S2_CHECK_TAG "_LOCK] first_failure_code=%d stage=%s line=%u restart_required no_auto_retry\r\n",
                  locked_code, locked_stage, locked_line);
    }
    for (unsigned part = 0; part < PRINTER_S2_CHECK_PARTS; ++part)
        if (frames[part]) printer_map_psram_free(frames[part]);
    return result;
}

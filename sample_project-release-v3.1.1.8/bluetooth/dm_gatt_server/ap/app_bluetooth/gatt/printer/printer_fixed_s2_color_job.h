/* Fixed S2 per-color inspection, ordinary dose and symmetric line order.
 * Included after the shared scan/feed/power helpers; no new build unit.
 */
static int printer_run_fixed_s2_color_check(const uint8_t *bin, uint32_t length) {
    static bool failure_locked = false;
    static uint32_t completed;
    static const char *const names[3] = {"CYAN", "MAGENTA", "YELLOW"};
    static const uint8_t masks[3] = {0x03, 0x0C, 0x30};
    static const uint8_t plan[6] = {0, 1, 2, 2, 1, 0};
    static const uint32_t expected_crc[3] = {0x93764BC7U, 0xA4034E9DU, 0xDD0181CDU};
    uint8_t *frames[3] = {NULL, NULL, NULL};
    uint32_t first[3], last[3];
    uint32_t job_crc = 0xFFFFFFFFU;
    int result;
    if (failure_locked || !printer_link_is_ready()) return -7;

    /* Keep the existing native-F2 format validation, but ignore APP pixels.
     * Release parser memory before allocating the three sealed color frames.
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

    for (unsigned color = 0; color < 3U; ++color) {
        frames[color] = printer_map_psram_alloc(PRINTER_IDLE_AB_BYTES);
        if (!frames[color]) { result = -2; goto done; }
        if (printer_fixed_s2_build(frames[color], PRINTER_IDLE_AB_BYTES)) {
            result = -4; goto done;
        }
        first[color] = PRINTER_PAGE_COLUMNS;
        last[color] = 0;
        for (unsigned x = 0; x < PRINTER_PAGE_COLUMNS; ++x) {
            unsigned any = 0;
            for (unsigned slot = 0; slot < 100U; ++slot) {
                uint8_t *byte = frames[color] + x * 100U + slot;
                *byte &= masks[color];
                any |= *byte;
            }
            if (any) {
                if (first[color] == PRINTER_PAGE_COLUMNS) first[color] = x;
                last[color] = x;
            }
            if ((x & 63U) == 63U) printer_map_yield_cpu();
        }
        if (first[color] == PRINTER_PAGE_COLUMNS ||
            printer_crc32(frames[color], PRINTER_IDLE_AB_BYTES) != expected_crc[color]) {
            failure_locked = true; result = -11; goto done;
        }
    }
    for (unsigned line = 0; line < 6U; ++line)
        job_crc = printer_recovery_crc_update(job_crc, frames[plan[line]], 1U);
    job_crc ^= 0xFFFFFFFFU;
    if (job_crc != 0x7764411CU) { failure_locked = true; result = -11; goto done; }
    os_printf("[COLOR_CHECK_BEGIN] run=%lu order=C,M,Y,Y,M,C scans=6 repeats=1 head_sessions=1 APP_pixels_ignored no_priming\r\n",
              (unsigned long)(completed + 1U));
    os_printf("[COLOR_CHECK_RX_EXPECT] columns=7080 bytes=708000 crc32=%08lX A=004A2912 S=02 D=3F dots_D1_D6=5688,5688,5688,5688,5688,5688\r\n",
              (unsigned long)job_crc);

    failure_locked = true;
    s_recovery_repeats = 1U;
    if (tph_mcu_set_head_power(0) != BK_OK) { result = -8; goto stop; }
    carriage_verified_power_up();
    if (tph_mcu_set_head_power(1) != BK_OK) { result = -8; goto stop; }
    rtos_delay_milliseconds(PRINTER_PAGE_HEAD_SETTLE_MS);
    for (unsigned line = 0; line < 6U; ++line) {
        unsigned color = plan[line];
        uint64_t lead_us = 0;
        s_idle_ab_first_col = first[color];
        s_idle_ab_last_col = last[color];
        s_recovery_line_crc[0] = expected_crc[color];
        os_printf("[COLOR_CHECK_LINE] line=%u/6 expected=%s D_mask=%02X frame_crc32=%08lX first=%lu last=%lu selected_nozzles=8 per_nozzle_commands=711 repeats=1\r\n",
                  line + 1U, names[color], (unsigned)masks[color],
                  (unsigned long)expected_crc[color],
                  (unsigned long)first[color], (unsigned long)last[color]);
        result = printer_idle_ab_scan(frames[color], expected_crc[color],
                    "COLOR_CHECK", names[color], line + 1U, 0, &lead_us);
        if (result) goto stop;
        result = printer_idle_ab_feed(line == 5U ? PRINTER_IDLE_AB_GROUP_FEED : PRINTER_IDLE_AB_LINE_FEED,
                                     "COLOR_CHECK", line + 1U);
        if (result) goto stop;
    }
    result = 0;
stop:
    s_recovery_repeats = 1U;
    if (tph_mcu_set_head_power(0) != BK_OK && !result) result = -8;
    bk_motor_set_state(0, 0);
    carriage_verified_power_down();
    if (!result) { failure_locked = false; ++completed; }
    os_printf("[COLOR_CHECK_END] code=%d completed_runs=%lu failure_locked=%u planned_per_nozzle_job_commands=1422 head_OFF_requested boost_OFF ink_unverified\r\n",
              result, (unsigned long)completed, (unsigned)failure_locked);
done:
    for (unsigned color = 0; color < 3U; ++color)
        if (frames[color]) printer_map_psram_free(frames[color]);
    return result;
}

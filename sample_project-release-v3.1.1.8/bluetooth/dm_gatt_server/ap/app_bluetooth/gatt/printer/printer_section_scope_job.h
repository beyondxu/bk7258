/* Keep HC32 waveform untouched; select one D2/Addr4 nozzle per sweep. */
static int printer_run_section_scope(const uint8_t *bin, uint32_t length) {
    static bool failed = false;
    if (failed || !printer_link_is_ready()) return -7;
    printer_map_config_t cfg;
    printer_map_default_config(&cfg);
    cfg.x_pitch_centi_um = PRINTER_MAP_X_PITCH_CENTI_UM;
    cfg.x_direction = PRINTER_MAP_X_DIRECTION;
    cfg.first_nozzle = PRINTER_MAP_FIRST_NOZZLE;
    cfg.reverse_y = PRINTER_MAP_REVERSE_Y;
    if (PRINTER_NOZZLE_TIME_SCALE != 1U) return -1;
    printer_map_stats_t stats;
    int result = printer_map_bin_dry_run(bin, length, &cfg, &stats,
                                       printer_map_psram_alloc, printer_map_psram_free, printer_map_yield_cpu);
    if (result) return result;
    uint8_t *columns = printer_map_psram_alloc(4U * 118000U);
    if (!columns) return -2;
    for (unsigned section = 0; section < 4; ++section) {
        result = printer_map_section_scope(columns + section * 118000U, 118000U, 1180U, section, &cfg);
        if (result) goto done;
    }
    os_printf("[SECTION_SCOPE_BEGIN] generated_C_D2_addr4 dots_each=600 sweeps=4 feed=0 motor_scale=1 HC32_waveform_unchanged\r\n");
    failed = true;
    for (unsigned section = 0; section < 4; ++section) {
        result = tph_mcu_set_head_power(0);
        if (result != BK_OK) goto done;
        os_printf("[SECTION_SCOPE_ARM] section=%u used_row=%u slot=%u data=0x02 D2_PA2 addr4 settle_ms=3000\r\n",
                  section + 1U, 26U + 50U * section, 12U + section);
        rtos_delay_milliseconds(3000);
        result = printer_execute_cyan_line(columns + section * 118000U);
        if (result) goto done;
        os_printf("[SECTION_SCOPE_PASS] section=%u code=0 columns=1180 expected_dots=600 ink_unverified\r\n", section + 1U);
    }
    failed = false;
done:
    printer_map_psram_free(columns);
    os_printf("[SECTION_SCOPE_END] code=%d locked=%u ink_unverified\r\n", result, (unsigned)failed);
    return result;
}

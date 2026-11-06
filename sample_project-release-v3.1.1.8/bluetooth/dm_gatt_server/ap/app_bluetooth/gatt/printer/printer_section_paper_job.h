/* Paper comparison: one section per sweep, same D2, all 25 addresses. */
static int printer_run_section_paper(const uint8_t *bin, uint32_t length) {
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
        result = printer_map_section_paper_order(columns + section * 118000U, 118000U, 1180U, section, PRINTER_SECTION_REVERSE_COLUMNS, &cfg);
        if (result) goto done;
    }
    os_printf("[SECTION_PAPER_BEGIN] source=GENERATED C_D2 addr=1..25 sections=1..4 nozzles=100 dots_each_nozzle=96 sweeps=4 feed=0 waveform_unchanged\r\n");
    os_printf("[SECTION_PAPER_ORDER] reverse_columns=%u expected_column_mapping=%s nozzle_rows_unchanged waveform_unchanged\r\n",
              (unsigned)PRINTER_SECTION_REVERSE_COLUMNS, PRINTER_SECTION_REVERSE_COLUMNS ? "5,4,3,2,1" : "1,2,3,4,5");
    os_printf("[SECTION_PAPER_SCAN] ink_on_return=%u same_R_chart_same_nozzles position_counts_only inspect_registration\r\n", (unsigned)PRINTER_SECTION_RETURN_SCAN);
    failed = true;
    for (unsigned section = 0; section < 4; ++section) {
        os_printf("[SECTION_PAPER_PASS] section=%u addresses=25 expected_dots=2400 max_bits_per_column=5 feed=0\r\n", section + 1U);
#if PRINTER_SECTION_RETURN_SCAN
        result = printer_execute_section_return_line(columns + section * 118000U);
#else
        result = printer_execute_cyan_line(columns + section * 118000U);
#endif
        if (result) goto done;
    }
    failed = false;
done:
    printer_map_psram_free(columns);
    os_printf("[SECTION_PAPER_END] code=%d locked=%u ink_unverified\r\n", result, (unsigned)failed);
    return result;
}

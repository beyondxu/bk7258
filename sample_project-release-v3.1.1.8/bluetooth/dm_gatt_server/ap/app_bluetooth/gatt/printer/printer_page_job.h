/* Private hardware orchestrator. Included after verified shuttle helpers. */
static int printer_run_full_page(const uint8_t *bin, uint32_t length) {
    static bool failure_locked = false;
    if (failure_locked || !printer_link_is_ready()) {
        os_printf("[PAGE_BLOCKED] previous_failure_or_link_unavailable restart_and_restore_origin\r\n");
        return -7;
    }
    printer_map_config_t cfg;
    printer_map_default_config(&cfg);
    cfg.x_pitch_centi_um = PRINTER_MAP_X_PITCH_CENTI_UM;
    cfg.x_direction = PRINTER_MAP_X_DIRECTION;
    cfg.first_nozzle = PRINTER_MAP_FIRST_NOZZLE;
    cfg.reverse_y = PRINTER_MAP_REVERSE_Y;
    printer_page_t page;
    int result = printer_page_prepare(bin, length, &cfg, &page,
                         printer_map_psram_alloc, printer_map_psram_free, printer_map_yield_cpu);
    if (result) {
        os_printf("[PAGE_REJECT] code=%d no_motion\r\n", result);
        return result;
    }
    result = printer_page_transform(&page, PRINTER_PAGE_TRANSFORM, printer_map_yield_cpu);
    if (result) { printer_page_release(&page, printer_map_psram_free); return result; }
    uint8_t *columns = printer_map_psram_alloc(PRINTER_PAGE_COLUMNS * 100U);
    uint8_t *scratch = printer_map_psram_alloc(page.width * 72U);
    uint8_t *pass_columns = PRINTER_PAGE_COLOR_MODE == 2 ? printer_map_psram_alloc(PRINTER_PAGE_COLUMNS * 100U) : NULL;
    if (!columns || !scratch || (PRINTER_PAGE_COLOR_MODE == 2 && !pass_columns)) { result = -2; goto done; }
    os_printf("[PAGE_PLAN] source=1180x1476 output=%lux%lu swaths=%lu columns=1180 max_delay=%lu pitch=%lu y_pitch=4233 direction=%ld feed_steps=%u transform=%u color_mode=%u CANDIDATE_FEED aspect_fit\r\n",
              (unsigned long)page.width, (unsigned long)page.height, (unsigned long)page.swaths,
              (unsigned long)page.max_delay, (unsigned long)cfg.x_pitch_centi_um,
              (long)cfg.x_direction, (unsigned)PRINTER_PAGE_FEED_STEPS, (unsigned)PRINTER_PAGE_TRANSFORM, (unsigned)PRINTER_PAGE_COLOR_MODE);
    os_printf("[PAGE_DOTS] source_CMY=%lu,%lu,%lu resized_CMY=%lu,%lu,%lu\r\n",
              (unsigned long)page.source_dots[0], (unsigned long)page.source_dots[1], (unsigned long)page.source_dots[2],
              (unsigned long)page.output_dots[0], (unsigned long)page.output_dots[1], (unsigned long)page.output_dots[2]);
    for (uint32_t strip = 0; strip < page.swaths; ++strip) {
        uint32_t dots[3];
        result = printer_page_render(&page, strip, columns, PRINTER_PAGE_COLUMNS * 100U,
                                    scratch, page.width * 72U, dots, printer_map_yield_cpu);
        if (result) goto done;
        os_printf("[PAGE_SWATH] index=%lu/%lu y=%lu dots_CMY=%lu,%lu,%lu\r\n",
                  (unsigned long)(strip + 1U), (unsigned long)page.swaths, (unsigned long)(strip * 192U),
                  (unsigned long)dots[0], (unsigned long)dots[1], (unsigned long)dots[2]);
        unsigned groups = PRINTER_PAGE_COLOR_MODE == 2 ? 2U : 1U;
        for (unsigned group = 0; group < groups; ++group) {
            unsigned mask = groups == 1 ? 7U : group == 0 ? 1U : 6U;
            uint32_t pass_dots = 0;
            for (unsigned ch = 0; ch < 3; ++ch) if (mask & (1U << ch)) pass_dots += dots[ch];
            if (!pass_dots) continue;
            const uint8_t *data = columns;
            if (groups == 2) {
                result = printer_page_color_columns(columns, pass_columns, PRINTER_PAGE_COLUMNS * 100U, mask);
                if (result) goto done;
                data = pass_columns;
            }
            os_printf("[PAGE_PASS] swath=%lu group=%u/%u mask=%u dots=%lu no_feed_between_passes\r\n",
                      (unsigned long)(strip + 1U), group + 1U, groups, mask, (unsigned long)pass_dots);
            failure_locked = true;
            result = printer_execute_cyan_line(data);
            if (result) goto done;
        }
        if (strip + 1U < page.swaths) {
            uint32_t sent = 0;
            failure_locked = true;
            if (tph_mcu_set_head_power(0) != BK_OK) { result = -8; goto done; }
            result = motor_page_feed(PRINTER_PAGE_FEED_STEPS, PRINTER_PAGE_FEED_DIRECTION, &sent);
            os_printf("[PAGE_FEED] after=%lu pulses=%lu/%u result=%d distance_unverified\r\n",
                      (unsigned long)(strip + 1U), (unsigned long)sent, (unsigned)PRINTER_PAGE_FEED_STEPS, result);
            if (result != BK_OK || sent != PRINTER_PAGE_FEED_STEPS) { result = -10; goto done; }
        }
    }
    failure_locked = false;
    os_printf("[PAGE_DONE] swaths=%lu no_auto_eject feed_distance_and_ink_unverified\r\n", (unsigned long)page.swaths);
done:
    if (columns) printer_map_psram_free(columns);
    if (scratch) printer_map_psram_free(scratch);
    if (pass_columns) printer_map_psram_free(pass_columns);
    printer_page_release(&page, printer_map_psram_free);
    return result;
}

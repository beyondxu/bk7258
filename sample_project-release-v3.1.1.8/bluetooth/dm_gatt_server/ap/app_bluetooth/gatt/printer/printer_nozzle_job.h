#if PRINTER_SECTION_SCOPE
#include "printer_section_scope_job.h"
#endif
/* APP job is fully parsed before any movement; its pixels do not select this chart. */
static int printer_run_nozzle_check(const uint8_t *bin, uint32_t length) {
#if PRINTER_SECTION_SCOPE
    return printer_run_section_scope(bin, length);
#else
    static bool failed = false;
    if (failed || !printer_link_is_ready()) return -7;
    printer_map_config_t cfg;
    printer_map_default_config(&cfg);
    cfg.x_pitch_centi_um = PRINTER_MAP_X_PITCH_CENTI_UM;
    cfg.x_direction = PRINTER_MAP_X_DIRECTION;
    cfg.first_nozzle = PRINTER_MAP_FIRST_NOZZLE;
    cfg.reverse_y = PRINTER_MAP_REVERSE_Y;
    /* Chart legend is fixed to the known baseline geometry. */
    if (PRINTER_PAGE_FEED_STEPS != 3170 || PRINTER_PAGE_FEED_DIRECTION != 0 || cfg.x_pitch_centi_um != 3000 || cfg.x_direction != -1 || cfg.first_nozzle || cfg.reverse_y) return -1;
    printer_map_stats_t stats;
    int result = printer_map_bin_dry_run(bin, length, &cfg, &stats,
                                       printer_map_psram_alloc, printer_map_psram_free, printer_map_yield_cpu);
    if (result) return result;
    uint8_t *columns = printer_map_psram_alloc(3U * PRINTER_NOZZLE_BATCHES * 118000U);
    if (!columns) return -2;
    for (unsigned ch = 0; ch < 3; ++ch) {
        for (unsigned batch = 0; batch < PRINTER_NOZZLE_BATCHES; ++batch) {
            result = printer_map_nozzle_lines_batch(columns + (ch * PRINTER_NOZZLE_BATCHES + batch) * 118000U,
                                                    118000U, 1180U, ch, batch, PRINTER_NOZZLE_BATCHES, &cfg);
            if (result) goto done;
        }
    }
    os_printf("[NOZZLE_LINES_BEGIN] source=GENERATED used_rows=192 of_200 length_columns=96 blocks=6 lines_each=32 dots_each_color=18432 feed_steps=3170 pulses_unchanged\r\n");
    os_printf("[NOZZLE_LOAD] forward_motor_delay_scale=%u start_low_us=%u cruise_low_us=%u return_scale=1 combined_chart_unchanged head_waveform_unchanged actual_fire_interval_unmeasured\r\n",
              (unsigned)PRINTER_NOZZLE_TIME_SCALE, (unsigned)(3000U * PRINTER_NOZZLE_TIME_SCALE),
              (unsigned)(260U * PRINTER_NOZZLE_TIME_SCALE));
    failed = true;
    for (unsigned ch = 0; ch < 3; ++ch) {
        for (unsigned batch = 0; batch < PRINTER_NOZZLE_BATCHES; ++batch) {
            os_printf("[NOZZLE_BATCH] channel=%u batch=%u/%u expected_dots=%u no_feed_between_batches motor_scale=%u\r\n",
                      ch, batch + 1U, (unsigned)PRINTER_NOZZLE_BATCHES, (unsigned)(18432U / PRINTER_NOZZLE_BATCHES), (unsigned)PRINTER_NOZZLE_TIME_SCALE);
            result = printer_execute_cyan_line(columns + (ch * PRINTER_NOZZLE_BATCHES + batch) * 118000U);
            if (result) goto done;
        }
        if (ch < 2) {
            uint32_t sent = 0;
            result = tph_mcu_set_head_power(0);
            if (result != BK_OK) goto done;
            result = motor_page_feed(PRINTER_PAGE_FEED_STEPS, PRINTER_PAGE_FEED_DIRECTION, &sent);
            os_printf("[NOZZLE_LINES_FEED] after_color=%u pulses=%lu/3170 result=%d\r\n", ch, (unsigned long)sent, result);
            if (result != BK_OK || sent != PRINTER_PAGE_FEED_STEPS) { result = -10; goto done; }
        }
    }
    failed = false; /* Successful chart allows another explicit APP task. */
done:
    printer_map_psram_free(columns);
    os_printf("[NOZZLE_LINES_END] code=%d locked=%u ink_unverified\r\n", result, (unsigned)failed);
    return result;
#endif
}

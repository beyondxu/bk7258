/* Private hardware orchestration; included after the verified scan helper. */
#include "printer_nozzle_check.h"
static int printer_run_nozzle_check(const uint8_t *bin, uint32_t length) {
    static bool failure_locked = false;
    if (failure_locked || !printer_link_is_ready()) {
        os_printf("[NOZZLE_CHECK_BLOCKED] restart_and_restore_origin\r\n"); return -7;
    }
    printer_map_config_t cfg;
    printer_map_default_config(&cfg);
    cfg.x_pitch_centi_um = PRINTER_MAP_X_PITCH_CENTI_UM;
    cfg.x_direction = PRINTER_MAP_X_DIRECTION;
    cfg.first_nozzle = PRINTER_MAP_FIRST_NOZZLE;
    cfg.reverse_y = PRINTER_MAP_REVERSE_Y;
    /* Validate the complete APP payload before using it solely as a trigger. */
    printer_page_t page;
    int result = printer_page_prepare(bin, length, &cfg, &page,
                          printer_map_psram_alloc, printer_map_psram_free, printer_map_yield_cpu);
    if (result) return result;
    printer_page_release(&page, printer_map_psram_free);
    if (printer_nc_validate(&cfg)) return -4;
    uint8_t *columns = printer_map_psram_alloc(PRINTER_NC_BYTES);
    if (!columns) return -2;
    uint32_t planned_crc[3][PRINTER_NC_GROUPS];
    /* Validate every group before the first power/motor operation. */
    for (unsigned ch = 0; ch < 3; ++ch) for (unsigned group = 0; group < PRINTER_NC_GROUPS; ++group) {
        result = printer_nc_build(columns, PRINTER_NC_BYTES, ch, group, &cfg);
        if (result) goto done;
        planned_crc[ch][group] = printer_crc32(columns, PRINTER_NC_BYTES);
        printer_map_yield_cpu();
    }
    os_printf("[NOZZLE_CHECK_BEGIN] APP_image_is_trigger_only colors=CMY nozzles_each=200 groups_each=4 scans=12 dots_each_nozzle=16 max_bits_per_column=1 no_feed_within_color\r\n");
    unsigned scans = 0, feeds = 0;
    for (unsigned ch = 0; ch < 3; ++ch) {
        for (unsigned group = 0; group < PRINTER_NC_GROUPS; ++group) {
            result = printer_nc_build(columns, PRINTER_NC_BYTES, ch, group, &cfg);
            if (result) goto done;
            uint32_t crc = printer_crc32(columns, PRINTER_NC_BYTES);
            if (crc != planned_crc[ch][group]) { result = -11; goto done; }
            os_printf("[NOZZLE_CHECK_PASS] scan=%u/12 color=%c group=%u/4 section=S%u physical_rows=%u..%u planned_nozzles=50 requested_dots=800 crc32=%08lX\r\n",
                      scans + 1U, "CMY"[ch], group + 1U, group + 1U, group * 50U + 1U,
                      (group + 1U) * 50U, (unsigned long)crc);
            failure_locked = true;
            result = printer_execute_cyan_line(columns); /* existing power/2s settle/forward/blank return */
            if (result) goto done;
            if (printer_crc32(columns, PRINTER_NC_BYTES) != crc) { result = -11; goto done; }
            ++scans;
            os_printf("[NOZZLE_CHECK_PASS_DONE] scan=%u/12 color=%c group=%u/4 result=0 ink_unverified\r\n",
                      scans, "CMY"[ch], group + 1U);
        }
        if (ch < 2U) {
            /* Two existing calibrated strides separate a full 200-row band.
             * Each call stays within the existing <=5000 pulse feed bound. */
            for (unsigned part = 0; part < 2; ++part) {
                uint32_t sent = 0;
                failure_locked = true;
                if (tph_mcu_set_head_power(0) != BK_OK) { result = -8; goto done; }
                result = motor_page_feed(PRINTER_PAGE_FEED_STEPS, PRINTER_PAGE_FEED_DIRECTION, &sent);
                os_printf("[NOZZLE_CHECK_FEED] after_color=%c part=%u/2 pulses=%lu/%u result=%d distance_unverified\r\n",
                          "CMY"[ch], part + 1U, (unsigned long)sent, (unsigned)PRINTER_PAGE_FEED_STEPS, result);
                if (result != BK_OK || sent != PRINTER_PAGE_FEED_STEPS) { result = -10; goto done; }
                ++feeds;
            }
        }
    }
    failure_locked = false;
    os_printf("[NOZZLE_CHECK_DONE] scans=%u/12 feeds=%u/4 planned_unique_nozzles=600 requested_dots=9600 no_auto_eject ink_unverified\r\n", scans, feeds);
done:
    if (result) os_printf("[NOZZLE_CHECK_STOP] code=%d no_further_motion failure_locked=%u\r\n", result, (unsigned)failure_locked);
    printer_map_psram_free(columns);
    return result;
}

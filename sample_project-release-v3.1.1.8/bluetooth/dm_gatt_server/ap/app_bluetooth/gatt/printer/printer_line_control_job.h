#include "printer_line_control.h"
/* Only the known original APP swath 5 is used; all five plans are ready and
 * fingerprinted before motion. Existing head OFF and motor sequencing retained. */
static int printer_run_line_control(const uint8_t *bin, uint32_t length) {
    static bool failure_locked = false;
    /* One bounded cache survives subsequent APP tasks, never grows per job.
     * New APP dithering must not silently change the selected physical nozzles. */
    static uint8_t *frozen_base = NULL;
    static uint32_t frozen_crc, frozen_total, frozen_counts[3], frozen_pass_crc[5];
    static printer_map_route_t frozen_chosen[3];
    if (failure_locked || !printer_link_is_ready()) return -7;
    printer_map_config_t cfg;
    printer_map_default_config(&cfg);
    cfg.x_pitch_centi_um = PRINTER_MAP_X_PITCH_CENTI_UM;
    cfg.x_direction = PRINTER_MAP_X_DIRECTION;
    cfg.first_nozzle = PRINTER_MAP_FIRST_NOZZLE;
    cfg.reverse_y = PRINTER_MAP_REVERSE_Y;
    printer_page_t page;
    int result = printer_page_prepare(bin, length, &cfg, &page,
                          printer_map_psram_alloc, printer_map_psram_free, printer_map_yield_cpu);
    if (result) return result;
    uint8_t *base = NULL, *out = NULL, *scratch = NULL;
    bool reuse = frozen_base != NULL;
    printer_map_route_t chosen[3];
    uint32_t counts[3], total, crc[5], baseline_crc;
    out = printer_map_psram_alloc(PRINTER_LC_BYTES);
    if (!out) { result = -2; goto done; }
    if (reuse) {
        base = frozen_base;
        printer_page_release(&page, printer_map_psram_free);
        baseline_crc = frozen_crc;
        if (printer_crc32(base, PRINTER_LC_BYTES) != frozen_crc) {
            failure_locked = true; result = -11; goto done;
        }
        memcpy(chosen, frozen_chosen, sizeof(chosen));
        memcpy(counts, frozen_counts, sizeof(counts));
        memcpy(crc, frozen_pass_crc, sizeof(crc));
        total = frozen_total;
    } else {
        if (page.swaths <= 4U) { result = -4; goto done; }
        result = printer_page_transform(&page, PRINTER_PAGE_TRANSFORM, printer_map_yield_cpu);
        if (result) goto done;
        base = printer_map_psram_alloc(PRINTER_LC_BYTES);
        scratch = printer_map_psram_alloc(page.width * 72U);
        if (!base || !scratch) { result = -2; goto done; }
        uint32_t rendered[3];
        result = printer_page_render(&page, 4U, base, PRINTER_LC_BYTES, scratch, page.width * 72U,
                                     rendered, printer_map_yield_cpu);
        if (result) goto done;
        printer_map_psram_free(scratch); scratch = NULL;
        printer_page_release(&page, printer_map_psram_free);
        result = printer_lc_select(base, &cfg, chosen, counts, &total);
        if (result) goto done;
        baseline_crc = printer_crc32(base, PRINTER_LC_BYTES);
        for (unsigned pass = 0; pass < 5U; ++pass) {
            result = printer_lc_build(base, out, PRINTER_LC_BYTES, pass, chosen);
            if (result) goto done;
            crc[pass] = printer_crc32(out, PRINTER_LC_BYTES);
            printer_map_yield_cpu();
        }
        frozen_base = base;
        frozen_crc = baseline_crc;
        frozen_total = total;
        memcpy(frozen_chosen, chosen, sizeof(chosen));
        memcpy(frozen_counts, counts, sizeof(counts));
        memcpy(frozen_pass_crc, crc, sizeof(crc));
    }
    os_printf("[LINE_CONTROL_CACHE] state=%s baseline_crc=%08lX incoming_crc=%08lX retained_bytes=118000 restart_to_change_plan\r\n",
              reuse ? "REUSED" : "CAPTURED", (unsigned long)baseline_crc,
              (unsigned long)printer_crc32(bin, length));
    os_printf("[LINE_CONTROL_BEGIN] original_swath=5 scans=5 order=CMY,C1,M1,Y1,CMY baseline_crc=%08lX baseline_dots=%lu min_single_dots=256 no_new_dots\r\n",
              (unsigned long)baseline_crc, (unsigned long)total);
    for (unsigned ch = 0; ch < 3U; ++ch)
        os_printf("[LINE_CONTROL_PICK] color=%c physical_row=%lu addr=%lu section=S%lu mask=0x%02lX source_dots=%lu\r\n",
                  "CMY"[ch], (unsigned long)(chosen[ch].nozzle_row + 1U),
                  (unsigned long)(chosen[ch].slot / 4U + 1U), (unsigned long)(chosen[ch].slot % 4U + 1U),
                  (unsigned long)chosen[ch].mask, (unsigned long)counts[ch]);
    for (unsigned pass = 0; pass < 5U; ++pass) {
        result = printer_lc_build(base, out, PRINTER_LC_BYTES, pass, chosen);
        if (result) goto done;
        if (printer_crc32(out, PRINTER_LC_BYTES) != crc[pass] || printer_crc32(base, PRINTER_LC_BYTES) != baseline_crc) {
            result = -11; goto done;
        }
        os_printf("[LINE_CONTROL_PASS] scan=%u/5 mode=%s requested_dots=%lu crc32=%08lX\r\n", pass + 1U,
                  pass == 0U || pass == 4U ? "ORIGINAL_CMY" : pass == 1U ? "C_ONE" : pass == 2U ? "M_ONE" : "Y_ONE",
                  (unsigned long)(pass == 0U || pass == 4U ? total : counts[pass - 1U]), (unsigned long)crc[pass]);
        failure_locked = true;
        result = printer_execute_cyan_line(out);
        if (result) goto done;
        if (printer_crc32(out, PRINTER_LC_BYTES) != crc[pass] || printer_crc32(base, PRINTER_LC_BYTES) != baseline_crc) {
            result = -11; goto done;
        }
        if (pass < 4U) {
            uint32_t sent = 0;
            if (tph_mcu_set_head_power(0) != BK_OK) { result = -8; goto done; }
            result = motor_page_feed(PRINTER_PAGE_FEED_STEPS, PRINTER_PAGE_FEED_DIRECTION, &sent);
            os_printf("[LINE_CONTROL_FEED] after=%u pulses=%lu/%u result=%d distance_unverified\r\n",
                      pass + 1U, (unsigned long)sent, (unsigned)PRINTER_PAGE_FEED_STEPS, result);
            if (result != BK_OK || sent != PRINTER_PAGE_FEED_STEPS) { result = -10; goto done; }
        }
    }
    failure_locked = false;
    os_printf("[LINE_CONTROL_DONE] scans=5/5 feeds=4/4 first_last_same_buffer_data ink_unverified\r\n");
done:
    if (result) os_printf("[LINE_CONTROL_STOP] code=%d failure_locked=%u no_further_motion\r\n", result, (unsigned)failure_locked);
    if (scratch) printer_map_psram_free(scratch);
    if (out) printer_map_psram_free(out);
    if (result && failure_locked && frozen_base) {
        if (base == frozen_base) base = NULL;
        printer_map_psram_free(frozen_base); frozen_base = NULL;
    }
    if (base && base != frozen_base) printer_map_psram_free(base);
    printer_page_release(&page, printer_map_psram_free);
    return result;
}

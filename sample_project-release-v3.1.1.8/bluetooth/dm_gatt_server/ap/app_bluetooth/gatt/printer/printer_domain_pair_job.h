#include "printer_domain_pair.h"
/* 11700 feed pulses / 30mm calibration: ~2mm within a group, ~4mm after it. */
#define PRINTER_LC_LINE_FEED_STEPS 780U
#define PRINTER_LC_GROUP_FEED_STEPS 1560U
/* Only the known original APP swath 5 is used; all eight plans are ready and
 * fingerprinted before motion. Existing head OFF and motor sequencing retained. */
static int printer_run_domain_pair(const uint8_t *bin, uint32_t length) {
    static bool failure_locked = false;
    /* One bounded cache survives subsequent APP tasks, never grows per job.
     * New APP dithering must not silently change the selected physical nozzles. */
    static uint8_t *frozen_base = NULL;
    static uint32_t frozen_crc, frozen_total, frozen_dots, frozen_pass_crc[8], successful_groups;
    static printer_map_route_t frozen_source;
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
    printer_map_route_t source;
    uint32_t dots, total, crc[8], baseline_crc;
    out = printer_map_psram_alloc(PRINTER_LC_BYTES);
    if (!out) { result = -2; goto done; }
    if (reuse) {
        base = frozen_base;
        printer_page_release(&page, printer_map_psram_free);
        baseline_crc = frozen_crc;
        if (printer_crc32(base, PRINTER_LC_BYTES) != frozen_crc) {
            failure_locked = true; result = -11; goto done;
        }
        source = frozen_source;
        dots = frozen_dots;
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
        result = printer_dp_select(base, &cfg, &source, &dots, &total);
        if (result) goto done;
        baseline_crc = printer_crc32(base, PRINTER_LC_BYTES);
        for (unsigned pass = 0; pass < 8U; ++pass) {
            result = printer_dp_build(base, out, PRINTER_LC_BYTES, &source, pass);
            if (result) goto done;
            crc[pass] = printer_crc32(out, PRINTER_LC_BYTES);
            printer_map_yield_cpu();
        }
        frozen_base = base;
        frozen_crc = baseline_crc;
        frozen_total = total;
        frozen_source = source;
        frozen_dots = dots;
        memcpy(frozen_pass_crc, crc, sizeof(crc));
    }
    unsigned reverse = successful_groups & 1U;
    os_printf("[DOMAIN_PAIR_CACHE] state=%s baseline_crc=%08lX incoming_crc=%08lX retained_bytes=118000 restart_to_change_plan\r\n",
              reuse ? "REUSED" : "CAPTURED", (unsigned long)baseline_crc, (unsigned long)printer_crc32(bin, length));
    os_printf("[DOMAIN_PAIR_BEGIN] group=%lu order=%s scans=8 address=2,5 section=S2 masks=01,02,03 active_columns=%lu\r\n",
              (unsigned long)(successful_groups + 1U), reverse ? "REVERSE" : "FORWARD", (unsigned long)dots);
    os_printf("[DOMAIN_PAIR_TEMPLATE] physical_row=%lu addr=%lu section=S2 mask=02 source_dots=%lu\r\n",
              (unsigned long)(source.nozzle_row + 1U), (unsigned long)(source.slot / 4U + 1U), (unsigned long)dots);
    for (unsigned pass = 0; pass < 8U; ++pass) {
        unsigned which = printer_dp_case(pass, reverse);
        result = printer_dp_build(base, out, PRINTER_LC_BYTES, &source, which);
        if (result) goto done;
        if (printer_crc32(out, PRINTER_LC_BYTES) != crc[which] || printer_crc32(base, PRINTER_LC_BYTES) != baseline_crc) {
            result = -11; goto done;
        }
        static const char *const names[8] = {"ORIGINAL_CMY", "A2_D1", "A2_D2", "A2_D1_D2", "A5_D1", "A5_D2", "A5_D1_D2", "ORIGINAL_CMY"};
        os_printf("[DOMAIN_PAIR_PASS] scan=%u/8 case=%s crc32=%08lX\r\n", pass + 1U, names[which], (unsigned long)crc[which]);
        if (which && which != 7U) {
            unsigned address = printer_dp_address(which), mask = printer_dp_mask(which);
            os_printf("[DOMAIN_PAIR_EXPECT] addr=%u section=S2 mask=%02X D=%02X A=%08lX S=02 dots_D1_D6=%lu,%lu,0,0,0,0 bytes=118000\r\n",
                      address, mask, mask, (unsigned long)(1UL << (address - 1U)),
                      (unsigned long)((mask & 1U) ? dots : 0U), (unsigned long)((mask & 2U) ? dots : 0U));
        }
        failure_locked = true;
        result = printer_execute_cyan_line(out);
        if (result) goto done;
        if (printer_crc32(out, PRINTER_LC_BYTES) != crc[which] || printer_crc32(base, PRINTER_LC_BYTES) != baseline_crc) {
            result = -11; goto done;
        }
        { /* Advance after every line; final spacing separates the next APP task. */
            uint32_t sent = 0;
            uint32_t requested = pass < 7U ? PRINTER_LC_LINE_FEED_STEPS : PRINTER_LC_GROUP_FEED_STEPS;
            if (tph_mcu_set_head_power(0) != BK_OK) { result = -8; goto done; }
            result = motor_page_feed(requested, PRINTER_PAGE_FEED_DIRECTION, &sent);
            os_printf("[DOMAIN_PAIR_FEED] after=%u pulses=%lu/%u result=%d distance_unverified\r\n",
                      pass + 1U, (unsigned long)sent, (unsigned)requested, result);
            if (result != BK_OK || sent != requested) { result = -10; goto done; }
        }
    }
    failure_locked = false;
    ++successful_groups;
    os_printf("[DOMAIN_PAIR_DONE] group=%lu scans=8/8 feeds=8/8 next_order=%s ink_unverified\r\n",
              (unsigned long)successful_groups, (successful_groups & 1U) ? "REVERSE" : "FORWARD");
done:
    if (result) os_printf("[DOMAIN_PAIR_STOP] code=%d failure_locked=%u no_further_motion\r\n", result, (unsigned)failure_locked);
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

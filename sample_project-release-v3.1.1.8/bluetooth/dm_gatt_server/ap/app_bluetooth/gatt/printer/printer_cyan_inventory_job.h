#include "printer_cyan_inventory.h"
/* 11700 feed pulses / 30mm calibration: ~2mm within a group, ~4mm after it. */
#define PRINTER_LC_LINE_FEED_STEPS 780U
#define PRINTER_LC_GROUP_FEED_STEPS 1560U
/* Original APP swath 5, original CMY controls, and each long C trace.
 * All plans are validated before motion. Keep normal head-OFF sequencing. */
static int printer_run_cyan_inventory(const uint8_t *bin, uint32_t length) {
    static bool failure_locked = false;
    /* One bounded cache survives subsequent APP tasks, never grows per job.
     * New APP dithering must not silently change the selected physical nozzles. */
    static uint8_t *frozen_base = NULL;
    static uint32_t frozen_crc, frozen_pass_crc[PRINTER_CI_MAX_PASSES], successful_groups;
    static printer_ci_plan_t frozen_plan;
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
    printer_ci_plan_t plan;
    uint32_t crc[PRINTER_CI_MAX_PASSES] = {0}, baseline_crc;
    out = printer_map_psram_alloc(PRINTER_LC_BYTES);
    if (!out) { result = -2; goto done; }
    if (reuse) {
        base = frozen_base;
        printer_page_release(&page, printer_map_psram_free);
        baseline_crc = frozen_crc;
        if (printer_crc32(base, PRINTER_LC_BYTES) != frozen_crc) {
            failure_locked = true; result = -11; goto done;
        }
        plan = frozen_plan;
        memcpy(crc, frozen_pass_crc, sizeof(crc));
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
        result = printer_ci_collect(base, &cfg, &plan);
        if (result) goto done;
        baseline_crc = printer_crc32(base, PRINTER_LC_BYTES);
        for (unsigned pass = 0; pass < plan.count + 2U; ++pass) {
            result = printer_ci_build(base, out, PRINTER_LC_BYTES, &plan, pass, 0U);
            if (result) goto done;
            crc[pass] = printer_crc32(out, PRINTER_LC_BYTES);
            printer_map_yield_cpu();
        }
        frozen_base = base;
        frozen_crc = baseline_crc;
        frozen_plan = plan;
        memcpy(frozen_pass_crc, crc, sizeof(crc));
    }
    unsigned reverse = successful_groups & 1U;
    unsigned passes = plan.count + 2U;
    os_printf("[CYAN_INVENTORY_CACHE] state=%s baseline_crc=%08lX incoming_crc=%08lX retained_bytes=118000 restart_to_change_plan\r\n",
              reuse ? "REUSED" : "CAPTURED", (unsigned long)baseline_crc, (unsigned long)printer_crc32(bin, length));
    os_printf("[CYAN_INVENTORY_BEGIN] group=%lu order=%s nozzles=%lu scans=%u baseline_dots=%lu cyan_dots=%lu omitted_short_nozzles=%lu omitted_short_dots=%lu line_gap_steps=780 group_gap_steps=1560\r\n",
              (unsigned long)(successful_groups + 1U), reverse ? "DESC" : "ASC", (unsigned long)plan.count, passes,
              (unsigned long)plan.baseline_dots, (unsigned long)plan.cyan_dots,
              (unsigned long)plan.short_nozzles, (unsigned long)plan.short_dots);
    for (unsigned i = 0; i < plan.count; ++i)
        os_printf("[CYAN_INVENTORY_NOZZLE] id=C%02u physical_row=%lu addr=%lu section=S%lu domain=D%lu mask=0x%02lX source_dots=%lu crc32=%08lX\r\n",
                  i + 1U, (unsigned long)(plan.route[i].nozzle_row + 1U), (unsigned long)(plan.route[i].slot / 4U + 1U),
                  (unsigned long)(plan.route[i].slot % 4U + 1U), (unsigned long)(plan.route[i].mask == 1U ? 1U : 2U),
                  (unsigned long)plan.route[i].mask, (unsigned long)plan.dots[i], (unsigned long)crc[i + 1U]);
    for (unsigned pass = 0; pass < plan.count + 2U; ++pass) {
        unsigned index = !pass || pass == passes - 1U ? pass : printer_ci_index(&plan, pass, reverse) + 1U;
        result = printer_ci_build(base, out, PRINTER_LC_BYTES, &plan, pass, reverse);
        if (result) goto done;
        if (printer_crc32(out, PRINTER_LC_BYTES) != crc[index] || printer_crc32(base, PRINTER_LC_BYTES) != baseline_crc) {
            result = -11; goto done;
        }
        if (!pass || pass == passes - 1U)
            os_printf("[CYAN_INVENTORY_PASS] scan=%u/%u mode=ORIGINAL_CMY requested_dots=%lu crc32=%08lX\r\n",
                      pass + 1U, passes, (unsigned long)plan.baseline_dots, (unsigned long)crc[index]);
        else
            os_printf("[CYAN_INVENTORY_PASS] scan=%u/%u mode=CYAN_ONE id=C%02u physical_row=%lu requested_dots=%lu crc32=%08lX\r\n",
                      pass + 1U, passes, index, (unsigned long)(plan.route[index - 1U].nozzle_row + 1U),
                      (unsigned long)plan.dots[index - 1U], (unsigned long)crc[index]);
        failure_locked = true;
        result = printer_execute_cyan_line(out);
        if (result) goto done;
        if (printer_crc32(out, PRINTER_LC_BYTES) != crc[index] || printer_crc32(base, PRINTER_LC_BYTES) != baseline_crc) {
            result = -11; goto done;
        }
        { /* Advance after every line; final spacing separates the next APP task. */
            uint32_t sent = 0;
            uint32_t requested = pass + 1U < passes ? PRINTER_LC_LINE_FEED_STEPS : PRINTER_LC_GROUP_FEED_STEPS;
            if (tph_mcu_set_head_power(0) != BK_OK) { result = -8; goto done; }
            result = motor_page_feed(requested, PRINTER_PAGE_FEED_DIRECTION, &sent);
            os_printf("[CYAN_INVENTORY_FEED] after=%u pulses=%lu/%u result=%d distance_unverified\r\n",
                      pass + 1U, (unsigned long)sent, (unsigned)requested, result);
            if (result != BK_OK || sent != requested) { result = -10; goto done; }
        }
    }
    failure_locked = false;
    ++successful_groups;
    os_printf("[CYAN_INVENTORY_DONE] group=%lu scans=%u/%u feeds=%u/%u next_order=%s ink_unverified\r\n",
              (unsigned long)successful_groups, passes, passes, passes, passes, (successful_groups & 1U) ? "DESC" : "ASC");
done:
    if (result) os_printf("[CYAN_INVENTORY_STOP] code=%d failure_locked=%u no_further_motion\r\n", result, (unsigned)failure_locked);
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

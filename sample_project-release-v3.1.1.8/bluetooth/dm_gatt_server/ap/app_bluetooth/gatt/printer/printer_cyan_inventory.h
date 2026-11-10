#ifndef PRINTER_CYAN_INVENTORY_H
#define PRINTER_CYAN_INVENTORY_H
#include "printer_line_control.h"
#define PRINTER_CI_MAX_NOZZLES 12U
#define PRINTER_CI_MAX_PASSES (PRINTER_CI_MAX_NOZZLES + 2U)
typedef struct {
    printer_map_route_t route[PRINTER_CI_MAX_NOZZLES];
    uint32_t dots[PRINTER_CI_MAX_NOZZLES];
    uint32_t count, baseline_dots, cyan_dots, short_dots, short_nozzles;
} printer_ci_plan_t;
/* Only the long C traces actually present in the retained original F2 band.
 * Both C domains are included; do not rank nozzle health from command counts.
 * Bound the local Y span so the 2mm feed cannot overlap neighboring traces. */
static int printer_ci_collect(const uint8_t *base, const printer_map_config_t *cfg,
                              printer_ci_plan_t *plan) {
    if (!plan || !cfg || cfg->first_nozzle || cfg->reverse_y) return -1;
    memset(plan, 0, sizeof(*plan));
    printer_map_route_t strongest[3]; uint32_t strongest_dots[3];
    int result = printer_lc_select(base, cfg, strongest, strongest_dots, &plan->baseline_dots);
    if (result) return result;
    unsigned domains = 0, first = 0, last = 0, section = 4U;
    for (unsigned row = 0; row < PRINTER_MAP_ROWS; ++row) {
        printer_map_route_t r;
        if (printer_map_route(0, row, cfg, &r)) return -1;
        uint32_t dots = 0;
        for (unsigned x = 0; x < 1180U; ++x) dots += !!(base[x * 100U + r.slot] & r.mask);
        plan->cyan_dots += dots;
        if (!dots) continue;
        if (dots < PRINTER_LC_MIN_DOTS) { ++plan->short_nozzles; plan->short_dots += dots; continue; }
        if (plan->count >= PRINTER_CI_MAX_NOZZLES) return -4; /* never silently truncate */
        if (!plan->count) { first = row; section = r.slot % 4U; }
        last = row;
        if (r.slot % 4U != section || last - first > 16U) return -4;
        domains |= r.mask;
        plan->route[plan->count] = r;
        plan->dots[plan->count++] = dots;
    }
    if (plan->count < 2U || domains != 3U) return -6;
    return 0;
}
/* Same labeled physical nozzle has the same bytes in either execution order. */
static unsigned printer_ci_index(const printer_ci_plan_t *plan, unsigned pass, unsigned reverse) {
    return reverse ? plan->count - pass : pass - 1U;
}
static int printer_ci_build(const uint8_t *base, uint8_t *out, uint32_t bytes,
                            const printer_ci_plan_t *plan, unsigned pass, unsigned reverse) {
    if (!plan || !plan->count || plan->count > PRINTER_CI_MAX_NOZZLES ||
        pass > plan->count + 1U || reverse > 1U) return -1;
    printer_map_route_t chosen[3] = {{0}};
    if (!pass || pass == plan->count + 1U) return printer_lc_build(base, out, bytes, 0U, chosen);
    chosen[0] = plan->route[printer_ci_index(plan, pass, reverse)];
    return printer_lc_build(base, out, bytes, 1U, chosen);
}
#endif

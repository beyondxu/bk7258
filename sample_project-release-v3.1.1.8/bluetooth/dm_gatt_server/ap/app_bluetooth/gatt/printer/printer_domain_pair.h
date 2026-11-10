#ifndef PRINTER_DOMAIN_PAIR_H
#define PRINTER_DOMAIN_PAIR_H
#include "printer_line_control.h"
#define PRINTER_DP_PASSES 8U
/* Canonical cases: control, A2/D1,D2,D1+D2, A5/D1,D2,D1+D2, control. */
static unsigned printer_dp_case(unsigned pass, unsigned reverse) {
    return (!pass || pass == 7U || !reverse) ? pass : 7U - pass;
}
static unsigned printer_dp_address(unsigned which) { return which <= 3U ? 2U : 5U; }
static unsigned printer_dp_mask(unsigned which) { return 1U + (which - 1U) % 3U; }
/* The template is a long source D2/S2 trace. Prefer the already observed
 * A2/D2 trace, otherwise use a present long D2/S2 trace. No extra dots. */
static int printer_dp_select(const uint8_t *base, const printer_map_config_t *cfg,
                             printer_map_route_t *source, uint32_t *dots, uint32_t *total) {
    if (!cfg || cfg->first_nozzle || cfg->reverse_y || !source || !dots || !total) return -1;
    printer_map_route_t selected[3]; uint32_t counts[3];
    int result = printer_lc_select(base, cfg, selected, counts, total);
    if (result) return result;
    *dots = 0;
    for (unsigned row = 0; row < PRINTER_MAP_ROWS; ++row) {
        printer_map_route_t r;
        if (printer_map_route(0, row, cfg, &r)) return -1;
        if (r.mask != 2U || r.slot % 4U != 1U) continue;
        uint32_t count = 0;
        for (unsigned x = 0; x < 1180U; ++x) count += !!(base[x * 100U + r.slot] & r.mask);
        if (count < PRINTER_LC_MIN_DOTS) continue;
        if (count > *dots) { *source = r; *dots = count; }
        if (r.slot == 5U) { *source = r; *dots = count; break; }
    }
    return *dots ? 0 : -6;
}
/* The six cases share identical active column positions, address and mask
 * being the only planned changes. D1+D2 selects two physical cyan nozzles. */
static int printer_dp_build(const uint8_t *base, uint8_t *out, uint32_t bytes,
                            const printer_map_route_t *source, unsigned which) {
    if (!base || !out || base == out || bytes < PRINTER_LC_BYTES || !source || which >= PRINTER_DP_PASSES) return -1;
    if (!which || which == 7U) {
        printer_map_route_t unused[3] = {{0}};
        return printer_lc_build(base, out, bytes, 0U, unused);
    }
    if (source->slot >= 100U || source->mask != 2U || source->slot % 4U != 1U) return -1;
    memset(out, 0, PRINTER_LC_BYTES);
    unsigned slot = (printer_dp_address(which) - 1U) * 4U + 1U, mask = printer_dp_mask(which);
    for (unsigned x = 0; x < 1180U; ++x)
        if (base[x * 100U + source->slot] & source->mask) out[x * 100U + slot] = (uint8_t)mask;
    return 0;
}
#endif

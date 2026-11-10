#ifndef PRINTER_LINE_CONTROL_H
#define PRINTER_LINE_CONTROL_H
#include "printer_mapper.h"
#include <string.h>
#define PRINTER_LC_BYTES 118000U
#define PRINTER_LC_MIN_DOTS 256U
/* Pick one strongest, central physical row per color from the actual retained
 * swath. Never move dots or invent extra firing pulses. */
static int printer_lc_select(const uint8_t *base, const printer_map_config_t *cfg,
                            printer_map_route_t chosen[3], uint32_t counts[3], uint32_t *total) {
    if (!base || !cfg || !chosen || !counts || !total) return -1;
    *total = 0;
    for (unsigned x = 0; x < 1180U; ++x) {
        unsigned active = 0;
        for (unsigned s = 0; s < 100U; ++s) {
            uint8_t b = base[x * 100U + s];
            if (b & 0xC0U) return -1;
            for (unsigned bit = 0; bit < 6; ++bit) active += (b >> bit) & 1U;
        }
        /* This is a bounded F2 comparison, not a general full-coverage image. */
        if (active > 32U) return -4;
        *total += active;
    }
    for (unsigned ch = 0; ch < 3; ++ch) {
        uint16_t dots[PRINTER_MAP_ROWS];
        unsigned max = 0, first = 0, last = 0;
        for (unsigned row = 0; row < PRINTER_MAP_ROWS; ++row) {
            printer_map_route_t route;
            if (printer_map_route(ch, row, cfg, &route)) return -1;
            unsigned count = 0;
            for (unsigned x = 0; x < 1180U; ++x) count += !!(base[x * 100U + route.slot] & route.mask);
            dots[row] = (uint16_t)count;
            if (count > max) { max = count; first = last = row; }
            else if (count == max) last = row;
        }
        if (max < PRINTER_LC_MIN_DOTS) return -6;
        unsigned midpoint = (first + last) / 2U, selected = first, distance = 200U;
        for (unsigned row = first; row <= last; ++row) if (dots[row] == max) {
            unsigned d = row > midpoint ? row - midpoint : midpoint - row;
            if (d < distance) { selected = row; distance = d; }
        }
        if (printer_map_route(ch, selected, cfg, &chosen[ch])) return -1;
        counts[ch] = max;
    }
    return 0;
}
/* Pass order: original CMY, selected C, selected M, selected Y, original CMY.
 * Single-nozzle passes preserve exactly the source nozzle's original columns. */
static int printer_lc_build(const uint8_t *base, uint8_t *out, uint32_t bytes,
                           unsigned pass, const printer_map_route_t chosen[3]) {
    if (!base || !out || base == out || bytes < PRINTER_LC_BYTES || pass >= 5U || !chosen) return -1;
    if (pass == 0U || pass == 4U) { memcpy(out, base, PRINTER_LC_BYTES); return 0; }
    const printer_map_route_t *r = &chosen[pass - 1U];
    if (r->slot >= 100U || !r->mask || r->mask > 32U || (r->mask & (r->mask - 1U)) ||
        !(r->mask & (3U << ((pass - 1U) * 2U)))) return -1;
    memset(out, 0, PRINTER_LC_BYTES);
    for (unsigned x = 0; x < 1180U; ++x) out[x * 100U + r->slot] = (uint8_t)(base[x * 100U + r->slot] & r->mask);
    return 0;
}
#endif

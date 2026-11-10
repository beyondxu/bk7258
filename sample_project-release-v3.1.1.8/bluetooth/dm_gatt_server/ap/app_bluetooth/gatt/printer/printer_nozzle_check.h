#ifndef PRINTER_NOZZLE_CHECK_H
#define PRINTER_NOZZLE_CHECK_H
#include "printer_mapper.h"
#include <string.h>
#define PRINTER_NC_NOZZLES 200U
#define PRINTER_NC_GROUP_SIZE 50U
#define PRINTER_NC_GROUPS 4U
#define PRINTER_NC_COLUMNS 1180U
#define PRINTER_NC_BYTES (PRINTER_NC_COLUMNS * 100U)
#define PRINTER_NC_FIRST 80U
#define PRINTER_NC_SPACING 20U
#define PRINTER_NC_LENGTH 16U
/* Diagnostic row is physical Y descending, 0..199, NOT a vendor domain ID.
 * Reuse the existing 192-row mapper without changing normal image routing.
 * The final eight rows use the legal first_nozzle=8 placement. */
static int printer_nc_route(unsigned channel, unsigned nozzle,
                            const printer_map_config_t *cfg, printer_map_route_t *route) {
    if (!cfg || channel >= 3U || nozzle >= PRINTER_NC_NOZZLES ||
        cfg->first_nozzle || cfg->reverse_y) return -1;
    printer_map_config_t shifted = *cfg;
    unsigned row = nozzle;
    if (row >= PRINTER_MAP_ROWS) { shifted.first_nozzle = 8U; row -= 8U; }
    return printer_map_route(channel, row, &shifted, route);
}
/* Prove all 100 address/section slots and six data domains occur exactly once.
 * This checks the selected software mapping, not wiring or actual ink. */
static int printer_nc_validate(const printer_map_config_t *cfg) {
    uint8_t seen[100][6] = {{0}};
    for (unsigned ch = 0; ch < 3; ++ch) for (unsigned n = 0; n < PRINTER_NC_NOZZLES; ++n) {
        printer_map_route_t route;
        if (printer_nc_route(ch, n, cfg, &route) || route.slot >= 100U) return -1;
        unsigned bit = 0;
        while (bit < 6U && route.mask != (1U << bit)) ++bit;
        if (bit >= 6U || bit / 2U != ch || seen[route.slot][bit]) return -1;
        seen[route.slot][bit] = 1;
    }
    for (unsigned slot = 0; slot < 100; ++slot) for (unsigned bit = 0; bit < 6; ++bit)
        if (!seen[slot][bit]) return -1;
    return 0;
}
/* One color/group. Every nozzle gets 16 requested dots; at most one active
 * bit per transmitted column, including the per-domain X compensation. */
static int printer_nc_build(uint8_t *columns, uint32_t bytes, unsigned channel,
                            unsigned group, const printer_map_config_t *cfg) {
    if (!columns || bytes < PRINTER_NC_BYTES || channel >= 3U || group >= PRINTER_NC_GROUPS) return -1;
    memset(columns, 0, PRINTER_NC_BYTES);
    for (unsigned i = 0; i < PRINTER_NC_GROUP_SIZE; ++i) {
        printer_map_route_t route;
        if (printer_nc_route(channel, group * PRINTER_NC_GROUP_SIZE + i, cfg, &route) ||
            route.slot >= 100U || route.slot % 4U != group || !route.mask || route.mask > 32U) return -1;
        uint32_t first = PRINTER_NC_FIRST + i * PRINTER_NC_SPACING + route.delay_columns;
        if (first > PRINTER_NC_COLUMNS - PRINTER_NC_LENGTH) return -1;
        for (unsigned x = first; x < first + PRINTER_NC_LENGTH; ++x) {
            /* Reject any overlap instead of silently increasing peak load. */
            uint8_t *col = columns + x * 100U;
            for (unsigned slot = 0; slot < 100; ++slot) if (col[slot]) return -1;
            col[route.slot] = (uint8_t)route.mask;
        }
    }
    return 0;
}
#endif

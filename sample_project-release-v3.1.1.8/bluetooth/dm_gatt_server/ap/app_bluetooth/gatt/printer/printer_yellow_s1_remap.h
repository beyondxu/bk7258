/* Bounded nozzle comparison for the audited six-band APP image, yellow only.
 * Validate completely before changing page pixels; never drive hardware here.
 */
#ifndef PRINTER_YELLOW_S1_REMAP_H
#define PRINTER_YELLOW_S1_REMAP_H
#include "printer_page.h"
static unsigned yellow_page_bit(const uint8_t *p, unsigned y) {
    return (p[y / 8U] >> (7U - y % 8U)) & 1U;
}
static int printer_yellow_s1_remap(printer_page_t *page) {
    const unsigned base = 384U, first = 9U, last = 23U, shift = 20U;
    if (!page || !page->planes || page->height <= base + last + shift ||
        page->cfg.first_nozzle != 0U || page->cfg.reverse_y != 0U) return -4;
    for (unsigned row = first; row <= last; ++row) {
        printer_map_route_t a,b;
        if (printer_map_route(2U,row,&page->cfg,&a) ||
            printer_map_route(2U,row+shift,&page->cfg,&b) ||
            a.slot % 4U != 0U || b.slot % 4U != 0U ||
            a.mask != b.mask || a.slot == b.slot) return -4;
    }
    uint8_t *yellow = page->planes + 2U * page->width * page->stride;
    uint32_t moved = 0;
    /* Expected upper band's pixels only; reject a different APP layout rather
     * than merge pixels, lose dose, or move another S1 band unnoticed.
     */
    for (unsigned x = 0; x < page->width; ++x) {
        const uint8_t *p = yellow + x * page->stride;
        for (unsigned row = 0; row < 50U; ++row) if (yellow_page_bit(p,base+row)) {
            if (row < first || row > last) {
                os_printf("[YELLOW_S1_REJECT] unexpected_row=%u no_motion\r\n",row);
                return -4;
            }
            ++moved;
        }
    }
    if (!moved) { os_printf("[YELLOW_S1_REJECT] upper_band_empty no_motion\r\n"); return -6; }
    for (unsigned x = 0; x < page->width; ++x) {
        uint8_t *p = yellow + x * page->stride;
        for (unsigned row = first; row <= last; ++row) {
            unsigned src=base+row, dst=src+shift;
            if (yellow_page_bit(p,src)) {
                p[src/8U] &= (uint8_t)~(0x80U >> (src%8U));
                p[dst/8U] |= (uint8_t)(0x80U >> (dst%8U));
            }
        }
    }
    os_printf("[YELLOW_S1_REMAP] swath=3 old_rows=9..23 new_rows=29..43 dots=%lu D_pair_preserved S1_only S3_unchanged no_extra_fires\r\n",(unsigned long)moved);
    return 0;
}
#endif

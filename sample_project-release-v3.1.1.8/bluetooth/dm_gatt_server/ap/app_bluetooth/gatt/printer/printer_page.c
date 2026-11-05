#include "printer_page.h"
#include <string.h>
#define SRC_STRIDE (PRINTER_PAGE_SOURCE_PADDED / 8U)
#define SRC_PLANE (PRINTER_PAGE_SOURCE_WIDTH * SRC_STRIDE)
typedef struct { uint8_t *source; uint32_t seen; } load_context_t;
static int load_strip(const uint8_t *planes, uint32_t width, uint32_t y, void *context) {
    load_context_t *load = context;
    if (width != PRINTER_PAGE_SOURCE_WIDTH || y % 192U || y + 192U > PRINTER_PAGE_SOURCE_PADDED ||
        y != load->seen * 192U) return -4;
    for (uint32_t ch = 0; ch < 3; ++ch)
        for (uint32_t x = 0; x < width; ++x)
            memcpy(load->source + ch * SRC_PLANE + x * SRC_STRIDE + y / 8U,
                   planes + ch * width * 24U + x * 24U, 24);
    ++load->seen;
    return 0;
}
static unsigned bit(const uint8_t *p, uint32_t y) { return (p[y / 8U] >> (7U - y % 8U)) & 1U; }
void printer_page_release(printer_page_t *page, void (*release)(void *)) {
    if (!page || !release) return;
    if (page->planes) release(page->planes);
    memset(page, 0, sizeof(*page));
}
int printer_page_prepare(const uint8_t *bin, uint32_t length,
                         const printer_map_config_t *cfg, printer_page_t *page,
                         void *(*allocate)(size_t), void (*release)(void *), void (*yield_cpu)(void)) {
    if (!page || !allocate || !release) return -1;
    memset(page, 0, sizeof(*page));
    printer_map_route_t route;
    uint32_t max_delay = 0;
    for (uint32_t ch = 0; ch < 3; ++ch) for (uint32_t row = 0; row < 192; ++row) {
        if (printer_map_route(ch, row, cfg, &route)) return -1;
        if (route.delay_columns > max_delay) max_delay = route.delay_columns;
    }
    if (max_delay >= PRINTER_PAGE_COLUMNS) return -4;
    uint32_t width = PRINTER_PAGE_COLUMNS - max_delay;
    uint64_t divisor = (uint64_t)PRINTER_PAGE_SOURCE_WIDTH * PRINTER_PAGE_Y_PITCH;
    uint32_t height = (uint32_t)(((uint64_t)width * PRINTER_PAGE_SOURCE_HEIGHT * cfg->x_pitch_centi_um + divisor / 2) / divisor);
    if (!height || height > PRINTER_PAGE_SOURCE_PADDED || width > PRINTER_PAGE_SOURCE_WIDTH) return -4;
    load_context_t load = {allocate(SRC_PLANE * 3U), 0};
    if (!load.source) return -2;
    memset(load.source, 0, SRC_PLANE * 3U);
    printer_map_stats_t stats;
    int result = printer_map_visit_bin(bin, length, cfg, &stats, load_strip, &load, allocate, release, yield_cpu);
    uint16_t *ymap = NULL;
    if (result) goto done;
    if (load.seen != PRINTER_PAGE_SOURCE_PADDED / 192U) { result = -4; goto done; }
    /* Only trim the known native padding, never discard nonzero source data. */
    for (uint32_t ch = 0; ch < 3; ++ch) for (uint32_t x = 0; x < PRINTER_PAGE_SOURCE_WIDTH; ++x) {
        const uint8_t *p = load.source + ch * SRC_PLANE + x * SRC_STRIDE;
        for (uint32_t y = PRINTER_PAGE_SOURCE_HEIGHT; y < PRINTER_PAGE_SOURCE_PADDED; ++y)
            if (bit(p, y)) { result = -4; goto done; }
    }
    page->width = width; page->height = height; page->stride = (height + 7U) / 8U;
    page->swaths = (height + 191U) / 192U; page->max_delay = max_delay; page->cfg = *cfg;
    page->planes = allocate((size_t)width * page->stride * 3U);
    ymap = allocate(PRINTER_PAGE_SOURCE_HEIGHT * sizeof(*ymap));
    if (!page->planes || !ymap) { result = -2; goto done; }
    memset(page->planes, 0, (size_t)width * page->stride * 3U);
    for (uint32_t y = 0; y < PRINTER_PAGE_SOURCE_HEIGHT; ++y)
        ymap[y] = (uint16_t)((uint64_t)y * height / PRINTER_PAGE_SOURCE_HEIGHT);
    /* Forward area union preserves isolated source pixels/lines when reducing.
     * It can darken an already-halftoned photograph; density is not calibrated.
     */
    for (uint32_t ch = 0; ch < 3; ++ch) for (uint32_t x = 0; x < PRINTER_PAGE_SOURCE_WIDTH; ++x) {
        uint32_t dx = x * width / PRINTER_PAGE_SOURCE_WIDTH;
        const uint8_t *source = load.source + ch * SRC_PLANE + x * SRC_STRIDE;
        uint8_t *target = page->planes + ch * width * page->stride + dx * page->stride;
        for (uint32_t y = 0; y < PRINTER_PAGE_SOURCE_HEIGHT; ++y) if (bit(source, y)) {
            ++page->source_dots[ch];
            target[ymap[y] / 8U] |= (uint8_t)(0x80U >> (ymap[y] % 8U));
        }
        if (yield_cpu && (x & 31U) == 0U) yield_cpu();
    }
    for (uint32_t ch = 0; ch < 3; ++ch) for (uint32_t x = 0; x < width; ++x)
        for (uint32_t y = 0; y < height; ++y)
            page->output_dots[ch] += bit(page->planes + ch * width * page->stride + x * page->stride, y);
done:
    if (ymap) release(ymap);
    release(load.source);
    if (result) printer_page_release(page, release);
    return result;
}
int printer_page_render(const printer_page_t *page, uint32_t swath,
                        uint8_t *columns, uint32_t column_bytes,
                        uint8_t *scratch, uint32_t scratch_bytes,
                        uint32_t dots[3], void (*yield_cpu)(void)) {
    if (!page || !page->planes || !page->width || !columns || !scratch || !dots || swath >= page->swaths ||
        column_bytes < PRINTER_PAGE_COLUMNS * 100U || scratch_bytes < page->width * 72U) return -1;
    memset(scratch, 0, page->width * 72U); memset(dots, 0, 3U * sizeof(*dots));
    for (uint32_t ch = 0; ch < 3; ++ch) for (uint32_t x = 0; x < page->width; ++x) {
        const uint8_t *p = page->planes + ch * page->width * page->stride + x * page->stride;
        for (uint32_t row = 0; row < 192; ++row) {
            uint32_t y = swath * 192U + row;
            if (y < page->height && bit(p, y)) {
                scratch[ch * page->width * 24U + x * 24U + row / 8U] |= (uint8_t)(0x80U >> (row % 8U));
                ++dots[ch];
            }
        }
    }
    for (uint32_t x = 0; x < PRINTER_PAGE_COLUMNS; ++x) {
        if (printer_map_column(scratch, page->width, x, &page->cfg, columns + x * 100U)) return -1;
        if (yield_cpu && (x & 31U) == 0U) yield_cpu();
    }
    return 0;
}

int printer_page_transform(printer_page_t *page, uint32_t flags, void (*yield_cpu)(void)) {
    if (!page || !page->planes || (flags & ~3U)) return -1;
    if (flags & 1U) for (uint32_t ch = 0; ch < 3; ++ch)
        for (uint32_t x = 0; x < page->width / 2U; ++x) {
            uint8_t *a = page->planes + ch * page->width * page->stride + x * page->stride;
            uint8_t *b = page->planes + ch * page->width * page->stride + (page->width - 1U - x) * page->stride;
            for (uint32_t i = 0; i < page->stride; ++i) { uint8_t value = a[i]; a[i] = b[i]; b[i] = value; }
            if (yield_cpu && (x & 31U) == 0U) yield_cpu();
        }
    if (flags & 2U) for (uint32_t ch = 0; ch < 3; ++ch)
        for (uint32_t x = 0; x < page->width; ++x) {
            uint8_t *p = page->planes + ch * page->width * page->stride + x * page->stride;
            for (uint32_t y = 0; y < page->height / 2U; ++y) {
                uint32_t other = page->height - 1U - y;
                if (bit(p, y) != bit(p, other)) {
                    p[y / 8U] ^= (uint8_t)(0x80U >> (y % 8U));
                    p[other / 8U] ^= (uint8_t)(0x80U >> (other % 8U));
                }
            }
            if (yield_cpu && (x & 31U) == 0U) yield_cpu();
        }
    return 0;
}

int printer_page_color_columns(const uint8_t *source, uint8_t *target,
                               uint32_t bytes, uint32_t channel_mask) {
    if (!source || !target || source == target || !bytes || !channel_mask || (channel_mask & ~7U)) return -1;
    uintptr_t a = (uintptr_t)source, b = (uintptr_t)target;
    if (a < b ? b - a < bytes : a - b < bytes) return -1;
    uint8_t domains = 0;
    for (unsigned ch = 0; ch < 3; ++ch) if (channel_mask & (1U << ch)) domains |= (uint8_t)(3U << (2U * ch));
    for (uint32_t i = 0; i < bytes; ++i) if (source[i] & 0xC0U) return -1;
    for (uint32_t i = 0; i < bytes; ++i) target[i] = source[i] & domains;
    return 0;
}

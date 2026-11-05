#ifndef PRINTER_PAGE_H
#define PRINTER_PAGE_H
#include "printer_mapper.h"
#define PRINTER_PAGE_COLUMNS 1180U
#define PRINTER_PAGE_SOURCE_WIDTH 1180U
#define PRINTER_PAGE_SOURCE_HEIGHT 1476U
#define PRINTER_PAGE_SOURCE_PADDED 1536U
#define PRINTER_PAGE_Y_PITCH 4233U
/* Column-major full-page CMY bitmap. All source parsing/resizing finishes
 * before hardware may run; preserve physical aspect inside known travel.
 */
typedef struct {
    uint8_t *planes;
    uint32_t width, height, stride, swaths, max_delay;
    uint32_t source_dots[3], output_dots[3];
    printer_map_config_t cfg;
} printer_page_t;
int printer_page_prepare(const uint8_t *bin, uint32_t length,
                         const printer_map_config_t *cfg, printer_page_t *page,
                         void *(*allocate)(size_t), void (*release)(void *), void (*yield_cpu)(void));
/* flags bit0=mirror X, bit1=flip whole-page Y; no motor direction changes. */
/* Color-only filtering: mask bit0=C, bit1=M, bit2=Y. Source stays intact. */
int printer_page_color_columns(const uint8_t *source, uint8_t *target,
                               uint32_t bytes, uint32_t channel_mask);
int printer_page_transform(printer_page_t *page, uint32_t flags, void (*yield_cpu)(void));
void printer_page_release(printer_page_t *page, void (*release)(void *));
int printer_page_render(const printer_page_t *page, uint32_t swath,
                        uint8_t *columns, uint32_t column_bytes,
                        uint8_t *scratch, uint32_t scratch_bytes,
                        uint32_t dots[3], void (*yield_cpu)(void));
#endif

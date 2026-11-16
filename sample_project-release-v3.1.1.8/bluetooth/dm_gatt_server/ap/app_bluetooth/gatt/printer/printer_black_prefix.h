/* Build one CMY overlay of the current upper yellow band, before any motion. */
#ifndef PRINTER_BLACK_PREFIX_H
#define PRINTER_BLACK_PREFIX_H
#include "printer_page.h"
#include "printer_audit_utils.h"
#include <string.h>
static int printer_build_black_prefix(const printer_page_t *page, uint8_t *columns, uint8_t *scratch) {
#if PRINTER_YELLOW_S1_REMAP
    const unsigned first=29U,last=43U;
#else
    const unsigned first=9U,last=23U;
#endif
    if (!page || !page->planes || !columns || !scratch || page->height<=384U+last) return -4;
    printer_page_t black = *page;
    black.height=192U;black.stride=24U;black.swaths=1U;
    black.planes=printer_map_psram_alloc(page->width*72U);
    if (!black.planes) return -2;
    memset(black.planes,0,page->width*72U);
    uint32_t selected=0;
    for (unsigned x=0;x<page->width;++x) for (unsigned row=first;row<=last;++row) {
        const uint8_t *p=page->planes+2U*page->width*page->stride+x*page->stride;
        unsigned y=384U+row;
        if ((p[y/8U]>>(7U-y%8U))&1U) {
            for (unsigned ch=0;ch<3U;++ch)
                black.planes[ch*page->width*24U+x*24U+row/8U]|=(uint8_t)(0x80U>>(row%8U));
            ++selected;
        }
    }
    int result=-6;
    if (selected) {
        uint32_t dots[3];
        result=printer_page_render(&black,0,columns,118000U,scratch,page->width*72U,dots,printer_map_yield_cpu);
        if (!result && (dots[0]!=selected || dots[1]!=selected || dots[2]!=selected)) result=-11;
        if (!result) os_printf("[BLACK_PREFIX_PLAN] pixels_per_CMY=%lu rows=%u..%u bytes=118000 crc32=%08lX copies=1 CMY_black_ink_color_unverified\r\n",
            (unsigned long)selected,first,last,(unsigned long)printer_crc32(columns,118000U));
    }
    printer_map_psram_free(black.planes);
    return result;
}
#endif

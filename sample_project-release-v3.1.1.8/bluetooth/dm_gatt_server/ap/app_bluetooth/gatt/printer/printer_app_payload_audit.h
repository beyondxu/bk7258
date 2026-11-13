/* Actual APP data inspection. No GPIO, SPI, motor, or head calls. */
#ifndef PRINTER_APP_PAYLOAD_AUDIT_H
#define PRINTER_APP_PAYLOAD_AUDIT_H
#include "printer_page.h"
#include "printer_audit_utils.h"
#if PRINTER_FIXED_S1_COMPARE
#define APP_AUDIT_SECTION_INDEX 0U
#define APP_AUDIT_SECTION_NAME "S1"
#else
#define APP_AUDIT_SECTION_INDEX 1U
#define APP_AUDIT_SECTION_NAME "S2"
#endif
#include <stdio.h>
static int printer_run_app_payload_audit(const uint8_t *bin, uint32_t length) {
    if (!bin || !length) return -1;
    const uint32_t limit = 131072U;
    uint32_t exported = length < limit ? length : limit;
    uint32_t raw_crc = printer_crc32(bin, length);
    os_printf("[APP_BIN_BEGIN] bytes=%lu exported=%lu crc32=%08lX complete=%u\r\n",
              (unsigned long)length, (unsigned long)exported, (unsigned long)raw_crc,
              (unsigned)(exported == length));
    for (uint32_t offset = 0; offset < exported; offset += 64U) {
        char hex[129];
        uint32_t n = exported - offset; if (n > 64U) n = 64U;
        static const char digits[] = "0123456789ABCDEF";
        for (uint32_t j = 0; j < n; ++j) {
            hex[2U*j] = digits[bin[offset+j] >> 4];
            hex[2U*j+1U] = digits[bin[offset+j] & 15U];
        }
        hex[2U*n] = 0;
        os_printf("[APP_BIN_HEX] offset=%08lX data=%s\r\n", (unsigned long)offset, hex);
        /* Pace UART output below 115200 baud; let BLE/service tasks run. */
        rtos_delay_milliseconds(20);
    }
    os_printf("[APP_BIN_END] exported=%lu crc32=%08lX\r\n",
              (unsigned long)exported, (unsigned long)raw_crc);
    printer_map_config_t cfg;
    printer_map_default_config(&cfg);
    cfg.x_pitch_centi_um = PRINTER_MAP_X_PITCH_CENTI_UM;
    cfg.x_direction = PRINTER_MAP_X_DIRECTION;
    cfg.first_nozzle = PRINTER_MAP_FIRST_NOZZLE;
    cfg.reverse_y = PRINTER_MAP_REVERSE_Y;
    printer_page_t page;
    int result = printer_page_prepare(bin, length, &cfg, &page,
        printer_map_psram_alloc, printer_map_psram_free, printer_map_yield_cpu);
    if (result) {
        os_printf("[APP_DECODE_END] code=%d raw_preserved_in_log no_motion_no_ink\r\n", result);
        return result;
    }
    os_printf("[APP_GEOMETRY] width=%lu height=%lu swaths=%lu first_nozzle=%lu reverse_y=%lu x_direction=%ld x_pitch=%lu fixed_recipe_NOT_used\r\n",
              (unsigned long)page.width, (unsigned long)page.height, (unsigned long)page.swaths,
              (unsigned long)cfg.first_nozzle, (unsigned long)cfg.reverse_y,
              (long)cfg.x_direction, (unsigned long)cfg.x_pitch_centi_um);
    for (unsigned c = 0; c < 3; ++c)
        os_printf("[APP_CHANNEL] id=%u decoder_label=%c source_dots=%lu resized_dots=%lu\r\n",
                  c+1U, "CMY"[c], (unsigned long)page.source_dots[c], (unsigned long)page.output_dots[c]);
    uint8_t *columns = printer_map_psram_alloc(PRINTER_PAGE_COLUMNS * 100U);
    uint8_t *scratch = printer_map_psram_alloc(page.width * 72U);
    uint32_t counts[6][4] = {{0}}, addresses[6][4] = {{0}}, selected[6] = {0};
    /* Compare the current fixed inspection's A/S selection to actual APP pixels. */
    static const unsigned recipe[2][4] = {{23,5,12,18},{14,20,2,9}};
    if (!columns || !scratch) { result = -2; goto done; }
    for (uint32_t swath = 0; swath < page.swaths; ++swath) {
        uint32_t dots[3];
        result = printer_page_render(&page, swath, columns, PRINTER_PAGE_COLUMNS * 100U,
                   scratch, page.width * 72U, dots, printer_map_yield_cpu);
        if (result) goto done;
        for (uint32_t x = 0; x < PRINTER_PAGE_COLUMNS; ++x) {
            for (unsigned slot = 0; slot < 100U; ++slot) {
                unsigned v = columns[x * 100U + slot];
                unsigned sec = slot % 4U, add = slot / 4U;
                for (unsigned d = 0; d < 6U; ++d) if (v & (1U << d)) {
                    ++counts[d][sec]; addresses[d][sec] |= 1U << add;
                    if (sec == APP_AUDIT_SECTION_INDEX)
                        for (unsigned k = 0; k < 4U; ++k)
                            if (add + 1U == recipe[d & 1U][k]) ++selected[d];
                }
            }
            if ((x & 31U) == 0U) printer_map_yield_cpu();
        }
    }
    for (unsigned d = 0; d < 6U; ++d) {
        for (unsigned sec = 0; sec < 4U; ++sec)
            os_printf("[APP_ROUTE] D=%u S=%u dots=%lu A=%08lX\r\n", d+1U, sec+1U,
                      (unsigned long)counts[d][sec], (unsigned long)addresses[d][sec]);
        os_printf("[APP_SELECTED] area=" APP_AUDIT_SECTION_NAME " D=%u dots=%lu actual_APP_pixels fixed_pattern_NOT_generated\r\n",
                  d+1U, (unsigned long)selected[d]);
    }
done:
    if (columns) printer_map_psram_free(columns);
    if (scratch) printer_map_psram_free(scratch);
    printer_page_release(&page, printer_map_psram_free);
    os_printf("[APP_DECODE_END] code=%d no_motion_no_ink\r\n", result);
    return result;
}
#endif

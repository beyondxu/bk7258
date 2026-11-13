/* Fixed S2 diagnostic recipe v1, independent of APP pixels and reboot state.
 * A values below are one-based address LUT indices, NOT raw A-pin patterns.
 * S2 uses Appendix-B physical rows 55..62 (zero-based). S1 control changes
 * only the section slot; it selects different nozzles, four per D domain.
 * The old frame cannot be reconstructed from CRC/statistics; this is a NEW
 * reproducible baseline using its A/S support, not its original pixel bytes.
 */
#ifndef PRINTER_FIXED_S2_H
#define PRINTER_FIXED_S2_H
#include <stdint.h>
#include <string.h>
#ifndef PRINTER_FIXED_S1_COMPARE
#define PRINTER_FIXED_S1_COMPARE 0
#endif
#if PRINTER_FIXED_S1_COMPARE != 0 && PRINTER_FIXED_S1_COMPARE != 1
#error PRINTER_FIXED_S1_COMPARE must be 0 or 1
#endif
#if PRINTER_FIXED_S1_COMPARE
#define PRINTER_FIXED_SECTION_INDEX 0U
#define PRINTER_FIXED_SECTION_MASK 0x01U
#define PRINTER_FIXED_SECTION_NAME "S1"
#else
#define PRINTER_FIXED_SECTION_INDEX 1U
#define PRINTER_FIXED_SECTION_MASK 0x02U
#define PRINTER_FIXED_SECTION_NAME "S2"
#endif
#define PRINTER_FIXED_S2_ACTIVE_COLUMNS 772U
#if PRINTER_FIXED_S1_COMPARE
#define PRINTER_FIXED_S2_CRC32 0x1CFD33E1U
#else
#define PRINTER_FIXED_S2_CRC32 0xEA748497U
#endif
static int printer_fixed_s2_build(uint8_t *columns, uint32_t bytes) {
    static const uint8_t addresses[2][4] = {{23,5,12,18}, {14,20,2,9}};
    /* D1..D6 offsets for the existing negative-X, 30um column geometry. */
    static const uint8_t offsets[6] = {61,59,31,30,1,0};
    if (!columns || bytes != 118000U) return -1;
    memset(columns, 0, bytes);
    for (unsigned d = 0; d < 6U; ++d) {
        for (unsigned n = 0; n < 4U; ++n) {
            unsigned slot = (addresses[d & 1U][n] - 1U) * 4U + PRINTER_FIXED_SECTION_INDEX;
            for (unsigned x = 150U + offsets[d]; x < 861U + offsets[d]; ++x)
                columns[x * 100U + slot] |= (uint8_t)(1U << d);
        }
    }
    return 0;
}
#endif

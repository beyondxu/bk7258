#ifndef PRINTER_INTERLACE_MASK_H
#define PRINTER_INTERLACE_MASK_H
#include <stdint.h>
/* Stable source-coordinate mask, identical for CMY and either scan direction.
 * The former (x+y)%blocks made each pass a visible diagonal grating when
 * nozzle output or pass registration differed. Mix both coordinates to avoid
 * that coherent grating. This does not compensate missing/misaligned drops.
 * blocks is the positive number of physically eligible nozzle blocks.
 */
static inline uint32_t printer_interlace_block(uint32_t x, uint32_t y, uint32_t blocks) {
    uint32_t v = x * UINT32_C(0x9E3779B1) ^ y * UINT32_C(0x85EBCA77);
    v ^= v >> 16;
    v *= UINT32_C(0x7FEB352D);
    v ^= v >> 15;
    v *= UINT32_C(0x846CA68B);
    v ^= v >> 16;
    return v % blocks;
}
#endif

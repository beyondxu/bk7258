#ifndef PRINTER_INTERLACE_MASK_H
#define PRINTER_INTERLACE_MASK_H
#include <stdint.h>
typedef struct {
    uint8_t first_block[4], block_count[4], sections[4], section_count;
} printer_interlace_choices_t;

static inline printer_interlace_choices_t printer_interlace_choices(uint32_t first_row, uint32_t rows, uint32_t blocks) {
    printer_interlace_choices_t choices = {0};
    for (uint32_t block = 0; block < blocks; ++block) {
        /* M0333N rows in physical Y order have 50 consecutive rows per section. */
        unsigned section = (first_row + block * rows) / 50U;
        if (!choices.block_count[section]) {
            choices.first_block[section] = (uint8_t)block;
            choices.sections[choices.section_count++] = (uint8_t)section;
        }
        ++choices.block_count[section];
    }
    return choices;
}

/* The same source coordinate selects the same section and block for CMY and
 * both scan directions. Each available section has equal weight. */
static inline uint32_t printer_interlace_block(uint32_t x, uint32_t y, const printer_interlace_choices_t *choices) {
    uint32_t v = x * UINT32_C(0x9E3779B1) ^ y * UINT32_C(0x85EBCA77);
    v ^= v >> 16;
    v *= UINT32_C(0x7FEB352D);
    v ^= v >> 15;
    v *= UINT32_C(0x846CA68B);
    v ^= v >> 16;
    unsigned section = choices->sections[v % choices->section_count];
    return choices->first_block[section] + (v / choices->section_count) % choices->block_count[section];
}
#endif

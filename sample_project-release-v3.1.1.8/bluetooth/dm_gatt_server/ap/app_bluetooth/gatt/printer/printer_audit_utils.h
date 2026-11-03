#ifndef PRINTER_AUDIT_UTILS_H
#define PRINTER_AUDIT_UTILS_H
#include <stdint.h>
static inline uint32_t printer_crc32_update(uint32_t crc, const uint8_t *data, uint32_t len)
{
    while (len--) {
        crc ^= *data++;
        for (unsigned bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ ((0U - (crc & 1U)) & 0xEDB88320U);
    }
    return crc;
}
static inline uint32_t printer_crc32(const uint8_t *data, uint32_t len)
{
    return printer_crc32_update(0xFFFFFFFFU, data, len) ^ 0xFFFFFFFFU;
}
#endif

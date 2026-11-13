/* Sealed fixed-S2 inspection plans. Addresses/rows are supplied by recipe v1. */
#ifndef PRINTER_FIXED_S2_CHECK_PLAN_H
#define PRINTER_FIXED_S2_CHECK_PLAN_H
#include <stdint.h>
#if PRINTER_FIXED_S2_DOMAIN_CHECK
#define PRINTER_S2_CHECK_PARTS 6U
#define PRINTER_S2_CHECK_LINES 12U
#define PRINTER_S2_CHECK_NOZZLES 4U
#define PRINTER_S2_CHECK_TAG "DOMAIN_CHECK"
#define PRINTER_S2_CHECK_ORDER_TEXT "D1,D2,D3,D4,D5,D6,D6,D5,D4,D3,D2,D1"
#define PRINTER_S2_CHECK_JOB_CRC 0xA13F47F7U
static const char *const s_s2_check_names[6] = {"D1_CYAN","D2_CYAN","D3_MAGENTA","D4_MAGENTA","D5_YELLOW","D6_YELLOW"};
static const uint8_t s_s2_check_masks[6] = {1,2,4,8,16,32};
static const uint8_t s_s2_check_order[12] = {0,1,2,3,4,5,5,4,3,2,1,0};
static const uint32_t s_s2_check_crc[6] = {0xCAF90008U,0x48E40DD2U,0xC030ADC3U,0x7558A543U,0xD2195130U,0x1E7396E0U};
#else
#define PRINTER_S2_CHECK_PARTS 3U
#define PRINTER_S2_CHECK_LINES 6U
#define PRINTER_S2_CHECK_NOZZLES 8U
#define PRINTER_S2_CHECK_TAG "COLOR_CHECK"
#define PRINTER_S2_CHECK_ORDER_TEXT "C,M,Y,Y,M,C"
#define PRINTER_S2_CHECK_JOB_CRC 0x7764411CU
static const char *const s_s2_check_names[3] = {"CYAN","MAGENTA","YELLOW"};
static const uint8_t s_s2_check_masks[3] = {3,12,48};
static const uint8_t s_s2_check_order[6] = {0,1,2,2,1,0};
static const uint32_t s_s2_check_crc[3] = {0x93764BC7U,0xA4034E9DU,0xDD0181CDU};
#endif
#endif

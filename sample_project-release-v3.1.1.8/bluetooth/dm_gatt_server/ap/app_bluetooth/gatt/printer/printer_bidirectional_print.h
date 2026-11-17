#ifndef PRINTER_BIDIRECTIONAL_PRINT_H
#define PRINTER_BIDIRECTIONAL_PRINT_H
#include "printer_service_geometry.h"
/* Counted endpoints only: motor/ink feedback is not available. A partial move
 * invalidates position and never triggers a blind return. */
static bool s_bidi_right = false, s_bidi_known = true;
static uint32_t s_bidi_max_column_us;
static uint64_t s_bidi_column_us;
#define PRINTER_BIDI_START_LOW_US 1000U
#define PRINTER_BIDI_CRUISE_LOW_US 80U
#define PRINTER_BIDI_RAMP_STEPS 250U
static bool printer_bidi_move(uint8_t direction, uint32_t steps, const uint8_t *columns) {
    if (direction) GPIO_UP(TPH_MOT_DIR); else GPIO_DOWN(TPH_MOT_DIR);
    delay_us(50);
    /* Forward fires before edges at200+6*n. Reverse must start206 steps in:
     * 7480-206-6*n = 200+6*(1179-n). This avoids a one-column seam shift. */
    const uint32_t lead = direction ? 200U : 206U;
    for (uint32_t i=0;i<steps;++i) {
        if (columns && i>=lead && (i-lead)%6U==0U && (i-lead)/6U<1180U) {
            uint64_t start=bk_aon_rtc_get_us();
            if (tph_fire_raw_column_bytes(columns+s_blank_spi_columns*100U)!=BK_OK) return false;
            uint32_t elapsed=(uint32_t)(bk_aon_rtc_get_us()-start);
            s_bidi_column_us+=elapsed;
            if (elapsed>s_bidi_max_column_us) s_bidi_max_column_us=elapsed;
            ++s_blank_spi_columns;
        }
        uint32_t edge=i<steps-1U-i ? i : steps-1U-i;
        uint32_t low=edge<PRINTER_BIDI_RAMP_STEPS
            ? PRINTER_BIDI_START_LOW_US-(PRINTER_BIDI_START_LOW_US-PRINTER_BIDI_CRUISE_LOW_US)*edge/PRINTER_BIDI_RAMP_STEPS
            : PRINTER_BIDI_CRUISE_LOW_US;
        GPIO_UP(TPH_MOT_STEP); delay_us(4); GPIO_DOWN(TPH_MOT_STEP); delay_us(low);
        ++s_blank_spi_steps[direction?1:0];
        if (!columns && (i&127U)==127U) rtos_delay_milliseconds(1);
    }
    return !columns || s_blank_spi_columns==1180U;
}
static int printer_execute_bidirectional_line(const uint8_t *columns, uint8_t direction) {
    if (!columns || direction>1U) return -1;
    if (!s_bidi_known || direction!=(s_bidi_right?0U:1U)) return -7;
    int result=0;
    s_blank_spi_columns=0;
    s_blank_spi_steps[0]=s_blank_spi_steps[1]=0;
    s_bidi_column_us=0; s_bidi_max_column_us=0;
    uint64_t start=bk_aon_rtc_get_us();
    if (tph_mcu_set_head_power(0)!=BK_OK) return -8;
    carriage_verified_power_up();
    if (!printer_bidi_move(direction,PRINTER_SERVICE_MARGIN_STEPS,NULL)) {result=-9;goto stop;}
    if (tph_mcu_set_head_power(1)!=BK_OK) {result=-8;goto stop;}
#if PRINTER_PAGE_HEAD_SETTLE_MS > 0
    rtos_delay_milliseconds(PRINTER_PAGE_HEAD_SETTLE_MS);
#endif
    if (!printer_bidi_move(direction,PRINTER_SERVICE_BASE_STEPS,columns)) {result=-9;goto stop;}
    if (tph_mcu_set_head_power(0)!=BK_OK) {result=-8;goto stop;}
    if (!printer_bidi_move(direction,PRINTER_SERVICE_MARGIN_STEPS,NULL)) {result=-9;goto stop;}
    s_bidi_right=direction?true:false;
stop:
    if (result) {
        s_bidi_known=false;
        (void)tph_mcu_set_head_power(0);
        os_printf("[BIDI_STOP] code=%d position_unknown no_blind_return restart_restore_left\r\n",result);
    }
    carriage_verified_power_down();
    os_printf("[BIDI_SCAN] direction=%s code=%d columns=%lu/1180 forward=%lu reverse=%lu elapsed_us=%lu column_total_us=%lu column_max_us=%lu endpoint=%s motion_and_ink_unverified\r\n",
              direction?"RIGHT":"LEFT",result,(unsigned long)s_blank_spi_columns,
              (unsigned long)s_blank_spi_steps[1],(unsigned long)s_blank_spi_steps[0],
              (unsigned long)(bk_aon_rtc_get_us()-start),(unsigned long)s_bidi_column_us,
              (unsigned long)s_bidi_max_column_us,s_bidi_known?(s_bidi_right?"RIGHT":"LEFT"):"UNKNOWN");
    return result;
}
static int printer_bidirectional_finish(void) {
    if (!s_bidi_known) return -7;
    if (!s_bidi_right) return 0;
    if (tph_mcu_set_head_power(0)!=BK_OK) {s_bidi_known=false;return -8;}
    carriage_verified_power_up();
    bool ok=printer_bidi_move(0U,PRINTER_SERVICE_SPAN_STEPS,NULL);
    carriage_verified_power_down();
    if (!ok) {s_bidi_known=false;return -9;}
    s_bidi_right=false;
    os_printf("[BIDI_PARK] endpoint=LEFT head_OFF columns=0 counted_position_only\r\n");
    return 0;
}
#endif

#ifndef PRINTER_BIDIRECTIONAL_PRINT_H
#define PRINTER_BIDIRECTIONAL_PRINT_H
#include "printer_service_geometry.h"
/* A page owns the settled boost. Counted endpoints are not encoder feedback. */
static bool s_bidi_right=false, s_bidi_known=true, s_bidi_power=false;
static uint32_t s_bidi_max_column_us, s_bidi_long_column_waits;
static int s_bidi_move_error;
static uint64_t s_bidi_column_us;
#define PRINTER_BIDI_START_LOW_US 1000U
#define PRINTER_BIDI_CRUISE_LOW_US 260U
#define PRINTER_BIDI_RAMP_STEPS 250U
static void printer_bidirectional_abort(void) {
    if (!s_bidi_power) return; /* preflight failure must not move/enable hardware */
    s_bidi_known=false;
    (void)tph_mcu_set_head_power(0);
    carriage_verified_power_down();
    s_bidi_power=false;
}
static int printer_bidi_power_up(void) {
    if (s_bidi_power) {carriage_wake_on_powered_rail();return 0;}
    if (tph_mcu_set_head_power(0)!=BK_OK) return -8;
    carriage_verified_power_up();
    s_bidi_power=true;
    return 0;
}
/* Fixed per-pulse HIGH/LOW delays. SPI may extend the preceding STEP-low
 * interval, but its latency is NEVER recovered with shorter subsequent pulses.
 * No RTC-derived per-step pacing and no explicit task sleep inside the stroke.
 * Only the two outer endpoints ramp; maintenance/image boundaries do not. */
static bool printer_bidi_move(uint8_t direction,const uint8_t *columns) {
    if (direction) GPIO_UP(TPH_MOT_DIR);else GPIO_DOWN(TPH_MOT_DIR);
    delay_us(50);
    s_bidi_move_error=-9;
    const uint32_t lead=PRINTER_SERVICE_MARGIN_STEPS+(direction?200U:206U);
    const uint32_t image_end=PRINTER_SERVICE_MARGIN_STEPS+PRINTER_SERVICE_BASE_STEPS;
    for (uint32_t i=0;i<PRINTER_SERVICE_SPAN_STEPS;++i) {
        if (columns && i==PRINTER_SERVICE_MARGIN_STEPS && tph_mcu_set_head_power(1)!=BK_OK) {s_bidi_move_error=-8;return false;}
        if (columns && i==image_end && tph_mcu_set_head_power(0)!=BK_OK) {s_bidi_move_error=-8;return false;}
        if (columns && i>=lead && (i-lead)%6U==0U && (i-lead)/6U<1180U) {
            uint64_t tx=bk_aon_rtc_get_us();
            if (tph_fire_raw_column_bytes(columns+s_blank_spi_columns*100U)!=BK_OK) return false;
            uint32_t elapsed=(uint32_t)(bk_aon_rtc_get_us()-tx);
            s_bidi_column_us+=elapsed;
            if (elapsed>1000U)++s_bidi_long_column_waits;
            if (elapsed>s_bidi_max_column_us)s_bidi_max_column_us=elapsed;
            ++s_blank_spi_columns;
        }
        uint32_t edge=i<PRINTER_SERVICE_SPAN_STEPS-1U-i?i:PRINTER_SERVICE_SPAN_STEPS-1U-i;
        uint32_t low=edge<PRINTER_BIDI_RAMP_STEPS
            ? PRINTER_BIDI_START_LOW_US-(PRINTER_BIDI_START_LOW_US-PRINTER_BIDI_CRUISE_LOW_US)*edge/PRINTER_BIDI_RAMP_STEPS
            : PRINTER_BIDI_CRUISE_LOW_US;
        GPIO_UP(TPH_MOT_STEP);delay_us(4);GPIO_DOWN(TPH_MOT_STEP);delay_us(low);
        ++s_blank_spi_steps[direction?1:0];
    }
    return !columns || s_blank_spi_columns==1180U;
}
static int printer_execute_bidirectional_line(const uint8_t *columns,uint8_t direction) {
    if (direction>1U)return -1;
    if (!s_bidi_known || direction!=(s_bidi_right?0U:1U))return -7;
    s_blank_spi_columns=0;s_blank_spi_steps[0]=s_blank_spi_steps[1]=0;
    s_bidi_column_us=0;s_bidi_max_column_us=0;s_bidi_long_column_waits=0;
    uint64_t start=bk_aon_rtc_get_us();
    int result=printer_bidi_power_up();
    if (result)return result;
    if (!printer_bidi_move(direction,columns))result=s_bidi_move_error;
    if (!result)s_bidi_right=direction?true:false;
    else {
        printer_bidirectional_abort();
        os_printf("[BIDI_STOP] code=%d position_unknown no_blind_return restart_restore_left\r\n",result);
    }
    os_printf("[BIDI_SCAN] direction=%s code=%d blank=%u columns=%lu forward=%lu reverse=%lu elapsed_us=%lu column_total_us=%lu column_max_us=%lu long_column_waits=%lu endpoint=%s counted_motion_ink_unverified\r\n",
              direction?"RIGHT":"LEFT",result,columns?0U:1U,(unsigned long)s_blank_spi_columns,
              (unsigned long)s_blank_spi_steps[1],(unsigned long)s_blank_spi_steps[0],
              (unsigned long)(bk_aon_rtc_get_us()-start),(unsigned long)s_bidi_column_us,
              (unsigned long)s_bidi_max_column_us,(unsigned long)s_bidi_long_column_waits,
              s_bidi_known?(s_bidi_right?"RIGHT":"LEFT"):"UNKNOWN");
    return result;
}
static int printer_bidirectional_feed(uint32_t steps,uint8_t direction,uint32_t *sent) {
    if (!s_bidi_power || !s_bidi_known)return -1;
    uint64_t start=bk_aon_rtc_get_us();
    int result=motor_page_feed_fast_keep_power(steps,direction,sent);
    os_printf("[BIDI_FEED] steps=%lu elapsed_us=%lu code=%d head_OFF boost_held distance_unverified\r\n",
              (unsigned long)(sent?*sent:0),(unsigned long)(bk_aon_rtc_get_us()-start),result);
    return result;
}
static int printer_bidirectional_finish(void) {
    if (!s_bidi_known)return -7;
    if (!s_bidi_power)return 0;
    if (s_bidi_right) {
        carriage_wake_on_powered_rail();
        if (!printer_bidi_move(0U,NULL)) {printer_bidirectional_abort();return -9;}
        s_bidi_right=false;
        os_printf("[BIDI_PARK] endpoint=LEFT head_OFF columns=0 counted_position_only\r\n");
    }
    carriage_verified_power_down();s_bidi_power=false;
    return 0;
}
#endif

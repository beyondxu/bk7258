#ifndef PRINTER_BOOT_CARRIAGE_LOOP_H
#define PRINTER_BOOT_CARRIAGE_LOOP_H
/* Only called before BLE and page/maintenance workers exist. Temporary
 * counted legs use the same bounded hardware timer, with optional OC3 mode.
 */
#ifndef PRINTER_BOOT_LOOP_STEPS
#define PRINTER_BOOT_LOOP_STEPS PRINTER_SERVICE_SPAN_STEPS
#endif
#if PRINTER_BOOT_LOOP_STEPS < 1 || PRINTER_BOOT_LOOP_STEPS > PRINTER_SERVICE_SPAN_STEPS
#error Temporary counted loop stroke exceeds the configured print stroke
#endif
static int printer_boot_counted_round_trip(void) {
    if(!s_bidi_known || !s_bidi_at_wiper || s_bidi_right) return -7;
    int32_t left=s_bidi_left_offset;
    int result=printer_buffered_reposition(1U,PRINTER_BOOT_LOOP_STEPS);
    if(result) return result;
    s_bidi_right=true;s_bidi_at_wiper=false;
    result=printer_buffered_reposition(0U,PRINTER_BOOT_LOOP_STEPS);
    if(result) return result;
    s_bidi_right=false;s_bidi_at_wiper=true;s_bidi_left_offset=left;s_bidi_right_offset=0;
    return 0;
}
int printer_run_boot_carriage_loop(void) {
#if PRINTER_BOOT_CARRIAGE_LOOP && PRINTER_BUFFERED_LINE && PRINTER_FULL_PAGE && PRINTER_PAGE_BIDIRECTIONAL
    if (!printer_link_is_ready()) return -7;
    int result=0;
    uint32_t rounds=0;
    if (tph_mcu_set_head_power(0)!=BK_OK) return -8;
    os_printf("[BOOT_CARRIAGE_LOOP] TEMP head_OFF feed=0 STEP_us=%u high_us=%u start_us=%u ramp=%u ignore_GPIO24=%u counted_leg_steps=%u stop=power_OFF\r\n",
              PRINTER_BOOT_LOOP_STEP_US,PRINTER_CANVAS_STEP_HIGH_US,PL_START_PERIOD_US,
              PL_RAMP_STEPS,PRINTER_BOOT_LOOP_IGNORE_OC3,PRINTER_BOOT_LOOP_STEPS);
    s_buf_boot_loop_profile=1U;
    for (;;) {
        /* At the LEFT starting anchor the RIGHT sensor must be released. */
        if (printer_buffered_uses_oc3()&&motor_oc3_active()) {result=PRINTER_OC3_STUCK;break;}
        uint32_t right_before=s_blank_spi_steps[1],left_before=s_blank_spi_steps[0];
        os_printf("[BOOT_CARRIAGE_LOOP_BEGIN] next=%lu gpio24_active=%u known=%u at_wiper=%u\r\n",
                  (unsigned long)(rounds+1U),(unsigned)(printer_buffered_uses_oc3()?(unsigned)motor_oc3_active():0U),(unsigned)s_bidi_known,(unsigned)s_bidi_at_wiper);
#if PRINTER_BOOT_LOOP_IGNORE_OC3
        result=printer_boot_counted_round_trip();
#else
        result=printer_buffered_boot_round_trip();
#endif
        os_printf("[BOOT_CARRIAGE_LOOP_RESULT] next=%lu code=%d right_pulses=%lu left_pulses=%lu gpio24_active=%u known=%u at_wiper=%u last_steps=%lu last_irq_error=%lu last_max_gap_us=%lu\r\n",
                  (unsigned long)(rounds+1U),result,
                  (unsigned long)(s_blank_spi_steps[1]-right_before),(unsigned long)(s_blank_spi_steps[0]-left_before),
                  (unsigned)(printer_buffered_uses_oc3()?(unsigned)motor_oc3_active():0U),(unsigned)s_bidi_known,(unsigned)s_bidi_at_wiper,
                  (unsigned long)s_buf_steps,(unsigned long)s_buf_error,(unsigned long)s_buf_max_gap);
        if (result) break;
        ++rounds;
        os_printf("[BOOT_CARRIAGE_LOOP_ROUND] completed=%lu endpoint=WIPER ink=OFF paper_steps=0\r\n",(unsigned long)rounds);
        rtos_delay_milliseconds(1U);
    }
    printer_buffered_stop();
    s_buf_boot_loop_profile=0U;
    printer_bidirectional_abort();
    os_printf("[BOOT_CARRIAGE_LOOP_STOP] code=%d rounds=%lu no_retry_no_blind_return\r\n",result,(unsigned long)rounds);
    return result;
#else
    return -7;
#endif
}
#endif

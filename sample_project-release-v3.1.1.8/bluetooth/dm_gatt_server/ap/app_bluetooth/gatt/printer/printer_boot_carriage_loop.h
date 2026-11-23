#ifndef PRINTER_BOOT_CARRIAGE_LOOP_H
#define PRINTER_BOOT_CARRIAGE_LOOP_H
/* Only called before BLE and page/maintenance workers exist. Reuse the
 * optical RIGHT seek and fixed LEFT return; no raw/unbounded motor loop.
 */
int printer_run_boot_carriage_loop(void) {
#if PRINTER_BOOT_CARRIAGE_LOOP && PRINTER_BUFFERED_LINE && PRINTER_FULL_PAGE && PRINTER_PAGE_BIDIRECTIONAL
    if (!printer_link_is_ready()) return -7;
    int result=0;
    uint32_t rounds=0;
    if (tph_mcu_set_head_power(0)!=BK_OK) return -8;
    os_printf("[BOOT_CARRIAGE_LOOP] TEMP head_OFF feed=0 STEP_us=%u high_us=%u start_us=%u ramp=%u OC3_gpio24 RIGHT bounded_seek=%u LEFT_return=%u stop=power_OFF\r\n",
              PRINTER_BOOT_LOOP_STEP_US,PRINTER_CANVAS_STEP_HIGH_US,PL_START_PERIOD_US,
              PL_RAMP_STEPS,PRINTER_OC3_BOOT_SEARCH_STEPS,PRINTER_OC3_BOOT_RETURN_STEPS);
    s_buf_boot_loop_profile=1U;
    for (;;) {
        /* At the LEFT starting anchor the RIGHT sensor must be released. */
        if (motor_oc3_active()) {result=PRINTER_OC3_STUCK;break;}
        result=printer_buffered_boot_round_trip();
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

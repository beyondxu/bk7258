#ifndef PRINTER_STARTUP_PREPARE_H
#define PRINTER_STARTUP_PREPARE_H
/* Counted LEFT origin is still required. These are head-OFF service strokes,
 * not sensor-based homing, and not proof of wiping/capping or successful ink. */
static int printer_startup_prepare(void) {
    if (!s_bidi_known || s_bidi_right) return -7;
    os_printf("[PAGE_PREP_BEGIN] load_steps=%lu head_OFF no_image_pixels unchanged_stroke\r\n",
              (unsigned long)PRINTER_STARTUP_LOAD_STEPS);
    int result=printer_execute_bidirectional_line(NULL,1U);
    if (result) return result;
    os_printf("[PAGE_PREP_STAGE] stage=RIGHT_SERVICE head_OFF counted_position_only\r\n");
    if (tph_mcu_set_head_power(0)!=BK_OK) return -8;
#if PRINTER_STARTUP_LOAD_STEPS > 0
    uint32_t sent=0;
    result=motor_page_load_keep_power(PRINTER_STARTUP_LOAD_STEPS,
                                     PRINTER_PAGE_FEED_DIRECTION,&sent);
    os_printf("[PAGE_PREP_LOAD] pulses=%lu/%lu code=%d distance_unverified\r\n",
              (unsigned long)sent,(unsigned long)PRINTER_STARTUP_LOAD_STEPS,result);
    if (result!=BK_OK || sent!=PRINTER_STARTUP_LOAD_STEPS) return -10;
#else
    os_printf("[PAGE_PREP_LOAD] disabled reason=OFFSET_UNCALIBRATED pulses=0\r\n");
#endif
    result=printer_execute_bidirectional_line(NULL,0U);
    if (result) return result;
    os_printf("[PAGE_PREP_DONE] endpoint=LEFT head_OFF next=image_row_zero mechanical_contact_unverified\r\n");
    return 0;
}
#endif

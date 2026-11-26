#ifndef PRINTER_STARTUP_PREPARE_H
#define PRINTER_STARTUP_PREPARE_H
/* Counted LEFT origin is still required. No sensor homing is implied. */
static int printer_startup_prepare(unsigned for_page) {
    if (!s_bidi_known || s_bidi_right) return -7;
    os_printf("[PAGE_PREP_BEGIN] head_OFF no_image_pixels\r\n");
    int result=0;
    if (for_page) {
#if PRINTER_BUFFERED_LINE
        if (tph_line_probe()!=BK_OK) return -13;
#endif
        result=printer_bidi_power_up();
        if (result) return result;
#if PRINTER_BUFFERED_LINE && PRINTER_STARTUP_WIPE_STEPS > 0
        /* Run the short head-OFF carriage preparation before loading. */
        result=printer_buffered_short_round_trips(PRINTER_STARTUP_WIPE_STEPS,PRINTER_STARTUP_WIPE_ROUNDS,
                                                  PRINTER_PAGE_SHORT_STEP_US);
        if(result==PRINTER_OC3_RETURNED&&s_bidi_known&&s_bidi_at_wiper)
            result=printer_buffered_short_round_trips(PRINTER_STARTUP_WIPE_STEPS,PRINTER_STARTUP_WIPE_ROUNDS,
                                                  PRINTER_PAGE_SHORT_STEP_US);
        if (result) return result;
#endif
#if PRINTER_PRESPRAY
        result=printer_run_prespray("PAGE",3U,true);
        if(result)return result;
        if(!s_bidi_known||s_bidi_at_wiper||s_bidi_right)return -7;
        /* 11700 feed steps / 30 mm = 3900 steps for the provisional 10 mm. */
        const uint32_t load_steps=3900U;
        uint32_t sent=0;
        result=motor_page_load_keep_power(load_steps,PRINTER_PAGE_FEED_DIRECTION,&sent);
        os_printf("[PAGE_PREP_LOAD] pulses=%lu/%lu code=%d head_OFF after_last_pad_visit carriage=PRINT_LEFT\r\n",
                  (unsigned long)sent,(unsigned long)load_steps,result);
        if(result!=BK_OK||sent!=load_steps)return -10;
#elif PRINTER_STARTUP_LOAD_STEPS > 0
        const uint32_t legs=PRINTER_STARTUP_LOAD_ROUNDS*2U;
        os_printf("[PAGE_LOAD_PLAN] round_trips=%u legs=%lu paper_steps_per_leg=%u total_paper_steps=%lu head_OFF\r\n",
                  PRINTER_STARTUP_LOAD_ROUNDS,(unsigned long)legs,PRINTER_STARTUP_LOAD_STEPS,
                  (unsigned long)(legs*PRINTER_STARTUP_LOAD_STEPS));
        for(uint32_t leg=0;leg<legs;++leg){
#if PRINTER_BUFFERED_LINE
        result=printer_buffered_load_while_moving(PRINTER_STARTUP_LOAD_STEPS,PRINTER_PAGE_FEED_DIRECTION);
        if(result)return result;
#else
        uint32_t sent=0;
        result=motor_page_load_keep_power(PRINTER_STARTUP_LOAD_STEPS,
                                         PRINTER_PAGE_FEED_DIRECTION,&sent);
        os_printf("[PAGE_PREP_LOAD] pulses=%lu/%lu code=%d distance_unverified\r\n",
                  (unsigned long)sent,(unsigned long)PRINTER_STARTUP_LOAD_STEPS,result);
        if (result!=BK_OK || sent!=PRINTER_STARTUP_LOAD_STEPS) return -10;
#endif
            os_printf("[PAGE_LOAD_LEG_DONE] leg=%lu/%lu cumulative_paper_steps=%lu head_OFF\r\n",
                      (unsigned long)(leg+1U),(unsigned long)legs,
                      (unsigned long)((leg+1U)*PRINTER_STARTUP_LOAD_STEPS));
        }
#else
        os_printf("[PAGE_PREP_LOAD] disabled reason=OFFSET_UNCALIBRATED pulses=0\r\n");
#endif
#if PRINTER_BUFFERED_LINE && !PRINTER_PRESPRAY
        result=printer_buffered_leave_wiper();
        if(result)return result;
#endif
        os_printf("[PAGE_PREP_DONE] head_OFF load_complete next=image_row_zero\r\n");
        return 0; /* only now may the image controller ARM a firing row */
    }
    /* Boot remains one full head-OFF round trip, zero loading and zero wiping. */
    os_printf("[PAGE_PREP_LOAD] skipped reason=BOOT_NO_FEED pulses=0\r\n");
#if PRINTER_BUFFERED_LINE
    result=printer_buffered_boot_round_trip();
    if(result)return result;
#else
    result=printer_execute_bidirectional_line(NULL,1U);
    if (result) return result;
    os_printf("[PAGE_PREP_STAGE] stage=RIGHT_SERVICE head_OFF counted_position_only\r\n");
    if (tph_mcu_set_head_power(0)!=BK_OK) return -8;
    result=printer_execute_bidirectional_line(NULL,0U);
    if (result) return result;
#endif
    os_printf("[PAGE_PREP_DONE] endpoint=LEFT head_OFF boot_round_trip_complete\r\n");
    return 0;
}
#endif

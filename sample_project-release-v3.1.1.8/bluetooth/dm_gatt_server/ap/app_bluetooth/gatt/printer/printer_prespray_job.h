#ifndef PRINTER_PRESPRAY_JOB_H
#define PRINTER_PRESPRAY_JOB_H
#include "printer_prespray_pattern.h"
#include "printer_ink_idle.h"
#include "printer_waste_motion.h"
/* Serialized boot/page worker only. No temperature or recovery claim. */
static int printer_run_prespray(const char *reason){
    if(!printer_link_is_ready()||!s_bidi_known||!s_bidi_at_wiper||s_bidi_right)return -7;
    printer_map_config_t cfg;printer_map_default_config(&cfg);
    cfg.x_pitch_centi_um=PRINTER_MAP_X_PITCH_CENTI_UM;cfg.x_direction=PRINTER_MAP_X_DIRECTION;
    cfg.first_nozzle=PRINTER_MAP_FIRST_NOZZLE;cfg.reverse_y=PRINTER_MAP_REVERSE_Y;
    uint8_t *row=printer_map_psram_alloc(PRINTER_NC_BYTES);
    if(!row)return -2;
    uint64_t idle_us;unsigned idle_known;
    const unsigned rounds=printer_prespray_plan(bk_aon_rtc_get_us(),&idle_us,&idle_known);
    const unsigned first=rounds>PRINTER_PRESPRAY_ROW_ROUNDS?PRINTER_PRESPRAY_ROW_ROUNDS:rounds;
    int result=printer_prespray_build(row,PRINTER_NC_BYTES,first,&cfg);
    if(result)goto done; /* Reject bad mapping before any movement. */
    s_waste_station_ready=0U;
    os_printf("[PRESPRAY_BEGIN] reason=%s idle_known=%u idle_s=%lu nozzles=600 rounds=%u total_requests=%lu prespray_only_pad_nops=32 nominal_extra_ns160 target_unmeasured no_TSR no_preheat peak_requests_per_column=48 domain_mode=D1_to_D6 one_D_two_S_per_column row_rounds=40 burst_max=8 repeat_us=673 between_rows_ms=20 paper_steps=0\r\n",
              reason,idle_known,(unsigned long)(idle_us/1000000U),rounds,(unsigned long)(rounds*600U));
    result=printer_waste_position();if(result)goto stop;
    unsigned batches=0U;
    for(unsigned sent=0U;sent<rounds;){
        unsigned chunk=rounds-sent;if(chunk>PRINTER_PRESPRAY_ROW_ROUNDS)chunk=PRINTER_PRESPRAY_ROW_ROUNDS;
        result=printer_prespray_build(row,PRINTER_NC_BYTES,chunk,&cfg);if(result)goto stop;
        result=printer_waste_fire_prespray(row);if(result)goto stop;
        printer_prespray_note_ink();sent+=chunk;++batches;
        os_printf("[PRESPRAY_BATCH] reason=%s completed_rounds=%u/%u cache_rows=%u\r\n",reason,sent,rounds,batches);
        /* HC32 has shut the head OFF before returning line DONE. */
        if(sent<rounds)rtos_delay_milliseconds(20U);
    }
    if(tph_mcu_set_head_power(0)!=BK_OK){result=-8;goto stop;}
    s_waste_station_ready=0U;
    result=printer_bidirectional_finish();if(result)goto stop;
    os_printf("[PRESPRAY_DONE] reason=%s stationary_cache_rows=%u parked=WIPER no_extra_wipe no_paper_feed recovery_unverified\r\n",reason,batches);
    goto done;
stop:
    s_waste_station_ready=0U;
    printer_bidirectional_abort();
    os_printf("[PRESPRAY_STOP] reason=%s code=%d no_retry print_blocked\r\n",reason,result);
done:
    printer_map_psram_free(row);return result;
}
#endif

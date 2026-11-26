#ifndef PRINTER_PRESPRAY_JOB_H
#define PRINTER_PRESPRAY_JOB_H
#include "printer_prespray_pattern.h"
#include "printer_line_protocol.h"
#include "printer_ink_idle.h"
#include "printer_waste_motion.h"
static bool s_prespray_boot_pending_page;
/* Serialized boot/page worker only. No temperature or recovery claim. */
static int printer_run_prespray(const char *reason,unsigned visits,bool finish_at_print_left){
    if(!visits||!printer_link_is_ready()||!s_bidi_known||!s_bidi_at_wiper||s_bidi_right)return -7;
    printer_map_config_t cfg;printer_map_default_config(&cfg);
    cfg.x_pitch_centi_um=PRINTER_MAP_X_PITCH_CENTI_UM;cfg.x_direction=PRINTER_MAP_X_DIRECTION;
    cfg.first_nozzle=PRINTER_MAP_FIRST_NOZZLE;cfg.reverse_y=PRINTER_MAP_REVERSE_Y;
    uint8_t *row=printer_map_psram_alloc(PRINTER_NC_BYTES);
    if(!row)return -2;
    uint64_t idle_us;unsigned idle_known;
    const unsigned idle_rounds=printer_prespray_plan(bk_aon_rtc_get_us(),&idle_us,&idle_known);
    const bool boot_followup=finish_at_print_left&&s_prespray_boot_pending_page;
    const unsigned rounds=!finish_at_print_left?PRINTER_PRESPRAY_BOOT_ROUNDS:
        boot_followup?PRINTER_PRESPRAY_LONG_ROUNDS:idle_rounds;
    unsigned prepared_rounds=0U;
    int result=0;
    s_waste_station_ready=0U;
    os_printf("[PRESPRAY_BEGIN] reason=%s idle_known=%u idle_s=%lu boot_followup=%u nozzles=600 rounds=%u total_requests=%lu prespray_only_pad_nops=32 nominal_extra_ns160 target_unmeasured no_TSR no_preheat peak_requests_per_column=%u domain_mode=D1_to_D6 one_D_two_S_per_column row_rounds=40 burst_max=8 repeat_us=%u between_rows_ms=5 paper_steps=0\r\n",
              reason,idle_known,(unsigned long)(idle_us/1000000U),(unsigned)boot_followup,rounds,(unsigned long)(rounds*600U),PRINTER_PRESPRAY_PEAK_BITS,PL_PRESPRAY_PERIOD_US);
    unsigned batches=0U,sent=0U;
    for(unsigned visit=0;visit<visits;++visit){
        const unsigned remaining_visits=visits-visit;
        const unsigned remaining=rounds-sent;
        const unsigned dose=(remaining+remaining_visits-1U)/remaining_visits;
        const unsigned end=sent+dose;
        const unsigned first=dose>PRINTER_PRESPRAY_ROW_ROUNDS?PRINTER_PRESPRAY_ROW_ROUNDS:dose;
        if(first!=prepared_rounds){
            result=printer_prespray_build(row,PRINTER_NC_BYTES,first,&cfg);
            if(result){if(visit)goto stop;goto done;}
            prepared_rounds=first;
        }
        const bool print_range=finish_at_print_left&&visit+1U==visits;
        if(print_range)result=printer_waste_seek_from_print_left();
        else if(finish_at_print_left)result=printer_waste_seek_for_page();
        else result=printer_waste_seek(PRINTER_OC3_PREP_SEARCH_STEPS);
        if(result)goto stop;
        const uint64_t flash_start=bk_aon_rtc_get_us();
        for(;sent<end;){
            unsigned chunk=end-sent;if(chunk>PRINTER_PRESPRAY_ROW_ROUNDS)chunk=PRINTER_PRESPRAY_ROW_ROUNDS;
            if(chunk!=prepared_rounds){
                result=printer_prespray_build(row,PRINTER_NC_BYTES,chunk,&cfg);
                if(result)goto stop;
                prepared_rounds=chunk;
            }
            result=printer_waste_fire_prespray(row);if(result)goto stop;
            printer_prespray_note_ink();sent+=chunk;++batches;
            os_printf("[PRESPRAY_BATCH] reason=%s completed_rounds=%u/%u cache_rows=%u\r\n",reason,sent,rounds,batches);
            /* HC32 has shut the head OFF before returning line DONE. */
            if(sent<end)rtos_delay_milliseconds(PL_PRESPRAY_PAUSE_MS);
        }
        if(tph_mcu_set_head_power(0)!=BK_OK){result=-8;goto stop;}
        const unsigned long flash_ms=(unsigned long)((bk_aon_rtc_get_us()-flash_start)/1000U);
        s_waste_station_ready=0U;
        result=print_range?printer_waste_return_to_print_left():printer_waste_park_for_preparation();
        if(result)goto stop;
        os_printf("[PRESPRAY_VISIT_DONE] visit=%u/%u rounds=%u/%u visit_rounds=%u flash_ms=%lu parked=%s\r\n",
                  visit+1U,visits,sent,rounds,dose,flash_ms,print_range?"PRINT_LEFT":"SEAL");
    }
    s_prespray_boot_pending_page=!finish_at_print_left;
    os_printf("[PRESPRAY_DONE] reason=%s stationary_cache_rows=%u parked=%s no_paper_feed recovery_unverified\r\n",
              reason,batches,finish_at_print_left?"PRINT_LEFT":"SEAL");
    goto done;
stop:
    s_waste_station_ready=0U;
    printer_bidirectional_abort();
    os_printf("[PRESPRAY_STOP] reason=%s code=%d no_retry print_blocked\r\n",reason,result);
done:
    printer_map_psram_free(row);return result;
}
#endif

#ifndef PRINTER_WASTE_CLEAN_JOB_H
#define PRINTER_WASTE_CLEAN_JOB_H
#include "printer_waste_motion.h"
static int printer_run_waste_clean(void){
 s_waste_station_ready=0U;
 if(!printer_link_is_ready()||!s_bidi_known||!s_bidi_at_wiper||s_bidi_right)return -7;
 printer_map_config_t cfg;printer_map_default_config(&cfg);
 cfg.x_pitch_centi_um=PRINTER_MAP_X_PITCH_CENTI_UM;cfg.x_direction=PRINTER_MAP_X_DIRECTION;
 cfg.first_nozzle=PRINTER_MAP_FIRST_NOZZLE;cfg.reverse_y=PRINTER_MAP_REVERSE_Y;
 if(printer_nc_validate(&cfg))return -4;
 uint8_t *row=printer_map_psram_alloc(PRINTER_NC_BYTES);if(!row)return -2;
 int result=0;unsigned completed=0;
 for(unsigned ch=0;ch<3U;++ch)for(unsigned group=0;group<4U;++group){
  result=printer_maint_build(row,PRINTER_NC_BYTES,PRINTER_MAINT_CLEAN,ch,group,1U,&cfg);
  if(result)goto done;
  printer_map_yield_cpu();
 }
 os_printf("[WASTE_BEGIN] colors=CMY nozzles=600 dots_each=32 batches=12 total_requests=19200 paper_feed=0 normal_drive\r\n");
 result=printer_waste_position();if(result)goto stop;
 for(unsigned ch=0;ch<3U;++ch)for(unsigned group=0;group<4U;++group){
  result=printer_maint_build(row,PRINTER_NC_BYTES,PRINTER_MAINT_CLEAN,ch,group,1U,&cfg);if(result)goto stop;
  os_printf("[WASTE_PASS] color=%c group=%u/4 dots_each=32 crc32=%08lX\r\n","CMY"[ch],group+1U,(unsigned long)printer_crc32(row,PRINTER_NC_BYTES));
  result=printer_waste_fire(row);if(result)goto stop;
  ++completed;printer_map_yield_cpu();
 }
 if(tph_mcu_set_head_power(0)!=BK_OK){result=-8;goto stop;}
 s_waste_station_ready=0U;
 result=printer_buffered_park_at_wiper();if(result)goto stop;
#if PRINTER_STARTUP_WIPE_STEPS > 0
 result=printer_buffered_wipe(PRINTER_STARTUP_WIPE_STEPS,PRINTER_STARTUP_WIPE_ROUNDS);if(result)goto stop;
#endif
 result=printer_bidirectional_finish();if(result)goto stop;
 os_printf("[WASTE_DONE] batches=%u/12 nozzle_requests=19200 paper_feed=0 head_OFF parked=1 recovery_unverified\r\n",completed);
 goto done;
stop:
 s_waste_station_ready=0U;
 if(result==PRINTER_OC3_RETURNED&&s_bidi_known&&s_bidi_at_wiper)(void)printer_bidirectional_finish();
 else printer_bidirectional_abort();
 os_printf("[WASTE_STOP] code=%d completed=%u no_retry_no_further_ink\r\n",result,completed);
done:
 s_waste_station_ready=0U;printer_map_psram_free(row);return result;
}
#endif

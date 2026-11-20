#ifndef PRINTER_MAINTENANCE_JOB_H
#define PRINTER_MAINTENANCE_JOB_H
#include "printer_maintenance_pattern.h"
/* Runs only in the existing worker. Paper must be loaded by the operator:
 * the current status field is not a calibrated paper-presence interlock. */
static int printer_run_maintenance(uint8_t mode){
 if(mode>PRINTER_MAINT_CLEAN)return -19;
 if(!printer_link_is_ready()||!s_bidi_known||s_bidi_right||!s_bidi_at_wiper)return -7;
 printer_map_config_t cfg;printer_map_default_config(&cfg);
 cfg.x_pitch_centi_um=PRINTER_MAP_X_PITCH_CENTI_UM;cfg.x_direction=PRINTER_MAP_X_DIRECTION;
 cfg.first_nozzle=PRINTER_MAP_FIRST_NOZZLE;cfg.reverse_y=PRINTER_MAP_REVERSE_Y;
 if(printer_nc_validate(&cfg))return -4;
 uint32_t feed_plan[2];
 for(unsigned ch=0;ch<2U;++ch){
  feed_plan[ch]=(uint32_t)(((uint64_t)(ch+1U)*200U*PRINTER_PAGE_FEED_STEPS)/192U-((uint64_t)ch*200U*PRINTER_PAGE_FEED_STEPS)/192U);
  if(!feed_plan[ch]||feed_plan[ch]>5000U)return -4;
 }
 uint8_t *row=printer_map_psram_alloc(PRINTER_NC_BYTES);if(!row)return -2;
 int result=0;unsigned scans=0,feeds=0;
 unsigned phases=mode==PRINTER_MAINT_CLEAN?3U:1U;
 /* Validate every route and dose before power or motion. */
 for(unsigned ch=0;ch<3U;++ch)for(unsigned group=0;group<4U;++group)for(unsigned phase=0;phase<phases;++phase){
  result=printer_maint_build(row,PRINTER_NC_BYTES,mode,ch,group,phase,&cfg);
  if(result)goto done;
  printer_map_yield_cpu();
 }
 os_printf("[MAINT_BEGIN] mode=%s paper_required=1 colors=CMY nozzles_each=200 phases=%u scans=%u normal_drive nonpaper_ink=0\r\n",mode?"PAPER_CLEAN":"NOZZLE_CHECK",phases,12U*phases);
 result=printer_startup_prepare(1U);if(result)goto stop;
 for(unsigned ch=0;ch<3U;++ch){
  for(unsigned group=0;group<4U;++group)for(unsigned phase=0;phase<phases;++phase){
   result=printer_maint_build(row,PRINTER_NC_BYTES,mode,ch,group,phase,&cfg);if(result)goto stop;
   uint8_t direction=s_bidi_right?0U:1U;
   if(!direction)printer_maint_reverse(row);
   os_printf("[MAINT_PASS] color=%c group=%u/4 phase=%s dots_each=%u direction=%s crc32=%08lX\r\n","CMY"[ch],group+1U,mode?(phase==0?"BEFORE":phase==1?"RECOVER":"AFTER"):"CHECK",mode?printer_maint_dots(phase):16U,direction?"RIGHT":"LEFT",(unsigned long)printer_crc32(row,PRINTER_NC_BYTES));
   result=printer_execute_bidirectional_line(row,direction);if(result)goto stop;
   ++scans;printer_map_yield_cpu();
  }
  if(ch<2U){
   uint32_t sent=0;
   /* Cumulative floor for a200-row color band; each increment<=5000. */
   uint32_t steps=feed_plan[ch];
   if(tph_mcu_set_head_power(0)!=BK_OK){result=-8;goto stop;}
   int feed=printer_bidirectional_feed(steps,PRINTER_PAGE_FEED_DIRECTION,&sent);
   os_printf("[MAINT_FEED] color=%c pulses=%lu/%lu code=%d\r\n","CMY"[ch],(unsigned long)sent,(unsigned long)steps,feed);
   if(feed!=BK_OK||sent!=steps){result=-10;goto stop;}
   ++feeds;
  }
 }
 if(tph_mcu_set_head_power(0)!=BK_OK){result=-8;goto stop;}
 result=printer_buffered_park_at_wiper();if(result)goto stop;
#if PRINTER_STARTUP_WIPE_STEPS > 0
 if(mode==PRINTER_MAINT_CLEAN){result=printer_buffered_wipe(PRINTER_STARTUP_WIPE_STEPS,PRINTER_STARTUP_WIPE_ROUNDS);if(result)goto stop;}
#endif
 result=printer_bidirectional_finish();if(result)goto stop;
 os_printf("[MAINT_DONE] mode=%u scans=%u feeds=%u planned_nozzle_requests=%u head_OFF parked=1 ink_recovery_unverified\r\n",mode,scans,feeds,mode?38400U:9600U);
 goto done;
stop:
 /* Never retry a partially fired scan or continue feeding after a fault. */
 if(result==PRINTER_OC3_RETURNED&&s_bidi_known&&s_bidi_at_wiper)(void)printer_bidirectional_finish();
 else printer_bidirectional_abort();
 os_printf("[MAINT_STOP] mode=%u code=%d scans=%u no_further_ink_or_feed\r\n",mode,result,scans);
done:
 printer_map_psram_free(row);return result;
}
#endif

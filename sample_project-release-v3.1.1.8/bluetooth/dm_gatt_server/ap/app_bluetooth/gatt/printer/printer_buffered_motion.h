#ifndef PRINTER_BUFFERED_MOTION_H
#define PRINTER_BUFFERED_MOTION_H
#include <driver/timer.h>
#include "printer_canvas_geometry.h"
#define PL_MOTOR_TIMER TIMER_ID4
#define PL_SERVICE_STEP_US 82U
/* V32: shorten the measured slow endpoint phase; keep cruise and ramps. */
#define PL_START_PERIOD_US 400U
#define PL_RAMP_STEPS 64U
#define PRINTER_OC3_RETURNED (-15)
#define PRINTER_OC3_STUCK (-16)
#define PRINTER_OC3_NOT_FOUND (-18)
#define PRINTER_OC3_HIGH_CONFIRM_US 2000U
/* RIGHT may be the optical service endpoint or the inset print endpoint. */
static int32_t s_bidi_right_offset;
static uint32_t s_bidi_full_stroke;
static uint32_t printer_buffered_print_span(void){
 return s_bidi_full_stroke?s_bidi_full_stroke-PRINTER_PRINT_SHORTER_STEPS-PRINTER_PAD_PRINT_RETREAT_STEPS:
      PRINTER_SERVICE_SPAN_STEPS;
}
static uint32_t printer_buffered_optical_inset(void){
 return s_bidi_full_stroke?PRINTER_PAD_PRINT_RETREAT_STEPS:PRINTER_PRINT_ENDPOINT_INSET_STEPS;
}
static uint32_t printer_buffered_full_return(void){
 return s_bidi_full_stroke?s_bidi_full_stroke:PRINTER_OC3_FINAL_RETURN_STEPS;
}
static int32_t printer_buffered_seal_offset(void){
 return (int32_t)(printer_buffered_print_span()+printer_buffered_optical_inset())-
      (int32_t)printer_buffered_full_return();
}
static uint32_t s_oc3_reposition_return=PRINTER_OC3_FINAL_RETURN_STEPS;
static volatile uint32_t s_oc3_hit;
static volatile unsigned s_oc3_returning;
static int printer_buffered_oc3_return(uint32_t return_steps);
static int printer_buffered_return_from_reference(uint32_t return_steps,uint32_t inset,uint32_t cruise_step_us);
static int printer_buffered_reposition(uint8_t direction,uint32_t steps);
static int printer_buffered_leave_wiper(void);
static int printer_buffered_park_at_wiper(void);
static volatile uint32_t s_buf_active,s_buf_steps,s_buf_sync,s_buf_error,s_buf_last_irq,s_buf_max_gap,s_buf_cruise_max_gap,s_buf_period,s_buf_target,s_buf_lead;
static volatile uint8_t s_buf_direction;
static uint8_t s_buf_ink,s_buf_claimed;
static uint32_t s_buf_cruise_step_us;
static uint32_t s_buf_short_step_us;
static volatile bool s_buf_past_oc3;
static bool s_buf_seek_oc3_release;
static uint32_t s_buf_oc3_release_limit;
static volatile uint32_t s_buf_oc3_extra,s_buf_oc3_min,s_buf_oc3_max,s_buf_oc3_reference;
static volatile uint8_t s_buf_oc3_high_samples;
static uint32_t s_buf_oc3_high_since;
static volatile uint32_t s_buf_sync_us;
static volatile uint64_t s_buf_endpoint_us;
static volatile uint8_t s_buf_endpoint_direction,s_buf_endpoint_seen;
/* GPIO24 can interrupt a high pulse, and the RTC rounds to a tick. */
#define PRINTER_TURN_HOLD_US 1000064U
static void printer_buffered_record_endpoint(uint64_t now){
 s_buf_endpoint_us=now;
 s_buf_endpoint_direction=s_buf_direction;
 s_buf_endpoint_seen=1U;
}
static int printer_buffered_set_direction(uint8_t direction){
 if(s_buf_endpoint_seen&&s_buf_endpoint_direction!=direction){
  unsigned polls=0U;
  while(bk_aon_rtc_get_us()-s_buf_endpoint_us<PRINTER_TURN_HOLD_US){
   if(++polls>2000U)return -14;
   rtos_delay_milliseconds(1U);
  }
 }
 if(direction)GPIO_UP(TPH_MOT_DIR);else GPIO_DOWN(TPH_MOT_DIR);
 return 0;
}
static uint32_t printer_buffered_ramp_steps(void){return s_buf_ink?PRINTER_CANVAS_RAMP_STEPS:PL_RAMP_STEPS;}
static uint32_t printer_buffered_period(uint32_t i){
 uint32_t ramp=printer_buffered_ramp_steps();
 uint32_t cruise=s_buf_cruise_step_us;
 uint32_t edge=i<s_buf_target-1U-i?i:s_buf_target-1U-i;
 return edge<ramp?PL_START_PERIOD_US-(PL_START_PERIOD_US-cruise)*edge/ramp:cruise;
}
static void printer_buffered_stop(void){
 s_buf_active=0U;
 if(s_buf_claimed){(void)bk_timer_stop(PL_MOTOR_TIMER);(void)bk_timer_cancel(PL_MOTOR_TIMER);s_buf_claimed=0U;}
 bk_gpio_set_output_high(PIN_BK_SYNC_OUT);
}
/* GPIO ISR: stop future STEP edges and tell HC32 to stop firing. Direction
 * reversal and SPI power-OFF acknowledgement run later in task context. */
static void printer_buffered_oc3_stop(void){
 if(!s_buf_active||!s_buf_direction||s_oc3_returning||s_buf_past_oc3)return;
 if(s_buf_oc3_extra){s_oc3_hit=1U;return;}
 printer_buffered_record_endpoint(bk_aon_rtc_get_us());
 s_oc3_hit=1U;s_buf_error=3U;s_buf_active=0U;
 (void)bk_timer_stop(PL_MOTOR_TIMER);
 bk_gpio_set_output_high(PIN_BK_SYNC_OUT);
}
static void printer_buffered_tick(timer_id_t id){
 if(id!=PL_MOTOR_TIMER||!s_buf_active)return;
 const uint32_t now=(uint32_t)bk_aon_rtc_get_us();
 if(!s_buf_direction&&s_buf_seek_oc3_release&&!motor_oc3_active()){
  printer_buffered_record_endpoint(bk_aon_rtc_get_us());
  s_buf_active=0U;(void)bk_timer_stop(id);return;
 }
 if(!s_buf_direction&&s_buf_oc3_release_limit&&s_buf_steps>=s_buf_oc3_release_limit&&motor_oc3_active()){
  s_buf_error=4U;s_buf_active=0U;(void)bk_timer_stop(id);return;
 }
 bool hold_for_oc3=false;
 if(s_buf_oc3_extra){
  const bool optical_low=motor_oc3_active();
  if(s_buf_oc3_reference||(s_oc3_hit||optical_low)){
   if(!s_buf_oc3_reference&&(s_buf_steps<s_buf_oc3_min||s_buf_steps>s_buf_oc3_max)){
    s_buf_error=4U;s_buf_active=0U;(void)bk_timer_stop(id);return;
   }
   if(!optical_low){
    /* Hold at the current step while confirming a brief GPIO24 release. */
    if(s_buf_oc3_high_samples++==0U)s_buf_oc3_high_since=now;
    if(now-s_buf_oc3_high_since>=PRINTER_OC3_HIGH_CONFIRM_US){
     s_buf_error=4U;s_buf_active=0U;(void)bk_timer_stop(id);return;
    }
    hold_for_oc3=true;
   }else{
    s_buf_oc3_high_samples=0U;
    if(!s_buf_oc3_reference){
     /* Count completed pulses, including one interrupted by the GPIO edge. */
     s_buf_oc3_reference=s_buf_steps;
     s_buf_target=s_buf_steps+s_buf_oc3_extra;
    }
   }
  }
 }else{
  if(s_buf_past_oc3&&!motor_oc3_active()){
   s_buf_error=4U;s_buf_active=0U;(void)bk_timer_stop(id);return;
  }
  if(!s_buf_past_oc3&&s_buf_direction&&motor_oc3_active()){printer_buffered_oc3_stop();return;}
 }
 uint32_t gap=now-s_buf_last_irq;
 if(s_buf_steps&&gap>s_buf_period*2U+96U){s_buf_error=1U;s_buf_active=0U;(void)bk_timer_stop(id);return;}
 if(gap>s_buf_max_gap)s_buf_max_gap=gap;
 if(s_buf_steps>printer_buffered_ramp_steps()+2U&&s_buf_target>2U*printer_buffered_ramp_steps()&&s_buf_steps<s_buf_target-printer_buffered_ramp_steps()&&gap>s_buf_cruise_max_gap)s_buf_cruise_max_gap=gap;
 s_buf_last_irq=now;
 if(hold_for_oc3)return;
 if(s_buf_ink&&s_buf_steps==s_buf_lead){s_buf_sync_us=now;s_buf_sync=1U;bk_gpio_set_output_low(PIN_BK_SYNC_OUT);}
 if(!s_buf_active)return;
 GPIO_UP(TPH_MOT_STEP);delay_us(PRINTER_CANVAS_STEP_HIGH_US);GPIO_DOWN(TPH_MOT_STEP);
 ++s_buf_steps;++s_blank_spi_steps[s_buf_direction?1:0];
 if(!s_buf_active)return; /* OC3 may interrupt an already-started STEP high pulse */
 if(s_buf_steps==s_buf_target){
  if(s_buf_oc3_extra&&!s_buf_oc3_reference&&(s_oc3_hit||motor_oc3_active()))
   return; /* Resolve an edge on the last search pulse at the next tick. */
  if(s_buf_short_step_us)s_buf_endpoint_seen=0U;
  else printer_buffered_record_endpoint(bk_aon_rtc_get_us());
  s_buf_active=0U;(void)bk_timer_stop(id);return;
 }
 uint32_t next=printer_buffered_period(s_buf_steps);
 if(next!=s_buf_period){if(bk_timer_set_period_us(id,next)!=BK_OK){s_buf_error=2U;s_buf_active=0U;(void)bk_timer_stop(id);}else s_buf_period=next;}
}
static int printer_buffered_claim(void){
 if(!motor_oc3_ready()){os_printf("[OC3_BLOCKED] GPIO24_not_initialized no_motion\r\n");return -17;}
 motor_oc3_set_handler(printer_buffered_oc3_stop);
 if(s_buf_claimed)return 0;
 if(bk_timer_driver_init()!=BK_OK){os_printf("[BUFFERED_TIMER] init_failed no_motion\r\n");return -14;}
 uint32_t enabled=bk_timer_get_enable_status(),period=bk_timer_get_period(PL_MOTOR_TIMER);
 if((enabled&(1U<<PL_MOTOR_TIMER))||period!=0U){os_printf("[BUFFERED_TIMER] id=4 unavailable enable_mask=%lu period=%lu no_motion\r\n",(unsigned long)enabled,(unsigned long)period);return -14;}
 s_buf_claimed=1U;return 0;
}
/* This function performs ALL SPI operations while the timer is stopped. */
static int printer_execute_buffered_line_at_speed(const uint8_t*columns,uint8_t direction,uint32_t cruise_step_us){
 tph_line_ticket_t ticket={0};
 int result=0,armed=0;
 if(direction>1U)return -1;
 if(!s_bidi_known||direction!=(s_bidi_right?0U:1U))return -7;
 if(s_bidi_at_wiper){
  if(!direction)return -7;
  result=printer_buffered_leave_wiper();if(result)return result;
 }
 if(!direction&&s_bidi_right_offset){
  int32_t offset=s_bidi_right_offset;
  result=printer_buffered_reposition(0U,(uint32_t)offset);
  if(result)return result;
  s_bidi_right_offset=0;
 }
 if(s_buf_active)return -14;
 s_oc3_hit=0U;s_buf_steps=0;s_buf_sync=0;s_buf_error=0;s_buf_max_gap=0;s_buf_cruise_max_gap=0;
 if(!s_bidi_power&&(tph_mcu_set_head_power(0)!=BK_OK||tph_line_probe()!=BK_OK))return -13;
 if(printer_buffered_claim())return -14;
 s_blank_spi_columns=0;s_blank_spi_steps[0]=s_blank_spi_steps[1]=0;
 uint64_t prepare_start=bk_aon_rtc_get_us();
 if(printer_bidi_power_up()!=0){result=-8;goto stop;}
 result=printer_buffered_set_direction(direction);if(result)goto stop;
 if(columns){
  if(tph_line_prepare(columns,PRINTER_CANVAS_COLUMN_US,&ticket)!=BK_OK){result=-13;goto stop;}
  armed=1;
 }
 uint32_t prepare_us=(uint32_t)(bk_aon_rtc_get_us()-prepare_start);
 delay_us(50U);
 s_buf_direction=direction;s_buf_ink=columns?1U:0U;s_oc3_hit=0U;s_buf_steps=0;s_buf_sync=0;s_buf_error=0;s_buf_max_gap=0;s_buf_cruise_max_gap=0;
 s_buf_cruise_step_us=cruise_step_us;
 s_buf_target=printer_buffered_print_span();
 s_buf_lead=(s_buf_target*PRINTER_CANVAS_STEP_US-
             (PRINTER_CANVAS_COLUMNS-1U)*PRINTER_CANVAS_COLUMN_US)/(2U*PRINTER_CANVAS_STEP_US);
 s_buf_period=printer_buffered_period(0U);s_buf_last_irq=(uint32_t)bk_aon_rtc_get_us();
 __sync_synchronize();
 uint32_t begin=s_buf_last_irq;unsigned saw_low=0;
 s_buf_active=1U;
 if(bk_timer_start_us(PL_MOTOR_TIMER,s_buf_period,printer_buffered_tick)!=BK_OK){s_buf_active=0;result=-14;goto stop;}
 while(s_buf_active){
  if(columns&&s_buf_sync){
   if(!bk_gpio_get_input(PIN_BK_READY_IN))saw_low=1U;
   else if(saw_low){result=-13;break;} /* HC32 early fault, not normal DONE */
   if(!saw_low&&(uint32_t)(bk_aon_rtc_get_us()-s_buf_sync_us)>10000U){result=-13;break;}
  }
  if((uint32_t)(bk_aon_rtc_get_us()-begin)>5000000U){result=-14;break;}
  rtos_delay_milliseconds(1U); /* STEP runs in the independent timer IRQ */
 }
 (void)bk_timer_stop(PL_MOTOR_TIMER);
 s_buf_active=0U;
 bk_gpio_set_output_high(PIN_BK_SYNC_OUT); /* release or abort HC32 line */
 unsigned optical_end=direction&&(s_oc3_hit||motor_oc3_active());
 if(optical_end&&s_bidi_full_stroke){
  os_printf("[OC3_PRINT_ENDPOINT_MISMATCH] gpio24_hit=1 steps=%lu expected=%lu\r\n",
            (unsigned long)s_buf_steps,(unsigned long)s_buf_target);
  result=PRINTER_OC3_NOT_FOUND;goto stop;
 }
 if((s_buf_error&&!optical_end)||(!optical_end&&s_buf_steps!=s_buf_target)){result=-14;goto stop;}
 if(columns){
  if(!saw_low||tph_line_finish(&ticket)!=BK_OK){
   if(optical_end)os_printf("[OC3_PRINT_WINDOW] data_not_complete_at_endpoint calibration_required\r\n");
   result=-13;goto stop;
  }
  s_blank_spi_columns=PL_COLUMNS;armed=0;
 }
 if(!direction&&motor_oc3_active()){s_bidi_known=false;result=PRINTER_OC3_STUCK;goto stop;}
 s_bidi_right=direction?true:false;
 s_bidi_right_offset=optical_end?(int32_t)printer_buffered_optical_inset():0;
 if(!direction)s_bidi_left_offset=0;
 s_bidi_at_wiper=!direction&&s_bidi_left_offset==printer_buffered_seal_offset();
 if(optical_end){s_oc3_hit=0U;s_buf_error=0U;os_printf("[OC3_ENDPOINT] gpio=24 print_row_complete next=LEFT no_page_failure\r\n");}
 os_printf("[BUFFERED_SCAN] direction=%s blank=%u code=0 pulses=%lu columns=%lu prepare_us=%lu motion_us=%lu cruise_step_us=%u step_high_us=%u max_irq_gap_us=%lu cruise_max_gap_us=%lu one_start_SYNC ink_position_unverified\r\n",direction?"RIGHT":"LEFT",columns?0U:1U,(unsigned long)s_buf_steps,(unsigned long)s_blank_spi_columns,(unsigned long)prepare_us,(unsigned long)(bk_aon_rtc_get_us()-begin),(unsigned)cruise_step_us,PRINTER_CANVAS_STEP_HIGH_US,(unsigned long)s_buf_max_gap,(unsigned long)s_buf_cruise_max_gap);
 return 0;
stop:
 printer_buffered_stop();
 if(armed&&!s_buf_sync)(void)tph_line_cancel(&ticket);
 if(s_oc3_hit)return printer_buffered_oc3_return(printer_buffered_full_return());
 printer_bidirectional_abort();
 os_printf("[BUFFERED_STOP] code=%d irq_error=%lu pulses=%lu no_blind_return\r\n",result,(unsigned long)s_buf_error,(unsigned long)s_buf_steps);
 return result;
}
static int printer_execute_buffered_line(const uint8_t*columns,uint8_t direction){
 return printer_execute_buffered_line_at_speed(columns,direction,columns?PRINTER_CANVAS_STEP_US:PRINTER_CANVAS_BLANK_STEP_US);
}
/* Head-OFF positioning between maintenance and print anchors. Never changes
 * the image stroke or its first-column timing. No SPI while the timer runs. */
static int printer_buffered_reposition_at_speed(uint8_t direction,uint32_t steps,uint32_t cruise_step_us){
 uint32_t limit=PRINTER_OC3_TRAVEL_LIMIT_STEPS;
 if((direction&&s_oc3_reposition_return==PRINTER_OC3_BOOT_RETURN_STEPS)||
    (!direction&&s_oc3_returning))
  limit=PRINTER_OC3_PREP_SEARCH_STEPS;
 if(!direction&&s_oc3_returning&&s_bidi_full_stroke==PRINTER_NEW_SEAL_TO_OC3_STEPS&&
    (steps==PRINTER_NEW_FULL_STROKE_STEPS||
     steps==PRINTER_NEW_SEAL_TO_OC3_STEPS+PRINTER_PAGE_PAD_ADVANCE_STEPS))
  limit=steps;
 if(!s_bidi_known||s_buf_active||direction>1U||!steps||steps>limit)return -7;
 if(tph_mcu_set_head_power(0)!=BK_OK)return -8;
 if(printer_buffered_claim())return -14;
 if(printer_bidi_power_up()!=0)return -8;
 if(printer_buffered_set_direction(direction))goto failed;
 delay_us(50U);
 s_buf_target=steps;s_buf_direction=direction;s_buf_ink=0;s_buf_sync=0;
 s_buf_cruise_step_us=cruise_step_us;
 s_oc3_hit=0U;s_buf_steps=0;s_buf_error=0;s_buf_max_gap=0;s_buf_cruise_max_gap=0;
 s_buf_period=printer_buffered_period(0U);s_buf_last_irq=(uint32_t)bk_aon_rtc_get_us();
 uint32_t begin=s_buf_last_irq;s_bidi_known=false;
 __sync_synchronize();s_buf_active=1U;
 if(bk_timer_start_us(PL_MOTOR_TIMER,s_buf_period,printer_buffered_tick)!=BK_OK)goto failed;
 while(s_buf_active){
  if((uint32_t)(bk_aon_rtc_get_us()-begin)>5000000U)goto failed;
  rtos_delay_milliseconds(1U);
 }
 (void)bk_timer_stop(PL_MOTOR_TIMER);
 if(direction&&!s_buf_past_oc3&&motor_oc3_active()){s_oc3_hit=1U;goto failed;}
 if(s_buf_error||(s_buf_steps!=steps&&!(s_buf_seek_oc3_release&&!motor_oc3_active())))goto failed;
 s_bidi_known=true;return 0;
failed:
 if(s_oc3_hit&&!s_oc3_returning)
  return printer_buffered_return_from_reference(s_oc3_reposition_return,printer_buffered_optical_inset(),cruise_step_us);
 printer_buffered_stop();printer_bidirectional_abort();return -14;
}
static int printer_buffered_reposition(uint8_t direction,uint32_t steps){
 return printer_buffered_reposition_at_speed(direction,steps,PL_SERVICE_STEP_US);
}
/* Only callable from task context after an optical seek. No ink or SYNC. */
static inline int printer_buffered_advance_past_oc3(void){
 if(!motor_oc3_active()||s_buf_active||s_buf_claimed||!s_bidi_known||!s_bidi_right)
  return -7;
 s_buf_past_oc3=true;
 int result=printer_buffered_reposition(1U,PRINTER_PAD_ADVANCE_STEPS);
 s_buf_past_oc3=false;
 printer_buffered_stop();
 if(!result&&!motor_oc3_active())return PRINTER_OC3_NOT_FOUND;
 return result;
}
static int printer_buffered_leave_wiper(void){
 if(!s_bidi_known)return -7;
 if(!s_bidi_at_wiper)return 0;
 int32_t offset=s_bidi_left_offset;
 s_oc3_reposition_return=printer_buffered_full_return();
 if(offset){
  int result=printer_buffered_reposition(offset<0?1U:0U,(uint32_t)(offset<0?-offset:offset));
  if(result)return result;
 }
 s_bidi_left_offset=0;s_bidi_at_wiper=false;s_bidi_right=false;
 os_printf("[PRINT_ANCHOR] endpoint=PRINT_LEFT from_phase_offset=%ld head_OFF\r\n",(long)offset);
 return 0;
}
static int printer_buffered_park_at_wiper_at_speed(uint32_t full_step_us){
 if(!s_bidi_known)return -7;
 const int32_t target=printer_buffered_seal_offset();
 if(s_bidi_at_wiper&&!s_bidi_right&&s_bidi_left_offset==target)return 0;
 int32_t current=s_bidi_right?(int32_t)printer_buffered_print_span()+s_bidi_right_offset:s_bidi_left_offset;
 int32_t delta=current-target;
 uint32_t steps=(uint32_t)(delta>0?delta:-delta);
 uint32_t cruise_step_us=steps>=printer_buffered_print_span()?full_step_us:PL_SERVICE_STEP_US;
 s_oc3_reposition_return=printer_buffered_full_return();
 if(delta){
  int result=printer_buffered_reposition_at_speed(delta>0?0U:1U,steps,cruise_step_us);
  if(result)return result;
 }
 s_bidi_left_offset=target;s_bidi_at_wiper=true;s_bidi_right=false;
 if(motor_oc3_active()){s_bidi_known=false;printer_bidirectional_abort();return PRINTER_OC3_STUCK;}
 os_printf("[BIDI_PARK] endpoint=SEAL pulses=%lu return_from_OC3=%u head_OFF counted_position_only cruise_step_us=%u\r\n",(unsigned long)steps,(unsigned)printer_buffered_full_return(),(unsigned)cruise_step_us);
 return 0;
}
static int printer_buffered_park_at_wiper(void){
 return printer_buffered_park_at_wiper_at_speed(PL_SERVICE_STEP_US);
}
static int printer_buffered_boot_round_trip(void){
 if(!s_bidi_known||!s_bidi_at_wiper||s_bidi_right)return -7;
 s_oc3_reposition_return=PRINTER_OC3_BOOT_RETURN_STEPS;
 int result=printer_buffered_reposition_at_speed(1U,PRINTER_OC3_BOOT_SEARCH_STEPS,PRINTER_FULL_TRAVEL_STEP_US);
 s_oc3_reposition_return=printer_buffered_full_return();
 if(result==PRINTER_OC3_RETURNED&&s_bidi_known&&s_bidi_at_wiper)return 0;
 if(result)return result;
 os_printf("[OC3_NOT_FOUND] stage=BOOT search_cap=%u gpio24=%u stopped_no_blind_return\r\n",PRINTER_OC3_BOOT_SEARCH_STEPS,(unsigned)!motor_oc3_active());
 printer_bidirectional_abort();return PRINTER_OC3_NOT_FOUND;
}
/* Head-OFF short round trips within the counted full stroke. */
static int printer_buffered_short_round_trips(uint32_t excursion,uint32_t rounds,uint32_t cruise_step_us){
 if(!s_bidi_known||!s_bidi_at_wiper||s_bidi_right||s_buf_active||!excursion||excursion>=PRINTER_MAINTENANCE_SPAN_STEPS||!rounds||rounds>2U)return -7;
 if(tph_mcu_set_head_power(0)!=BK_OK)return -8;
 if(printer_buffered_claim())return -14;
 if(printer_bidi_power_up()!=0)return -8;
 s_buf_short_step_us=cruise_step_us;
 s_bidi_known=false; /* do not expose the short-leg turnaround as an endpoint */
 for(uint32_t leg=0;leg<rounds*2U;++leg){
  uint32_t direction=(leg&1U)?0U:1U;
  if(printer_buffered_set_direction((uint8_t)direction))goto failed;
  delay_us(50U);
  s_buf_target=excursion;s_buf_direction=direction;s_buf_ink=0;s_buf_sync=0;
  s_buf_cruise_step_us=cruise_step_us;
  s_oc3_hit=0U;s_buf_steps=0;s_buf_error=0;s_buf_max_gap=0;s_buf_cruise_max_gap=0;
  s_buf_period=printer_buffered_period(0U);s_buf_last_irq=(uint32_t)bk_aon_rtc_get_us();
  uint32_t begin=s_buf_last_irq;
  __sync_synchronize();s_buf_active=1U;
  if(bk_timer_start_us(PL_MOTOR_TIMER,s_buf_period,printer_buffered_tick)!=BK_OK)goto failed;
  while(s_buf_active){
   if((uint32_t)(bk_aon_rtc_get_us()-begin)>2000000U)goto failed;
   rtos_delay_milliseconds(1U);
  }
  (void)bk_timer_stop(PL_MOTOR_TIMER);
  if(direction&&!s_buf_past_oc3&&motor_oc3_active()){s_oc3_hit=1U;goto failed;}
  if(s_buf_error||s_buf_steps!=excursion)goto failed;
  os_printf("[WIPER_LEG] direction=%s pulses=%lu cruise_step_us=%u head_OFF contact_unverified\r\n",direction?"AWAY_FROM_LEFT":"BACK_TO_LEFT",(unsigned long)s_buf_steps,(unsigned)cruise_step_us);
 }
 s_buf_short_step_us=0U;
 s_bidi_known=true;s_bidi_right=false;
 if(motor_oc3_active()){s_bidi_known=false;printer_bidirectional_abort();return PRINTER_OC3_STUCK;}
 return 0;
failed:
 printer_buffered_stop();s_buf_short_step_us=0U;
 if(s_oc3_hit&&!s_oc3_returning)return printer_buffered_oc3_return(printer_buffered_full_return());
 printer_bidirectional_abort();return -14;
}
static inline int printer_buffered_wipe(uint32_t excursion,uint32_t rounds){
 return printer_buffered_short_round_trips(excursion,rounds,PRINTER_WIPE_STEP_US);
}
#if !PRINTER_PRESPRAY
/* A normal optical endpoint halts U4 but does not cancel the current 1.8mm
 * U5 increment. The next LEFT leg follows as soon as that increment is done. */
static int printer_buffered_load_continue(void){return s_buf_error==0U||s_buf_error==3U;}
/* The carriage runs in TIMER4 while this task feeds paper. Image ARM/SYNC is
 * deliberately absent. Alternate direction from the current counted endpoint. */
static int printer_buffered_load_while_moving(uint32_t paper_steps,uint8_t paper_direction){
 if(!s_bidi_known||(!s_bidi_right&&!s_bidi_at_wiper)||s_buf_active||!paper_steps||paper_steps>5000U||paper_direction>1U)return -7;
 if(tph_mcu_set_head_power(0)!=BK_OK)return -8;
 if(printer_buffered_claim())return -14;
 if(printer_bidi_power_up()!=0)return -8;
 uint8_t carriage_direction=s_bidi_right?0U:1U;
 if(printer_buffered_set_direction(carriage_direction))return -14;
 delay_us(50U);
 s_buf_target=carriage_direction?PRINTER_OC3_PREP_SEARCH_STEPS:PRINTER_OC3_PREP_RETURN_STEPS;s_buf_direction=carriage_direction;s_buf_ink=0U;s_buf_sync=0U;
 s_buf_cruise_step_us=PL_SERVICE_STEP_US;
 s_oc3_hit=0U;s_buf_steps=0;s_buf_error=0;s_buf_max_gap=0;s_buf_cruise_max_gap=0;
 s_buf_period=printer_buffered_period(0U);
 os_printf("[PAGE_LOAD_MOVE_BEGIN] paper_steps=%lu carriage=%s head_OFF no_ARM_no_SYNC\r\n",(unsigned long)paper_steps,carriage_direction?"RIGHT":"LEFT");
 s_buf_last_irq=(uint32_t)bk_aon_rtc_get_us();
 uint32_t begin=s_buf_last_irq,sent=0;int result=-14;
 s_bidi_known=false;__sync_synchronize();s_buf_active=1U;
 if(bk_timer_start_us(PL_MOTOR_TIMER,s_buf_period,printer_buffered_tick)!=BK_OK)goto failed;
 int feed=motor_page_load_keep_power_guarded(paper_steps,paper_direction,&sent,printer_buffered_load_continue);
 if(feed!=BK_OK||sent!=paper_steps){result=s_buf_error?-14:-10;goto failed;}
 while(s_buf_active){
  if((uint32_t)(bk_aon_rtc_get_us()-begin)>8000000U)goto failed;
  rtos_delay_milliseconds(1U);
 }
 (void)bk_timer_stop(PL_MOTOR_TIMER);
 if(carriage_direction&&(s_oc3_hit||motor_oc3_active())){
  os_printf("[OC3_ENDPOINT] gpio=24 steps=%lu feed_complete=1 next=LEFT_fixed_%u\r\n",(unsigned long)s_buf_steps,PRINTER_OC3_PREP_RETURN_STEPS);
  s_oc3_hit=0U;s_buf_error=0U;
 }else if(carriage_direction){result=PRINTER_OC3_NOT_FOUND;goto failed;}
 else if(s_buf_error||s_buf_steps!=PRINTER_OC3_PREP_RETURN_STEPS)goto failed;
 s_bidi_known=true;s_bidi_right=carriage_direction?true:false;s_bidi_at_wiper=!s_bidi_right;
 s_bidi_right_offset=carriage_direction?(int32_t)PRINTER_PRINT_ENDPOINT_INSET_STEPS:0;
 if(!carriage_direction)s_bidi_left_offset=(int32_t)PRINTER_OC3_REFERENCE_STEPS-(int32_t)PRINTER_OC3_PREP_RETURN_STEPS;
 if(!carriage_direction&&motor_oc3_active()){result=PRINTER_OC3_STUCK;goto failed;}
 os_printf("[PAGE_PREP_LOAD] pulses=%lu/%lu code=0 parallel_carriage_steps=%lu endpoint=%s head_OFF distance_unverified\r\n",(unsigned long)sent,(unsigned long)paper_steps,(unsigned long)s_buf_steps,s_bidi_right?"RIGHT":"LEFT");
 return 0;
failed:
 if(s_oc3_hit&&!s_oc3_returning)return printer_buffered_oc3_return(PRINTER_OC3_PREP_RETURN_STEPS);
 os_printf("[PAGE_LOAD_MOVE_STOP] code=%d paper_pulses=%lu carriage_pulses=%lu irq_error=%lu no_blind_return\r\n",result,(unsigned long)sent,(unsigned long)s_buf_steps,(unsigned long)s_buf_error);
 printer_buffered_stop();printer_bidirectional_abort();return result;
}
#endif
/* GPIO24 LOW is a physical RIGHT reference. A fixed, head-OFF LEFT stroke
 * returns to the wiper even when the outward count was interrupted. */
static int printer_buffered_return_from_reference(uint32_t return_steps,uint32_t inset,uint32_t cruise_step_us){
 uint32_t stopped=s_buf_steps;
 printer_buffered_stop();
 if(tph_mcu_set_head_power(0)!=BK_OK){printer_bidirectional_abort();return -8;}
 s_oc3_returning=1U;s_oc3_hit=0U;
 s_bidi_known=true;s_bidi_right=true;s_bidi_right_offset=(int32_t)inset;s_bidi_at_wiper=false;
 os_printf("[OC3_RETURN_BEGIN] stopped_steps=%lu gpio=24 LOW reverse=LEFT fixed_steps=%u head_OFF\r\n",(unsigned long)stopped,return_steps);
 int result=printer_buffered_reposition_at_speed(0U,return_steps,cruise_step_us);
 if(!result){
  s_bidi_left_offset=(int32_t)(printer_buffered_print_span()+inset)-(int32_t)return_steps;
  s_bidi_right=false;s_bidi_right_offset=0;s_bidi_at_wiper=true;
 }
 s_oc3_returning=0U;
 if(!result&&motor_oc3_active()){s_bidi_known=false;result=PRINTER_OC3_STUCK;}
 if(result){printer_bidirectional_abort();return result;}
 os_printf("[OC3_RETURN_DONE] endpoint=SEAL steps=%u gpio24_released=1 cruise_step_us=%u\r\n",return_steps,cruise_step_us);
 return PRINTER_OC3_RETURNED;
}
static int printer_buffered_oc3_return(uint32_t return_steps){
 return printer_buffered_return_from_reference(return_steps,printer_buffered_optical_inset(),PL_SERVICE_STEP_US);
}
#endif

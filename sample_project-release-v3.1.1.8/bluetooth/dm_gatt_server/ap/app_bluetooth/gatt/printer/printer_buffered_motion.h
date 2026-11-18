#ifndef PRINTER_BUFFERED_MOTION_H
#define PRINTER_BUFFERED_MOTION_H
#include <driver/timer.h>
#include "printer_canvas_geometry.h"
#define PL_MOTOR_TIMER TIMER_ID4
#define PL_STEP_PERIOD_US PRINTER_CANVAS_STEP_US
#define PL_START_PERIOD_US 1000U
#define PL_RAMP_STEPS 400U
static volatile uint32_t s_buf_active,s_buf_steps,s_buf_sync,s_buf_error,s_buf_last_irq,s_buf_max_gap,s_buf_cruise_max_gap,s_buf_period,s_buf_target;
static uint8_t s_buf_direction,s_buf_ink,s_buf_claimed;
static volatile uint32_t s_buf_sync_us;
static uint32_t printer_buffered_period(uint32_t i){uint32_t edge=i<s_buf_target-1U-i?i:s_buf_target-1U-i;return edge<PL_RAMP_STEPS?PL_START_PERIOD_US-(PL_START_PERIOD_US-PL_STEP_PERIOD_US)*edge/PL_RAMP_STEPS:PL_STEP_PERIOD_US;}
static void printer_buffered_stop(void){
 s_buf_active=0U;
 if(s_buf_claimed){(void)bk_timer_stop(PL_MOTOR_TIMER);(void)bk_timer_cancel(PL_MOTOR_TIMER);s_buf_claimed=0U;}
 bk_gpio_set_output_high(PIN_BK_SYNC_OUT);
}
static void printer_buffered_tick(timer_id_t id){
 if(id!=PL_MOTOR_TIMER||!s_buf_active)return;
 uint32_t now=(uint32_t)bk_aon_rtc_get_us();
 uint32_t gap=now-s_buf_last_irq;
 if(s_buf_steps&&gap>s_buf_period*2U+96U){s_buf_error=1U;s_buf_active=0U;(void)bk_timer_stop(id);return;}
 if(gap>s_buf_max_gap)s_buf_max_gap=gap;
 if(s_buf_steps>PL_RAMP_STEPS+2U&&s_buf_target>2U*PL_RAMP_STEPS&&s_buf_steps<s_buf_target-PL_RAMP_STEPS&&gap>s_buf_cruise_max_gap)s_buf_cruise_max_gap=gap;
 s_buf_last_irq=now;
 uint32_t lead=PRINTER_CANVAS_LEAD_STEPS;
 if(s_buf_ink&&s_buf_steps==lead){s_buf_sync_us=now;s_buf_sync=1U;bk_gpio_set_output_low(PIN_BK_SYNC_OUT);}
 GPIO_UP(TPH_MOT_STEP);delay_us(4U);GPIO_DOWN(TPH_MOT_STEP);
 ++s_buf_steps;++s_blank_spi_steps[s_buf_direction?1:0];
 if(s_buf_steps==s_buf_target){s_buf_active=0U;(void)bk_timer_stop(id);return;}
 uint32_t next=printer_buffered_period(s_buf_steps);
 if(next!=s_buf_period){if(bk_timer_set_period_us(id,next)!=BK_OK){s_buf_error=2U;s_buf_active=0U;(void)bk_timer_stop(id);}else s_buf_period=next;}
}
static int printer_buffered_claim(void){
 if(s_buf_claimed)return 0;
 if(bk_timer_driver_init()!=BK_OK){os_printf("[BUFFERED_TIMER] init_failed no_motion\r\n");return -14;}
 uint32_t enabled=bk_timer_get_enable_status(),period=bk_timer_get_period(PL_MOTOR_TIMER);
 if((enabled&(1U<<PL_MOTOR_TIMER))||period!=0U){os_printf("[BUFFERED_TIMER] id=4 unavailable enable_mask=%lu period=%lu no_motion\r\n",(unsigned long)enabled,(unsigned long)period);return -14;}
 s_buf_claimed=1U;return 0;
}
/* This function performs ALL SPI operations while the timer is stopped. */
static int printer_execute_buffered_line(const uint8_t*columns,uint8_t direction){
 tph_line_ticket_t ticket={0};
 int result=0,armed=0;
 if(direction>1U)return -1;
 if(!s_bidi_known||direction!=(s_bidi_right?0U:1U))return -7;
 if(s_buf_active)return -14;
 s_buf_steps=0;s_buf_sync=0;s_buf_error=0;s_buf_max_gap=0;s_buf_cruise_max_gap=0;
 if(!s_bidi_power&&(tph_mcu_set_head_power(0)!=BK_OK||tph_line_probe()!=BK_OK))return -13;
 if(printer_buffered_claim())return -14;
 s_blank_spi_columns=0;s_blank_spi_steps[0]=s_blank_spi_steps[1]=0;
 uint64_t prepare_start=bk_aon_rtc_get_us();
 if(printer_bidi_power_up()!=0){result=-8;goto stop;}
 if(columns){
  if(tph_line_prepare(columns,PRINTER_CANVAS_COLUMN_US,&ticket)!=BK_OK){result=-13;goto stop;}
  armed=1;
 }
 uint32_t prepare_us=(uint32_t)(bk_aon_rtc_get_us()-prepare_start);
 if(direction)GPIO_UP(TPH_MOT_DIR);else GPIO_DOWN(TPH_MOT_DIR);
 delay_us(50U);
 s_buf_direction=direction;s_buf_ink=columns?1U:0U;s_buf_steps=0;s_buf_sync=0;s_buf_error=0;s_buf_max_gap=0;s_buf_cruise_max_gap=0;
 s_buf_target=PRINTER_SERVICE_SPAN_STEPS;
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
 if(s_buf_error||s_buf_steps!=PRINTER_SERVICE_SPAN_STEPS){result=-14;goto stop;}
 if(columns){
  if(!saw_low||tph_line_finish(&ticket)!=BK_OK){result=-13;goto stop;}
  s_blank_spi_columns=PL_COLUMNS;armed=0;
 }
 s_bidi_right=direction?true:false;
 os_printf("[BUFFERED_SCAN] direction=%s blank=%u code=0 pulses=%lu columns=%lu prepare_us=%lu motion_us=%lu cruise_step_us=%u max_irq_gap_us=%lu cruise_max_gap_us=%lu one_start_SYNC ink_position_unverified\r\n",direction?"RIGHT":"LEFT",columns?0U:1U,(unsigned long)s_buf_steps,(unsigned long)s_blank_spi_columns,(unsigned long)prepare_us,(unsigned long)(bk_aon_rtc_get_us()-begin),PL_STEP_PERIOD_US,(unsigned long)s_buf_max_gap,(unsigned long)s_buf_cruise_max_gap);
 return 0;
stop:
 printer_buffered_stop();
 if(armed&&!s_buf_sync)(void)tph_line_cancel(&ticket);
 printer_bidirectional_abort();
 os_printf("[BUFFERED_STOP] code=%d irq_error=%lu pulses=%lu no_blind_return\r\n",result,(unsigned long)s_buf_error,(unsigned long)s_buf_steps);
 return result;
}
/* Two head-OFF wipe excursions, entirely within the counted full stroke.
 * The motor timer is shared with normal printing; no SPI in any pulse loop. */
static int printer_buffered_wipe(uint32_t excursion,uint32_t rounds){
 if(!s_bidi_known||s_bidi_right||s_buf_active||!excursion||excursion>=PRINTER_SERVICE_SPAN_STEPS||!rounds||rounds>2U)return -7;
 if(tph_mcu_set_head_power(0)!=BK_OK)return -8;
 if(printer_buffered_claim())return -14;
 if(printer_bidi_power_up()!=0)return -8;
 s_bidi_known=false; /* do not expose the short-leg turnaround as an endpoint */
 for(uint32_t leg=0;leg<rounds*2U;++leg){
  uint32_t direction=(leg&1U)?0U:1U;
  if(direction)GPIO_UP(TPH_MOT_DIR);else GPIO_DOWN(TPH_MOT_DIR);
  delay_us(50U);
  s_buf_target=excursion;s_buf_direction=direction;s_buf_ink=0;s_buf_sync=0;
  s_buf_steps=0;s_buf_error=0;s_buf_max_gap=0;s_buf_cruise_max_gap=0;
  s_buf_period=printer_buffered_period(0U);s_buf_last_irq=(uint32_t)bk_aon_rtc_get_us();
  uint32_t begin=s_buf_last_irq;
  __sync_synchronize();s_buf_active=1U;
  if(bk_timer_start_us(PL_MOTOR_TIMER,s_buf_period,printer_buffered_tick)!=BK_OK)goto failed;
  while(s_buf_active){
   if((uint32_t)(bk_aon_rtc_get_us()-begin)>2000000U)goto failed;
   rtos_delay_milliseconds(1U);
  }
  (void)bk_timer_stop(PL_MOTOR_TIMER);
  if(s_buf_error||s_buf_steps!=excursion)goto failed;
  os_printf("[WIPER_LEG] direction=%s pulses=%lu head_OFF contact_unverified\r\n",direction?"AWAY_FROM_LEFT":"BACK_TO_LEFT",(unsigned long)s_buf_steps);
 }
 s_bidi_known=true;s_bidi_right=false;
 return 0;
failed:
 printer_buffered_stop();printer_bidirectional_abort();return -14;
}
static int printer_buffered_load_continue(void){return s_buf_error==0U;}
/* The carriage runs in TIMER4 while this task feeds paper. Image ARM/SYNC is
 * deliberately absent. Alternate direction from the current counted endpoint. */
static int printer_buffered_load_while_moving(uint32_t paper_steps,uint8_t paper_direction){
 if(!s_bidi_known||s_buf_active||!paper_steps||paper_steps>5000U||paper_direction>1U)return -7;
 if(tph_mcu_set_head_power(0)!=BK_OK)return -8;
 if(printer_buffered_claim())return -14;
 if(printer_bidi_power_up()!=0)return -8;
 uint8_t carriage_direction=s_bidi_right?0U:1U;
 if(carriage_direction)GPIO_UP(TPH_MOT_DIR);else GPIO_DOWN(TPH_MOT_DIR);
 delay_us(50U);
 s_buf_target=PRINTER_SERVICE_SPAN_STEPS;s_buf_direction=carriage_direction;s_buf_ink=0U;s_buf_sync=0U;
 s_buf_steps=0;s_buf_error=0;s_buf_max_gap=0;s_buf_cruise_max_gap=0;
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
 if(s_buf_error||s_buf_steps!=PRINTER_SERVICE_SPAN_STEPS)goto failed;
 s_bidi_known=true;s_bidi_right=carriage_direction?true:false;
 os_printf("[PAGE_PREP_LOAD] pulses=%lu/%lu code=0 parallel_carriage_steps=%lu endpoint=%s head_OFF distance_unverified\r\n",(unsigned long)sent,(unsigned long)paper_steps,(unsigned long)s_buf_steps,s_bidi_right?"RIGHT":"LEFT");
 return 0;
failed:
 os_printf("[PAGE_LOAD_MOVE_STOP] code=%d paper_pulses=%lu carriage_pulses=%lu irq_error=%lu no_blind_return\r\n",result,(unsigned long)sent,(unsigned long)s_buf_steps,(unsigned long)s_buf_error);
 printer_buffered_stop();printer_bidirectional_abort();return result;
}
#endif

#ifndef PRINTER_WASTE_MOTION_H
#define PRINTER_WASTE_MOTION_H
/* User reports the optical endpoint itself is centered over the pad; the
 * former374-step retreat moved the head off it. Hold at OC3 for stationary
 * firing. Image endpoints/insets and the final wiper return remain unchanged.
 */
/* GPIO24 supplies the physical anchor. Allow it to trigger slightly early
 * while retaining the calibrated step count as the hard travel limit. */
#define PRINTER_OC3_PRINT_EARLY_STEPS 64U
static bool printer_buffered_print_endpoint_matches(void){
 return (s_buf_error==0U||s_buf_error==3U)&&s_buf_steps<=s_buf_target&&
      s_buf_target-s_buf_steps<=PRINTER_OC3_PRINT_EARLY_STEPS;
}
static uint8_t s_waste_station_ready;
static int printer_waste_seek_from(uint32_t search_steps,bool print_range){
 s_waste_station_ready=0U;
 if(!s_bidi_known||s_bidi_at_wiper==print_range||s_bidi_right||s_buf_active)return -7;
 if(print_range&&(!s_bidi_full_stroke||s_bidi_left_offset!=0))return -7;
 if(!motor_oc3_ready())return -17;
 if(motor_oc3_active())return PRINTER_OC3_STUCK;
 if(tph_mcu_set_head_power(0)!=BK_OK)return -8;
 if(printer_buffered_claim())return -14;
 if(printer_bidi_power_up()!=0)return -8;
 GPIO_UP(TPH_MOT_DIR);delay_us(50U);
 s_buf_target=search_steps;s_buf_direction=1U;s_buf_ink=0U;s_buf_sync=0U;
 s_oc3_hit=0U;s_buf_steps=0U;s_buf_error=0U;s_buf_max_gap=0U;s_buf_cruise_max_gap=0U;
 s_buf_period=printer_buffered_period(0U);s_buf_last_irq=(uint32_t)bk_aon_rtc_get_us();
 uint32_t begin=s_buf_last_irq;s_bidi_known=false;
 __sync_synchronize();s_buf_active=1U;
 int result=-14;
 if(bk_timer_start_us(PL_MOTOR_TIMER,s_buf_period,printer_buffered_tick)!=BK_OK)goto failed;
 while(s_buf_active){
  if((uint32_t)(bk_aon_rtc_get_us()-begin)>5000000U)goto failed;
  rtos_delay_milliseconds(1U);
 }
 printer_buffered_stop();
 if(s_buf_error&&s_buf_error!=3U)goto failed;
 if(!(s_oc3_hit||motor_oc3_active())){result=PRINTER_OC3_NOT_FOUND;goto failed;}
 const uint32_t min_print_steps=((PRINTER_CANVAS_COLUMNS-1U)*PRINTER_CANVAS_COLUMN_US+
                                PRINTER_CANVAS_STEP_US-1U)/PRINTER_CANVAS_STEP_US+
                                2U*(PRINTER_CANVAS_RAMP_STEPS+32U);
 const uint32_t min_seek=(search_steps==PRINTER_OC3_PREP_SEARCH_STEPS)?
                           min_print_steps+PRINTER_PRINT_SHORTER_STEPS+PRINTER_PAD_PRINT_RETREAT_STEPS:PRINTER_SERVICE_SPAN_STEPS;
 if(print_range&&!printer_buffered_print_endpoint_matches()){
  os_printf("[WASTE_PRINT_RANGE] endpoint_mismatch steps=%lu expected=%lu no_ink\r\n",
            (unsigned long)s_buf_steps,(unsigned long)search_steps);
  result=PRINTER_OC3_NOT_FOUND;goto failed;
 }
 if(print_range&&s_buf_steps!=search_steps)
  os_printf("[WASTE_PRINT_RANGE] reanchor=GPIO24 steps=%lu nominal=%lu\r\n",
            (unsigned long)s_buf_steps,(unsigned long)search_steps);
 if(!print_range&&s_buf_steps<min_seek){
  os_printf("[WASTE_REFERENCE_REJECT] seek_steps=%lu below_minimum=%u no_ink\r\n",(unsigned long)s_buf_steps,(unsigned)min_seek);
  result=-7;goto failed;
 }
 os_printf("[WASTE_REFERENCE] OC3_LOW seek_steps=%lu head_OFF\r\n",(unsigned long)s_buf_steps);
 s_oc3_hit=0U;s_buf_error=0U;
 s_bidi_known=true;s_bidi_right=true;s_bidi_at_wiper=false;
 s_bidi_right_offset=(int32_t)printer_buffered_optical_inset();
 /* Timer is stopped but U4 remains enabled to hold its microstep position. */
 rtos_delay_milliseconds(20U);
 if(!motor_oc3_active()){result=PRINTER_OC3_NOT_FOUND;goto failed;}
 s_waste_station_ready=1U;
 os_printf("[WASTE_POSITION] endpoint=OC3_RIGHT retract_steps=0 settle_ms=20 gpio24_LOW motor_hold head_OFF pad_alignment_hardware_check\r\n");
 return 0;
failed:
 printer_buffered_stop();s_waste_station_ready=0U;
 printer_bidirectional_abort();return result;
}
static int printer_waste_seek(uint32_t search_steps){
 return printer_waste_seek_from(search_steps,false);
}

static int printer_waste_position(void){
 return printer_waste_seek(PRINTER_MAINTENANCE_SPAN_STEPS);
}

static int printer_waste_park_for_preparation(void){
 /* The seek started from the counted seal position. Return the same number
  * of steps; the search limit is not a calibrated mechanical stroke. */
 s_bidi_full_stroke=s_buf_steps;
 os_printf("[CARRIAGE_STROKE] full_steps=%lu print_steps=%lu seal_inset=%u pad_inset=%u\r\n",
           (unsigned long)s_bidi_full_stroke,(unsigned long)printer_buffered_print_span(),
           PRINTER_PRINT_SHORTER_STEPS,(unsigned)printer_buffered_optical_inset());
 int result=printer_buffered_oc3_return(s_bidi_full_stroke);
 if(result!=PRINTER_OC3_RETURNED)return result;
 printer_buffered_stop();
 carriage_verified_power_down();
 s_bidi_power=false;
 return 0;
}

/* The last page visit starts from PRINT_LEFT and references the pad for flashing. */
static int printer_waste_seek_from_print_left(void){
 if(!s_bidi_full_stroke||!s_bidi_known||!s_bidi_at_wiper||s_bidi_right)return -7;
 int result=printer_buffered_leave_wiper();
 if(result)return result;
 return printer_waste_seek_from(printer_buffered_print_span()+printer_buffered_optical_inset(),true);
}

static int printer_waste_return_to_print_left(void){
 const uint32_t steps=printer_buffered_print_span();
 const uint32_t retreat=printer_buffered_optical_inset();
 int result=printer_buffered_reposition(0U,retreat);
 if(result)return result;
 if(motor_oc3_active()){printer_bidirectional_abort();return PRINTER_OC3_STUCK;}
 s_bidi_right_offset=0;
 os_printf("[PAGE_PREP_PAD_RETREAT] steps=%lu toward=SEAL head_OFF\r\n",(unsigned long)retreat);
 result=printer_execute_buffered_line(NULL,0U);
 if(result)return result;
 printer_buffered_stop();
 os_printf("[PAGE_PREP_LAST_VISIT_DONE] steps_each=%lu parked=PRINT_LEFT head_OFF\r\n",
           (unsigned long)steps);
 return 0;
}

/* Execute an ordinary cached row without U4 motion. HC supplies the finite
 * column count/deadline checks. Never do SPI while its SYNC is held LOW. */
static int printer_waste_fire_profile(const uint8_t *row,unsigned prespray){
 if(!row||!s_waste_station_ready||!s_bidi_known||!s_bidi_right||s_bidi_at_wiper||s_bidi_right_offset!=(int32_t)printer_buffered_optical_inset()||s_buf_active||s_buf_claimed||!motor_oc3_active())return -7;
 tph_line_ticket_t ticket={0};int result=-13;
 const char *stage="prepare";
 const uint32_t period_us=prespray?PL_PRESPRAY_PERIOD_US:PRINTER_CANVAS_COLUMN_US;
 if((prespray?tph_line_prepare_prespray(row,period_us,&ticket):tph_line_prepare(row,period_us,&ticket))!=BK_OK)goto stop;
 if(!motor_oc3_active()){
  os_printf("[WASTE_POSITION_LOST] stage=BEFORE_SYNC GPIO24_released no_fire\r\n");
  (void)tph_line_cancel(&ticket);goto stop;
 }
 uint32_t begin=(uint32_t)bk_aon_rtc_get_us();unsigned saw_low=0U;
 stage="wait_ready_low";
 bk_gpio_set_output_low(PIN_BK_SYNC_OUT);
 for(unsigned loops=0;loops<11U;++loops){
  if(!bk_gpio_get_input(PIN_BK_READY_IN)){saw_low=1U;break;}
  if((uint32_t)(bk_aon_rtc_get_us()-begin)>10000U)break;
  rtos_delay_milliseconds(1U);
 }
 if(!saw_low)goto stop;
 stage="playback";
 begin=(uint32_t)bk_aon_rtc_get_us();
 const uint32_t run_us=PL_COLUMNS*period_us+2000U;
 unsigned complete=0U;
 /* The independent iteration bound also terminates a stalled RTC. */
 for(uint32_t loops=0;loops<(run_us+50000U)/1000U+1U;++loops){
  if(!motor_oc3_active()){
   os_printf("[WASTE_POSITION_LOST] stage=FIRING GPIO24_released abort_line\r\n");goto stop;
  }
  if(bk_gpio_get_input(PIN_BK_READY_IN))goto stop;
  if((uint32_t)(bk_aon_rtc_get_us()-begin)>=run_us){complete=1U;break;}
  rtos_delay_milliseconds(1U);
 }
 if(!complete){result=-14;goto stop;}
 bk_gpio_set_output_high(PIN_BK_SYNC_OUT);
 if(prespray){
  /* HC32 powers off and audits the short scan before arming the status DMA.
   * Wait here without clocking SPI or extending the firing interval. */
  stage="status_ready";
  const uint32_t released=(uint32_t)bk_aon_rtc_get_us();
  for(unsigned polls=0;!bk_gpio_get_input(PIN_BK_READY_IN);++polls){
   if(!motor_oc3_active()||polls>=500U||
      (uint32_t)(bk_aon_rtc_get_us()-released)>=500000U)goto stop;
   rtos_delay_milliseconds(1U);
  }
  if(!motor_oc3_active())goto stop;
 }
 stage="status";
 if(tph_line_finish(&ticket)!=BK_OK)goto stop;
 os_printf("[WASTE_FIRED] token=%lu columns=%u period_us=%u U4_steps=0 paper_steps=0 ink_unverified\r\n",(unsigned long)ticket.token,PL_COLUMNS,(unsigned)period_us);
 return 0;
stop:
 bk_gpio_set_output_high(PIN_BK_SYNC_OUT);
 (void)tph_mcu_set_head_power(0);
 os_printf("[WASTE_FIRE_STOP] stage=%s token=%lu period_us=%lu code=%d\r\n",stage,
           (unsigned long)ticket.token,(unsigned long)period_us,result);
 s_waste_station_ready=0U;
 return result;
}
static int printer_waste_fire(const uint8_t *row){return printer_waste_fire_profile(row,0U);}
static int printer_waste_fire_prespray(const uint8_t *row){return printer_waste_fire_profile(row,1U);}
#endif

#ifndef PRINTER_WASTE_MOTION_H
#define PRINTER_WASTE_MOTION_H
/* Validate the last visit against the preceding full seek. The calibrated
 * target still caps travel even if the optical edge is later than expected. */
#define PRINTER_OC3_PRINT_WINDOW_STEPS 64U
#define PRINTER_WASTE_CLEAN_COLUMN_US 673U
static uint8_t s_waste_station_ready;
static uint32_t s_waste_pad_advance;
static uint32_t s_waste_measured_full;
static uint32_t s_waste_last_full_seek;

static int printer_waste_seek_from(uint32_t search_steps,bool print_range,uint32_t advance){
 const bool continuous=advance==PRINTER_PAGE_PAD_ADVANCE_STEPS;
 s_waste_pad_advance=0U;
 s_waste_station_ready=0U;
 if(!s_bidi_known||s_bidi_at_wiper==print_range||s_bidi_right||s_buf_active)return -7;
 if(print_range&&(!s_bidi_full_stroke||s_bidi_left_offset!=0||
                 s_waste_last_full_seek<=PRINTER_PRINT_SHORTER_STEPS))return -7;
 const uint32_t expected=print_range?s_waste_last_full_seek-PRINTER_PRINT_SHORTER_STEPS:0U;
 if(advance&&!print_range)s_waste_last_full_seek=0U;
 const uint32_t min_print_steps=((PRINTER_CANVAS_COLUMNS-1U)*PRINTER_CANVAS_COLUMN_US+
                                PRINTER_CANVAS_STEP_US-1U)/PRINTER_CANVAS_STEP_US+
                                2U*(PRINTER_CANVAS_RAMP_STEPS+32U);
 const uint32_t min_seek=(search_steps==PRINTER_OC3_PREP_SEARCH_STEPS)?
                           min_print_steps+PRINTER_PRINT_SHORTER_STEPS+PRINTER_PAD_PRINT_RETREAT_STEPS:PRINTER_SERVICE_SPAN_STEPS;
 if(!motor_oc3_ready())return -17;
 if(motor_oc3_active())return PRINTER_OC3_STUCK;
 if(tph_mcu_set_head_power(0)!=BK_OK)return -8;
 if(printer_buffered_claim())return -14;
 if(printer_bidi_power_up()!=0)return -8;
 if(printer_buffered_set_direction(1U)){printer_bidirectional_abort();return -14;}
 delay_us(50U);
 s_buf_oc3_extra=continuous?advance:0U;
 s_buf_oc3_reference=0U;
 s_buf_oc3_high_samples=0U;
 s_buf_oc3_min=print_range?expected-PRINTER_OC3_PRINT_WINDOW_STEPS:min_seek;
 s_buf_oc3_max=print_range&&expected+PRINTER_OC3_PRINT_WINDOW_STEPS<search_steps?
                 expected+PRINTER_OC3_PRINT_WINDOW_STEPS:search_steps;
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
 const uint32_t seek_steps=continuous?s_buf_oc3_reference:s_buf_steps;
 if(continuous&&(!seek_steps||s_buf_steps!=seek_steps+advance)){
  result=PRINTER_OC3_NOT_FOUND;goto failed;
 }
 s_buf_oc3_extra=0U;
 if(print_range){
  const uint32_t difference=seek_steps>expected?seek_steps-expected:expected-seek_steps;
  if(difference>PRINTER_OC3_PRINT_WINDOW_STEPS){
   os_printf("[WASTE_PRINT_RANGE] endpoint_mismatch steps=%lu expected=%lu cap=%lu no_ink\r\n",
             (unsigned long)seek_steps,(unsigned long)expected,(unsigned long)search_steps);
   result=PRINTER_OC3_NOT_FOUND;goto failed;
  }
  os_printf("[WASTE_PRINT_RANGE] reanchor=GPIO24 steps=%lu expected=%lu cap=%lu\r\n",
            (unsigned long)seek_steps,(unsigned long)expected,(unsigned long)search_steps);
 }
 if(!print_range&&seek_steps<min_seek){
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
 if(advance){
  if(!continuous){
   result=printer_buffered_advance_past_oc3();
   if(result)goto failed;
   rtos_delay_milliseconds(20U);
   if(!motor_oc3_active()){result=PRINTER_OC3_NOT_FOUND;goto failed;}
  }
  s_bidi_right_offset+=advance;
  s_waste_pad_advance=advance;
  if(!print_range)s_waste_last_full_seek=seek_steps;
  s_waste_measured_full=seek_steps+advance+(print_range?PRINTER_PRINT_SHORTER_STEPS:0U);
  os_printf("[WASTE_MEASURED_STROKE] gpio_steps=%lu advance_steps=%lu measured_full_steps=%lu continuous=%u\r\n",
            (unsigned long)seek_steps,(unsigned long)advance,(unsigned long)s_waste_measured_full,continuous);
 }
 s_waste_station_ready=1U;
 os_printf("[WASTE_POSITION] endpoint=%s advance_steps=%u settle_ms=20 gpio24_LOW motor_hold head_OFF\r\n",
           advance?"PAD_EXTENDED":"OC3_RIGHT",(unsigned)advance);
 return 0;
failed:
 if(continuous)
  os_printf("[WASTE_CONTINUOUS_STOP] steps=%lu expected=%lu cap=%lu error=%lu gpio24_high_samples=%u\r\n",
            (unsigned long)s_buf_steps,(unsigned long)expected,(unsigned long)search_steps,
            (unsigned long)s_buf_error,(unsigned)s_buf_oc3_high_samples);
 s_buf_oc3_extra=0U;
 printer_buffered_stop();s_waste_station_ready=0U;
 printer_bidirectional_abort();return result;
}
static int printer_waste_seek(uint32_t search_steps){
 return printer_waste_seek_from(search_steps,false,search_steps==PRINTER_OC3_PREP_SEARCH_STEPS?PRINTER_PAD_ADVANCE_STEPS:0U);
}

static int printer_waste_seek_for_page(void){
 return printer_waste_seek_from(PRINTER_OC3_PREP_SEARCH_STEPS,false,PRINTER_PAGE_PAD_ADVANCE_STEPS);
}

static int printer_waste_position(void){
 return printer_waste_seek(PRINTER_MAINTENANCE_SPAN_STEPS);
}

static int printer_waste_park_for_preparation(void){
 /* Return to a fixed distance from GPIO24 on every boot, including when
  * the carriage already started from the extended seal position. */
 if(!s_waste_pad_advance)return -7;
 s_bidi_full_stroke=PRINTER_NEW_SEAL_TO_OC3_STEPS;
 const uint32_t return_steps=PRINTER_NEW_SEAL_TO_OC3_STEPS+s_waste_pad_advance;
 os_printf("[CARRIAGE_STROKE] full_steps=%lu print_steps=%lu seal_inset=%u pad_inset=%u\r\n",
           (unsigned long)return_steps,(unsigned long)printer_buffered_print_span(),
           PRINTER_PRINT_SHORTER_STEPS,(unsigned)printer_buffered_optical_inset()+s_waste_pad_advance);
 int result=printer_buffered_return_from_reference(return_steps,
             printer_buffered_optical_inset()+s_waste_pad_advance);
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
 return printer_waste_seek_from(printer_buffered_print_span()+printer_buffered_optical_inset(),true,PRINTER_PAGE_PAD_ADVANCE_STEPS);
}

static int printer_waste_return_to_print_left(void){
 const uint32_t steps=printer_buffered_print_span();
 if(!s_waste_pad_advance)return -7;
 const uint32_t retreat=(uint32_t)s_bidi_right_offset;
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
 if(!row||!s_waste_station_ready||!s_bidi_known||!s_bidi_right||s_bidi_at_wiper||s_bidi_right_offset!=(int32_t)(printer_buffered_optical_inset()+s_waste_pad_advance)||s_buf_active||s_buf_claimed||!motor_oc3_active())return -7;
 tph_line_ticket_t ticket={0};int result=-13;
 const char *stage="prepare";
 const uint32_t period_us=prespray?PL_PRESPRAY_PERIOD_US:PRINTER_WASTE_CLEAN_COLUMN_US;
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

#ifndef PRINTER_WASTE_MOTION_H
#define PRINTER_WASTE_MOTION_H
/* User-confirmed waste pad covers the existing PRINT_RIGHT position.
 * Reference it from OC3, retract the existing374-step inset, then allow ink.
 * This proof flag belongs to this serialized maintenance job, not image scans. */
static uint8_t s_waste_station_ready;
static int printer_waste_position(void){
 s_waste_station_ready=0U;
 if(!s_bidi_known||!s_bidi_at_wiper||s_bidi_right||s_buf_active)return -7;
 if(!motor_oc3_ready())return -17;
 if(motor_oc3_active())return PRINTER_OC3_STUCK;
 if(tph_mcu_set_head_power(0)!=BK_OK)return -8;
 if(printer_buffered_claim())return -14;
 if(printer_bidi_power_up()!=0)return -8;
 GPIO_UP(TPH_MOT_DIR);delay_us(50U);
 s_buf_target=PRINTER_OC3_BOOT_SEARCH_STEPS;s_buf_direction=1U;s_buf_ink=0U;s_buf_sync=0U;
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
 if(s_buf_steps<PRINTER_SERVICE_SPAN_STEPS){
  os_printf("[WASTE_REFERENCE_REJECT] seek_steps=%lu below_minimum=%u no_ink\r\n",(unsigned long)s_buf_steps,PRINTER_SERVICE_SPAN_STEPS);
  result=-7;goto failed;
 }
 os_printf("[WASTE_REFERENCE] OC3_LOW seek_steps=%lu head_OFF\r\n",(unsigned long)s_buf_steps);
 s_oc3_hit=0U;s_buf_error=0U;
 s_bidi_known=true;s_bidi_right=true;s_bidi_at_wiper=false;
 s_bidi_right_offset=PRINTER_PRINT_ENDPOINT_INSET_STEPS;
 result=printer_buffered_reposition(0U,PRINTER_PRINT_ENDPOINT_INSET_STEPS);
 if(result)goto failed;
 printer_buffered_stop();
 s_bidi_right_offset=0;
 if(motor_oc3_active()){result=PRINTER_OC3_STUCK;goto failed;}
 s_waste_station_ready=1U;
 os_printf("[WASTE_POSITION] endpoint=PRINT_RIGHT retract_from_OC3=%u motor_stopped head_OFF pad_coverage_user_confirmed\r\n",PRINTER_PRINT_ENDPOINT_INSET_STEPS);
 return 0;
failed:
 printer_buffered_stop();s_waste_station_ready=0U;
 printer_bidirectional_abort();return result;
}
/* Execute an ordinary cached row without U4 motion. HC supplies the finite
 * column count/deadline checks. Never do SPI while its SYNC is held LOW. */
static int printer_waste_fire(const uint8_t *row){
 if(!row||!s_waste_station_ready||!s_bidi_known||!s_bidi_right||s_bidi_at_wiper||s_bidi_right_offset||s_buf_active||s_buf_claimed||motor_oc3_active())return -7;
 tph_line_ticket_t ticket={0};int result=-13;
 if(tph_line_prepare(row,PRINTER_CANVAS_COLUMN_US,&ticket)!=BK_OK)goto stop;
 uint32_t begin=(uint32_t)bk_aon_rtc_get_us();unsigned saw_low=0U;
 bk_gpio_set_output_low(PIN_BK_SYNC_OUT);
 for(unsigned loops=0;loops<11U;++loops){
  if(!bk_gpio_get_input(PIN_BK_READY_IN)){saw_low=1U;break;}
  if((uint32_t)(bk_aon_rtc_get_us()-begin)>10000U)break;
  rtos_delay_milliseconds(1U);
 }
 if(!saw_low)goto stop;
 begin=(uint32_t)bk_aon_rtc_get_us();
 const uint32_t run_us=PL_COLUMNS*PRINTER_CANVAS_COLUMN_US+2000U;
 unsigned complete=0U;
 /* The independent iteration bound also terminates a stalled RTC. */
 for(uint32_t loops=0;loops<(run_us+50000U)/1000U+1U;++loops){
  if(bk_gpio_get_input(PIN_BK_READY_IN))goto stop;
  if((uint32_t)(bk_aon_rtc_get_us()-begin)>=run_us){complete=1U;break;}
  rtos_delay_milliseconds(1U);
 }
 if(!complete){result=-14;goto stop;}
 bk_gpio_set_output_high(PIN_BK_SYNC_OUT);
 if(tph_line_finish(&ticket)!=BK_OK)goto stop;
 os_printf("[WASTE_FIRED] token=%lu columns=%u period_us=%u U4_steps=0 paper_steps=0 ink_unverified\r\n",(unsigned long)ticket.token,PL_COLUMNS,PRINTER_CANVAS_COLUMN_US);
 return 0;
stop:
 bk_gpio_set_output_high(PIN_BK_SYNC_OUT);
 (void)tph_mcu_set_head_power(0);
 s_waste_station_ready=0U;
 return result;
}
#endif

"""Actual BK cached protocol and timer motion with independent mock HC column clock."""
from pathlib import Path
import subprocess,tempfile,os
D=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
H=Path(os.environ.get('HC32_SOURCE',str(Path(__file__).resolve().parents[4].parent/'hc32f460/projects/ev_hc32f460_lqfp100_v2/examples/spi/spi_dma/source')))
assert (D/'printer_line_protocol.h').read_bytes()==(H/'printer_line_protocol.h').read_bytes(), 'BK/HC32 protocol headers differ'
finish_source=(D/'printer_bidirectional_print.h').read_text()
finish=finish_source[finish_source.index('static int printer_bidirectional_finish(void)'):finish_source.rindex('#endif')]
transport=(D/'tph_driver.c').read_bytes().decode('latin1');transport=transport[transport.index('/* LBUFv1:'):]
h=r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "printer_line_cache.h"
#include "printer_service_geometry.h"
#include "printer_canvas_geometry.h"
typedef int bk_err_t;typedef int timer_id_t;typedef void(*timer_isr_t)(timer_id_t);
#define BK_OK 0
#define BK_FAIL -1
#define TIMER_ID4 4
#define TPH_MOT_DIR 50
#define TPH_MOT_STEP 19
#define PIN_BK_SYNC_OUT 1
#define PIN_BK_READY_IN 0
#define SPI_ID_1 1
#define TPH_LOGI printf
#define TPH_LOGE printf
#define os_printf printf
static printer_line_cache_t cache;
static uint64_t now_us,next_bk,next_hc;
static unsigned timer_enabled,timer_period,timer_reserved,boost,step,dir,sync_high=1,mode,hc_fires,irq_stall;
static int32_t pos,max_pos;
static timer_isr_t callback;
static uint32_t s_blank_spi_columns,s_blank_spi_steps[2];
static bool s_bidi_at_wiper=false;
static bool s_bidi_right=false,s_bidi_known=true,s_bidi_power=false;
typedef struct {uint32_t token,period_us,crc;}tph_line_ticket_t;
static void advance(unsigned us);
static uint64_t bk_aon_rtc_get_us(void){return now_us/32U*32U;}
static void delay_us(unsigned n){assert(n==4||n==50);now_us+=n;}
static void up(unsigned p){if(p==50){assert(!timer_enabled&&!step);dir=1;return;}assert(p==19&&boost&&!step);step=1;pos+=dir?1:-1;if(pos>max_pos)max_pos=pos;assert(pos>=-(int32_t)PRINTER_MAINTENANCE_EXTRA_STEPS&&pos<=((int32_t)PRINTER_SERVICE_SPAN_STEPS));}
static void down(unsigned p){if(p==50){assert(!timer_enabled&&!step);dir=0;return;}assert(p==19&&step);step=0;}
#define GPIO_UP(p) up(p)
#define GPIO_DOWN(p) down(p)
static void bk_gpio_set_output_low(unsigned pin){
 assert(pin==1);sync_high=0;
 if(cache.state==PL_ARMED){
  cache.state=PL_RUNNING;cache.fired=0;hc_fires=0;
  assert(timer_enabled);assert(pos==(dir?(int32_t)PRINTER_CANVAS_LEAD_STEPS:((int32_t)PRINTER_SERVICE_SPAN_STEPS)-(int32_t)PRINTER_CANVAS_LEAD_STEPS));
  assert(cache.period_us==PRINTER_CANVAS_COLUMN_US);
  ++cache.fired;++hc_fires;next_hc=now_us+cache.period_us;
 }
}
static void bk_gpio_set_output_high(unsigned pin){assert(pin==1);sync_high=1;if(cache.state==PL_RUNNING)pl_fail(&cache,PL_CANCELED);}
static unsigned bk_gpio_get_input(unsigned pin){assert(pin==0);return !sync_high&&(cache.state==PL_RUNNING||cache.state==PL_DONE)?0U:1U;}
static bk_err_t wait_hc32_level(unsigned level){assert(!timer_enabled);return bk_gpio_get_input(0)==level?0:0;}
static bk_err_t bk_spi_read_bytes(unsigned id,void*p,unsigned n){assert(id==1&&n==100&&!timer_enabled);pl_status(&cache,p);if(mode==1)((uint8_t*)p)[2]^=1;return 0;}
static bk_err_t tph_mcu_set_head_power(uint8_t on){assert(!on&&!timer_enabled);cache.state=PL_IDLE;cache.error=PL_OK;return 0;}
static bk_err_t tph_fire_raw_column_bytes(const uint8_t*p){assert(!timer_enabled);if(mode==2&&cache.state==PL_LOADING&&cache.received==500){uint8_t bad[100];memcpy(bad,p,100);bad[8]^=1;pl_receive(&cache,bad);}else pl_receive(&cache,p);return 0;}
TRANSPORT
static int bk_timer_driver_init(void){return 0;}
static uint32_t bk_timer_get_enable_status(void){return mode==3?(1U<<4):timer_enabled?(1U<<4):0;}
static uint32_t bk_timer_get_period(timer_id_t id){assert(id==4);return timer_reserved;}
static int bk_timer_stop(timer_id_t id){assert(id==4);timer_enabled=0;return 0;}
static int bk_timer_cancel(timer_id_t id){assert(id==4&&!timer_enabled);timer_reserved=0;return 0;}
static int bk_timer_start_us(timer_id_t id,unsigned period,timer_isr_t cb){assert(id==4&&!timer_enabled);timer_period=period;timer_reserved=period;callback=cb;timer_enabled=1;next_bk=now_us+period;return 0;}
static int bk_timer_set_period_us(timer_id_t id,unsigned period){assert(id==4&&timer_enabled);timer_period=period;timer_reserved=period;return 0;}
static void rtos_delay_milliseconds(unsigned n){assert(n==1);advance(mode==5?7000U:1000U);}
static int printer_bidi_power_up(void){boost=1;s_bidi_power=true;return 0;}
static void printer_bidirectional_abort(void);
static int motor_page_load_keep_power_guarded(uint32_t,uint8_t,uint32_t*,int(*)(void));
#include "printer_buffered_motion.h"
static void printer_bidirectional_abort(void){printer_buffered_stop();boost=0;s_bidi_power=false;s_bidi_known=false;}
#ifndef TEST_LOAD_STEPS
#define TEST_LOAD_STEPS 702U
#endif
#define PRINTER_STARTUP_LOAD_STEPS TEST_LOAD_STEPS
#define PRINTER_PAGE_FEED_DIRECTION 0
#define PRINTER_STARTUP_LOAD_ROUNDS 3U
#define PRINTER_BUFFERED_LINE 1
#define PRINTER_STARTUP_WIPE_STEPS 4114U
#define PRINTER_STARTUP_WIPE_ROUNDS 2U
static unsigned load_calls,total_paper;
static int motor_page_load_keep_power_guarded(uint32_t steps,uint8_t direction,uint32_t*sent,int(*keep_running)(void)){
 assert(pos==(dir?-(int32_t)PRINTER_MAINTENANCE_EXTRA_STEPS:(int32_t)PRINTER_SERVICE_SPAN_STEPS)&&s_bidi_right==!dir&&boost&&timer_enabled&&sync_high&&!hc_fires);
 assert(cache.state==PL_IDLE&&steps==702&&!direction&&keep_running);
 int32_t start_pos=pos;++load_calls;*sent=0;
 for(unsigned i=0;i<steps;i++){
  if(!keep_running())return -1;
  if(mode==9&&i==100)return -1;
  advance(1200U);++*sent;++total_paper;
  assert(sync_high&&!hc_fires&&cache.state==PL_IDLE);
 }
 assert(pos!=start_pos); /* carriage progressed while paper was feeding, not afterwards */
 return 0;
}
static void carriage_verified_power_down(void){assert(!timer_enabled);boost=0;}
REAL_FINISH
#define printer_execute_bidirectional_line printer_execute_buffered_line
#include "printer_startup_prepare.h"
static void advance(unsigned us){
 uint64_t target=now_us+us;
 while(timer_enabled || cache.state==PL_RUNNING){
  uint64_t b=timer_enabled?next_bk:UINT64_MAX,h=cache.state==PL_RUNNING?next_hc:UINT64_MAX;
  uint64_t next=b<h?b:h;if(next>target)break;
  now_us=next;
  if(h<=b){
   if(mode!=4){
    int32_t delta=(cache.fired*PRINTER_CANVAS_COLUMN_US+PRINTER_CANVAS_STEP_US-1U)/PRINTER_CANVAS_STEP_US;
    assert(pos==(dir?(int32_t)PRINTER_CANVAS_LEAD_STEPS+delta:((int32_t)PRINTER_SERVICE_SPAN_STEPS)-(int32_t)PRINTER_CANVAS_LEAD_STEPS-delta));
    /* Every column occurs after acceleration and before deceleration. */
    assert(pos>450&&pos<((int32_t)PRINTER_SERVICE_SPAN_STEPS)-450);
   }
   ++cache.fired;++hc_fires;
   if(cache.fired==1180)cache.state=PL_DONE;else next_hc+=cache.period_us;
  }else{
   uint64_t epoch=next_bk;
   if(!irq_stall&&((mode==4&&s_buf_steps==3000)||((mode==8||mode==10||mode==12)&&s_buf_steps==60))){irq_stall=1;next_bk+=(mode==8||mode==10||mode==12)?10000U:1000U;continue;}
   callback(4);if(timer_enabled)next_bk=epoch+timer_period;
  }
 }
 if(now_us<target)now_us=target;
}
int main(int argc,char**argv){
 assert(argc==2);mode=atoi(argv[1]);if(mode>=6){pos=-(int32_t)PRINTER_MAINTENANCE_EXTRA_STEPS;s_bidi_at_wiper=true;}
 uint8_t*row=malloc(PL_ROW_BYTES);for(unsigned i=0;i<PL_ROW_BYTES;i++)row[i]=(i*7+i/100)&63U;
 if(mode==7||mode==8){
  assert(printer_buffered_wipe(0U,2U)==-7&&pos==-(int32_t)PRINTER_MAINTENANCE_EXTRA_STEPS);
  assert(printer_buffered_wipe(PRINTER_MAINTENANCE_SPAN_STEPS,2U)==-7&&pos==-(int32_t)PRINTER_MAINTENANCE_EXTRA_STEPS);
  assert(printer_buffered_wipe(4114U,3U)==-7&&pos==-(int32_t)PRINTER_MAINTENANCE_EXTRA_STEPS);
  int wipe=printer_buffered_wipe(4114U,2U);
  if(mode==7){
   assert(!wipe&&pos==-(int32_t)PRINTER_MAINTENANCE_EXTRA_STEPS&&s_bidi_known&&!s_bidi_right&&max_pos==3740);
   assert(s_blank_spi_steps[0]==8228&&s_blank_spi_steps[1]==8228);
  }
  else assert(wipe==-14&&pos==-(int32_t)PRINTER_MAINTENANCE_EXTRA_STEPS+60&&!s_bidi_known);
  assert(!hc_fires&&!timer_enabled&&sync_high);printer_buffered_stop();free(row);
  puts("PASS short timer wipe: zero fires, exact excursions, no blind return on IRQ fault");return 0;
 }
 if(mode==9||mode==10){
  int result=printer_buffered_load_while_moving(702U,0U);
  assert(result==(mode==9?-10:-14)&&!s_bidi_known&&!timer_enabled&&!hc_fires&&sync_high);
  if(mode==9)assert(pos>-(int32_t)PRINTER_MAINTENANCE_EXTRA_STEPS&&pos<(int32_t)PRINTER_SERVICE_SPAN_STEPS);
  else assert(pos==-(int32_t)PRINTER_MAINTENANCE_EXTRA_STEPS+60);
  free(row);puts("PASS concurrent loading fault: both motors stop, no image fires and no blind return");return 0;
 }
 if(mode==11){
  assert(!printer_buffered_boot_round_trip());
  assert(pos==-(int32_t)PRINTER_MAINTENANCE_EXTRA_STEPS&&s_bidi_at_wiper&&!s_bidi_right&&s_bidi_known&&!load_calls&&!hc_fires);
  assert(s_buf_steps==PRINTER_MAINTENANCE_SPAN_STEPS);printer_buffered_stop();free(row);return 0;
 }
 if(mode==12){
  pos=PRINTER_SERVICE_SPAN_STEPS;s_bidi_at_wiper=false;s_bidi_right=true;
  assert(printer_buffered_park_at_wiper()==-14&&!s_bidi_known&&pos==(int32_t)PRINTER_SERVICE_SPAN_STEPS-60);
  assert(!timer_enabled&&!hc_fires);free(row);return 0;
 }
 if(mode==6){
  assert(!printer_startup_prepare(1U));
  assert(s_bidi_known&&!hc_fires&&!timer_enabled&&sync_high);
#if PRINTER_STARTUP_LOAD_STEPS > 0
  assert(load_calls==6&&total_paper==4212&&pos==0&&!s_bidi_right&&!s_bidi_at_wiper);
  assert(!printer_execute_buffered_line(row,1));
  assert(pos==(int32_t)PRINTER_SERVICE_SPAN_STEPS);
  assert(!printer_execute_buffered_line(NULL,0)&&pos==0);
#else
  assert(!load_calls&&pos==0&&!s_bidi_right&&!s_bidi_at_wiper);
  assert(!printer_execute_buffered_line(row,1));
  assert(pos==(int32_t)PRINTER_SERVICE_SPAN_STEPS);
  assert(!printer_execute_buffered_line(NULL,0)&&pos==0);
#endif
  assert(hc_fires==1180&&cache.state==PL_DONE);
  assert(!printer_bidirectional_finish()&&pos==-(int32_t)PRINTER_MAINTENANCE_EXTRA_STEPS&&s_bidi_at_wiper&&!boost);
  printer_buffered_stop();free(row);puts("PASS startup: two 22mm wipes, six concurrent 1.8mm feeds, total 10.8mm, final LEFT, no fires until loading completes, image row starts from actual endpoint");return 0;
 }
 int rc=printer_execute_buffered_line(row,1);
 if(!mode||mode==5){
  assert(!rc&&pos==((int32_t)PRINTER_SERVICE_SPAN_STEPS)&&s_bidi_right&&hc_fires==1180&&s_blank_spi_columns==1180&&cache.state==PL_DONE);
  assert(!memcmp(cache.data,row,PL_ROW_BYTES));
  assert(!printer_execute_buffered_line(row,0));assert(pos==0&&!s_bidi_right&&hc_fires==1180);
  assert(!printer_execute_buffered_line(NULL,1));assert(pos==((int32_t)PRINTER_SERVICE_SPAN_STEPS)&&s_blank_spi_columns==0);
  assert(!printer_execute_buffered_line(NULL,0));assert(pos==0);
  assert(!printer_buffered_park_at_wiper()&&pos==-(int32_t)PRINTER_MAINTENANCE_EXTRA_STEPS&&s_buf_steps==PRINTER_MAINTENANCE_EXTRA_STEPS);
  assert(!printer_buffered_leave_wiper()&&pos==0);
  assert(!printer_execute_buffered_line(NULL,1)&&pos==(int32_t)PRINTER_SERVICE_SPAN_STEPS);
  assert(!printer_bidirectional_finish()&&pos==-(int32_t)PRINTER_MAINTENANCE_EXTRA_STEPS&&s_buf_steps==PRINTER_MAINTENANCE_SPAN_STEPS&&!boost);
  assert(!printer_buffered_leave_wiper()&&pos==0);printer_buffered_stop();
 }else if(mode==4){assert(rc==-14&&!s_bidi_known&&s_buf_steps==3000&&!timer_enabled&&pos==3000);}
 else {assert(rc==(mode==3?-14:-13));assert(pos==0&&!s_blank_spi_steps[0]&&!s_blank_spi_steps[1]&&!timer_enabled&&!hc_fires);}
 free(row);puts("PASS real BK cache transport and timer engine: both directions, fixed HC column clock, worker stall, blank moves, bad peer/CRC/busy timer and late IRQ stop");
}
'''.replace('TRANSPORT',transport).replace('REAL_FINISH',finish)
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.c').write_text(h);(p/'driver').mkdir();(p/'driver/timer.h').write_text('/* timer API provided by deterministic test platform */\n')
 subprocess.run(['cc','-std=c99','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(p),'-I'+str(D),'-I'+str(H),str(p/'test.c'),'-o',str(p/'test')],check=True)
 for mode in range(13):subprocess.run([str(p/'test'),str(mode)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

 # Also exercise the explicitly disabled loading configuration.
 subprocess.run(['cc','-std=c99','-O1','-Wall','-Wextra','-Werror','-DTEST_LOAD_STEPS=0','-fsanitize=address,undefined','-I'+str(p),'-I'+str(D),'-I'+str(H),str(p/'test.c'),'-o',str(p/'test-zero')],check=True)
 subprocess.run([str(p/'test-zero'),'6'],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

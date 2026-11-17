"""Actual BK cached protocol and timer motion with independent mock HC column clock."""
from pathlib import Path
import subprocess,tempfile,os
D=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
H=Path(os.environ.get('HC32_SOURCE',str(Path(__file__).resolve().parents[4].parent/'hc32f460/projects/ev_hc32f460_lqfp100_v2/examples/spi/spi_dma/source')))
assert (D/'printer_line_protocol.h').read_bytes()==(H/'printer_line_protocol.h').read_bytes(), 'BK/HC32 protocol headers differ'
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
static int32_t pos;
static timer_isr_t callback;
static uint32_t s_blank_spi_columns,s_blank_spi_steps[2];
static bool s_bidi_right=false,s_bidi_known=true,s_bidi_power=false;
typedef struct {uint32_t token,period_us,crc;}tph_line_ticket_t;
static void advance(unsigned us);
static uint64_t bk_aon_rtc_get_us(void){return now_us/32U*32U;}
static void delay_us(unsigned n){assert(n==4||n==50);now_us+=n;}
static void up(unsigned p){if(p==50){assert(!timer_enabled&&!step);dir=1;return;}assert(p==19&&boost&&!step);step=1;pos+=dir?1:-1;assert(pos>=0&&pos<=10660);}
static void down(unsigned p){if(p==50){assert(!timer_enabled&&!step);dir=0;return;}assert(p==19&&step);step=0;}
#define GPIO_UP(p) up(p)
#define GPIO_DOWN(p) down(p)
static void bk_gpio_set_output_low(unsigned pin){
 assert(pin==1);sync_high=0;
 if(cache.state==PL_ARMED){
  cache.state=PL_RUNNING;cache.fired=0;hc_fires=0;
  assert(timer_enabled);assert(pos==(dir?1790:8864));
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
#include "printer_buffered_motion.h"
static void printer_bidirectional_abort(void){printer_buffered_stop();boost=0;s_bidi_power=false;s_bidi_known=false;}
static void advance(unsigned us){
 uint64_t target=now_us+us;
 while(timer_enabled || cache.state==PL_RUNNING){
  uint64_t b=timer_enabled?next_bk:UINT64_MAX,h=cache.state==PL_RUNNING?next_hc:UINT64_MAX;
  uint64_t next=b<h?b:h;if(next>target)break;
  now_us=next;
  if(h<=b){
   if(mode!=4)assert(pos==(dir?1790+(int32_t)cache.fired*6:8864-(int32_t)cache.fired*6));
   ++cache.fired;++hc_fires;
   if(cache.fired==1180)cache.state=PL_DONE;else next_hc+=cache.period_us;
  }else{
   uint64_t epoch=next_bk;
   if(mode==4&&!irq_stall&&s_buf_steps==3000){irq_stall=1;next_bk+=1000;continue;}
   callback(4);if(timer_enabled)next_bk=epoch+timer_period;
  }
 }
 if(now_us<target)now_us=target;
}
int main(int argc,char**argv){
 assert(argc==2);mode=atoi(argv[1]);uint8_t*row=malloc(PL_ROW_BYTES);for(unsigned i=0;i<PL_ROW_BYTES;i++)row[i]=(i*7+i/100)&63U;
 int rc=printer_execute_buffered_line(row,1);
 if(!mode||mode==5){
  assert(!rc&&pos==10660&&s_bidi_right&&hc_fires==1180&&s_blank_spi_columns==1180&&cache.state==PL_DONE);
  assert(!memcmp(cache.data,row,PL_ROW_BYTES));
  assert(!printer_execute_buffered_line(row,0));assert(pos==0&&!s_bidi_right&&hc_fires==1180);
  assert(!printer_execute_buffered_line(NULL,1));assert(pos==10660&&s_blank_spi_columns==0);
  assert(!printer_execute_buffered_line(NULL,0));assert(pos==0);printer_buffered_stop();
 }else if(mode==4){assert(rc==-14&&!s_bidi_known&&s_buf_steps==3000&&!timer_enabled&&pos==3000);}
 else {assert(rc==(mode==3?-14:-13));assert(pos==0&&!s_blank_spi_steps[0]&&!s_blank_spi_steps[1]&&!timer_enabled&&!hc_fires);}
 free(row);puts("PASS real BK cache transport and timer engine: both directions, fixed HC column clock, worker stall, blank moves, bad peer/CRC/busy timer and late IRQ stop");
}
'''.replace('TRANSPORT',transport)
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.c').write_text(h);(p/'driver').mkdir();(p/'driver/timer.h').write_text('/* timer API provided by deterministic test platform */\n')
 subprocess.run(['cc','-std=c99','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(p),'-I'+str(D),'-I'+str(H),str(p/'test.c'),'-o',str(p/'test')],check=True)
 for mode in range(6):subprocess.run([str(p/'test'),str(mode)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

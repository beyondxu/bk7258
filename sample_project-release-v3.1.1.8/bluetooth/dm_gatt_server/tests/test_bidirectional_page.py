"""Actual page renderer/controller + fast motor engine: check ink at physical X in BOTH directions."""
from pathlib import Path
import subprocess,tempfile,sys,os
sys.path.insert(0,str(Path(__file__).resolve().parent))
from test_full_page import blob
D=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
h=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "printer_page.h"
#include "printer_service_geometry.h"
#define BK_OK 0
#define PRINTER_PAGE_BAND_ROWS 24U
#define PRINTER_PAGE_BIDIRECTIONAL 1
#define PRINTER_PAGE_COLOR_MODE 1
#define PRINTER_PAGE_TRANSFORM 0
#define PRINTER_PAGE_FEED_STEPS 3170U
#define PRINTER_PAGE_FEED_DIRECTION 0
#define PRINTER_PAGE_HEAD_SETTLE_MS 0
#define PRINTER_MAP_X_PITCH_CENTI_UM 3000
#define PRINTER_MAP_X_DIRECTION -1
#define PRINTER_MAP_FIRST_NOZZLE 0
#define PRINTER_MAP_REVERSE_Y 0
#define TPH_MOT_DIR 50
#define TPH_MOT_STEP 19
#define os_printf printf
static uint32_t s_blank_spi_columns,s_blank_spi_steps[2];
static uint64_t us,last_column_start,last_pulse_fall;
static unsigned leg_index,last_tx_us;
static int32_t pos;
static unsigned dir,step,head,boost,commands,scans,feed_count,spi_count,mode;
static unsigned expected_band, expected_bands=42;
static unsigned live,powerups,powerdowns;
static void*printer_map_psram_alloc(size_t n){void*p=malloc(n);if(p)++live;return p;}
static void printer_map_psram_free(void*p){if(p){assert(live);--live;free(p);}}
static void printer_map_yield_cpu(void){}
static bool printer_link_is_ready(void){return true;}
static uint64_t bk_aon_rtc_get_us(void){return mode==6?(us/32U)*32U:us;}
static void up(unsigned pin){
 if(pin==50){assert(!step);dir=1;leg_index=0;return;}
 assert(pin==19&&!step&&boost);if(leg_index)assert(us-last_pulse_fall>=(dir?230U:260U));++leg_index;step=1;
 if(head)assert(pos>=((int32_t)PRINTER_SERVICE_MARGIN_STEPS)&&pos<=((int32_t)PRINTER_SERVICE_MARGIN_STEPS+7480));
 pos+=dir?1:-1;assert(pos>=0&&pos<=((int32_t)PRINTER_SERVICE_SPAN_STEPS));
}
static void down(unsigned pin){if(pin==50){assert(!step);dir=0;leg_index=0;return;}assert(pin==19&&step);step=0;last_pulse_fall=us;}
#define GPIO_UP(p) up(p)
#define GPIO_DOWN(p) down(p)
static void delay_us(unsigned n){
 if(step)assert(n==4);
 else if(n==50)assert(leg_index==0);
 else {assert(leg_index);unsigned i=leg_index-1U,edge=i<(PRINTER_SERVICE_SPAN_STEPS-1U)-i?i:(PRINTER_SERVICE_SPAN_STEPS-1U)-i;unsigned ramp=dir?250U:400U,start=dir?1000U:1200U,cruise=dir?230U:260U;unsigned low=edge<ramp?start-(start-cruise)*edge/ramp:cruise;assert(n==low);}
 us+=n;
}
/* No explicit task sleep is allowed in the pulse loop. */
static void carriage_verified_power_up(void){assert(!head&&!step&&!boost);boost=1;++powerups;us+=75000;}
static void carriage_verified_power_down(void){assert(!head&&!step);boost=0;++powerdowns;}
static int tph_mcu_set_head_power(uint8_t on){
 ++commands;
 if((mode==2&&commands==2)||(mode==3&&commands==3))return -1;
 if(on)assert(pos==((int32_t)PRINTER_SERVICE_MARGIN_STEPS)||pos==((int32_t)PRINTER_SERVICE_MARGIN_STEPS+7480));
 head=on;return 0;
}
static int tph_fire_raw_column_bytes(const uint8_t*p){
 assert(head&&boost&&!step);
 assert(pos>=((int32_t)PRINTER_SERVICE_MARGIN_STEPS+200)&&pos<=((int32_t)PRINTER_SERVICE_MARGIN_STEPS+7274)&&(pos-((int32_t)PRINTER_SERVICE_MARGIN_STEPS+200))%6==0);
 unsigned physical_col=(pos-((int32_t)PRINTER_SERVICE_MARGIN_STEPS+200))/6;
 uint8_t expected[100]={0};
 unsigned rows=992U-expected_band*24U;if(rows>24)rows=24;
 printer_map_config_t cfg={3000,-1,0,0};
 for(unsigned ch=0;ch<3;ch++)for(unsigned row=0;row<rows;row++){
  printer_map_route_t route;assert(!printer_map_route(ch,row,&cfg,&route));
  unsigned x=(400U+ch)*1119U/1180U+route.delay_columns;
  if(x==physical_col)expected[route.slot]|=route.mask;
 }
 if(s_blank_spi_columns)assert(us-last_column_start==6U*(dir?234U:264U)+last_tx_us);
 last_column_start=us;
 assert(!memcmp(p,expected,100)); /* physical alignment, not merely reverse bytes */
 if(mode==1&&spi_count==10)return -1;
 ++spi_count;last_tx_us=mode==6?(s_blank_spi_columns%64==0?7157U:s_blank_spi_columns%8==0?813U:350U):350U;us+=last_tx_us;return 0;
}
static int motor_page_feed_fast_keep_power(uint32_t,uint8_t,uint32_t*);
#include "printer_bidirectional_print.h"
/* Inspect full motion BEFORE the real page controller can proceed to feed. */
static int execute_scan(const uint8_t*p,uint8_t direction){
 unsigned start_spi=spi_count;
 uint64_t start=us;
 assert(direction==((scans%expected_bands)%2==0?1:0));
 int rc=printer_execute_bidirectional_line(p,direction);
 if(!rc){assert(spi_count-start_spi==(p?1180U:0U));assert(boost);assert(pos==(direction?((int32_t)PRINTER_SERVICE_SPAN_STEPS):0));assert(us-start<4200000);++scans;}
 return rc;
}
#define printer_execute_bidirectional_line execute_scan
static int motor_page_feed_fast_keep_power(uint32_t steps,uint8_t direction,uint32_t*sent){
 assert(!head&&!step&&boost&&direction==0);
 unsigned i=feed_count%41;
 assert(steps==((i+1)*3170U)/8U-(i*3170U)/8U);
 if(mode==4){*sent=10;return -1;}
 ++feed_count;expected_band=(feed_count-1)%41+1;*sent=steps;return 0;
}
#include "printer_page_job.h"
int main(int argc,char**argv){
 assert(argc==3);mode=atoi(argv[2]);FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);uint8_t*b=malloc(n);assert(fread(b,1,n,f)==(size_t)n);fclose(f);
 int rc=printer_run_full_page(b,n);assert(!live&&!head&&!boost&&!step);
 if(!mode||mode==5||mode==6){assert(!rc&&scans==expected_bands&&feed_count==41&&pos==0&&powerups==1&&powerdowns==1);expected_band=0;assert(!printer_run_full_page(b,n));assert(scans==2*expected_bands&&feed_count==82&&pos==0&&powerups==2&&powerdowns==2);
 /* Odd number of scans: the final park is one head-OFF move, zero image calls. */
 printer_map_config_t cfg={3000,-1,0,0};printer_page_t page;
 assert(!printer_page_prepare(b,n,&cfg,&page,printer_map_psram_alloc,printer_map_psram_free,printer_map_yield_cpu));
 uint8_t *columns=malloc(118000),*scratch=malloc(page.width*72U);uint32_t dots[3];
 assert(!printer_page_render_band(&page,0,24,columns,118000,scratch,page.width*72U,dots,NULL));
 expected_band=0;assert(!execute_scan(columns,1));assert(pos==((int32_t)PRINTER_SERVICE_SPAN_STEPS));
 unsigned old_spi=spi_count;assert(!printer_bidirectional_finish());assert(pos==0&&spi_count==old_spi&&!head&&!boost);
 assert(!printer_bidirectional_finish());assert(pos==0&&spi_count==old_spi);
 free(columns);free(scratch);printer_page_release(&page,printer_map_psram_free);assert(!live);
 }
 else{assert(rc==(mode==1?-9:mode==4?-10:-8));unsigned pulses=s_blank_spi_steps[0]+s_blank_spi_steps[1];assert(printer_run_full_page(b,n)==-7);assert(pulses==s_blank_spi_steps[0]+s_blank_spi_steps[1]);if(mode==4)assert(pos==((int32_t)PRINTER_SERVICE_SPAN_STEPS)&&scans==1);else assert(!scans&&!feed_count);}
 free(b);puts("PASS: alternating directions, exact physical CMY positions, 24-row/final-band routing, fractional feeds, stop/lock after failure");
}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.c').write_text(h);(p/'fixture.bin').write_bytes(blob({(ch,400+ch,y)for ch in range(3)for y in range(1476)}))
 subprocess.run(['cc','-std=c99','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(D),str(p/'test.c'),str(D/'printer_mapper.c'),str(D/'printer_page.c'),'-o',str(p/'test')],check=True)
 (p/'sparse.bin').write_bytes(blob({(ch,400+ch,y)for ch in range(3)for y in range(1476)if (y*992//1476)//24 in [0,2,41]}))
 for mode in range(7):subprocess.run([str(p/'test'),str(p/('sparse.bin' if mode==5 else 'fixture.bin')),str(mode)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

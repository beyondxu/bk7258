"""Actual BIN rendering, frozen A/B orchestrator and real stream/power helpers.
Virtual monotonic clock; tests logical scheduling, not ink or physical rails.
"""
from pathlib import Path
import sys,tempfile,subprocess
P=Path(__file__).resolve().parents[1];D=P/'ap/app_bluetooth/gatt/printer'
sys.path.insert(0,str(P/'tests'));sys.path.insert(0,str(P/'tools'))
from test_full_page import blob
from page_preview import Renderer
s=(D/'hw_printer_stub.c').read_text();a=s.index('/* Separate comparison path:');b=s.index('void standalone_carriage_blank_spi_test',a);real=s[a:b]
assert '#if PRINTER_IDLE_AB\n#include "printer_idle_ab_job.h"' in s
assert 'return printer_run_idle_ab(bin, length);' in (D/'printer_page_job.h').read_text()
assert '#define PRINTER_IDLE_AB 1' in (D/'printer_diagnostics.h').read_text()
h=r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "printer_page.h"
#include "printer_audit_utils.h"
#define PRINTER_IDLE_AB 1
#define PRINTER_MAP_X_PITCH_CENTI_UM 3000
#define PRINTER_MAP_X_DIRECTION -1
#define PRINTER_MAP_FIRST_NOZZLE 0
#define PRINTER_MAP_REVERSE_Y 0
#define PRINTER_PAGE_TRANSFORM 0
#define PRINTER_PAGE_FEED_DIRECTION 0
#define PRINTER_PAGE_HEAD_SETTLE_MS 2000
#define PRINTER_PAGE_COLUMNS 1180U
#define TPH_COLUMN_BYTES 100
#define SHUTTLE_TOTAL_STEPS 7480
#define ACCEL_RAMP_STEPS 250
#define BK_OK 0
#define TPH_MOT_DIR 50
#define TPH_MOT_STEP 19
#define TPH_MOT_MS1 23
#define TPH_MOT_MS2 22
#define TPH_MOT_MS3 15
#define TPH_MOT_SLEEP 26
#define TPH_MOT_RES 21
#define TPH_MOT_EN 49
static uint8_t gold[118000],*frozen_ptr;
static unsigned live,oom,link=1,scans,feeds,head,boost,dir,step,ons,offs,boost_ons,boost_offs,gpio_inits,commands,legs,sends,corrupted;
static unsigned job_lines,job_feeds,run_number;
static int scenario;
static uint64_t us=100000,first_active[24],last_active[24];
static char phase[24],kind[24];
static unsigned line_index;
static char current_phase,current_kind;
static bool printer_link_is_ready(void){return link;}
static void *printer_map_psram_alloc(size_t n){if(oom&&n==118000)return NULL;void*p=malloc(n);if(p){++live;if(n==118000)frozen_ptr=p;}return p;}
static void printer_map_psram_free(void*p){assert(live);--live;free(p);}
static void printer_map_yield_cpu(void){}
static uint64_t bk_aon_rtc_get_us(void){return us;}
static void log_message(const char*format,...){
 char text[600];va_list ap;va_start(ap,format);vsnprintf(text,sizeof(text),format,ap);va_end(ap);
 if(strstr(text,"[IDLE_AB_LINE_BEGIN]")){
  char p[40],k[10];unsigned n;assert(sscanf(text,"[IDLE_AB_LINE_BEGIN] phase=%39s kind=%9s line=%u/3",p,k,&n)==3);
  current_phase=p[0];current_kind=k[0];line_index=scans;assert(line_index<24);phase[line_index]=p[0];kind[line_index]=k[0];assert(head&&boost&&live==1);
 }
 if(strstr(text,"state=head_OFF_boost_OFF"))assert(!head&&!boost);
 if(strstr(text,"state=head_ON"))assert(head&&boost);
 if(scenario==6&&!corrupted&&strstr(text,"state=head_ON_boost_ON_Ax0_Dx0")){assert(frozen_ptr);frozen_ptr[9]^=1;corrupted=1;}
}
#define os_printf log_message
static void up(unsigned p){if(p==50){dir=1;++legs;++scans;++job_lines;}if(p==19){assert(!step&&head&&boost);step=1;}}
static void down(unsigned p){if(p==50){dir=0;++legs;}if(p==19){assert(step);step=0;}}
#define GPIO_UP(p) up(p)
#define GPIO_DOWN(p) down(p)
static void delay_us(unsigned n){us+=n;}
static void rtos_delay_milliseconds(unsigned n){assert(!step);us+=(uint64_t)n*1000U;}
static int bk_motor_gpio_config_init(void){assert(!head);++gpio_inits;boost=0;return 0;}
static void bk_motor_boost_power(unsigned on){if(on){assert(!head);++boost_ons;}else{assert(!head);++boost_offs;}boost=on;}
static void bk_motor_set_state(unsigned motor,unsigned active){assert(motor==0&&!active);}
static int tph_mcu_set_head_power(unsigned on){
 ++commands;us+=12000;
 if(on){assert(boost&&!head);head=1;++ons;if(scenario==3&&ons==1)return -1;if(scenario==7&&ons==2)us+=500000;}
 else{head=0;++offs;if(scenario==4&&commands==8)return -1;}
 return 0;
}
/* These globals are introduced by REAL below, so no references to them here. */
static unsigned active_first,active_last;
static int tph_fire_raw_column_bytes(const uint8_t*p){
 assert(head&&boost&&dir&&!step);unsigned x=sends%1180;
 assert(!memcmp(p,gold+x*100,100));++sends;us+=200;
 if(scenario==10&&current_phase=='A'&&current_kind=='O'&&job_lines==4&&x==active_first)us+=200000;
 if(x==active_first)first_active[line_index]=us;
 if(x==active_last)last_active[line_index]=us;
 if((scenario==1&&sends==11)||(scenario==2&&job_lines==10&&x==10))return -1;
 return 0;
}
static int motor_page_feed_keep_power(uint32_t n,uint8_t d,uint32_t*sent){
 assert(head&&boost&&!step&&d==0);assert(n==780||n==1560);assert(job_lines==job_feeds+1);++feeds;++job_feeds;
 us+=1800000;*sent=scenario==5&&feeds==1?n-1:n;return scenario==12&&feeds==1?-1:0;
}
REAL
#include "printer_idle_ab_job.h"
static uint8_t* readfile(const char*path,uint32_t*n){FILE*f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);*n=ftell(f);rewind(f);uint8_t*p=malloc(*n);assert(fread(p,1,*n,f)==*n);fclose(f);return p;}
static void check_run(unsigned offset,bool reverse){
 for(unsigned p=0;p<2;++p){unsigned j=offset+p*6;char expected=(p^(unsigned)reverse)?'B':'A';
  for(unsigned i=0;i<6;++i){assert(phase[j+i]==expected&&kind[j+i]==(i<3?'R':'O'));}
  uint64_t gap=first_active[j+3]-last_active[j+2];assert(gap>=59900000ULL&&gap<=60100000ULL);
 }
}
int main(int argc,char**argv){
 assert(argc==6);scenario=atoi(argv[5]);uint32_t n,z,e,alt_n;uint8_t*b=readfile(argv[1],&n),*blank=readfile(argv[2],&z),*expected=readfile(argv[3],&e),*alt=readfile(argv[4],&alt_n);assert(e==118000);memcpy(gold,expected,e);free(expected);
 active_first=1180;for(unsigned x=0;x<1180;++x){unsigned any=0;for(unsigned i=0;i<100;++i)any|=gold[x*100+i];if(any){if(active_first==1180)active_first=x;active_last=x;}}
 assert(printer_run_idle_ab(b,n-1));assert(!scans&&!feeds&&!live&&!head&&!boost);
 assert(printer_run_idle_ab(blank,z));assert(!scans&&!feeds&&!live&&!head&&!boost);
 oom=1;assert(printer_run_idle_ab(b,n));assert(!scans&&!feeds&&!live&&!head&&!boost);oom=0;
 link=0;assert(printer_run_idle_ab(b,n));assert(!scans&&!feeds&&!live&&!head&&!boost);link=1;
 int rc=printer_run_idle_ab(b,n);
 assert(!head&&!boost&&!step);
 if(!scenario||scenario==8){
  assert(!rc&&scans==12&&feeds==12&&sends==12*1180&&live==1);check_run(0,false);
  assert(ons==3&&offs==5&&gpio_inits==2&&boost_ons==2&&boost_offs==2); // no B idle reset/toggle
  if(scenario==8){frozen_ptr[0]^=1;unsigned old=scans;assert(printer_run_idle_ab(alt,alt_n)==-11);assert(scans==old&&!live);assert(printer_run_idle_ab(b,n)==-7);}
  else{job_lines=job_feeds=0;++run_number;assert(!printer_run_idle_ab(alt,alt_n));assert(scans==24&&feeds==24&&sends==24*1180&&live==1);check_run(12,true);assert(ons==6&&gpio_inits==4&&boost_ons==4&&boost_offs==4);}
 }else{
  assert(rc&&!live);unsigned old=scans;assert(printer_run_idle_ab(b,n)==-7&&scans==old);
  if(scenario==1)assert(scans==1&&legs==1&&!feeds);
  if(scenario==2)assert(scans==10&&legs==19&&feeds==9);
  if(scenario==3)assert(!scans&&!feeds);
  if(scenario==4)assert(scans==12&&feeds==12);
  if(scenario==5||scenario==12)assert(scans==1&&feeds==1);
  if(scenario==6)assert(scans==9&&feeds==9);
  if(scenario==7)assert(scans==3&&feeds==3);
  if(scenario==10)assert(scans==4&&feeds==3);
 }
 free(alt);free(blank);free(b);
}
'''.replace('\nREAL\n','\n'+real+'\n')
def run_scenarios(harness, scenarios):
 with tempfile.TemporaryDirectory() as tmp:
  t=Path(tmp);(t/'test.c').write_text(harness)
  points={(ch,x,y) for ch in range(3) for x in range(100,1000) for y in range(1225,1237)}
  data=blob(points);(t/'input.bin').write_bytes(data);(t/'blank.bin').write_bytes(blob(set()))
  renderer=Renderer();code,page=renderer.prepare(data);assert not code;gold,_=renderer.render(page,4);renderer.close(page);(t/'gold.bin').write_bytes(gold)
  alt=blob({(ch,x,y) for ch in range(3) for x in range(150,1050) for y in range(1340,1352)});(t/'alt.bin').write_bytes(alt)
  subprocess.run(['cc','-std=c99','-O1','-Wall','-Wextra','-Werror','-I'+str(D),str(t/'test.c'),str(D/'printer_page.c'),str(D/'printer_mapper.c'),'-o',str(t/'test')],check=True)
  arm=Path('/workspace/.setup/tools/usr/bin/arm-none-eabi-gcc')
  if arm.exists():subprocess.run([str(arm),'-std=gnu99','-mcpu=cortex-m33+nodsp','-mfpu=fpv5-sp-d16','-mfloat-abi=hard','-mcmse','-O2','-Wall','-Wextra','-Werror','-I'+str(D),'-c',str(t/'test.c'),'-o',str(t/'test.o')],check=True)
  for scenario in scenarios:
   subprocess.run([str(t/'test'),str(t/'input.bin'),str(t/'blank.bin'),str(t/'gold.bin'),str(t/'alt.bin'),str(scenario)],check=True)

if __name__ == '__main__':
 run_scenarios(h, [0,1,2,3,4,5,6,7,8,10,12])
 print('PASS: real render/frozen bytes, AB+BA, 60s active-ACK scheduling, 12 scans/feeds, B power continuity, native and ARM checks, rejection/OOM/SPI/power/feed/corruption/timing failure stops')

"""Actual yellow page controller: exact mapped bytes, feed placement and failure guards."""
from pathlib import Path
import os,subprocess,tempfile,sys,re,zlib
from test_full_page import blob,Renderer
R=Path(__file__).resolve().parents[1];D=R/'ap/app_bluetooth/gatt/printer'
h=r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "printer_page.h"
#define BK_OK 0
#define PRINTER_APP_YELLOW_ONLY 1
#define PRINTER_IDLE_AB 1
#define PRINTER_PAGE_COLOR_MODE 1
#define PRINTER_PAGE_TRANSFORM 0
#define PRINTER_MAP_X_PITCH_CENTI_UM 3000
#define PRINTER_MAP_X_DIRECTION -1
#define PRINTER_MAP_FIRST_NOZZLE 0
#define PRINTER_MAP_REVERSE_Y 0
#define PRINTER_PAGE_FEED_STEPS 3170U
#define PRINTER_PAGE_FEED_DIRECTION 0
static unsigned live,calls,fail_at,scans,feeds,current,off_calls;
static int mode;
static unsigned char *gold;
static bool printer_link_is_ready(void){return mode!=4;}
static void *printer_map_psram_alloc(size_t n){if(++calls==fail_at)return NULL;void*p=malloc(n);if(p)++live;return p;}
static void printer_map_psram_free(void*p){if(p){assert(live);--live;free(p);}}
static void printer_map_yield_cpu(void){}
int printer_run_idle_ab(const uint8_t*p,uint32_t n){(void)p;(void)n;assert(!"fixed recipe called");return -1;}
static void audit_log(const char*f,...){char b[500];va_list a;va_start(a,f);vsnprintf(b,sizeof(b),f,a);va_end(a);if(strstr(b,"[PAGE_SWATH]")){assert(sscanf(b,"[PAGE_SWATH] index=%u/",&current)==1);}}
#define os_printf audit_log
static int printer_execute_cyan_line(const uint8_t*p){
 assert(current>=1&&current<=6);assert(feeds==current-1);assert(!memcmp(p,gold+(current-1)*118000,118000));
 unsigned any=0;for(unsigned i=0;i<118000;i++){assert(!(p[i]&~0x30));any|=p[i];}assert(any);++scans;return mode==1?-9:0;
}
static int tph_mcu_set_head_power(uint8_t on){assert(!on);++off_calls;return mode==3?-1:0;}
static int motor_page_feed(uint32_t steps,uint8_t d,uint32_t*sent){assert(steps==3170&&!d&&current==feeds+1);++feeds;*sent=mode==2?0:steps;return mode==2?-1:0;}
#include "printer_page_job.h"
static uint8_t *readfile(const char*p,size_t*n){FILE*f=fopen(p,"rb");assert(f);fseek(f,0,SEEK_END);*n=ftell(f);rewind(f);uint8_t*b=malloc(*n);assert(fread(b,1,*n,f)==*n);fclose(f);return b;}
int main(int argc,char**argv){assert(argc==7);size_t n,g;uint8_t*b=readfile(argv[1],&n);gold=readfile(argv[2],&g);mode=atoi(argv[3]);fail_at=atoi(argv[4]);int expected=atoi(argv[5]);unsigned expected_scans=atoi(argv[6]);
 int rc=printer_run_full_page(b,n);assert(rc==expected&&!live);
 if(!rc){assert(scans==expected_scans&&feeds==5&&off_calls==5);}
 else if(rc==-9||rc==-10||rc==-8){unsigned old_scans=scans,old_feeds=feeds;assert(printer_run_full_page(b,n)==-7);assert(scans==old_scans&&feeds==old_feeds);}
 else assert(!scans&&!feeds&&!off_calls);
 free(gold);free(b);puts("PASS yellow controller scenario");}
'''
r=Renderer()
inputs=[blob({(0,300,100),(1,400,400),(2,500,600),(2,600,800)})]
if len(sys.argv)>1:inputs.append(Path(sys.argv[1]).read_bytes())
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(h)
 subprocess.run(['cc','-std=c99','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(D),str(p/'test.c'),str(D/'printer_page.c'),str(D/'printer_mapper.c'),'-o',str(p/'test')],check=True)
 for raw in inputs:
  code,page=r.prepare(raw);assert code==0
  frames=[]
  for sw in range(page.swaths):
   columns,dots=r.render(page,sw);frames.append(bytes(v&0x30 for v in columns))
  assert len(frames)==6
  active=sum(any(f) for f in frames);assert active
  print('gold active_swaths',[(i+1,f'{zlib.crc32(f):08X}',sum(v.bit_count() for v in f)) for i,f in enumerate(frames) if any(f)],flush=True)
  r.close(page);(p/'input.bin').write_bytes(raw);(p/'gold.bin').write_bytes(b''.join(frames))
  for mode,expected in [(0,0),(1,-9),(2,-10),(3,-8),(4,-7)]:
   subprocess.run([str(p/'test'),str(p/'input.bin'),str(p/'gold.bin'),str(mode),'0',str(expected),str(active)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
  for fail in range(1,7):
   subprocess.run([str(p/'test'),str(p/'input.bin'),str(p/'gold.bin'),'0',str(fail),'-2',str(active)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
 (p/'input.bin').write_bytes(blob({(0,300,100),(1,400,400)}))
 subprocess.run([str(p/'test'),str(p/'input.bin'),str(p/'gold.bin'),'0','0','-6','0'],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
print('PASS: actual APP yellow only, original A/S/X/strip bytes, no C/M or fixed recipe, preserved blank feeds, empty-yellow rejection, preflight/OOM and motion/feed/head/link failures')

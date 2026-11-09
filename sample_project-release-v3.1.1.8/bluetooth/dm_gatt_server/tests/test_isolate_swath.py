"""Original selected-swath bytes are the first physical scan, without feeds or other scans."""
from pathlib import Path
import subprocess,tempfile,sys
ROOT=Path(__file__).resolve().parents[1];SRC=ROOT/'ap/app_bluetooth/gatt/printer'
sys.path.insert(0,str(ROOT/'tools'));sys.path.insert(0,str(ROOT/'tests'))
from page_preview import Renderer
from test_full_page import blob
h=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "printer_page.h"
#define PRINTER_MAP_X_PITCH_CENTI_UM 3000
#define PRINTER_MAP_X_DIRECTION -1
#define PRINTER_MAP_FIRST_NOZZLE 0
#define PRINTER_MAP_REVERSE_Y 0
#define PRINTER_PAGE_TRANSFORM 0
#define PRINTER_PAGE_COLOR_MODE 1
#define PRINTER_PAGE_FEED_STEPS 3170
#define PRINTER_PAGE_FEED_DIRECTION 0
#define BK_OK 0
#define os_printf printf
static unsigned moves,live;static int fail,oom;static uint8_t expected[118000];
static void *printer_map_psram_alloc(size_t n){if(oom)return NULL;void*p=malloc(n);if(p)++live;return p;}
static void printer_map_psram_free(void*p){assert(live);--live;free(p);}
static void printer_map_yield_cpu(void){}
static bool printer_link_is_ready(void){return true;}
#if PRINTER_PAGE_REPEAT_SAME_POWER
#define printer_execute_cyan_line printer_execute_same_power_repeat
#endif
static int printer_execute_cyan_line(const uint8_t*p){assert(!memcmp(p,expected,118000));++moves;return fail?-9:0;}
static int tph_mcu_set_head_power(unsigned on){(void)on;abort();}
static int motor_page_feed(uint32_t n,uint8_t dir,uint32_t*sent){(void)n;(void)dir;(void)sent;abort();}
#include "printer_page_job.h"
static uint8_t*readfile(const char*name,unsigned*n){FILE*f=fopen(name,"rb");assert(f);fseek(f,0,SEEK_END);*n=ftell(f);rewind(f);uint8_t*p=malloc(*n);assert(fread(p,1,*n,f)==*n);fclose(f);return p;}
int main(int argc,char**argv){
 assert(argc==5);unsigned n,z,e;uint8_t*b=readfile(argv[1],&n),*blank=readfile(argv[2],&z),*gold=readfile(argv[3],&e);assert(e==118000);memcpy(expected,gold,e);free(gold);fail=atoi(argv[4]);
 assert(printer_run_full_page(b,n-1));assert(!moves&&!live);
 oom=1;assert(printer_run_full_page(b,n));assert(!moves&&!live);oom=0;
 assert(printer_run_full_page(blank,z));assert(!moves&&!live);
 int rc=printer_run_full_page(b,n);assert(!live);
 if(PRINTER_PAGE_ONLY_SWATH>=6){assert(rc&&!moves);}
 else if(fail){assert(rc&&moves==1);assert(printer_run_full_page(b,n)&&moves==1);}
 else{assert(!rc&&moves==1);assert(!printer_run_full_page(b,n));assert(moves==2);}
 assert(!live);free(blank);free(b);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(h)
 points={(ch,100+80*(y//192),y) for ch in range(3) for y in range(1476)}
 data=blob(points);(p/'input.bin').write_bytes(data);(p/'blank.bin').write_bytes(blob(set()))
 renderer=Renderer();code,page=renderer.prepare(data);assert code==0
 golden,_=renderer.render(page,4);renderer.close(page);(p/'golden.bin').write_bytes(golden)
 for selected,repeat in [(4,0),(6,0),(4,1)]:
  subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',f'-DPRINTER_PAGE_ONLY_SWATH={selected}',f'-DPRINTER_PAGE_REPEAT_SAME_POWER={repeat}','-I'+str(SRC),str(p/'test.c'),str(SRC/'printer_page.c'),str(SRC/'printer_mapper.c'),'-o',str(p/'test')],check=True)
  for fail in [0,1]:subprocess.run([str(p/'test'),str(p/'input.bin'),str(p/'blank.bin'),str(p/'golden.bin'),str(fail)],check=True,stdout=subprocess.DEVNULL)
 print('PASS: selected swath matches original 118000 bytes, no feed/other scan, empty/out-of-range/OOM rejection, failure lock and successful repeat')

"""Actual full-page decoder/render/orchestrator with fake physical devices."""
from pathlib import Path
import subprocess,tempfile,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(Path(__file__).resolve().parent))
from test_full_page import blob
src=ROOT/'ap/app_bluetooth/gatt/printer'
h=r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "printer_page.h"
#define BK_OK 0
#define PRINTER_MAP_X_PITCH_CENTI_UM 3000
#define PRINTER_MAP_X_DIRECTION -1
#define PRINTER_MAP_FIRST_NOZZLE 0
#define PRINTER_MAP_REVERSE_Y 0
#define PRINTER_PAGE_FEED_STEPS 2340U
#define PRINTER_PAGE_TRANSFORM 0
#define PRINTER_PAGE_FEED_DIRECTION 0
#define os_printf printf
static unsigned moves,feeds,off_calls,alloc_calls,live;
static int fail_motion,fail_feed,fail_allocate=-1;
static void *printer_map_psram_alloc(size_t n){if((int)alloc_calls++==fail_allocate)return NULL;void*p=malloc(n);if(p)++live;return p;}
static void printer_map_psram_free(void*p){if(p){assert(live);--live;free(p);}}
static void printer_map_yield_cpu(void){}
static bool printer_link_is_ready(void){return true;}
static int printer_execute_cyan_line(const uint8_t *columns){assert(columns);for(unsigned i=0;i<118000;i++)assert(!(columns[i]&0xC0));++moves;return fail_motion?-9:0;}
static int tph_mcu_set_head_power(uint8_t on){assert(!on);++off_calls;return 0;}
static int motor_page_feed(uint32_t steps,uint8_t direction,uint32_t*sent){assert(steps==2340&&direction==0&&off_calls==feeds+1);++feeds;*sent=fail_feed?10:steps;return fail_feed?-1:0;}
#include "printer_page_job.h"
int main(int argc,char**argv){
 assert(argc==3);FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);uint8_t*b=malloc(n);assert(fread(b,1,n,f)==(size_t)n);fclose(f);
 int mode=atoi(argv[2]);if(mode==1)fail_motion=1;if(mode==2)fail_feed=1;if(mode==3)fail_allocate=4;
 int result=printer_run_full_page(b,n);assert(live==0);
 if(mode==0){assert(!result&&moves==6&&feeds==5);assert(!printer_run_full_page(b,n));assert(moves==12&&feeds==10);}
 if(mode==1){assert(result==-9&&moves==1&&feeds==0);assert(printer_run_full_page(b,n)==-7);assert(moves==1);}
 if(mode==2){assert(result==-10&&moves==1&&feeds==1);assert(printer_run_full_page(b,n)==-7);assert(moves==1);}
 if(mode==3){assert(result==-2&&!moves&&!feeds);fail_allocate=-1;assert(!printer_run_full_page(b,n));assert(moves==6&&feeds==5);}
 assert(live==0);free(b);puts("PASS page orchestration");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(h);(p/'fixture.bin').write_bytes(blob({(ch,400+ch,y) for ch in range(3) for y in range(1476)}))
 subprocess.run(['cc','-std=c99','-O2','-Wall','-Wextra','-Werror','-I',str(src),str(p/'test.c'),str(src/'printer_mapper.c'),str(src/'printer_page.c'),'-o',str(p/'test')],check=True)
 for mode in range(4):subprocess.run([str(p/'test'),str(p/'fixture.bin'),str(mode)],check=True)

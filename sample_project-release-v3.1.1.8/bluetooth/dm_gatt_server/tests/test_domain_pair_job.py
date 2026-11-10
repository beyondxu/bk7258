"""Real frozen input and fixed-address D1/D2 cases, order reversal and failures."""
from pathlib import Path
import sys,tempfile,subprocess
P=Path(__file__).resolve().parents[1];D=P/'ap/app_bluetooth/gatt/printer'
sys.path.insert(0,str(P/'tests'));sys.path.insert(0,str(P/'tools'))
from test_full_page import blob
from page_preview import Renderer
h=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "printer_page.h"
#include "printer_audit_utils.h"
#define PRINTER_MAP_X_PITCH_CENTI_UM 3000
#define PRINTER_MAP_X_DIRECTION -1
#define PRINTER_MAP_FIRST_NOZZLE 0
#define PRINTER_MAP_REVERSE_Y 0
#define PRINTER_PAGE_FEED_STEPS 3170
#define PRINTER_PAGE_FEED_DIRECTION 0
#define PRINTER_PAGE_TRANSFORM 0
#define PRINTER_PAGE_COLOR_MODE 1
#define PRINTER_DOMAIN_PAIR 1
#define BK_OK 0
#define os_printf printf
static unsigned scans,feeds,offs,live,buffer_allocs,groups;static uint8_t*frozen_pointer;static int scenario,oom,link=1;static uint8_t gold[118000];
static bool printer_link_is_ready(void){return link;}
static void*printer_map_psram_alloc(size_t n){if(oom&&n==118000)return NULL;void*p=malloc(n);if(p){++live;if(n==118000&&++buffer_allocs==2)frozen_pointer=p;}return p;}
static void printer_map_psram_free(void*p){assert(live);--live;free(p);}
static void printer_map_yield_cpu(void){}
static int printer_execute_cyan_line(const uint8_t*p){
 assert(feeds==scans&&live==2);
 if(scans==0||scans==7)assert(!memcmp(p,gold,118000));
 else {unsigned which=(groups&1U)?7U-scans:scans,addr=which<=3?2:5,mask=1+(which-1)%3,slot=(addr-1)*4+1;
 for(unsigned x=0;x<1180;++x)for(unsigned s=0;s<100;++s){uint8_t expected=(s==slot&&(gold[x*100+5]&2))?mask:0;assert(p[x*100+s]==expected);}}
 ++scans;if((scenario==1&&scans==1)||(scenario==2&&scans==3)||(scenario==3&&scans==8))return -9;
 if(scenario==7)((uint8_t*)p)[0]^=1;
 return 0;
}
static int tph_mcu_set_head_power(unsigned on){assert(!on);++offs;return scenario==6?-1:0;}
static int motor_page_feed(uint32_t n,uint8_t dir,uint32_t*sent){assert(n==(scans<8?780:1560)&&!dir);assert(scans==feeds+1);++feeds;*sent=(scenario==5||(scenario==10&&scans==8))?n-1:n;return scenario==4?-1:0;}
#include "printer_domain_pair_job.h"
#include "printer_page_job.h"
static uint8_t*readfile(const char*path,uint32_t*n){FILE*f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);*n=ftell(f);rewind(f);uint8_t*b=malloc(*n);assert(fread(b,1,*n,f)==*n);fclose(f);return b;}
int main(int argc,char**argv){assert(argc==6);scenario=atoi(argv[5]);uint32_t n,z,e,alt_n;uint8_t*b=readfile(argv[1],&n),*blank=readfile(argv[2],&z),*expected=readfile(argv[3],&e);assert(e==118000);memcpy(gold,expected,e);free(expected);uint8_t*alt=readfile(argv[4],&alt_n);
 assert(printer_run_full_page(b,n-1));assert(!scans&&!feeds&&!live);
 assert(printer_run_full_page(blank,z));assert(!scans&&!feeds&&!live);
 oom=1;assert(printer_run_full_page(b,n));assert(!scans&&!feeds&&!live);oom=0;
 link=0;assert(printer_run_full_page(b,n));assert(!scans&&!feeds&&!live);link=1;
 buffer_allocs=0;int result=printer_run_full_page(b,n);
 if(!scenario||scenario==8||scenario==9){assert(!result&&scans==8&&feeds==8&&offs==8&&live==1);++groups;scans=feeds=offs=0;
 if(scenario==8){assert(frozen_pointer);frozen_pointer[0]^=1;assert(printer_run_full_page(alt,alt_n)==-11);assert(!scans&&!feeds&&!live);assert(printer_run_full_page(b,n)==-7);}
 else {if(scenario==9){oom=1;assert(printer_run_full_page(alt,alt_n));assert(!scans&&!feeds&&live==1);oom=0;}
 /* Changed APP pixels and nozzle rows must still print the original frozen plan. */
 for(unsigned repeat=0;repeat<3;++repeat){assert(!printer_run_full_page(alt,alt_n));assert(scans==8&&feeds==8&&live==1);++groups;scans=feeds=offs=0;}}}
 else {assert(result&&!live);unsigned before=scans;assert(printer_run_full_page(b,n));assert(scans==before&&!live);if(scenario==1||scenario==7)assert(scans==1&&!feeds);if(scenario==2)assert(scans==3&&feeds==2);if(scenario==3)assert(scans==8&&feeds==7);if(scenario==4||scenario==5)assert(scans==1&&feeds==1);if(scenario==6)assert(scans==1&&!feeds);if(scenario==10)assert(scans==8&&feeds==8);}
 free(alt);free(blank);free(b);}
'''
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'test.c').write_text(h)
 points={(ch,x,y) for ch in range(3) for x in range(100,1000) for y in range(1225,1237)}
 data=blob(points);(t/'input.bin').write_bytes(data);(t/'blank.bin').write_bytes(blob(set()))
 renderer=Renderer();code,page=renderer.prepare(data);assert not code;gold,_=renderer.render(page,4);renderer.close(page);(t/'gold.bin').write_bytes(gold);assert sum(bool(gold[x*100+5]&2) for x in range(1180))>=256
 alt_points={(ch,x,y) for ch in range(3) for x in range(150,1050) for y in range(1340,1352)}
 alt_data=blob(alt_points);(t/'alt.bin').write_bytes(alt_data)
 code,page=renderer.prepare(alt_data);assert not code;alt_gold,_=renderer.render(page,4);renderer.close(page);assert alt_gold!=gold
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-I'+str(D),str(t/'test.c'),str(D/'printer_page.c'),str(D/'printer_mapper.c'),'-o',str(t/'test')],check=True)
 arm=Path('/workspace/.setup/tools/usr/bin/arm-none-eabi-gcc')
 if arm.exists():subprocess.run([str(arm),'-std=gnu99','-mcpu=cortex-m33+nodsp','-mfpu=fpv5-sp-d16','-mfloat-abi=hard','-mcmse','-O2','-Wall','-Wextra','-Werror','-I'+str(D),'-c',str(t/'test.c'),'-o',str(t/'test.o')],check=True)
 for scenario in range(11):subprocess.run([str(t/'test'),str(t/'input.bin'),str(t/'blank.bin'),str(t/'gold.bin'),str(t/'alt.bin'),str(scenario)],check=True,stdout=subprocess.DEVNULL)
print('PASS: actual BIN/template and original CMY controls, six exact D1/D2/03 frames at both addresses/S2, frozen data with forward/reverse ordering across changed inputs, bounded cache, all failure stops and ARM object check')

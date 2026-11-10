"""Real BIN validation, planner and job controller: bounds, coverage and stop behavior."""
from pathlib import Path
import subprocess,tempfile,sys
P=Path(__file__).resolve().parents[1];D=P/'ap/app_bluetooth/gatt/printer'
sys.path.insert(0,str(P/'tests'));from test_full_page import blob
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
#define BK_OK 0
#define os_printf printf
static unsigned scans,feeds,offs,live,selection[100][6];static int scenario,oom,link=1;
static bool printer_link_is_ready(void){return link;}
static void*printer_map_psram_alloc(size_t n){if(oom&&n==118000)return NULL;void*p=malloc(n);if(p)++live;return p;}
static void printer_map_psram_free(void*p){assert(live);--live;free(p);}
static void printer_map_yield_cpu(void){}
static int printer_execute_cyan_line(const uint8_t*p){
 assert(feeds==(scans/4)*2);unsigned ch=scans/4;unsigned bits=0;
 for(unsigned x=0;x<1180;++x){unsigned active=0;for(unsigned s=0;s<100;++s){assert(!(p[x*100+s]&0xC0));for(unsigned b=0;b<6;++b)if(p[x*100+s]&(1U<<b)){assert(b/2==ch);++active;++selection[s][b];}}assert(active<=1);bits+=active;}
 assert(bits==800);++scans;
 if(scenario==1&&scans==1)return -9;
 if(scenario==2&&scans==5)return -9;
 if(scenario==3&&scans==12)return -9;
 if(scenario==7)((uint8_t*)p)[0]^=1;
 return 0;
}
static int tph_mcu_set_head_power(unsigned on){assert(!on);++offs;return scenario==6?-1:0;}
static int motor_page_feed(uint32_t n,uint8_t dir,uint32_t*sent){assert(n==3170&&!dir);assert(scans==4||scans==8);++feeds;*sent=scenario==5?3000:n;return scenario==4?-1:0;}
#include "printer_nozzle_check_job.h"
#define PRINTER_NOZZLE_CHECK 1
#define PRINTER_PAGE_ONLY_SWATH (-1)
#define PRINTER_PAGE_REPEAT_SAME_POWER 0
#define PRINTER_PAGE_COLOR_MODE 1
#define PRINTER_PAGE_TRANSFORM 0
#include "printer_page_job.h"
int main(int argc,char**argv){
 assert(argc==3);scenario=atoi(argv[2]);FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);uint32_t n=ftell(f);rewind(f);uint8_t*b=malloc(n);assert(fread(b,1,n,f)==n);fclose(f);
 assert(printer_run_full_page(b,n-1));assert(!scans&&!feeds&&!live);
 oom=1;assert(printer_run_full_page(b,n));assert(!scans&&!feeds&&!live);oom=0;
 link=0;assert(printer_run_full_page(b,n));assert(!scans&&!feeds&&!live);link=1;
 int result=printer_run_full_page(b,n);assert(!live);
 if(!scenario){assert(!result&&scans==12&&feeds==4&&offs==4);for(unsigned s=0;s<100;++s)for(unsigned bit=0;bit<6;++bit)assert(selection[s][bit]==16);scans=feeds=offs=0;memset(selection,0,sizeof(selection));assert(!printer_run_full_page(b,n));assert(scans==12&&feeds==4&&offs==4&&!live);for(unsigned s=0;s<100;++s)for(unsigned bit=0;bit<6;++bit)assert(selection[s][bit]==16);}
 else{assert(result);unsigned before=scans;assert(printer_run_full_page(b,n));assert(scans==before&&!live);if(scenario==1)assert(scans==1&&!feeds);if(scenario==2)assert(scans==5&&feeds==2);if(scenario==3)assert(scans==12&&feeds==4);if(scenario==4||scenario==5)assert(scans==4&&feeds==1);if(scenario==6)assert(scans==4&&!feeds);if(scenario==7)assert(scans==1&&!feeds);}
 free(b);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'test.c').write_text(h);(t/'input.bin').write_bytes(blob({(0,100,100)}))
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-I'+str(D),str(t/'test.c'),str(D/'printer_mapper.c'),str(D/'printer_page.c'),'-o',str(t/'test')],check=True)
 arm=Path('/workspace/.setup/tools/usr/bin/arm-none-eabi-gcc')
 if arm.exists():subprocess.run([str(arm),'-std=gnu99','-mcpu=cortex-m33+nodsp','-mfpu=fpv5-sp-d16','-mfloat-abi=hard','-mcmse','-O2','-Wall','-Wextra','-Werror','-I'+str(D),'-c',str(t/'test.c'),'-o',str(t/'test.o')],check=True)
 for scenario in range(8):subprocess.run([str(t/'test'),str(t/'input.bin'),str(scenario)],check=True,stdout=subprocess.DEVNULL)
print('PASS: malformed BIN/link/OOM rejection before motion, 12 ordered scans, 4 feeds, all 600 controls exactly16 times, early/mid/last scan failures, OFF/feed/short-feed/CRC failures stop and lock')

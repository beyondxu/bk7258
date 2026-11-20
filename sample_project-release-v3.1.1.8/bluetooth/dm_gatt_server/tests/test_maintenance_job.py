"""Actual paper-maintenance orchestration: route, sequence, direction and failures."""
from pathlib import Path
import subprocess,tempfile,os
D=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
h=r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "printer_maintenance_pattern.h"
#include "printer_audit_utils.h"
#include "printer_canvas_geometry.h"
#define BK_OK 0
#define PRINTER_MAP_X_PITCH_CENTI_UM PRINTER_CANVAS_X_PITCH_CENTI_UM
#define PRINTER_MAP_X_DIRECTION -1
#define PRINTER_MAP_FIRST_NOZZLE (fault==7?1U:0U)
#define PRINTER_MAP_REVERSE_Y 0
#define PRINTER_PAGE_FEED_STEPS (fault==15?5000U:3170U)
#define PRINTER_PAGE_FEED_DIRECTION 0
#define PRINTER_STARTUP_WIPE_STEPS 3740U
#define PRINTER_STARTUP_WIPE_ROUNDS 2U
#define PRINTER_OC3_RETURNED (-15)
#define os_printf printf
static unsigned fault,mode,scans,feeds,prep_calls,aborts,finishes,parks,wipes,allocs,paper;
static bool s_bidi_known=true,s_bidi_right=false,s_bidi_at_wiper=true;
static bool printer_link_is_ready(void){return fault!=6;}
static void *printer_map_psram_alloc(size_t n){if(fault==5)return NULL;++allocs;return malloc(n);}
static void printer_map_psram_free(void*p){assert(allocs);--allocs;free(p);}
static void printer_map_yield_cpu(void){}
static int printer_startup_prepare(unsigned page){assert(page&&s_bidi_at_wiper);++prep_calls;if(fault==2)return -10;paper=4212;s_bidi_at_wiper=false;return 0;}
static int printer_execute_bidirectional_line(const uint8_t*row,uint8_t dir){
 assert(s_bidi_known&&dir==!s_bidi_right&&!s_bidi_at_wiper);
 unsigned phases=mode?3:1,ch=scans/(4*phases),group=(scans/phases)%4,phase=scans%phases;
 uint8_t*gold=malloc(118000);printer_map_config_t cfg={PRINTER_MAP_X_PITCH_CENTI_UM,-1,0,0};
 assert(!printer_maint_build(gold,118000,mode,ch,group,phase,&cfg));if(!dir)printer_maint_reverse(gold);
 assert(!memcmp(row,gold,118000));free(gold);++scans;
 if(fault==3&&scans==5)return -13;
 if(fault==13&&scans==5){s_bidi_right=false;s_bidi_at_wiper=true;return PRINTER_OC3_RETURNED;}
 s_bidi_right=dir;return 0;
}
static int tph_mcu_set_head_power(unsigned on){assert(!on);return 0;}
static int printer_bidirectional_feed(uint32_t steps,uint8_t dir,uint32_t*sent){
 assert(!dir&&steps==3302U&&feeds<2);assert(scans==(feeds+1)*4U*(mode?3U:1U));
 ++feeds;*sent=fault==4?steps-1:steps;paper+=*sent;return fault==4?-1:0;
}
static int printer_buffered_park_at_wiper(void){++parks;if(fault==10)return -14;s_bidi_at_wiper=true;s_bidi_right=false;return 0;}
static int printer_buffered_wipe(uint32_t steps,uint32_t rounds){assert(s_bidi_at_wiper&&!s_bidi_right&&steps==3740&&rounds==2);++wipes;return fault==9?-14:0;}
static int printer_bidirectional_finish(void){assert(s_bidi_at_wiper);++finishes;return fault==11?-14:0;}
static void printer_bidirectional_abort(void){++aborts;s_bidi_known=false;}
#include "printer_maintenance_job.h"
int main(int argc,char**argv){
 assert(argc==3);mode=atoi(argv[1]);fault=atoi(argv[2]);if(fault==8)s_bidi_known=false;
 int result=printer_run_maintenance(mode);
 assert(!allocs);
 if(mode==2){assert(result==-19&&!prep_calls&&!scans);return 0;}
 if(!fault){assert(!result&&prep_calls==1&&scans==(mode?36U:12U)&&feeds==2&&paper==10816&&parks==1&&finishes==1&&wipes==(mode?1U:0U)&&!aborts);}
 else if(fault==5||fault==6||fault==7||fault==8||fault==15){assert(result<0&&!prep_calls&&!scans&&!feeds&&!aborts);}
 else if(fault==13){assert(result==PRINTER_OC3_RETURNED&&scans==5&&!aborts&&finishes==1);}
 else{assert(result<0&&aborts==1&&!s_bidi_known);if(fault==2)assert(!scans&&!feeds);if(fault==3)assert(scans==5);if(fault==4)assert(scans==4U*(mode?3U:1U)&&feeds==1);}
 puts("PASS actual maintenance sequencing, doses, reverse placement, bounded feeds, final wipe/park and fault stop");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'test.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(D),str(t/'test.c'),str(D/'printer_mapper.c'),'-o',str(t/'test')],check=True)
 for mode in [0,1,2]:
  for fault in ([0] if mode==2 else ([0,2,3,4,5,6,7,8,10,11,13,15]+([9] if mode else []))):subprocess.run([str(t/'test'),str(mode),str(fault)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

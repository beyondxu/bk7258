"""Actual waste-clean orchestration:600 nozzles, fixed dose, no paper loading or feed."""
from pathlib import Path
import os,subprocess,tempfile
D=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
h=r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "printer_maintenance_pattern.h"
#include "printer_audit_utils.h"
#include "printer_canvas_geometry.h"
#define PRINTER_WASTE_MOTION_H
#define BK_OK 0
#define PRINTER_MAP_X_PITCH_CENTI_UM PRINTER_CANVAS_X_PITCH_CENTI_UM
#define PRINTER_MAP_X_DIRECTION -1
#define PRINTER_MAP_FIRST_NOZZLE (fault==7?1U:0U)
#define PRINTER_MAP_REVERSE_Y 0
#define PRINTER_STARTUP_WIPE_STEPS 3740U
#define PRINTER_STARTUP_WIPE_ROUNDS 2U
#define PRINTER_OC3_RETURNED (-15)
#define os_printf printf
static unsigned fault,batches,positions,parks,wipes,finishes,aborts,allocs;
static bool s_bidi_known=true,s_bidi_right=false,s_bidi_at_wiper=true;
static uint8_t s_waste_station_ready;
static bool printer_link_is_ready(void){return fault!=6;}
static void *printer_map_psram_alloc(size_t n){if(fault==5)return NULL;++allocs;return malloc(n);}
static void printer_map_psram_free(void*p){assert(allocs);--allocs;free(p);}
static void printer_map_yield_cpu(void){}
static int printer_waste_position(void){
 assert(!s_waste_station_ready&&s_bidi_at_wiper);++positions;
 if(fault==2)return -18;
 s_bidi_right=true;s_bidi_at_wiper=false;s_waste_station_ready=1;return 0;
}
static int printer_waste_fire(const uint8_t*row){
 assert(s_waste_station_ready&&s_bidi_right&&!s_bidi_at_wiper&&batches<12);
 printer_map_config_t cfg={PRINTER_MAP_X_PITCH_CENTI_UM,-1,0,0};uint8_t*gold=malloc(118000);
 assert(!printer_maint_build(gold,118000,1,batches/4,batches%4,1,&cfg));assert(!memcmp(gold,row,118000));free(gold);
 ++batches;return fault==3&&batches==5?-13:0;
}
static int tph_mcu_set_head_power(unsigned on){assert(!on);return fault==4?-1:0;}
static int printer_buffered_park_at_wiper(void){assert(!s_waste_station_ready);++parks;if(fault==10)return -14;s_bidi_at_wiper=true;s_bidi_right=false;return 0;}
static int printer_buffered_wipe(uint32_t n,uint32_t rounds){
 assert(!s_waste_station_ready&&s_bidi_at_wiper&&n==3740&&rounds==2);++wipes;
 return fault==9?-14:fault==13?PRINTER_OC3_RETURNED:0;
}
static int printer_bidirectional_finish(void){assert(!s_waste_station_ready&&s_bidi_at_wiper);++finishes;return fault==11?-14:0;}
static void printer_bidirectional_abort(void){assert(!s_waste_station_ready);++aborts;s_bidi_known=false;}
#include "printer_waste_clean_job.h"
int main(int argc,char**argv){
 assert(argc==2);fault=atoi(argv[1]);if(fault==8)s_bidi_known=false;
 int result=printer_run_waste_clean();assert(!allocs&&!s_waste_station_ready);
 if(!fault)assert(!result&&positions==1&&batches==12&&parks==1&&wipes==1&&finishes==1&&!aborts);
 else if(fault==5||fault==6||fault==7||fault==8)assert(result<0&&!positions&&!batches&&!parks&&!aborts);
 else if(fault==13)assert(result==PRINTER_OC3_RETURNED&&batches==12&&finishes==1&&!aborts);
 else{assert(result<0&&aborts==1);if(fault==2)assert(!batches);if(fault==3)assert(batches==5&&!parks&&!wipes);}
 puts("PASS actual waste clean:12 ordinary-dose batches, no paper APIs, safe wipe/park and stop on every failure stage");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'test.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-Wno-unused-function','-fsanitize=address,undefined','-I'+str(D),str(t/'test.c'),str(D/'printer_mapper.c'),'-o',str(t/'test')],check=True)
 for fault in [0,2,3,4,5,6,7,8,9,10,11,13]:subprocess.run([str(t/'test'),str(fault)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

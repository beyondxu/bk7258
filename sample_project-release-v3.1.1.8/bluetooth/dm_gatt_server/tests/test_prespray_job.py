"""Prespray ordering, finite dose and stop paths. No paper/wipe APIs available."""
from pathlib import Path
import subprocess,tempfile,os
D=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
h=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "printer_prespray_pattern.h"
#define PRINTER_WASTE_MOTION_H
#define BK_OK 0
#define PRINTER_MAP_X_PITCH_CENTI_UM 4388
#define PRINTER_MAP_X_DIRECTION -1
#define PRINTER_MAP_FIRST_NOZZLE (fault==7?1U:0U)
#define PRINTER_MAP_REVERSE_Y 0
#define os_printf printf
static unsigned fault,allocs,positioned,fired,finished,aborted,off,pauses;
static uint64_t bk_aon_rtc_get_us(void){return UINT64_C(1000000000);}
static bool s_bidi_known=true,s_bidi_right=false,s_bidi_at_wiper=true;
static uint8_t s_waste_station_ready;
static void rtos_delay_milliseconds(unsigned ms){assert(ms==100&&fired&&s_waste_station_ready);++pauses;}
static bool printer_link_is_ready(void){return fault!=6;}
static void*printer_map_psram_alloc(size_t n){if(fault==5)return NULL;++allocs;return malloc(n);}
static void printer_map_psram_free(void*p){assert(allocs);--allocs;free(p);}
static int printer_waste_position(void){
 assert(!s_waste_station_ready&&s_bidi_at_wiper);++positioned;
 if(fault==2)return -18;
 s_bidi_at_wiper=false;s_bidi_right=true;s_waste_station_ready=1;return 0;
}
static int printer_waste_fire(const uint8_t*p){
 assert(positioned==1&&s_waste_station_ready&&s_bidi_right&&!s_bidi_at_wiper);++fired;
 unsigned bits=0;for(unsigned i=0;i<118000;++i)for(unsigned b=0;b<6;++b)bits+=(p[i]>>b)&1U;
 unsigned plan=fault==10?5U:fault==11?50U:200U;unsigned done=(fired-1U)*40U;unsigned chunk=plan-done;if(chunk>40U)chunk=40U;assert(bits==600U*chunk);return fault==3?-13:0;
}
static int tph_mcu_set_head_power(unsigned on){assert(!on&&fired>=1);++off;return fault==4?-1:0;}
static int printer_bidirectional_finish(void){assert(!s_waste_station_ready&&off==1);++finished;if(fault==9)return -14;s_bidi_right=false;s_bidi_at_wiper=true;return 0;}
static void printer_bidirectional_abort(void){assert(!s_waste_station_ready);++aborted;s_bidi_known=false;}
#include "printer_prespray_job.h"
int main(int argc,char**argv){
 assert(argc==2);fault=atoi(argv[1]);if(fault==8)s_bidi_known=false;
 if(fault>=10){s_prespray_have_ink=1;s_prespray_last_ink_us=bk_aon_rtc_get_us()-(fault==10?0U:fault==11?UINT64_C(180000000):UINT64_C(900000000));}
 int rc=printer_run_prespray("TEST");assert(!allocs&&!s_waste_station_ready);
 if(!fault||fault>=10)assert(pauses==fired-1U&&!rc&&positioned==1&&fired==(fault==10?1U:fault==11?2U:5U)&&finished==1&&!aborted&&s_bidi_at_wiper&&!s_bidi_right);
 else if(fault>=5&&fault<=8)assert(rc<0&&!positioned&&!fired&&!finished&&!aborted);
 else {assert(rc<0&&aborted==1);if(fault==2)assert(!fired);if(fault==3)assert(!off&&!finished);}
 puts("PASS prespray:validate->pad->bounded finite rows->OFF->park;failures stop,no paper/no extra wipe");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'a.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-Wno-unused-function','-fsanitize=address,undefined','-I'+str(D),str(t/'a.c'),str(D/'printer_mapper.c'),'-o',str(t/'a')],check=True)
 for mode in [0,2,3,4,5,6,7,8,9,10,11,12]:subprocess.run([str(t/'a'),str(mode)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

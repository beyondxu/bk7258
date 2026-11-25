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
static unsigned fault,allocs,positioned,fired,finished,aborted,off,pauses,requested_rounds;
static uint64_t bk_aon_rtc_get_us(void){return UINT64_C(1000000000);}
static bool s_bidi_known=true,s_bidi_right=false,s_bidi_at_wiper=true;
static uint8_t s_waste_station_ready;
static void rtos_delay_milliseconds(unsigned ms){assert(ms==5&&fired&&s_waste_station_ready);++pauses;}
static bool printer_link_is_ready(void){return fault!=6;}
static void*printer_map_psram_alloc(size_t n){if(fault==5)return NULL;++allocs;return malloc(n);}
static void printer_map_psram_free(void*p){assert(allocs);--allocs;free(p);}
#define PRINTER_OC3_PREP_SEARCH_STEPS 11315U
static int printer_waste_seek(unsigned steps){
 assert(steps==11315U);
 assert(!s_waste_station_ready&&s_bidi_at_wiper);++positioned;
 if(fault==2)return -18;
 s_bidi_at_wiper=false;s_bidi_right=true;s_waste_station_ready=1;return 0;
}
static int printer_waste_seek_from_print_left(void){
 assert(finished==2U);
 return printer_waste_seek(11315U);
}
static int printer_waste_fire_prespray(const uint8_t*p){
 assert(positioned==finished+1U&&s_waste_station_ready&&s_bidi_right&&!s_bidi_at_wiper);++fired;
 unsigned bits=0;for(unsigned i=0;i<118000;++i)for(unsigned b=0;b<6;++b)bits+=(p[i]>>b)&1U;
 assert(bits&&bits%600U==0U&&bits<=24000U);requested_rounds+=bits/600U;
 for(unsigned x=0;x<1180;++x){unsigned mask=0,sections=0;for(unsigned a=0;a<100;++a){unsigned v=p[x*100+a];mask|=v;if(v)sections|=1U<<(a%4);}assert((mask&(mask-1U))==0&&((sections&~3U)==0||(sections&~12U)==0));}return fault==3?-13:0;
}
static int tph_mcu_set_head_power(unsigned on){assert(!on&&fired>=1);++off;return fault==4?-1:0;}
static int printer_waste_park_for_preparation(void){assert(!s_waste_station_ready&&off==positioned);++finished;if(fault==9)return -14;s_bidi_right=false;s_bidi_at_wiper=true;return 0;}
static int printer_waste_return_to_print_left(void){
 assert(positioned==3U);
 int result=printer_waste_park_for_preparation();
 s_bidi_at_wiper=false;
 return result;
}
static void printer_bidirectional_abort(void){assert(!s_waste_station_ready);++aborted;s_bidi_known=false;}
#include "printer_prespray_job.h"
int main(int argc,char**argv){
 assert(argc==2);fault=atoi(argv[1]);if(fault==8)s_bidi_known=false;
 if(fault>=10){s_prespray_have_ink=1;s_prespray_last_ink_us=bk_aon_rtc_get_us()-(fault==10?0U:fault==11?UINT64_C(180000000):UINT64_C(900000000));}
 if(fault==13||fault==14){
  if(fault==14)s_prespray_last_ink_us=bk_aon_rtc_get_us();
  assert(!printer_run_prespray("PAGE",3U,true));
  assert(positioned==3U&&finished==3U&&off==3U&&!aborted&&!allocs&&!s_waste_station_ready);
  assert(fired==(fault==13?40U:3U)&&requested_rounds==(fault==13?1600U:10U));
  assert(pauses==fired-3U&&!s_bidi_at_wiper&&!s_bidi_right);
  puts("PASS three pad visits preserve the complete page recovery dose");
  return 0;
 }
 int rc=printer_run_prespray("TEST",1U,false);assert(!allocs&&!s_waste_station_ready);
 if(!fault||fault>=10)assert(pauses==fired-1U&&!rc&&positioned==1&&fired==(fault==10?1U:fault==11?3U:40U)&&requested_rounds==(fault==10?10U:fault==11?100U:1600U)&&finished==1&&!aborted&&s_bidi_at_wiper&&!s_bidi_right);
 else if(fault>=5&&fault<=8)assert(rc<0&&!positioned&&!fired&&!finished&&!aborted);
 else {assert(rc<0&&aborted==1);if(fault==2)assert(!fired);if(fault==3)assert(!off&&!finished);}
 puts("PASS prespray:validate->pad->bounded finite rows->OFF->park;failures stop,no paper/no extra wipe");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'a.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-Wno-unused-function','-fsanitize=address,undefined','-I'+str(D),str(t/'a.c'),str(D/'printer_mapper.c'),'-o',str(t/'a')],check=True)
 for mode in [0,2,3,4,5,6,7,8,9,10,11,12,13,14]:subprocess.run([str(t/'a'),str(mode)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

"""Actual bounded service pulse loop/controller: left start, right-only motion, no head ON, firing, feed or automatic return."""
from pathlib import Path
import os,subprocess,tempfile
D=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
s=(D/'motor_sdk_driver.c').read_text();leg=s[s.index('bk_err_t motor_carriage_service_leg('):]
leg=leg.replace('bk_err_t motor_carriage_service_leg(','static bk_err_t real_service_leg(',1)
h=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define BK_OK 0
#define BK_FAIL -1
#define TPH_MOT_STEP 19
#define TPH_MOT_DIR 50
#define os_printf printf
typedef int bk_err_t;
#include "printer_service_geometry.h"
static uint64_t us;
static unsigned dir,high,boost,active,calls,pulses,off_calls,scenario;
static int32_t position,minimum,maximum;
static unsigned leg_index,leg_steps;
static bool head=true,link=true;
static uint64_t bk_aon_rtc_get_us(void){return us;}
static void set_level(unsigned pin,unsigned value){assert(pin==50&&!high);dir=value;leg_index=0;}
static void up(unsigned pin){assert(pin==19&&!head&&boost&&active&&!high);high=1;++pulses;position+=dir?1:-1;if(position<minimum)minimum=position;if(position>maximum)maximum=position;assert(position>=0&&position<=10472);}
static void down(unsigned pin){assert(pin==19&&high);high=0;}
#define SET_GPIO_LEVEL(p,v) set_level(p,v)
#define SET_GPIO_HIGH(p) up(p)
#define SET_GPIO_LOW(p) down(p)
static void delay_us(unsigned n){
 if(high)assert(n==4);
 else if(n==50)assert(leg_index==0);
 else{assert(leg_index<leg_steps);unsigned edge=leg_index<leg_steps-1-leg_index?leg_index:leg_steps-1-leg_index;unsigned expected=edge<250?3000-2400*edge/250:600;assert(n==expected);++leg_index;}
 us+=n;
}
static void rtos_delay_milliseconds(unsigned n){assert(!high&&n==1);us+=(uint64_t)n*1000;}
LEG
static bk_err_t motor_carriage_service_leg(uint8_t d,uint32_t n,uint32_t*sent){assert(d==1);leg_steps=n;++calls;if((scenario==3||scenario==4)&&calls==1){leg_steps=n/2;int rc=real_service_leg(d,n/2,sent);return scenario==4?-1:rc;}return real_service_leg(d,n,sent);}
static bool printer_link_is_ready(void){return link;}
static int tph_mcu_set_head_power(uint8_t on){assert(!on);++off_calls;if(scenario==2)return -1;head=false;return 0;}
static void motor_carriage_test_power_up(void){assert(!head&&!boost&&!active);boost=active=1;}
static void bk_motor_set_state(unsigned m,unsigned on){assert(m==1&&!on&&!high);active=0;}
static void motor_all_power_down(void){assert(!high);boost=active=0;}
#include "printer_service_travel_test.h"
int main(int argc,char**argv){assert(argc==2);scenario=atoi(argv[1]);
uint32_t sent=99;assert(real_service_leg(1,10473,&sent)==-1&&!sent);assert(real_service_leg(2,10,&sent)==-1);assert(real_service_leg(1,0,&sent)==-1);assert(real_service_leg(1,10,NULL)==-1);assert(!pulses);
if(scenario==5){link=false;assert(printer_run_service_travel_test()==-7&&!off_calls&&!pulses);link=true;}
int rc=printer_run_service_travel_test();assert(!boost&&!active&&!high);
if(scenario==2){assert(rc==-8&&!pulses&&!calls);}
else if(scenario==3||scenario==4){assert(rc==-9&&calls==1&&position==5236&&pulses==5236);}
else{assert(!rc&&calls==1&&pulses==10472&&position==10472&&minimum==0&&maximum==10472);assert(us>7000000&&us<8500000);}
unsigned old=pulses;assert(printer_run_service_travel_test()==-7&&pulses==old);puts("PASS: left-origin/right-only trajectory, exact one leg, 7.3s speed profile with exact symmetric ramp, head OFF, one-shot, no blind return, argument/link/OFF/partial-failure guards");}
'''.replace('\nLEG\n','\n'+leg+'\n')
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(D),str(p/'test.c'),'-o',str(p/'test')],check=True)
 for scenario in [0,2,3,4,5]:subprocess.run([str(p/'test'),str(scenario)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
page=(D/'printer_page_job.h').read_text();entry=page[page.index('static int printer_run_full_page'):]
assert entry.index('return printer_run_service_travel_test();')<entry.index('return printer_run_idle_ab')
assert 'tph_fire' not in (D/'printer_service_travel_test.h').read_text()

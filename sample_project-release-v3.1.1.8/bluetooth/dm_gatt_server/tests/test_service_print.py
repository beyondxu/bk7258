"""Actual print stream and service pulse loop: paper-window-only firing and return."""
from pathlib import Path
import os,subprocess,tempfile
D=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
s=(D/'hw_printer_stub.c').read_text();stream=s[s.index('static bool carriage_stream_move_ramped'):s.index('/* Wake U4 without')]
m=(D/'motor_sdk_driver.c').read_text();leg=m[m.index('bk_err_t motor_carriage_service_leg('):].replace('bk_err_t motor_carriage_service_leg(','static bk_err_t real_service_leg(',1)
h=r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int bk_err_t;
#define BK_OK 0
#define BK_FAIL -1
#define TPH_MOT_STEP 19
#define TPH_MOT_DIR 50
#define TPH_COLUMN_BYTES 100
#define ACCEL_RAMP_STEPS 250
#define PRINTER_PAGE_HEAD_SETTLE_MS 2000
#define os_printf printf
#include "printer_service_geometry.h"
static uint32_t s_blank_spi_columns,s_blank_spi_steps[2];
static bool s_blank_spi_failed;
static unsigned scenario,head,boost,step,dir,commands,service_calls,spi_calls;
static int32_t position,minimum,maximum;
static uint64_t clock_us;
static uint8_t data[118000];
static void set_direction(unsigned v){assert(!step);dir=v;}
static void up(unsigned pin){if(pin==50){set_direction(1);return;}assert(pin==19&&!step&&boost);if(head)assert(dir&&position>=1590&&position<9070);step=1;position+=dir?1:-1;if(position<minimum)minimum=position;if(position>maximum)maximum=position;assert(position>=0&&position<=10660);}
static void down(unsigned pin){if(pin==50){set_direction(0);return;}assert(pin==19&&step);step=0;}
#define GPIO_UP(p) up(p)
#define GPIO_DOWN(p) down(p)
#define SET_GPIO_HIGH(p) up(p)
#define SET_GPIO_LOW(p) down(p)
#define SET_GPIO_LEVEL(p,v) do{assert(p==50);set_direction(v);}while(0)
static void delay_us(unsigned n){if(step)assert(n==4);else assert(n==50||(n>=260&&n<=3000));clock_us+=n;}
static void rtos_delay_milliseconds(unsigned n){assert(!step);if(n==2000)assert(head&&position==1590);else assert(!head&&(n==1||n==200||n==300));clock_us+=(uint64_t)n*1000;}
static uint64_t bk_aon_rtc_get_us(void){return clock_us;}
static int tph_fire_raw_column_bytes(const uint8_t*p){assert(head&&boost&&dir&&!step);assert(position==1590+200+(int32_t)s_blank_spi_columns*6);assert(!memcmp(p,data+s_blank_spi_columns*100,100));if(scenario==3&&s_blank_spi_columns==10)return -1;++spi_calls;return 0;}
STREAM
LEG
static bk_err_t motor_carriage_service_leg(uint8_t d,uint32_t n,uint32_t*sent){assert(!head);++service_calls;
if((scenario==5&&service_calls==2)||(scenario==6&&service_calls==3))return real_service_leg(d,n/2,sent);
return real_service_leg(d,n,sent);}
static int tph_mcu_set_head_power(uint8_t on){++commands;
if(on)assert(position==1590&&boost&&commands==2);
if((scenario==1&&commands==1)||(scenario==2&&on)||(scenario==4&&commands==3))return -1;
head=on;return 0;}
static void carriage_verified_power_up(void){assert(!head&&!position);boost=1;}
static void carriage_verified_power_down(void){assert(!step&&!head);boost=0;}
#include "printer_service_print.h"
int main(int argc,char**argv){assert(argc==2);scenario=atoi(argv[1]);for(unsigned i=0;i<sizeof(data);i++)data[i]=i%64;
int rc=printer_execute_service_line(data);assert(!head&&!boost&&!step);
if(!scenario){assert(!rc&&position==0&&minimum==0&&maximum==10660&&commands==3&&service_calls==3&&spi_calls==1180);assert(s_blank_spi_steps[0]==10660&&s_blank_spi_steps[1]==10660);}
if(scenario==1)assert(rc==-8&&commands==1&&!service_calls&&!spi_calls&&!position);
if(scenario==2)assert(rc==-8&&service_calls==1&&!spi_calls&&position==1590);
if(scenario==3)assert(rc==-9&&service_calls==1&&spi_calls==10&&position==1590+200+10*6);
if(scenario==4)assert(rc==-8&&service_calls==1&&spi_calls==1180&&position==9070);
if(scenario==5)assert(rc==-9&&service_calls==2&&position==9070+1590/2);
if(scenario==6)assert(rc==-9&&service_calls==3&&position==10660/2);
puts("PASS: exact image columns/cadence, head OFF in service/return, left origin restored, failure stop without blind travel");}
'''.replace('\nSTREAM\n','\n'+stream+'\n').replace('\nLEG\n','\n'+leg+'\n').replace('commands==1','commands==1')
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(D),str(p/'test.c'),'-o',str(p/'test')],check=True)
 for scenario in range(7):subprocess.run([str(p/'test'),str(scenario)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

"""Actual return-scan code: physical STEP/data correspondence, head gating, failure stops."""
from pathlib import Path
import subprocess,tempfile
SRC=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
h=r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#define BK_OK 0
#define TPH_MOT_DIR 50
#define TPH_MOT_STEP 19
#define os_printf printf
static uint32_t s_blank_spi_columns,s_blank_spi_steps[2];static bool s_blank_spi_failed;
static uint8_t data[118000];
static unsigned dir,step,phase,index_in_leg,seen,head,boost,commands,settles;
static int position,fail_column,fail_command;
static uint64_t delays[2];
static void up(unsigned pin){
 if(pin==50){dir=1;phase=0;index_in_leg=0;}
 else{assert(pin==19&&!step&&phase==1);if(dir)assert(!head);step=1;position+=dir?1:-1;}
}
static void down(unsigned pin){
 if(pin==50){dir=0;phase=0;index_in_leg=0;}
 else{assert(pin==19&&step&&phase==2);step=0;}
}
#define GPIO_UP(p) up(p)
#define GPIO_DOWN(p) down(p)
static void delay_us(unsigned us){
 if(!phase){assert(us==50&&!step);phase=1;return;}
 if(phase==1){assert(us==4&&step);phase=2;return;}
 unsigned i=index_in_leg,expected=i<250?3000-2740*i/250:i>=7230?3000-2740*(7479-i)/250:260;
 assert(us==expected&&!step);delays[dir]+=us;++index_in_leg;phase=1;
}
static void rtos_delay_milliseconds(unsigned ms){
 assert(!step);
 if(ms==2000){assert(PRINTER_HEAD_SETTLE_MS==2000&&head&&boost&&position==7480&&!seen);++settles;}
 else assert(ms==200||ms==300);
}
static int tph_mcu_set_head_power(unsigned on){
 ++commands;if((int)commands==fail_command)return -1;
 head=on;if(on)assert(position==7480&&s_blank_spi_steps[1]==7480);return 0;
}
static void carriage_verified_power_up(void){assert(!head);boost=1;}
static void carriage_verified_power_down(void){assert(!head);boost=0;}
static int tph_fire_raw_column_bytes(const uint8_t*p){
 assert(!dir&&head&&boost&&!step&&phase==1);
 unsigned source=1179-seen;
 assert(index_in_leg==206+seen*6);
 assert(position==(int)(200+source*6));
 assert(!memcmp(p,data+source*100,100));
 if((int)seen==fail_column)return -1;
 ++seen;return 0;
}
#include "printer_return_scan.h"
static void reset(void){
 dir=step=phase=index_in_leg=seen=head=boost=commands=position=settles=0;
 delays[0]=delays[1]=0;fail_column=fail_command=-1;
}
int main(void){
 for(unsigned i=0;i<1180;i++){data[i*100]=i&63;data[i*100+1]=(i>>6)&63;}
 reset();assert(!printer_execute_section_return_line(data));
 assert(position==0&&seen==1180&&s_blank_spi_columns==1180);
 assert(settles==(PRINTER_HEAD_SETTLE_MS?1U:0U));
 assert(s_blank_spi_steps[0]==7480&&s_blank_spi_steps[1]==7480);
 assert(delays[0]==2632780&&delays[1]==2632780&&!head&&!boost&&commands==3);
 reset();fail_column=10;assert(printer_execute_section_return_line(data)==-9);
 assert(position==7480-266&&seen==10&&s_blank_spi_failed);
 assert(s_blank_spi_steps[1]==7480&&s_blank_spi_steps[0]==266&&!head&&!boost);
 reset();fail_command=1;assert(printer_execute_section_return_line(data)==-8);assert(position==0&&!boost&&!seen);
 reset();fail_command=2;assert(printer_execute_section_return_line(data)==-8);assert(position==7480&&!seen&&!head&&!boost&&!settles);
 reset();fail_command=3;assert(printer_execute_section_return_line(data)==-8);assert(position==0&&seen==1180&&!head&&!boost);
 reset();assert(printer_execute_section_return_line(NULL)==-1);assert(!commands&&!boost);
 assert(!carriage_return_compare_move(1,7479,NULL));assert(!carriage_return_compare_move(1,7480,data));
 puts("PASS: 1180 reversed columns at identical physical positions; unchanged delay curve, OFF outward, ON return, no extra motion after failure");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'t.c').write_text(h)
 for settle in [0,2000]:
  subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',f'-DPRINTER_HEAD_SETTLE_MS={settle}','-I'+str(SRC),str(p/'t.c'),'-o',str(p/'t')],check=True)
  subprocess.run([str(p/'t')],check=True,stdout=subprocess.DEVNULL)
 print('PASS reverse movement, geometry, power, failure cases')

"""Run actual RTC step loop and startup controller using fake GPIO/clock."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
s=(root/'motor_sdk_driver.c').read_text();loop=s[s.index('static bk_err_t carriage_rtc_wait'):]
s=(root/'hw_printer_stub.c').read_text();controller=s[s.index('void printer_run_carriage_diagnostic'):]
harness=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
typedef int bk_err_t;
#define BK_OK 0
#define BK_FAIL -1
#define TPH_MOT_STEP 19
#define TPH_MOT_DIR 50
#define TPH_MOT_EN 49
#define TPH_MOT_nFA 48
#define CARRIAGE_RUN_STEPS 7480
#define os_printf printf
static uint64_t now,high_at,low_at;
static int direction,step,enabled,pulses[2],fault_after=-1,stalled,off_calls,powerups,powerdowns,paper_off,off_fail;
static uint64_t bk_aon_rtc_get_us(void){if(!stalled) now+=10;return now;}
static void high(int pin){
 if(pin==TPH_MOT_DIR){assert(!step);direction=1;}
 else if(pin==TPH_MOT_EN)enabled=0;
 else {assert(pin==TPH_MOT_STEP && !step);step=1;high_at=now;++pulses[direction];}
}
static void low(int pin){
 if(pin==TPH_MOT_DIR){assert(!step);direction=0;}
 else {assert(pin==TPH_MOT_STEP);if(step&&!stalled)assert(now-high_at>=40);step=0;low_at=now;}
}
#define SET_GPIO_HIGH(p) high(p)
#define SET_GPIO_LOW(p) low(p)
#define SET_GPIO_LEVEL(p,v) do{if(v) high(p);else low(p);}while(0)
static int bk_gpio_get_input(int pin){assert(pin==TPH_MOT_nFA);return fault_after<0 || pulses[0]+pulses[1]<fault_after;}
static void rtos_delay_milliseconds(unsigned ms){assert(!step);now+=(uint64_t)ms*1000;}
LOOP
static int tph_mcu_set_head_power(uint8_t on){assert(!on);++off_calls;return off_fail?-1:0;}
static void motor_all_power_up_and_enable(void){assert(off_calls);enabled=1;++powerups;}
static void bk_motor_set_state(int motor,int state){assert(motor==0&&state==0);++paper_off;}
static void motor_all_power_down(void){enabled=0;++powerdowns;}
CONTROLLER
static void reset(void){now=0;step=0;direction=0;pulses[0]=pulses[1]=0;enabled=1;fault_after=-1;stalled=0;off_calls=powerups=powerdowns=paper_off=off_fail=0;}
int main(void){
 uint32_t sent;
 reset();assert(motor_carriage_test_leg(1,7480,&sent)==0&&sent==7480&&pulses[1]==7480);assert(now>=7480ULL*1540);
 assert(motor_carriage_test_leg(0,7480,&sent)==0&&sent==7480&&pulses[0]==7480);
 reset();fault_after=13;assert(motor_carriage_test_leg(1,7480,&sent)!=0&&sent==13&&!step&&!enabled);
 reset();stalled=1;assert(motor_carriage_test_leg(1,7480,&sent)!=0&&sent==0);
 reset();assert(motor_carriage_test_leg(1,7481,&sent)!=0&&sent==0);
 reset();printer_run_carriage_diagnostic();assert(pulses[0]==7480&&pulses[1]==7480&&!enabled&&off_calls==1&&paper_off==1&&powerdowns==1);
 reset();fault_after=13;printer_run_carriage_diagnostic();assert(pulses[0]==0&&pulses[1]==13&&!enabled&&powerdowns==1);
 reset();off_fail=1;printer_run_carriage_diagnostic();assert(!powerups&&!pulses[0]&&!pulses[1]);
 puts("PASS: RTC minimum delays, bounded counts, both directions, fault/stalled clock abort, head OFF only, no reverse after failure");
}
'''.replace('LOOP',loop).replace('CONTROLLER',controller)
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(harness)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)

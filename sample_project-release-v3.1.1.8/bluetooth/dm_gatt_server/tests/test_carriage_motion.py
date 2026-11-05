"""Compare actual controller and pulse loop against the user's shuttle curve."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
s=(root/'motor_sdk_driver.c').read_text();loop=s[s.index('void motor_carriage_test_power_up'):s.index('/* Page feed:')]
wrapper=s[s.index('__IRAM_SEC void delay_us(uint32_t us)'):s.index('static void safe_gpio_config_output')]
s=(root/'hw_printer_stub.c').read_text();start=s.index('void printer_run_carriage_diagnostic')
controller=s[start:s.index('\n}',start)+2]
assert 'bk_gpio_get_input' not in loop
assert 'TPH_MOT_nFA' not in loop
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
#define TPH_MOT_MS1 23
#define TPH_MOT_MS2 22
#define TPH_MOT_MS3 15
#define TPH_MOT_SLEEP 26
#define TPH_MOT_RES 21
#define TPH_MOT_nFA 48
#define CARRIAGE_RUN_STEPS 7480
#define os_printf printf
static unsigned phase,index_in_leg,delay_calls;
static uint64_t now_us;
static uint64_t bk_aon_rtc_get_us(void){return now_us;}
#define __IRAM_SEC
static int direction,step,pulses[2],off_calls,powerups,powerdowns,off_fail;
static void high(int pin){if(pin==50){assert(!step);direction=1;phase=0;index_in_leg=0;}else if(pin==19){assert(!step&&phase==1);step=1;++pulses[direction];}}
static void low(int pin){if(pin==50){assert(!step);direction=0;phase=0;index_in_leg=0;}else if(pin==19){assert(step&&phase==2);step=0;}}
#define SET_GPIO_HIGH(p) high(p)
#define SET_GPIO_LOW(p) low(p)
#define SET_GPIO_LEVEL(p,v) do{if(v) high(p);else low(p);}while(0)
static void bk_delay_us(uint32_t us){
 now_us += us;
 ++delay_calls;
 if(phase==0){assert(us==50&&!step);phase=1;}
 else if(phase==1){assert(us==4&&step);phase=2;}
 else {
  assert(!step);unsigned i=index_in_leg;
  unsigned expected=i<250?1000-740*i/250:i>=7230?1000-740*(7479-i)/250:260;
  assert(us==expected);++index_in_leg;phase=1;
 }
}
static void rtos_delay_milliseconds(unsigned ms){assert(!step);(void)ms;}
static int bk_motor_gpio_config_init(void){++powerups;return 0;}
static void bk_motor_boost_power(int on){assert(on==1);}
WRAPPER
LOOP
static int tph_mcu_set_head_power(uint8_t on){assert(!on);++off_calls;return off_fail?-1:0;}
static void bk_motor_set_state(int motor,int state){assert(motor==1&&state==0);}
static void motor_all_power_down(void){++powerdowns;}
CONTROLLER
static void reset(void){phase=0;index_in_leg=0;delay_calls=0;now_us=0;step=0;pulses[0]=pulses[1]=0;off_calls=powerups=powerdowns=off_fail=0;}
int main(void){
 uint32_t sent;
 reset();assert(motor_carriage_test_leg(1,7480,&sent)==0&&sent==7480&&pulses[1]==7480&&delay_calls==14961);
 assert(now_us==2160750ULL);
 assert(motor_carriage_test_leg(0,7480,&sent)==0&&sent==7480&&pulses[0]==7480);
 reset();assert(motor_carriage_test_leg(1,7481,&sent)!=0&&sent==0&&!delay_calls);
 reset();printer_run_carriage_diagnostic();assert(pulses[0]==7480&&pulses[1]==7480&&off_calls==1&&powerups==1&&powerdowns==1);
 reset();off_fail=1;printer_run_carriage_diagnostic();assert(!powerups&&!pulses[0]&&!pulses[1]);
 puts("PASS: user's exact delay argument curve both directions, 7480 count, one cycle, OFF only, no nFAULT read, no motion if OFF handshake fails");
}
'''.replace('WRAPPER',wrapper).replace('LOOP',loop).replace('CONTROLLER',controller)
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(harness)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)

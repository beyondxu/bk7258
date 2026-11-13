"""Actual U5 feed pulse generator: no U4 enable or masked interrupts."""
from pathlib import Path
import subprocess,tempfile,re
src=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer/motor_sdk_driver.c'
s=src.read_text();func=s[s.index('static bk_err_t motor_page_feed_impl'):]
assert 'rtos_disable_int' not in func and '__disable_irq' not in func
h=r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
typedef int bk_err_t;
#define BK_OK 0
#define BK_FAIL -1
#define PAPER_MOT_STEP 18
#define PAPER_MOT_DIR 55
#define PAPER_MOT_MS1 46
#define PAPER_MOT_MS2 47
#define PAPER_MOT_MS3 17
#define PAPER_MOT_SLEEP 54
#define PAPER_MOT_RES 53
#define PAPER_MOT_EN 44
static unsigned pulses,high,boost,dir,boost_calls,inits;
static int bk_motor_gpio_config_init(void){boost=0;++inits;return 0;}
static void bk_motor_boost_power(unsigned on){boost=on;++boost_calls;}
static void up(unsigned pin){assert(pin==18||pin==55||pin==46||pin==47||pin==17||pin==54||pin==53);if(pin==18){assert(!high&&boost);high=1;++pulses;}if(pin==55)dir=1;}
static void down(unsigned pin){assert(pin==18||pin==55||pin==44);if(pin==18){assert(high);high=0;}if(pin==55)dir=0;}
#define SET_GPIO_HIGH(p) up(p)
#define SET_GPIO_LOW(p) down(p)
#define SET_GPIO_LEVEL(p,v) do{if(v)up(p);else down(p);}while(0)
static void bk_delay_us(unsigned us){if(high)assert(us==4);else assert(us>=50&&us<=3000);}
static void rtos_delay_milliseconds(unsigned ms){assert(!high);(void)ms;}
static void bk_motor_set_state(unsigned motor,unsigned state){assert(motor<=1&&state==0);}
FUNC
int main(void){uint32_t sent;assert(!motor_page_feed(3170,0,&sent));assert(sent==3170&&pulses==3170&&!boost&&!high&&dir==0);assert(motor_page_feed(5001,0,&sent)==-1);boost=1;unsigned calls=boost_calls;assert(!motor_page_feed_keep_power(3170,0,&sent));assert(sent==3170&&pulses==6340&&boost&&boost_calls==calls&&inits==1);assert(motor_page_feed_keep_power(0,0,&sent)==-1);assert(boost_calls==calls);assert(!motor_page_feed(780,0,&sent));assert(sent==780&&pulses==7120&&!boost&&!high);assert(!motor_page_feed(1560,0,&sent));assert(sent==1560&&pulses==8680&&!boost&&!high);}
'''.replace('FUNC',func)
# Exercise the actual MS3 pin selected by the production header.
ms3=int(re.search(r'#define PAPER_MOT_MS3\s+GPIO_(\d+)', src.with_suffix('.h').read_text()).group(1))
h=h.replace('#define PAPER_MOT_MS3 17',f'#define PAPER_MOT_MS3 {ms3}').replace('pin==17',f'pin=={ms3}')
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)

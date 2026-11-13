"""Actual U5 feed pulse generator: no U4 enable or masked interrupts."""
from pathlib import Path
import subprocess,tempfile
src=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer/motor_sdk_driver.c'
s=src.read_text();func=s[s.index('/* Read-only register evidence:'):]
assert 'rtos_disable_int' not in func and '__disable_irq' not in func
h=r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
typedef unsigned gpio_id_t;
typedef struct {int io_mode,pull_mode,func_mode;} gpio_config_t;
#define GPIO_OUTPUT_ENABLE 1
#define GPIO_PULL_DISABLE 0
#define GPIO_SECOND_FUNC_DISABLE 0
#define PAPER_MOT_nFA 45
#define PIN_VDD15V_EN 13
static uint32_t regs[64];
#define SOC_AON_GPIO_REG_BASE ((uintptr_t)regs)
static unsigned configured[64],reclaims;
static int config_fail,unmap_fail;
static int gpio_dev_unmap(unsigned p){assert(p!=13&&p!=19&&p!=1);if(unmap_fail&&(int)p==53)return -1;++reclaims;return 0;}
static int bk_gpio_set_config(unsigned p,const gpio_config_t*c){assert(c->io_mode==1&&!c->pull_mode&&!c->func_mode);if(config_fail&&(int)p==53)return -1;configured[p]=1;return 0;}
static unsigned bk_gpio_get_input(unsigned p){assert(p==45);return 1;}
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
static void up(unsigned pin){assert(pin==44||pin==18||pin==55||pin==46||pin==47||pin==17||pin==54||pin==53);regs[pin]=2;if(pin==18){assert(!high&&boost);assert(configured[18]&&configured[44]&&configured[53]&&configured[54]);assert(regs[53]==2&&regs[54]==2&&regs[44]==0);high=1;++pulses;}if(pin==55)dir=1;}
static void down(unsigned pin){assert(pin==18||pin==55||pin==44||pin==53||pin==54);regs[pin]=0;if(pin==18){high=0;}if(pin==55)dir=0;}
#define SET_GPIO_HIGH(p) up(p)
#define SET_GPIO_LOW(p) down(p)
#define SET_GPIO_LEVEL(p,v) do{if(v)up(p);else down(p);}while(0)
static void bk_delay_us(unsigned us){if(high)assert(us==4);else assert(us>=50&&us<=3000);}
static void rtos_delay_milliseconds(unsigned ms){assert(!high);(void)ms;}
static void bk_motor_set_state(unsigned motor,unsigned state){assert(motor<=1&&state==0);}
FUNC
int main(void){uint32_t sent;assert(!motor_page_feed(3170,0,&sent));assert(sent==3170&&pulses==3170&&!boost&&!high&&dir==0);assert(motor_page_feed(5001,0,&sent)==-1);boost=1;unsigned calls=boost_calls;assert(!motor_page_feed_keep_power(3170,0,&sent));assert(sent==3170&&pulses==6340&&boost&&boost_calls==calls&&inits==1);assert(motor_page_feed_keep_power(0,0,&sent)==-1);assert(boost_calls==calls);assert(reclaims==16);assert(!motor_page_feed(780,0,&sent));assert(sent==780&&pulses==7120&&!boost&&!high);assert(!motor_page_feed(1560,0,&sent));assert(sent==1560&&pulses==8680&&!boost&&!high);
unsigned old=pulses;config_fail=1;sent=99;assert(motor_page_feed(780,0,&sent)==-1);assert(!sent&&pulses==old&&!boost);config_fail=0;
boost=1;calls=boost_calls;unmap_fail=1;assert(motor_page_feed_keep_power(780,0,&sent)==-1);assert(!sent&&pulses==old&&boost&&boost_calls==calls);
puts("PASS: actual U5 reconfiguration, wake/reset/enable before steps, preserved boost in keep-power path, exact pulses and config-failure cleanup");}
'''.replace('\nFUNC\n','\n'+func+'\n')
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)

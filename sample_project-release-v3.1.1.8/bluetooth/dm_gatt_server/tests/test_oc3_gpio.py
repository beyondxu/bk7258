"""Exercise actual OC3 GPIO configuration/ISR and the real static board row."""
from pathlib import Path
import subprocess,tempfile
R=Path(__file__).resolve().parents[1];D=R/'ap/app_bluetooth/gatt/printer'
s=(D/'motor_sdk_driver.c').read_text();s=s[s.index('/* ADC_OC3 is'):s.index('bk_err_t bk_motor_gpio_config_init(void)')]
row=next(x for x in (R/'ap/config/bk7258_ap/usr_gpio_cfg.h').read_bytes().decode('latin1').splitlines() if '{GPIO_24,' in x)
assert 'GPIO_INPUT_ENABLE' in row and 'GPIO_PULL_UP_EN' in row and 'GPIO_INT_TYPE_FALLING_EDGE' in row
assert 'GPIO_SECOND_FUNC_DISABLE' in row and 'GPIO_DEV_INVALID' in row and 'GPIO_INIT_ENABLE' in row
h=r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
typedef int gpio_id_t;typedef int bk_err_t;typedef void(*gpio_isr_t)(gpio_id_t);
#define BK_OK 0
#define TPH_OC3_GPIO 24
#define GPIO_INPUT_ENABLE 1
#define GPIO_PULL_UP_EN 2
#define GPIO_SECOND_FUNC_DISABLE 3
#define GPIO_INT_TYPE_FALLING_EDGE 4
typedef struct{int io_mode,pull_mode,func_mode;}gpio_config_t;
static unsigned mode,phase,enabled,level=1,actions;
static gpio_isr_t registered;
static int bk_gpio_disable_interrupt(int pin){assert(pin==24);phase=0;enabled=0;registered=0;return 0;}
static int gpio_dev_unmap(int pin){assert(pin==24&&phase==0);phase=1;return 0;}
static int bk_gpio_set_config(int pin,const gpio_config_t*c){assert(pin==24&&phase==1&&c->io_mode==1&&c->pull_mode==2&&c->func_mode==3);phase=2;return mode==1?-9:0;}
static int bk_gpio_set_interrupt_type(int pin,int type){assert(pin==24&&type==4&&phase==2);phase=3;return mode==2?-9:0;}
static int bk_gpio_register_isr(int pin,gpio_isr_t cb){assert(pin==24&&cb&&phase==3);phase=4;registered=cb;return mode==3?-9:0;}
static int bk_gpio_clear_interrupt(int pin){assert(pin==24&&phase==4);phase=5;return mode==4?-9:0;}
static int bk_gpio_enable_interrupt(int pin){assert(pin==24&&phase==5&&registered);phase=6;if(mode==5)return -9;enabled=1;return 0;}
static unsigned bk_gpio_get_input(int pin){assert(pin==24);return level;}
ACTUAL
static void stopped(void){++actions;}
int main(int argc,char**argv){assert(argc==2);mode=atoi(argv[1]);
 assert(!motor_oc3_ready());motor_oc3_set_handler(stopped);
 int rc=motor_oc3_init();
 if(mode){assert(rc==-9&&!motor_oc3_ready()&&!enabled);return 0;}
 assert(!rc&&motor_oc3_ready()&&enabled&&!motor_oc3_active());
 registered(24);assert(!actions);level=0;registered(23);assert(!actions);
 registered(24);assert(actions==1&&motor_oc3_active());
 motor_oc3_set_handler(0);registered(24);assert(actions==1);
 puts("PASS actual GPIO24 init: input/pull-up, no alternate function, ordered falling IRQ init, LOW polarity and failure blocking");
}
'''.replace('ACTUAL',s)
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test')],check=True)
 for mode in range(6):subprocess.run([str(p/'test'),str(mode)],check=True)

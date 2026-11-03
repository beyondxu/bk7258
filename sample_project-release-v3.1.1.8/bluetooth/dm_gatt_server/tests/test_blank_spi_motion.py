"""Compare actual blank-SPI test with user's hardware-verified shuttle."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'ap/app_bluetooth/gatt/printer/hw_printer_stub.c').read_text()
a=s.index('#define SHUTTLE_TOTAL_STEPS');b=s.index('/* Separate comparison path:',a)
reference=s[a:b];new=s[b:]
assert 'bk_gpio_get_input' not in new
h=r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#define BK_OK 0
#define TPH_COLUMN_BYTES 100
#define TPH_MOT_DIR 50
#define TPH_MOT_STEP 19
#define TPH_MOT_MS1 23
#define TPH_MOT_MS2 22
#define TPH_MOT_MS3 15
#define TPH_MOT_SLEEP 26
#define TPH_MOT_RES 21
#define TPH_MOT_EN 49
#define os_printf printf
static unsigned dir,step,phase,index_in_leg,pulses[2],columns,delay_count,init_count,off_calls;
static int blank_mode,fail_column=-1,off_fail;
static uint32_t expected_delays[40000],actual_delays[40000];
static void up(unsigned pin){
 if(pin==50){assert(!step);dir=1;phase=0;index_in_leg=0;}
 else if(pin==19){assert(!step&&phase==1);step=1;++pulses[dir];}
}
static void down(unsigned pin){
 if(pin==50){assert(!step);dir=0;phase=0;index_in_leg=0;}
 else if(pin==19){assert(step&&phase==2);step=0;}
}
#define GPIO_UP(p) up(p)
#define GPIO_DOWN(p) down(p)
static void delay_us(uint32_t us){
 actual_delays[delay_count++]=us;
 if(!phase){assert(us==50&&!step);phase=1;}
 else if(phase==1){assert(us==4&&step);phase=2;}
 else {
  assert(!step);unsigned i=index_in_leg;
  unsigned expected=i<250?1000-740*i/250:i>=7230?1000-740*(7479-i)/250:260;
  assert(us==expected);++index_in_leg;phase=1;
 }
}
static void rtos_delay_milliseconds(unsigned ms){assert(!step);(void)ms;}
static int bk_motor_gpio_config_init(void){++init_count;return 0;}
static void bk_motor_boost_power(unsigned on){(void)on;}
static int tph_mcu_set_head_power(uint8_t on){assert(!on);++off_calls;return off_fail?-1:0;}
static int tph_fire_raw_column_bytes(const uint8_t *p){
 assert(blank_mode && dir==1 && !step && phase==1);
 assert(index_in_leg==200+columns*6);
 for(unsigned i=0;i<100;i++)assert(!p[i]);
 if((int)columns==fail_column)return -1;
 ++columns;return 0;
}
REFERENCE
NEW
static void reset(void){step=phase=delay_count=init_count=off_calls=columns=0;pulses[0]=pulses[1]=0;off_fail=0;fail_column=-1;}
int main(void){
 reset();standalone_carriage_shuttle_test(1);
 assert(pulses[0]==7480&&pulses[1]==7480&&columns==0);
 unsigned expected_count=delay_count;memcpy(expected_delays,actual_delays,delay_count*sizeof(uint32_t));
 reset();blank_mode=1;standalone_carriage_blank_spi_test();
 assert(off_calls==1&&columns==1180&&pulses[0]==7480&&pulses[1]==7480);
 assert(delay_count==expected_count&&!memcmp(expected_delays,actual_delays,delay_count*sizeof(uint32_t)));
 reset();fail_column=10;standalone_carriage_blank_spi_test();
 assert(s_blank_spi_failed&&columns==10&&pulses[1]==260&&pulses[0]==0&&!step);
 reset();off_fail=1;standalone_carriage_blank_spi_test();
 assert(!init_count&&!columns&&!pulses[0]&&!pulses[1]);
 puts("PASS: exact reference delay sequence/counts, 1180 zero columns spaced six steps, OFF only, no return after SPI failure, no motion on OFF failure");
}
'''.replace('REFERENCE',reference).replace('NEW',new)
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)

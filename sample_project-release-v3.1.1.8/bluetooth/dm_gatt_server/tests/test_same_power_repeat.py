"""Run real stream, carriage power and repeat controller; mock hardware edges only."""
from pathlib import Path
import subprocess,tempfile
P=Path(__file__).resolve().parents[1];D=P/'ap/app_bluetooth/gatt/printer'
s=(D/'hw_printer_stub.c').read_text()
a=s.index('/* Separate comparison path:');b=s.index('void standalone_carriage_blank_spi_test',a)
real=s[a:b]
h=r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "printer_audit_utils.h"
#define BK_OK 0
#define TPH_COLUMN_BYTES 100
#define PRINTER_PAGE_COLUMNS 1180
#define SHUTTLE_TOTAL_STEPS 7480
#define ACCEL_RAMP_STEPS 250
#define PRINTER_PAGE_HEAD_SETTLE_MS 2000
#define PRINTER_PAGE_FEED_STEPS 3170
#define PRINTER_PAGE_FEED_DIRECTION 0
#define TPH_MOT_DIR 50
#define TPH_MOT_STEP 19
#define TPH_MOT_MS1 23
#define TPH_MOT_MS2 22
#define TPH_MOT_MS3 15
#define TPH_MOT_SLEEP 26
#define TPH_MOT_RES 21
#define TPH_MOT_EN 49
#define os_printf printf
static uint8_t data[118000];
static unsigned boost,head,commands,ons,offs,legs,feeds,sends,dir,step,pulses[2],settles;
static int scenario;
static void up(unsigned p){if(p==50){dir=1;++legs;}if(p==19){assert(!step&&boost&&head);step=1;++pulses[dir];}}
static void down(unsigned p){if(p==50){dir=0;++legs;}if(p==19){assert(step);step=0;}}
#define GPIO_UP(p) up(p)
#define GPIO_DOWN(p) down(p)
static void delay_us(unsigned us){(void)us;}
static void rtos_delay_milliseconds(unsigned ms){assert(!step);if(ms==2000){assert(head&&boost&&!sends);++settles;}}
static int bk_motor_gpio_config_init(void){assert(!head);boost=0;return 0;}
static void bk_motor_boost_power(unsigned on){if(!on)assert(!head);boost=on;}
static void bk_motor_set_state(unsigned motor,unsigned active){assert(motor==0&&!active);}
static int tph_mcu_set_head_power(unsigned on){++commands;if(on){assert(boost);++ons;head=1;}else{++offs;head=0;}return (scenario==3&&on)||(scenario==4&&commands==3)?-1:0;}
static int tph_fire_raw_column_bytes(const uint8_t*p){
 assert(head&&boost&&dir&&!step);assert(!memcmp(p,data+(sends%1180)*100,100));
 ++sends;if((scenario==1&&sends==11)||(scenario==2&&sends==1191))return -1;return 0;
}
static int motor_page_feed_keep_power(uint32_t n,uint8_t d,uint32_t*sent){
 assert(head&&boost&&!step&&sends==1180&&legs==2&&n==3170&&d==0);++feeds;
 *sent=scenario==5?3000:n;if(scenario==6)data[9]^=1;return 0;
}
REAL
#include "printer_same_power_repeat.h"
int main(int argc,char**argv){
 assert(argc==2);scenario=argv[1][0]-'0';for(unsigned i=0;i<sizeof(data);++i)data[i]=(uint8_t)(i%64);
 int rc=printer_execute_same_power_repeat(data);assert(!head&&!boost&&!step);
 assert(ons==1&&offs==2&&commands==3);assert(settles==(scenario==3?0:1));
 if(!scenario){assert(!rc&&sends==2360&&legs==4&&feeds==1&&pulses[0]==14960&&pulses[1]==14960);}
 else{assert(rc);if(scenario==1)assert(sends==11&&legs==1&&!feeds);if(scenario==2)assert(sends==1191&&legs==3&&feeds==1);if(scenario==3)assert(!sends&&!legs&&!feeds);if(scenario==4)assert(sends==2360);if(scenario==5||scenario==6)assert(sends==1180&&legs==2&&feeds==1);}
}
'''.replace('REAL',real)
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-I'+str(D),str(p/'test.c'),'-o',str(p/'test')],check=True)
 arm=Path('/workspace/.setup/tools/usr/bin/arm-none-eabi-gcc')
 if arm.exists():
  subprocess.run([str(arm),'-std=gnu99','-mcpu=cortex-m33+nodsp','-mfpu=fpv5-sp-d16','-mfloat-abi=hard','-mcmse','-O2','-Wall','-Wextra','-Werror','-I'+str(D),'-c',str(p/'test.c'),'-o',str(p/'test.o')],check=True)
 for scenario in range(7):subprocess.run([str(p/'test'),str(scenario)],check=True,stdout=subprocess.DEVNULL)
 print('PASS: same bytes, one head ON, rail continuous, 2 complete round trips, one feed; SPI/ON/OFF/short-feed/buffer-corruption failures stop and power down')

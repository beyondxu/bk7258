"""Run the real streaming motion function: preserve columns/positions/data, stretch only forward low times."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'ap/app_bluetooth/gatt/printer/hw_printer_stub.c').read_text()
a=s.index('static bool carriage_stream_move_ramped(');b=s.index('static void carriage_verified_power_up',a)
function=s[a:b]
h=r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#define PRINTER_NOZZLE_CHECK 1
#define ACCEL_RAMP_STEPS 250
#define TPH_COLUMN_BYTES 100
#define TPH_MOT_DIR 50
#define TPH_MOT_STEP 19
#define BK_OK 0
static uint32_t s_blank_spi_columns,s_blank_spi_steps[2];
static bool s_blank_spi_failed;
static unsigned direction,step,index_in_leg,phase,stretch,seen,fail_at;
static uint8_t data[118000];static const uint8_t *expected;
static uint64_t low_total;
static void up(unsigned pin){if(pin==50){direction=1;phase=0;}else{assert(pin==19&&!step&&phase==1);step=1;}}
static void down(unsigned pin){if(pin==50){direction=0;phase=0;}else{assert(pin==19&&step&&phase==2);step=0;}}
#define GPIO_UP(x) up(x)
#define GPIO_DOWN(x) down(x)
static void delay_us(uint32_t us){
 if(!phase){assert(us==50&&!step);phase=1;return;}
 if(phase==1){assert(us==4&&step);phase=2;return;}
 unsigned i=index_in_leg;
 unsigned delay=i<250?3000-2740*i/250:i>=7230?3000-2740*(7479-i)/250:260;
 assert(!step&&us==delay*stretch);low_total+=us;++index_in_leg;phase=1;
}
static int tph_fire_raw_column_bytes(const uint8_t*p){
 assert(direction&&!step&&phase==1&&index_in_leg==200+seen*6);
 if(expected)assert(!memcmp(p,expected+seen*100,100));
 else for(unsigned i=0;i<100;i++)assert(!p[i]);
 if(seen==fail_at)return -1;
 ++seen;return 0;
}
FUNCTION
static uint64_t run(unsigned dir,const uint8_t*p){
 s_blank_spi_columns=s_blank_spi_steps[0]=s_blank_spi_steps[1]=seen=index_in_leg=phase=step=0;
 low_total=0;s_blank_spi_failed=false;expected=p;
 stretch=dir&&p?PRINTER_NOZZLE_TIME_SCALE:1;
 assert(carriage_stream_move_ramped(dir,7480,p));
 assert(s_blank_spi_steps[dir]==7480&&index_in_leg==7480&&!step);
 assert(seen==(dir?1180:0)&&s_blank_spi_columns==seen);
 return low_total;
}
int main(void){
 for(unsigned i=0;i<118000;i++)data[i]=i%64;
 fail_at=~0U;
 uint64_t blank=run(1,NULL),active=run(1,data),back=run(0,NULL);
 assert(blank==back&&active==blank*PRINTER_NOZZLE_TIME_SCALE);
 /* A failed transaction must stop before its STEP pulse, with no extra columns. */
 seen=index_in_leg=phase=step=s_blank_spi_columns=0;expected=data;stretch=PRINTER_NOZZLE_TIME_SCALE;fail_at=10;
 assert(!carriage_stream_move_ramped(1,7480,data));assert(seen==10&&index_in_leg==260&&!step&&s_blank_spi_failed);
 printf("PASS scale=%u: positions/data/count unchanged; total forward STEP-low=%llu us, return=%llu us; SPI failure stops\n",PRINTER_NOZZLE_TIME_SCALE,(unsigned long long)active,(unsigned long long)back);
}
'''.replace('FUNCTION',function)
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'t.c').write_text(h)
 for scale in [1,4]:
  subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',f'-DPRINTER_NOZZLE_TIME_SCALE={scale}',str(p/'t.c'),'-o',str(p/'t')],check=True)
  subprocess.run([str(p/'t')],check=True)

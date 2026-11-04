"""Exercise actual bounded controller with fake motors/SPI; never operates hardware."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'ap/app_bluetooth/gatt/printer/hw_printer_stub.c').read_text()
helper=s[s.index('extern bool printer_link_is_ready'):s.index('static void printer_worker_thread')]
harness=r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "printer_mapper.h"
#define os_printf printf
#define SWEEP_COLS_TOTAL 1180
#define TPH_COLUMN_BYTES 100
#define CARRIAGE_RUN_STEPS 7480
#define BK_OK 0
#define PRINTER_MAP_X_PITCH_CENTI_UM 4233
#define PRINTER_MAP_X_DIRECTION 1
#define PRINTER_MAP_FIRST_NOZZLE 0
#define PRINTER_MAP_REVERSE_Y 0
#define PRINTER_BOUNDED_CHANNEL_MASK 1
static uint8_t (*s_final_swath_cols)[100];
static int scenario,stage,sweeps,returns,powered,comm;
static void *printer_map_psram_alloc(size_t n){return malloc(n);}
static void printer_map_psram_free(void*p){free(p);}
static void printer_map_yield_cpu(void){}
bool printer_link_is_ready(void){return true;}
void printer_map_default_config(printer_map_config_t *c){memset(c,0,sizeof(*c));}
int printer_map_bin_capture(const uint8_t*b,uint32_t n,const printer_map_config_t*c,printer_map_stats_t*s,printer_map_capture_t*p,void*(*a)(size_t),void(*f)(void*),void(*y)(void)) {
 assert(p->column_limit==1180 && p->buffer_bytes==118000 && p->target_y==576);
 assert(p->first_row==175 && p->last_row==175 && p->channel_mask==1);
 assert(p->min_column==200 && p->max_column==999);
 p->selected_dots[0]=369;p->first_active=231;p->last_active=948;
 return scenario==1?-5:0;
}
static int printer_execute_cyan_line(const uint8_t *p){assert(p);++sweeps;return scenario==2?-9:0;}
static int tph_mcu_comm_init(void){++comm;return 0;}
static int tph_mcu_set_head_power(uint8_t on){
 if(on){assert(stage==2);stage=3;}else if(stage==0)stage=1;else if(stage==4)stage=5;
 return 0;
}
static void motor_all_power_up_and_enable(void){assert(stage==1);powered=1;stage=2;}
static void bk_motor_set_state(int motor,int state){assert(motor==0 && state==0);}
static bool sweep_and_fire_line_direct(void){assert(stage==3 && s_final_swath_cols);++sweeps;stage=4;return scenario!=2;}
static void motor_carriage_return_home(int steps){assert(stage==5 && steps==7480);++returns;}
static void motor_all_power_down(void){powered=0;}
HELPER
int main(int argc,char**argv){
 scenario=atoi(argv[1]);int result=printer_run_bounded_line(NULL,0);
 if(!PRINTER_BOUNDED_PRINT || scenario==1){assert(!comm&&!sweeps&&!returns);}
 else {
  assert(sweeps==1 && returns==0 && !powered && s_final_swath_cols==NULL);
  assert((result==0)==(scenario==0));
  if(scenario==0){assert(printer_run_bounded_line(NULL,0)==-7);assert(sweeps==1);}
 }
 puts("PASS bounded motion sequencing");
}
'''.replace('HELPER',helper)
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(harness)
 for physical in [0,1]:
  subprocess.run(['cc','-std=c99',f'-DPRINTER_BOUNDED_PRINT={physical}','-I',str(root/'ap/app_bluetooth/gatt/printer'),str(p/'test.c'),'-o',str(p/'test')],check=True)
  for scenario in [0,1,2]:subprocess.run([str(p/'test'),str(scenario)],check=True)

"""Head-OFF preparation ordering and stop at every failed stage."""
from pathlib import Path
import subprocess,tempfile
D=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
boot=(D/'hw_printer_stub.c').read_bytes().decode('latin1')
boot=boot[boot.index('int printer_run_boot_carriage_prepare(void) {'):]
assert (D.parents[2]/'ap_main.c').exists()
main=(D.parents[2]/'ap_main.c').read_bytes().decode('latin1')
assert main.index('int boot_prep=printer_run_boot_carriage_prepare();')<main.index('dm_gatts_init();')
h=r'''
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#define BK_OK 0
#define PRINTER_STARTUP_LOAD_STEPS 780U
#define PRINTER_STARTUP_WIPE_STEPS 187U
#define PRINTER_STARTUP_WIPE_ROUNDS 2U
#define PRINTER_BUFFERED_LINE 1
#define PRINTER_PAGE_FEED_DIRECTION 0
#define os_printf printf
static bool s_bidi_known=true,s_bidi_right=false;
static unsigned mode;
static char trace[32];static unsigned n;
static void record(char c){assert(n<31);trace[n++]=c;trace[n]=0;}
static int printer_bidi_power_up(void){record('P');return mode==10?-8:0;}
static int printer_execute_bidirectional_line(const uint8_t*p,unsigned dir){
 assert(!p&&s_bidi_known&&dir!=s_bidi_right);record(dir?'R':'L');
 if(mode==(dir?1U:4U))return -14;
 s_bidi_right=dir;return 0;
}
static int tph_mcu_set_head_power(unsigned on){
 assert(!on&&s_bidi_right);record('H');return mode==2?-1:0;
}
static int motor_page_load_keep_power(unsigned steps,unsigned dir,uint32_t*sent){
 assert(!s_bidi_right&&steps==780&&!dir);record('F');
 *sent=mode==5?779:780;return mode==3?-1:0;
}
static int tph_line_probe(void){return mode==11?-1:0;}
static int printer_buffered_wipe(unsigned steps,unsigned rounds){
 assert(!s_bidi_right&&steps==187&&rounds==2&&!strcmp(trace,"PF"));record('W');
 return mode==1?-14:mode==4?-8:0;
}
#include "printer_startup_prepare.h"
#define PRINTER_BOOT_CARRIAGE_PREP 1
#define PRINTER_FULL_PAGE 1
#define PRINTER_PAGE_BIDIRECTIONAL 1
#define PRINTER_STARTUP_PREP 1
static bool printer_link_is_ready(void){return true;}
static int printer_bidirectional_finish(void){assert(!s_bidi_right);record('D');return 0;}
static void printer_bidirectional_abort(void){s_bidi_known=false;}
BOOT_WRAPPER
int main(int argc,char**argv){
 assert(argc==2);mode=atoi(argv[1]);
 if(mode==6)s_bidi_known=false;
 if(mode==7)s_bidi_right=true;
 if(mode==9){
  assert(!printer_run_boot_carriage_prepare()&&!strcmp(trace,"RHLD")&&!s_bidi_right);
  assert(printer_run_boot_carriage_prepare()==-7&&!strcmp(trace,"RHLD"));
  puts("PASS boot wrapper: once before BLE, zero feed even with configured loading offset");return 0;
 }
 int rc=printer_startup_prepare((mode==8||mode==2)?0U:1U);
 if(mode==8){assert(!rc&&!strcmp(trace,"RHL")&&!s_bidi_right);return 0;}
 if(!mode)assert(!rc&&!strcmp(trace,"PFW")&&!s_bidi_right);
 else if(mode==1)assert(rc==-14&&!strcmp(trace,"PFW")&&!s_bidi_right);
 else if(mode==2)assert(rc==-8&&!strcmp(trace,"RH")&&s_bidi_right);
 else if(mode==3||mode==5)assert(rc==-10&&!strcmp(trace,"PF")&&!s_bidi_right);
 else if(mode==4)assert(rc==-8&&!strcmp(trace,"PFW")&&!s_bidi_right);
 else if(mode==10)assert(rc==-8&&!strcmp(trace,"P"));
 else if(mode==11)assert(rc==-13&&!n);
 else assert(rc==-7&&!n);
 puts("PASS preparation order and bounded stop: no image firing, no return after feed failure");
}
'''.replace('BOOT_WRAPPER',boot)
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-I'+str(D),str(p/'test.c'),'-o',str(p/'test')],check=True)
 for mode in range(12):subprocess.run([str(p/'test'),str(mode)],check=True)

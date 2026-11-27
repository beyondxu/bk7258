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
#include "printer_service_geometry.h"
#define BK_OK 0
#define PRINTER_OC3_RETURNED (-15)
#define PRINTER_STARTUP_LOAD_STEPS 702U
#define PRINTER_STARTUP_LOAD_ROUNDS 3U
#define PRINTER_STARTUP_WIPE_STEPS 3740U
#define PRINTER_BUFFERED_LINE 1
#define PRINTER_PAGE_FEED_DIRECTION 0
#define os_printf printf
static bool s_bidi_known=true,s_bidi_right=false,s_bidi_at_wiper=true;
static unsigned mode,load_calls,total_paper,wipe_calls;
static bool s_bidi_power;
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
static int printer_buffered_load_while_moving(unsigned steps,unsigned dir){
 assert(steps==702&&!dir&&trace[0]=='P'&&trace[1]=='W'&&n==load_calls+1U+wipe_calls);
 assert(s_bidi_right==((load_calls&1U)!=0U));++load_calls;record('F');
 if(mode==3||mode==5||(mode==12&&load_calls==3)||(mode==13&&load_calls==6))return -10;
 s_bidi_right=!s_bidi_right;total_paper+=steps;return 0;
}
static int tph_line_probe(void){return mode==11?-1:0;}
static int printer_buffered_short_round_trips(unsigned steps,unsigned rounds,unsigned cruise_step_us){
 assert(cruise_step_us==77U);
 assert(!s_bidi_right&&steps==3740&&rounds==1);
 assert(!strcmp(trace,wipe_calls?"PW":"P"));++wipe_calls;record('W');
 if(mode==15&&wipe_calls==1)return PRINTER_OC3_RETURNED;
 return mode==1?-14:mode==4?-8:0;
}
static int printer_buffered_leave_wiper(void){
 assert(!s_bidi_right&&load_calls==6&&total_paper==4212);record('A');
 return mode==14?-14:0;
}
static int printer_buffered_boot_round_trip(void){
 int result=printer_execute_bidirectional_line(NULL,1);
 if(result)return result;
 if(tph_mcu_set_head_power(0))return -8;
 return printer_execute_bidirectional_line(NULL,0);
}
#include "printer_startup_prepare.h"
#define PRINTER_BOOT_CARRIAGE_PREP 1
#define PRINTER_FULL_PAGE 1
#define PRINTER_PAGE_BIDIRECTIONAL 1
#define PRINTER_STARTUP_PREP 1
static bool printer_link_is_ready(void){return true;}
static void printer_buffered_stop(void){record('S');}
static void carriage_verified_power_down(void){record('D');}
static void printer_bidirectional_abort(void){s_bidi_known=false;}
BOOT_WRAPPER
int main(int argc,char**argv){
 assert(argc==2);mode=atoi(argv[1]);
 if(mode==6)s_bidi_known=false;
 if(mode==7)s_bidi_right=true;
 if(mode==9){
  assert(!printer_run_boot_carriage_prepare()&&!strcmp(trace,"RHLSD")&&!s_bidi_right);
  assert(printer_run_boot_carriage_prepare()==-7&&!strcmp(trace,"RHLSD"));
  puts("PASS boot wrapper: once before BLE, zero feed even with configured loading offset");return 0;
 }
 int rc=printer_startup_prepare((mode==8||mode==2)?0U:1U);
 if(mode==8){assert(!rc&&!strcmp(trace,"RHL")&&!s_bidi_right);return 0;}
 if(!mode)assert(!rc&&!strcmp(trace,"PWFFFFFFA")&&!s_bidi_right&&load_calls==6&&total_paper==4212);
 else if(mode==1)assert(rc==-14&&!strcmp(trace,"PW")&&!s_bidi_right);
 else if(mode==2)assert(rc==-8&&!strcmp(trace,"RH")&&s_bidi_right);
 else if(mode==3||mode==5)assert(rc==-10&&!strcmp(trace,"PWF")&&!s_bidi_right);
 else if(mode==4)assert(rc==-8&&!strcmp(trace,"PW")&&!s_bidi_right);
 else if(mode==10)assert(rc==-8&&!strcmp(trace,"P"));
 else if(mode==11)assert(rc==-13&&!n);
 else if(mode==12)assert(rc==-10&&load_calls==3&&total_paper==1404&&!s_bidi_right);
 else if(mode==13)assert(rc==-10&&load_calls==6&&total_paper==3510&&s_bidi_right);
 else if(mode==14)assert(rc==-14&&!strcmp(trace,"PWFFFFFFA")&&total_paper==4212);
 else if(mode==15)assert(!rc&&!strcmp(trace,"PWWFFFFFFA")&&total_paper==4212&&wipe_calls==2);
 else assert(rc==-7&&!n);
 puts("PASS preparation order and bounded stop: no image firing, no return after feed failure");
}
'''.replace('BOOT_WRAPPER',boot)
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-I'+str(D),str(p/'test.c'),'-o',str(p/'test')],check=True)
 for mode in range(16):subprocess.run([str(p/'test'),str(mode)],check=True)

# Active prespray configuration. Stub the three-visit job here; the real
# per-visit seek, dose and return are covered in the waste/prespray tests.
page = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#define PRINTER_BUFFERED_LINE 1
#define PRINTER_PRESPRAY 1
#define PRINTER_STARTUP_WIPE_STEPS 3740U
#define PRINTER_PAGE_FEED_DIRECTION 0U
#define PRINTER_OC3_RETURNED (-15)
#define BK_OK 0
#define os_printf printf
static bool s_bidi_known=true,s_bidi_right=false,s_bidi_at_wiper=true;
static unsigned stage,fault;
static int tph_line_probe(void){assert(stage++==0U);return 0;}
static int printer_bidi_power_up(void){assert(stage++==1U);return 0;}
static int printer_buffered_short_round_trips(unsigned steps,unsigned rounds,unsigned cruise_step_us){
 assert(cruise_step_us==77U);
 assert(stage++==2U&&steps==3740U&&rounds==1U&&s_bidi_at_wiper&&!s_bidi_right);
 return fault==4U?-14:0;
}
static int printer_run_prespray(const char *reason,unsigned visits,bool finish_at_print_left){
 assert(stage++==3U&&reason[0]=='P'&&visits==3U&&finish_at_print_left);
 if(fault==1U)return -13;
 if(fault==3U)return -18;
 s_bidi_at_wiper=false;return 0;
}
static int motor_page_load_keep_power(uint32_t steps,uint8_t dir,uint32_t *sent){
 assert(stage++==4U&&steps==6240U&&!dir&&!s_bidi_at_wiper&&!s_bidi_right);
 *sent=steps-(fault==2U);return 0;
}
static int printer_buffered_boot_round_trip(void){assert(0);return -1;}
#include "printer_startup_prepare.h"
int main(void){
 for(fault=0U;fault<5U;++fault){
  stage=0U;s_bidi_at_wiper=true;
  int result=printer_startup_prepare(1U);
  assert(stage==(fault==4U?3U:(fault==1U||fault==3U)?4U:5U));
  assert(result==(fault==1U?-13:fault==2U?-10:fault==3U?-18:fault==4U?-14:0));
 }
 puts("PASS one short round trip, three flashing visits, then single 6240-step feed");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'page.c').write_text(page)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-I'+str(D),str(p/'page.c'),'-o',str(p/'page')],check=True)
 subprocess.run([str(p/'page')],check=True)

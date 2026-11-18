"""Head-OFF preparation ordering and stop at every failed stage."""
from pathlib import Path
import subprocess,tempfile
D=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
h=r'''
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#define BK_OK 0
#define PRINTER_STARTUP_LOAD_STEPS 780U
#define PRINTER_PAGE_FEED_DIRECTION 0
#define os_printf printf
static bool s_bidi_known=true,s_bidi_right=false;
static unsigned mode,event;
static int printer_execute_bidirectional_line(const uint8_t*p,unsigned dir){
 assert(!p&&s_bidi_known&&dir!=s_bidi_right);
 assert(event==(dir?0U:3U));++event;
 if(mode==(dir?1U:4U))return -14;
 s_bidi_right=dir;return 0;
}
static int tph_mcu_set_head_power(unsigned on){
 assert(!on&&event==1&&s_bidi_right);++event;return mode==2?-1:0;
}
static int motor_page_load_keep_power(unsigned steps,unsigned dir,uint32_t*sent){
 assert(event==2&&steps==780&&!dir&&s_bidi_right);++event;
 *sent=mode==5?779:780;return mode==3?-1:0;
}
#include "printer_startup_prepare.h"
int main(int argc,char**argv){
 assert(argc==2);mode=atoi(argv[1]);
 if(mode==6)s_bidi_known=false;
 if(mode==7)s_bidi_right=true;
 int rc=printer_startup_prepare();
 if(!mode)assert(!rc&&event==4&&!s_bidi_right);
 else if(mode==1)assert(rc==-14&&event==1&&!s_bidi_right);
 else if(mode==2)assert(rc==-8&&event==2&&s_bidi_right);
 else if(mode==3||mode==5)assert(rc==-10&&event==3&&s_bidi_right);
 else if(mode==4)assert(rc==-14&&event==4&&s_bidi_right);
 else assert(rc==-7&&!event);
 puts("PASS preparation order and bounded stop: no image firing, no return after feed failure");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-I'+str(D),str(p/'test.c'),'-o',str(p/'test')],check=True)
 for mode in range(8):subprocess.run([str(p/'test'),str(mode)],check=True)

"""Actual boot wrapper/startup block: one short spit, no duplicate homing/clean."""
from pathlib import Path
import tempfile,subprocess,os
D=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
s=(D/'hw_printer_stub.c').read_text(encoding='latin1');a=s.index('int printer_run_boot_carriage_prepare(void) {');b=s.index('/* Before BLE/worker creation:',a);wrapper=s[a:b]
s=(D.parents[2]/'ap_main.c').read_text(encoding='latin1');a=s.index('    if (link_ready && !PRINTER_CARRIAGE_TEST && PRINTER_BOOT_CARRIAGE_PREP)');b=s.index('\n\t   //',a);block=s[a:b]
h=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define PRINTER_BOOT_CARRIAGE_PREP 1
#define PRINTER_FULL_PAGE 1
#define PRINTER_PAGE_BIDIRECTIONAL 1
#define PRINTER_STARTUP_PREP 1
#define PRINTER_PRESPRAY 1
#define PRINTER_CARRIAGE_TEST 0
#define PRINTER_BOOT_WASTE_CLEAN 0
#define os_printf printf
static unsigned mode,spits;
static bool s_printer_link_ready;
static bool printer_link_is_ready(void){return s_printer_link_ready;}
static int printer_run_prespray(const char*why){assert(!strcmp(why,"BOOT"));++spits;return mode==1?-13:0;}
static int printer_run_boot_waste_clean(void){assert(0);return -1;}
WRAPPER
int main(int argc,char**argv){
 assert(argc==2);mode=atoi(argv[1]);bool link_ready=mode!=2;s_printer_link_ready=link_ready;
 BLOCK
 if(mode==2)assert(!spits&&!link_ready);
 else {assert(spits==1);assert(link_ready==(mode==0));assert(printer_run_boot_carriage_prepare()==-7&&spits==1);}
 assert(s_printer_link_ready==link_ready);
 puts("PASS actual boot prespray:once before BLE,no duplicate homing/legacy clean,failed link/flash blocks printing");
}
'''.replace('WRAPPER',wrapper).replace('BLOCK',block)
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'a.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p/'a.c'),'-o',str(p/'a')],check=True)
 for mode in range(3):subprocess.run([str(p/'a'),str(mode)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

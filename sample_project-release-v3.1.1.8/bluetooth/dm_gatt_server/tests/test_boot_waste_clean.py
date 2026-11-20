"""Actual startup glue: link -> head-OFF reference -> one finite clean -> BLE."""
from pathlib import Path
import subprocess,tempfile,os
D=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
s=(D/'hw_printer_stub.c').read_text();wrapper=s[s.index('int printer_run_boot_waste_clean(void) {'):]
main=(D.parents[2]/'ap_main.c').read_text(encoding='latin1')
a=main.index('    if (link_ready && !PRINTER_CARRIAGE_TEST && PRINTER_BOOT_CARRIAGE_PREP)')
b=main.index('\n\t   //',a);block=main[a:b]
assert main.index('printer_run_boot_carriage_prepare();',a)<main.index('printer_run_boot_waste_clean();',a)<main.index('dm_gatts_init();',a)
h=r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "printer_maintenance.h"
#ifndef PRINTER_BOOT_WASTE_CLEAN
#define PRINTER_BOOT_WASTE_CLEAN 1
#endif
#define PRINTER_MAINTENANCE 1
#define PRINTER_WASTE_MAINTENANCE 1
#define PRINTER_BOOT_CARRIAGE_PREP 1
#define PRINTER_CARRIAGE_TEST 0
#define os_printf printf
static unsigned mode,homing_calls,clean_calls;
static bool s_printer_link_ready=true;
static bool printer_link_is_ready(void){return s_printer_link_ready;}
static int printer_run_boot_carriage_prepare(void){++homing_calls;return mode==3?-7:0;}
static int printer_run_maintenance(uint8_t kind){assert(homing_calls==1&&s_printer_link_ready&&kind==PRINTER_MAINT_WASTE);++clean_calls;return mode==1?-13:0;}
WRAPPER
int main(int argc,char**argv){
 assert(argc==2);mode=atoi(argv[1]);bool link_ready=mode!=2;s_printer_link_ready=link_ready;
 if(mode==4){s_printer_link_ready=false;assert(printer_run_boot_waste_clean()==(PRINTER_BOOT_WASTE_CLEAN?-7:0));assert(!clean_calls);s_printer_link_ready=true;}
 BLOCK
 if(!link_ready&&mode==2)assert(!homing_calls&&!clean_calls);
 else if(mode==3)assert(homing_calls==1&&!clean_calls&&!link_ready&&!s_printer_link_ready);
#if PRINTER_BOOT_WASTE_CLEAN
 else{
  assert(homing_calls==1&&clean_calls==1);
  assert(link_ready==(mode!=1)&&s_printer_link_ready==link_ready);
  assert(printer_run_boot_waste_clean()==-7&&clean_calls==1);
 }
#else
 else assert(homing_calls==1&&!clean_calls&&link_ready&&s_printer_link_ready);
#endif
 puts("PASS actual boot glue: homing before clean, one attempt, no firing after bad link/homing, failed clean blocks printing, disabled profile safe");
}
'''.replace('WRAPPER',wrapper).replace('BLOCK',block)
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'test.c').write_text(h)
 for flag in [1,0]:
  subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-Wno-unused-function',f'-DPRINTER_BOOT_WASTE_CLEAN={flag}','-fsanitize=address,undefined','-I'+str(D),str(t/'test.c'),'-o',str(t/'test')],check=True)
  for mode in [0,1,2,3,4]:subprocess.run([str(t/'test'),str(mode)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

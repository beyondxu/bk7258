"""Fixed address/domain scope pattern and actual no-feed four-pass controller."""
from pathlib import Path
import ctypes as C,sys,subprocess,tempfile,unittest,csv
ROOT=Path(__file__).resolve().parents[1];SRC=ROOT/'ap/app_bluetooth/gatt/printer'
sys.path.insert(0,str(ROOT/'tools'));sys.path.insert(0,str(ROOT/'tests'))
from map_audit import Mapper
from test_full_page import blob
class Scope(unittest.TestCase):
 def test_fixed_routes_from_csv_and_bounds(self):
  m=Mapper();cfg=m.config();cfg.x_pitch_centi_um=3000;cfg.x_direction=-1
  fn=m.lib.printer_map_section_scope
  fn.argtypes=[C.POINTER(C.c_uint8),C.c_uint32,C.c_uint32,C.c_uint32,C.c_void_p]
  with (ROOT/'tools/data/m0333n_v12_nozzles.csv').open() as f:rows=list(csv.DictReader(f))
  physical=sorted([r for r in rows if r['domain'] in ['1','2']],key=lambda r:-float(r['y_um']))
  for sec in range(4):
   r=physical[25+sec*50];self.assertEqual((r['address'],r['section'],r['domain']),('4',str(sec+1),'2'))
   out=(C.c_uint8*118000)();self.assertEqual(fn(out,len(out),1180,sec,C.byref(cfg)),0)
   expected=bytearray(118000)
   for x in range(300,900):expected[x*100+12+sec]=2
   self.assertEqual(bytes(out),expected)
  out=(C.c_uint8*118000)(*[255]*118000)
  for count,sec in [(1179,0),(1180,4)]:
   self.assertNotEqual(fn(out,len(out),count,sec,C.byref(cfg)),0);self.assertEqual(bytes(out),b'\xff'*118000)
  cfg.reverse_y=1;self.assertNotEqual(fn(out,len(out),1180,0,C.byref(cfg)),0)
  m.tmp.cleanup()
 def test_actual_job(self):
  harness=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "printer_page.h"
#define PRINTER_NOZZLE_TIME_SCALE 1
#define PRINTER_MAP_X_PITCH_CENTI_UM 3000
#define PRINTER_MAP_X_DIRECTION -1
#define PRINTER_MAP_FIRST_NOZZLE 0
#define PRINTER_MAP_REVERSE_Y 0
#define BK_OK 0
#define os_printf printf
static unsigned moves,off_calls,waits,live;static int scenario,oom;
static void *printer_map_psram_alloc(size_t n){if(oom)return NULL;void*p=malloc(n);if(p)++live;return p;}
static void printer_map_psram_free(void*p){assert(live);--live;free(p);}
static void printer_map_yield_cpu(void){}
static bool printer_link_is_ready(void){return true;}
static int tph_mcu_set_head_power(unsigned on){assert(!on);++off_calls;return scenario==2&&off_calls==2?-8:0;}
static void rtos_delay_milliseconds(unsigned ms){assert(ms==3000);++waits;}
static int printer_execute_cyan_line(const uint8_t*p){
 unsigned section=moves%4;assert(waits==moves+1);
 for(unsigned x=0;x<1180;x++)for(unsigned slot=0;slot<100;slot++)
 assert(p[x*100+slot]==(x>=300&&x<900&&slot==12+section?2:0));
 ++moves;return scenario==1&&moves==2?-9:0;
}
#include "printer_section_scope_job.h"
int main(int argc,char**argv){
 assert(argc==3);scenario=atoi(argv[2]);FILE*f=fopen(argv[1],"rb");assert(f);
 fseek(f,0,SEEK_END);unsigned n=ftell(f);rewind(f);uint8_t*b=malloc(n);assert(fread(b,1,n,f)==n);fclose(f);
 assert(printer_run_section_scope(b,n-1));assert(!moves&&!live&&!off_calls);
 oom=1;assert(printer_run_section_scope(b,n));assert(!moves&&!live&&!off_calls);oom=0;
 int rc=printer_run_section_scope(b,n);assert(!live);
 if(!scenario){assert(!rc&&moves==4&&waits==4&&off_calls==4);assert(!printer_run_section_scope(b,n));assert(moves==8);}
 else{assert(rc&&moves==(scenario==1?2:1));unsigned before=moves;assert(printer_run_section_scope(b,n));assert(before==moves);}
 assert(!live);free(b);puts("PASS scope controller");
}
'''
  with tempfile.TemporaryDirectory() as tmp:
   p=Path(tmp);(p/'t.c').write_text(harness);(p/'in.bin').write_bytes(blob(set()))
   subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-I'+str(SRC),str(p/'t.c'),str(SRC/'printer_mapper.c'),'-o',str(p/'t')],check=True)
   for case in range(3):subprocess.run([str(p/'t'),str(p/'in.bin'),str(case)],check=True,stdout=subprocess.DEVNULL)
if __name__=='__main__':unittest.main()

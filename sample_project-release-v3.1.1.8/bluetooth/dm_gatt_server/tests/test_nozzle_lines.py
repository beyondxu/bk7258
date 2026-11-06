"""Independent CSV verification of visible per-nozzle lines and actual job sequencing."""
from pathlib import Path
import ctypes as C,csv,sys,unittest,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1];SRC=ROOT/'ap/app_bluetooth/gatt/printer'
sys.path.insert(0,str(ROOT/'tools'));sys.path.insert(0,str(ROOT/'tests'))
from map_audit import Mapper
from test_full_page import blob
class NozzleLines(unittest.TestCase):
 def test_csv_geometry_every_used_nozzle_and_no_outside_bits(self):
  mapper=Mapper();fn=mapper.lib.printer_map_nozzle_lines
  fn.argtypes=[C.POINTER(C.c_uint8),C.c_uint32,C.c_uint32,C.c_uint32,C.c_void_p]
  with (ROOT/'tools/data/m0333n_v12_nozzles.csv').open() as f:records=list(csv.DictReader(f))
  for ch in range(3):
   rows=sorted([r for r in records if int(r['domain']) in [2*ch+1,2*ch+2]],key=lambda r:-float(r['y_um']))[:192]
   cfg=mapper.config();cfg.x_pitch_centi_um=3000;cfg.x_direction=-1
   out=(C.c_uint8*118000)();self.assertEqual(fn(out,len(out),1180,ch,C.byref(cfg)),0)
   expected=bytearray(118000)
   for row,r in enumerate(rows):
    delay=round((float(r['x_um'])-1042.84)/30)
    slot=(int(r['address'])-1)*4+int(r['section'])-1;mask=1<<(int(r['domain'])-1)
    for x in range(120+(row%6)*160+delay,216+(row%6)*160+delay):expected[x*100+slot]|=mask
   self.assertEqual(bytes(out),expected)
   self.assertEqual(sum(v.bit_count() for v in out),18432)
  out=(C.c_uint8*118000)(*[255]*118000)
  for size,count,ch in [(100,1180,0),(118000,1179,0),(118000,1180,3)]:
   self.assertNotEqual(fn(out,size,count,ch,C.byref(cfg)),0);self.assertEqual(bytes(out),b'\xff'*118000)
 def test_batch_union_exclusive_and_address_peak(self):
  mapper=Mapper();cfg=mapper.config();cfg.x_pitch_centi_um=3000;cfg.x_direction=-1
  fn=mapper.lib.printer_map_nozzle_lines_batch
  fn.argtypes=[C.POINTER(C.c_uint8),C.c_uint32,C.c_uint32,C.c_uint32,C.c_uint32,C.c_uint32,C.c_void_p]
  for ch in range(3):
   base=(C.c_uint8*118000)();self.assertEqual(fn(base,len(base),1180,ch,0,1,C.byref(cfg)),0)
   split=[]
   for batch in range(2):
    out=(C.c_uint8*118000)();self.assertEqual(fn(out,len(out),1180,ch,batch,2,C.byref(cfg)),0)
    split.append(bytes(out))
    self.assertEqual(sum(v.bit_count() for v in out),9216)
    self.assertEqual(max(sum(out[x*100+j].bit_count() for j in range(100)) for x in range(1180)),16)
    self.assertEqual(max(sum(out[x*100+a*4+j].bit_count() for j in range(4)) for a in range(25) for x in range(1180)),1)
   self.assertFalse(any(a&b for a,b in zip(*split)))
   self.assertEqual(bytes(a|b for a,b in zip(*split)),bytes(base))
  out=(C.c_uint8*118000)(*[255]*118000)
  for batch,batches in [(2,2),(0,0),(0,3)]:
   self.assertNotEqual(fn(out,len(out),1180,0,batch,batches,C.byref(cfg)),0)
   self.assertEqual(bytes(out),b'\xff'*118000)
 def test_actual_job_success_repeat_and_failures(self):
  harness=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "printer_page.h"
#define PRINTER_NOZZLE_TIME_SCALE 1U
#define PRINTER_MAP_X_PITCH_CENTI_UM 3000
#define PRINTER_MAP_X_DIRECTION -1
#define PRINTER_MAP_FIRST_NOZZLE 0
#define PRINTER_MAP_REVERSE_Y 0
#define PRINTER_PAGE_FEED_STEPS 3170
#define PRINTER_PAGE_FEED_DIRECTION 0
#define BK_OK 0
#define os_printf printf
static unsigned moves,feeds,live;static int scenario,fail_alloc;
static void *printer_map_psram_alloc(size_t n){if(fail_alloc)return NULL;void*p=malloc(n);if(p)++live;return p;}
static void printer_map_psram_free(void*p){assert(live);--live;free(p);}
static void printer_map_yield_cpu(void){}
static bool printer_link_is_ready(void){return true;}
static int printer_execute_cyan_line(const uint8_t *p){
 unsigned ch=(moves/PRINTER_NOZZLE_BATCHES)%3,dots=0;for(unsigned i=0;i<118000;i++){
 assert(!(p[i]&~(3U<<(2*ch))));dots+=__builtin_popcount(p[i]);}
 assert(dots==18432/PRINTER_NOZZLE_BATCHES);++moves;return scenario==1&&moves==2?-9:0;
}
static int tph_mcu_set_head_power(unsigned on){assert(!on);return 0;}
static int motor_page_feed(uint32_t steps,uint8_t dir,uint32_t*sent){
 assert(steps==3170&&!dir&&moves%PRINTER_NOZZLE_BATCHES==0&&(moves/PRINTER_NOZZLE_BATCHES)%3);++feeds;
 *sent=scenario==2?10:3170;return scenario==2?-1:0;
}
#include "printer_nozzle_job.h"
int main(int argc,char**argv){
 assert(argc==3);scenario=atoi(argv[2]);FILE*f=fopen(argv[1],"rb");assert(f);
 fseek(f,0,SEEK_END);unsigned n=ftell(f);rewind(f);uint8_t*b=malloc(n);assert(fread(b,1,n,f)==n);fclose(f);
 assert(printer_run_nozzle_check(b,n-1));assert(!moves&&!feeds&&!live);
 fail_alloc=1;assert(printer_run_nozzle_check(b,n));assert(!moves&&!feeds&&!live);fail_alloc=0;
 int rc=printer_run_nozzle_check(b,n);assert(!live);
 if(!scenario){assert(!rc&&moves==3*PRINTER_NOZZLE_BATCHES&&feeds==2);assert(!printer_run_nozzle_check(b,n));assert(moves==6*PRINTER_NOZZLE_BATCHES&&feeds==4);}
 else{assert(rc&&moves==(scenario==1?2:PRINTER_NOZZLE_BATCHES)&&feeds==(scenario==1?(PRINTER_NOZZLE_BATCHES==1?1:0):1));unsigned m=moves;assert(printer_run_nozzle_check(b,n));assert(moves==m);}
 assert(!live);free(b);puts("PASS nozzle job");
}
'''
  with tempfile.TemporaryDirectory() as tmp:
   p=Path(tmp);(p/'t.c').write_text(harness);(p/'in.bin').write_bytes(blob(set()))
   for batches in [1,2]:
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',f'-DPRINTER_NOZZLE_BATCHES={batches}','-I'+str(SRC),str(p/'t.c'),str(SRC/'printer_mapper.c'),'-o',str(p/'t')],check=True)
    for case in range(3):subprocess.run([str(p/'t'),str(p/'in.bin'),str(case)],check=True,stdout=subprocess.DEVNULL)
if __name__=='__main__':unittest.main()

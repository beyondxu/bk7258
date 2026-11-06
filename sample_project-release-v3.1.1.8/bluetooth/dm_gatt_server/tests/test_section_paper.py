"""Independently check C/D2 all-address paper comparison and actual job failures."""
from pathlib import Path
import ctypes as C,csv,sys,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1];SRC=ROOT/'ap/app_bluetooth/gatt/printer'
sys.path.insert(0,str(ROOT/'tools'));sys.path.insert(0,str(ROOT/'tests'))
from map_audit import Mapper
from test_full_page import blob
class Paper(unittest.TestCase):
 def test_independent_csv_all_100_nozzles(self):
  m=Mapper();cfg=m.config();cfg.x_pitch_centi_um=3000;cfg.x_direction=-1
  fn=m.lib.printer_map_section_paper;fn.argtypes=[C.POINTER(C.c_uint8),C.c_uint32,C.c_uint32,C.c_uint32,C.c_void_p]
  with (ROOT/'tools/data/m0333n_v12_nozzles.csv').open() as f:records=list(csv.DictReader(f))
  seen=set()
  for sec in range(4):
   rows=sorted([r for r in records if r['domain']=='2' and int(r['section'])==sec+1],key=lambda r:-float(r['y_um']))
   self.assertEqual(len(rows),25);out=(C.c_uint8*118000)();expected=bytearray(118000)
   for i,r in enumerate(rows):
    slot=(int(r['address'])-1)*4+sec;seen.add(slot)
    for x in range(160+(i%5)*180,256+(i%5)*180):expected[x*100+slot]=2
   self.assertEqual(fn(out,len(out),1180,sec,C.byref(cfg)),0);self.assertEqual(bytes(out),expected)
   self.assertEqual(sum(v.bit_count() for v in out),2400)
   self.assertEqual(max(sum(out[x*100+j].bit_count() for j in range(100)) for x in range(1180)),5)
   for slot in seen:
    if slot%4==sec:self.assertEqual(sum(bool(out[x*100+slot]) for x in range(1180)),96)
  self.assertEqual(seen,set(range(100)))
  out=(C.c_uint8*118000)(*[255]*118000)
  for size,count,sec in [(100,1180,0),(118000,1179,0),(118000,1180,4)]:
   self.assertNotEqual(fn(out,size,count,sec,C.byref(cfg)),0);self.assertEqual(bytes(out),b'\xff'*118000)
  m.tmp.cleanup()
 def test_job(self):
  h=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "printer_page.h"
#define PRINTER_SECTION_REVERSE_COLUMNS 1
#define PRINTER_NOZZLE_TIME_SCALE 1
#define PRINTER_MAP_X_PITCH_CENTI_UM 3000
#define PRINTER_MAP_X_DIRECTION -1
#define PRINTER_MAP_FIRST_NOZZLE 0
#define PRINTER_MAP_REVERSE_Y 0
#define BK_OK 0
#define os_printf printf
static unsigned moves,live;static int scenario,oom;
static void *printer_map_psram_alloc(size_t n){if(oom)return NULL;void*p=malloc(n);if(p)++live;return p;}
static void printer_map_psram_free(void*p){assert(live);--live;free(p);}
static void printer_map_yield_cpu(void){}
static bool printer_link_is_ready(void){return true;}
static int printer_execute_cyan_line(const uint8_t*p){
 unsigned sec=moves%4,total=0,hits[100]={0};
 for(unsigned x=0;x<1180;x++)for(unsigned slot=0;slot<100;slot++)if(p[x*100+slot]){
 assert(slot%4==sec&&p[x*100+slot]==2);++total;++hits[slot];}
 assert(total==2400);for(unsigned a=0;a<25;a++)assert(hits[a*4+sec]==96);
 /* CSV: address1 was leftmost and address2 rightmost. Verify actual dispatch swaps them. */
 for(unsigned x=0;x<1180;x++){
 assert(p[x*100+sec]==(x>=880&&x<976?2:0));
 assert(p[x*100+4+sec]==(x>=160&&x<256?2:0));
 }
 ++moves;return (scenario==1&&moves==2)||(scenario==2&&moves==1)?-9:0;
}
#include "printer_section_paper_job.h"
int main(int argc,char**argv){
 assert(argc==3);scenario=atoi(argv[2]);FILE*f=fopen(argv[1],"rb");assert(f);
 fseek(f,0,SEEK_END);unsigned n=ftell(f);rewind(f);uint8_t*b=malloc(n);assert(fread(b,1,n,f)==n);fclose(f);
 assert(printer_run_section_paper(b,n-1));assert(!moves&&!live);
 oom=1;assert(printer_run_section_paper(b,n));assert(!moves&&!live);oom=0;
 int rc=printer_run_section_paper(b,n);assert(!live);
 if(!scenario){assert(!rc&&moves==4);assert(!printer_run_section_paper(b,n));assert(moves==8);}
 else{assert(rc&&moves==(scenario==1?2:1));unsigned before=moves;assert(printer_run_section_paper(b,n));assert(before==moves);}
 assert(!live);free(b);
}
'''
  with tempfile.TemporaryDirectory() as tmp:
   p=Path(tmp);(p/'t.c').write_text(h);(p/'in.bin').write_bytes(blob(set()))
   subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-I'+str(SRC),str(p/'t.c'),str(SRC/'printer_mapper.c'),'-o',str(p/'t')],check=True)
   for case in range(3):subprocess.run([str(p/'t'),str(p/'in.bin'),str(case)],check=True,stdout=subprocess.DEVNULL)
if __name__=='__main__':unittest.main()

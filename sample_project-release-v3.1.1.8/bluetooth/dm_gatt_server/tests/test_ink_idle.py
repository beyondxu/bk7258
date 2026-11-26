"""Exact3/15-minute boundaries, unknown boot, successful ink updates and RTC rollback."""
from pathlib import Path
import subprocess,tempfile
D=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
h=r'''
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
static uint64_t now;
static uint64_t bk_aon_rtc_get_us(void){return now;}
#include "printer_ink_idle.h"
int main(void){
 uint64_t idle;unsigned known;
 assert(printer_prespray_plan(0,&idle,&known)==800&&!known);
 printer_prespray_note_ink(); /* A timestamp of zero is valid. */
 for(unsigned i=0;i<6;++i){
  const uint64_t gaps[]={0,179999999,180000000,899999999,900000000,UINT64_C(5000000000)};
  const unsigned rounds[]={5,5,50,50,800,800};
  assert(printer_prespray_plan(gaps[i],&idle,&known)==rounds[i]&&known&&idle==gaps[i]);
 }
 now=UINT64_C(5000000000);printer_prespray_note_ink();
 assert(printer_prespray_plan(now+1,&idle,&known)==5&&known&&idle==1);
 assert(printer_prespray_plan(now-1,&idle,&known)==800&&!known);
 assert(printer_prespray_plan(now+UINT64_C(180000000),&idle,&known)==50&&known);
 puts("PASS RTC policy: unknown=800, <3min=5, 3..15min=50, >=15min=800; 64-bit, zero timestamp, rollback");
}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'a.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-I'+str(D),str(p/'a.c'),'-o',str(p/'a')],check=True)
 subprocess.run([str(p/'a')],check=True)

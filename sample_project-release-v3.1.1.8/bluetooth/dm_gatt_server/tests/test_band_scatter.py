"""Optimized scatter renderer vs the original scratch + mapper-column algorithm."""
from pathlib import Path
import subprocess,tempfile,sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from test_full_page import blob
D=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
h=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "printer_page.h"
static unsigned bit(const uint8_t*p,unsigned y){return !!(p[y/8]&(0x80U>>(y%8)));}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);uint8_t*b=malloc(n);assert(fread(b,1,n,f)==(size_t)n);fclose(f);
 #ifndef TEST_PITCH
#define TEST_PITCH 3000
#endif
 printer_map_config_t cfg={TEST_PITCH,-1,0,0};printer_page_t page;
 assert(!printer_page_prepare(b,n,&cfg,&page,malloc,free,NULL));
 if(TEST_PITCH==4389){
  assert(page.width==1139&&page.height==1477);
  assert(page.width*TEST_PITCH>4990000U&&page.width*TEST_PITCH<5010000U);
  /* Physical frame aspect must still match the full APP canvas. */
  int64_t error=(int64_t)page.height*4233*1180-(int64_t)page.width*TEST_PITCH*1476;
  assert(llabs(error)<4233LL*1180);
 }
 uint8_t *scratch=malloc(page.width*72),*actual=malloc(118000),*expected=malloc(118000);
 unsigned sizes[]={192,48,24};
 for(unsigned size=0;size<3;size++)for(unsigned reverse=0;reverse<2;reverse++){
  unsigned total[3]={0};
  for(unsigned first=0;first<page.height;first+=sizes[size]){
   unsigned dots[3],gold[3]={0};
   int rc=reverse?printer_page_render_band_reverse(&page,first,sizes[size],actual,118000,scratch,page.width*72,dots,NULL)
                 :printer_page_render_band(&page,first,sizes[size],actual,118000,scratch,page.width*72,dots,NULL);
   assert(!rc);memset(scratch,0,page.width*72);
   for(unsigned ch=0;ch<3;ch++)for(unsigned x=0;x<page.width;x++)for(unsigned row=0;row<sizes[size];row++){
    unsigned y=first+row;
    if(y<page.height&&bit(page.planes+ch*page.width*page.stride+x*page.stride,y)){
     unsigned dx=reverse?page.width-1-x:x;
     scratch[ch*page.width*24+dx*24+row/8]|=0x80U>>(row%8);++gold[ch];
    }
   }
   printer_map_config_t route_cfg=page.cfg;if(reverse)route_cfg.x_direction=-route_cfg.x_direction;
   for(unsigned x=0;x<1180;x++)assert(!printer_map_column(scratch,page.width,x,&route_cfg,expected+x*100));
   assert(!memcmp(actual,expected,118000)&&!memcmp(dots,gold,sizeof(gold)));
   for(unsigned ch=0;ch<3;ch++)total[ch]+=dots[ch];
  }
  assert(!memcmp(total,page.output_dots,sizeof(total)));
 }
 free(scratch);free(actual);free(expected);printer_page_release(&page,free);free(b);
 puts("PASS: 192/48/24-row forward/reverse bytes match original mapper reference, all CMY pixels once");
}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.c').write_text(h)
 (p/'fixture.bin').write_bytes(blob({(ch,x,y)for ch in range(3)for x in [0,1,33,400+ch,590,1178,1179]for y in list(range(0,1476,19))+[0,1,191,192,193,1474,1475]}))
 for pitch in [3000,4389]:
  subprocess.run(['cc','-std=c99','-O2','-Wall','-Wextra','-Werror','-DTEST_PITCH='+str(pitch),'-I'+str(D),str(p/'test.c'),str(D/'printer_mapper.c'),str(D/'printer_page.c'),'-o',str(p/'test')],check=True)
  for source in [p/'fixture.bin']+[Path(a)for a in sys.argv[1:]]:subprocess.run([str(p/'test'),str(source)],check=True)

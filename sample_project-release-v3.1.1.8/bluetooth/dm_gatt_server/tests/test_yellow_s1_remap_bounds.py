"""Actual remapper must preserve all other pixels and reject before any mutation."""
from pathlib import Path
import os,subprocess,tempfile
D=Path(__file__).resolve().parents[1]/'ap/app_bluetooth/gatt/printer'
h=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define os_printf printf
#include "printer_yellow_s1_remap.h"
static uint8_t data[3*4*124], before[sizeof(data)];
static void mark(unsigned ch,unsigned x,unsigned y){data[ch*4*124+x*124+y/8]|=0x80>>(y%8);}
static unsigned get(const uint8_t*p,unsigned ch,unsigned x,unsigned y){return (p[ch*4*124+x*124+y/8]>>(7-y%8))&1;}
int main(void){printer_page_t p={0};p.planes=data;p.width=4;p.height=992;p.stride=124;printer_map_default_config(&p.cfg);
for(unsigned a=9;a<=23;a++)for(unsigned b=29;b<=43;b++){printer_map_route_t x,y;assert(!printer_map_route(2,a,&p.cfg,&x)&&!printer_map_route(2,b,&p.cfg,&y));assert(x.slot!=y.slot||x.mask!=y.mask);}
mark(0,0,393);mark(1,1,400);mark(2,0,393);mark(2,1,400);mark(2,3,407);mark(2,1,520);mark(2,0,194);memcpy(before,data,sizeof(data));
assert(!printer_yellow_s1_remap(&p));
for(unsigned c=0;c<3;c++)for(unsigned x=0;x<4;x++)for(unsigned y=0;y<992;y++){unsigned expected=get(before,c,x,y);if(c==2){if(y>=393&&y<=407)expected=0;else if(y>=413&&y<=427)expected=get(before,c,x,y-20);}assert(get(data,c,x,y)==expected);}
for(unsigned fail=0;fail<5;fail++){memcpy(data,before,sizeof(data));p.cfg.first_nozzle=0;p.cfg.reverse_y=0;
 if(fail==0)mark(2,2,408);if(fail==1)mark(2,2,413);if(fail==2)p.cfg.first_nozzle=1;if(fail==3)p.cfg.reverse_y=1;
 if(fail==4)for(unsigned x=0;x<4;x++)for(unsigned y=393;y<=407;y++)data[2*4*124+x*124+y/8]&=~(0x80>>(y%8));
 uint8_t saved[sizeof(data)];memcpy(saved,data,sizeof(data));assert(printer_yellow_s1_remap(&p)==(fail==4?-6:-4));assert(!memcmp(saved,data,sizeof(data)));}
assert(printer_yellow_s1_remap(NULL)==-4);puts("PASS: disjoint physical routes, exact pixel relocation, all other colors/rows unchanged, reject-without-mutation");}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(h)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-fsanitize=address,undefined','-I'+str(D),str(p/'test.c'),str(D/'printer_mapper.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

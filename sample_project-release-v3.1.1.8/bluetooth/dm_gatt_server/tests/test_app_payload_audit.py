"""Actual APP audit, exact raw export, absent/present color and allocation cleanup."""
from pathlib import Path
import os,subprocess,tempfile,sys,re,zlib
from test_full_page import blob
R=Path(__file__).resolve().parents[1];D=R/'ap/app_bluetooth/gatt/printer'
sys.path.insert(0,str(R/'tools'))
from extract_app_bin import extract_last_bin
h=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define os_printf printf
#define PRINTER_FIXED_S1_COMPARE 1
#define PRINTER_MAP_X_PITCH_CENTI_UM 3000
#define PRINTER_MAP_X_DIRECTION -1
#define PRINTER_MAP_FIRST_NOZZLE 0
#define PRINTER_MAP_REVERSE_Y 0
static unsigned live,calls,fail_at;
static void *printer_map_psram_alloc(size_t n){if(++calls==fail_at)return NULL;void*p=malloc(n);if(p)++live;return p;}
static void printer_map_psram_free(void*p){if(p){assert(live);--live;free(p);}}
static void printer_map_yield_cpu(void){}
static void rtos_delay_milliseconds(unsigned ms){assert(ms==20);}
#include "printer_app_payload_audit.h"
int main(int argc,char**argv){assert(argc==4);FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);uint8_t*b=malloc(n);assert(fread(b,1,n,f)==(size_t)n);fclose(f);fail_at=atoi(argv[2]);int result=printer_run_app_payload_audit(b,n);assert(result==atoi(argv[3])&&!live);free(b);}
'''
# A PackBits compressed native stream, independent of the production decoder.
def compressed(body):
    packed=bytearray();i=0
    while i<len(body):
        run=1
        while i+run<len(body) and body[i+run]==body[i] and run<127:run+=1
        if run>=3:packed.extend([257-run,body[i]]);i+=run
        else:packed.extend([0,body[i]]);i+=1
    return b'\x1b%9\x01'+packed
points={(0,200,50),(0,400,300),(1,201,60),(2,202,100),(2,450,1200)}
with tempfile.TemporaryDirectory() as tmp:
    p=Path(tmp);(p/'test.c').write_text(h)
    subprocess.run(['cc','-std=c99','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(D),str(p/'test.c'),str(D/'printer_page.c'),str(D/'printer_mapper.c'),'-o',str(p/'test')],check=True)
    for case in [points,{pt for pt in points if pt[0]!=2},set()]:
        raw=compressed(blob(case));assert len(raw)<131072
        (p/'input.bin').write_bytes(raw)
        out=subprocess.check_output([str(p/'test'),str(p/'input.bin'),'0','0'],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},text=True)
        assert extract_last_bin(out)==raw
        wrapped=re.sub(r'(\[APP_BIN_HEX\] offset=[0-9A-F]{8} data=)([0-9A-F]{47})([0-9A-F]+)',
                       lambda m:m[1]+m[2]+'\n[17:13:47.360]收←◆'+m[3],out,count=1)
        assert extract_last_bin(wrapped)==raw
        source={int(c):int(n) for c,n in re.findall(r'\[APP_CHANNEL\] id=(\d+) decoder_label=\w source_dots=(\d+)',out)}
        assert source=={ch+1:sum(c==ch for c,x,y in case) for ch in range(3)}
        routes=[tuple(map(int,v)) for v in re.findall(r'\[APP_ROUTE\] D=(\d+) S=(\d+) dots=(\d+)',out)]
        assert len(routes)==24
        resized=list(map(int,re.findall(r'\[APP_CHANNEL\].*resized_dots=(\d+)',out)))
        assert [sum(n for d,s,n in routes if (d-1)//2==ch) for ch in range(3)]==resized
        if not any(c==2 for c,x,y in case):assert all(n==0 for d,s,n in routes if d>=5)
    # Oversized exports explicitly report truncation; full-page analysis still runs.
    raw=blob(points);(p/'input.bin').write_bytes(raw)
    out=subprocess.check_output([str(p/'test'),str(p/'input.bin'),'0','0'],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},text=True)
    assert 'complete=0' in out and '[APP_DECODE_END] code=0' in out
    try:extract_last_bin(out)
    except ValueError:pass
    else:raise AssertionError('accepted deliberately truncated raw export')
    raw=compressed(blob(points));(p/'input.bin').write_bytes(raw)
    for fail in [1,2,3,4,5,6]:
        subprocess.run([str(p/'test'),str(p/'input.bin'),str(fail),'-2'],check=True,stdout=subprocess.DEVNULL,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
    (p/'input.bin').write_bytes(b'bad')
    out=subprocess.check_output([str(p/'test'),str(p/'input.bin'),'0','-3'],text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
    assert extract_last_bin(out)==b'bad' and '[APP_CHANNEL]' not in out
    for bad in [out.replace('data=626164','data=626165'),out.replace('offset=00000000','offset=00000001'),out.replace('complete=1','complete=0'),out.split('[APP_BIN_END]')[0]]:
        try:extract_last_bin(bad)
        except ValueError:pass
        else:raise AssertionError('accepted corrupt/incomplete raw export')
print('PASS: exact APP raw BIN recovery, real decoder color counts and D/S mapping, absent yellow, blank page, corrupt input, allocation cleanup and bad UART export rejection')

worker=(D/'hw_printer_stub.c').read_text();a=worker.index('static void printer_worker_thread');audit=worker.index('#if PRINTER_APP_PAYLOAD_AUDIT',a);normal=worker.index('#if PRINTER_FULL_PAGE',audit)
assert 'printer_run_app_payload_audit' in worker[audit:normal] and 'continue;' in worker[audit:normal]
assert not any(name in (D/'printer_app_payload_audit.h').read_text() for name in ['tph_mcu_set_head_power','carriage_stream_move','motor_page_feed(','tph_fire_raw'])

"""Independent CSV proves all600 physical nozzles receive the selected ordinary dose."""
from pathlib import Path
import ctypes as C,csv,subprocess,tempfile
from decimal import Decimal
P=Path(__file__).resolve().parents[1];D=P/'ap/app_bluetooth/gatt/printer'
class Config(C.Structure):_fields_=[('pitch',C.c_uint32),('direction',C.c_int32),('first',C.c_uint32),('reverse',C.c_uint32)]
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'a.c').write_text('#include "printer_prespray_pattern.h"\nint build(uint8_t*p,uint32_t n,unsigned rounds,const printer_map_config_t*c){return printer_prespray_build(p,n,rounds,c);}\n')
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-Wno-unused-function','-fPIC','-shared','-I'+str(D),str(t/'a.c'),str(D/'printer_mapper.c'),'-o',str(t/'a.so')],check=True)
 lib=C.CDLL(str(t/'a.so'));buf=(C.c_uint8*118000)();lib.build.argtypes=[C.POINTER(C.c_uint8),C.c_uint32,C.c_uint,C.POINTER(Config)]
 records=list(csv.DictReader((P/'tools/data/m0333n_v12_nozzles.csv').open()))
 for rounds in [1,5,8,10,20,39,40]:
  gold=bytearray(118000);seen=set()
  for ch in range(3):
   rows=sorted([r for r in records if (int(r['domain'])-1)//2==ch],key=lambda r:-Decimal(r['y_um']))
   assert len(rows)==200
   for n,r in enumerate(rows):
    a,s,d=map(int,[r['address'],r['section'],r['domain']]);seen.add((a,s,d))
    # Independent burst/group enumeration, instead of the C per-nozzle formula.
    for start in range(0,rounds,8):
     count=min(8,rounds-start)
     for shot in range(count):
      col=40+start*24+((d-1)*4+((s-1)//2)*2+(a-1)//13)*count+shot
      gold[col*100+(a-1)*4+s-1]|=1<<(d-1)
  assert len(seen)==600 and sum(v.bit_count() for v in gold)==600*rounds
  assert max(sum(v.bit_count() for v in gold[x*100:x*100+100]) for x in range(1180))==26
  for x in range(1180):
   col=gold[x*100:x*100+100];mask=0;sections=set()
   active=[a for a in range(25) if any(col[a*4:a*4+4])]
   assert not active or active[-1]-active[0]+1<=13
   for slot,v in enumerate(col):
    mask|=v
    if v:sections.add(slot%4)
   assert mask.bit_count()<=1 and (sections <= {0,1} or sections <= {2,3})
   assert all(sum(v.bit_count() for v in col[a*4:a*4+4])<=2 for a in range(25))
  for direction in [-1,1]:
   cfg=Config(4388,direction,0,0);assert not lib.build(buf,len(buf),rounds,C.byref(cfg));assert bytes(buf)==gold
   for r in records:
    a,s,d=map(int,[r['address'],r['section'],r['domain']]);cols=[x for x in range(1180)if buf[x*100+(a-1)*4+s-1]&(1<<(d-1))]
    assert len(cols)==rounds and all(b-a>=1 for a,b in zip(cols,cols[1:]))
    for start in range(0,rounds,8):
     burst=cols[start:start+8]
     assert all(b-a==1 for a,b in zip(burst,burst[1:]))
    run=1
    for a,b in zip(cols,cols[1:]):
     run=run+1 if b-a==1 else 1
     assert run<=8
 for cfg in [Config(4388,-1,1,0),Config(4388,-1,0,1)]:assert lib.build(buf,len(buf),5,C.byref(cfg))
 cfg=Config(4388,-1,0,0)
 assert lib.build(None,118000,5,C.byref(cfg));assert lib.build(buf,117999,5,C.byref(cfg));assert lib.build(buf,118000,5,None)
 for rounds in [0,41]:assert lib.build(buf,118000,rounds,C.byref(cfg))
print('PASS independent CSV:600 unique nozzles,1/5/8/10/20/39/40 exact doses,one Domain and paired Sections per column,two requests/address,8-shot bursts,invalid dose rejection')

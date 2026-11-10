"""Independent CSV: enumerate every long original C trace, both domains, both orders."""
from pathlib import Path
from decimal import Decimal
import ctypes as C,csv,tempfile,subprocess
P=Path(__file__).resolve().parents[1];D=P/'ap/app_bluetooth/gatt/printer'
class Config(C.Structure):_fields_=[('pitch',C.c_uint32),('direction',C.c_int32),('first',C.c_uint32),('reverse',C.c_uint32)]
class Route(C.Structure):_fields_=[('slot',C.c_uint32),('mask',C.c_uint32),('delay',C.c_uint32),('row',C.c_uint32)]
class Plan(C.Structure):_fields_=[('route',Route*12),('dots',C.c_uint32*12),('count',C.c_uint32),('baseline',C.c_uint32),('cyan',C.c_uint32),('short_dots',C.c_uint32),('short_nozzles',C.c_uint32)]
rows=list(csv.DictReader((P/'tools/data/m0333n_v12_nozzles.csv').open()))
geometry={ch:sorted([r for r in rows if (int(r['domain'])-1)//2==ch],key=lambda r:-Decimal(r['y_um'])) for ch in range(3)}
def raw(c_rows,short=False):
 b=bytearray(118000)
 for ch,ns in [(0,c_rows),(1,range(54,62)),(2,range(54,62))]:
  for i,n in enumerate(ns):
   r=geometry[ch][n];slot=(int(r['address'])-1)*4+int(r['section'])-1;mask=1<<(int(r['domain'])-1);offset=(int(Decimal(r['x_um'])*100)-104284+1500)//3000;length=895-(1 if ch==0 and i==0 else 0)
   for x in range(170+offset,170+offset+length):b[x*100+slot]|=mask
 if short:
  r=geometry[0][120];slot=(int(r['address'])-1)*4+int(r['section'])-1;b[slot]|=1<<(int(r['domain'])-1)
 return b
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'wrapper.c').write_text('''#include "printer_cyan_inventory.h"
int collect(const uint8_t*b,const printer_map_config_t*c,printer_ci_plan_t*p){return printer_ci_collect(b,c,p);}
int build(const uint8_t*b,uint8_t*out,uint32_t bytes,const printer_ci_plan_t*p,unsigned pass,unsigned rev){return printer_ci_build(b,out,bytes,p,pass,rev);}
''')
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fPIC','-shared','-I'+str(D),str(t/'wrapper.c'),str(D/'printer_mapper.c'),'-o',str(t/'lib.so')],check=True)
 lib=C.CDLL(str(t/'lib.so'));B=C.c_uint8*118000;cfg=Config(3000,-1,0,0);plan=Plan()
 lib.collect.argtypes=[C.POINTER(C.c_uint8),C.POINTER(Config),C.POINTER(Plan)];lib.build.argtypes=[C.POINTER(C.c_uint8),C.POINTER(C.c_uint8),C.c_uint32,C.POINTER(Plan),C.c_uint,C.c_uint]
 for size in [8,12]:
  gold=raw(range(54,54+size),True);base=B.from_buffer_copy(gold);assert not lib.collect(base,C.byref(cfg),C.byref(plan));assert plan.count==size and plan.short_dots==plan.short_nozzles==1
  assert list(plan.dots)[:size]==[894]+[895]*(size-1);assert plan.cyan==sum(plan.dots)+1
  by_id={}
  for rev in [0,1]:
   seen=set()
   for p in range(size+2):
    out=B();assert not lib.build(base,out,118000,C.byref(plan),p,rev)
    if p in [0,size+1]:assert bytes(out)==gold;continue
    i=size-p if rev else p-1;n=54+i;seen.add(n);r=geometry[0][n];route=plan.route[i]
    assert (route.row,route.slot,route.mask)==(n,(int(r['address'])-1)*4+int(r['section'])-1,1<<(int(r['domain'])-1))
    expected=bytearray(118000)
    for x in range(1180):expected[x*100+route.slot]=gold[x*100+route.slot]&route.mask
    assert bytes(out)==expected and sum(v.bit_count() for v in expected)==plan.dots[i]
    assert max(sum(v.bit_count() for v in expected[x*100:(x+1)*100]) for x in range(1180))==1
    if rev:assert bytes(out)==by_id[i]
    else:by_id[i]=bytes(out)
   assert seen==set(range(54,54+size));assert bytes(base)==gold
  assert lib.build(base,B(),118000,C.byref(plan),size+2,0);assert lib.build(base,B(),117999,C.byref(plan),1,0);assert lib.build(base,B(),118000,C.byref(plan),1,2)
 for invalid in [range(54,67),[54,56,58],[49,50],[54,72]]:assert lib.collect(B.from_buffer_copy(raw(invalid)),C.byref(cfg),C.byref(plan))
print('PASS: independent CSV enumeration, every eligible C nozzle once including D1/D2, exact original point subsets and count, ASC/DESC same per-nozzle bytes, original controls, 12-nozzle boundary, explicit short-bit accounting, no silent truncation/config/bounds rejection')

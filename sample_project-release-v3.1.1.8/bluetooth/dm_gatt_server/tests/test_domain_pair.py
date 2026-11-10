"""Fixed addresses/S2; same source time slots across D1,D2 and paired selection."""
from pathlib import Path
import ctypes as C,csv,tempfile,subprocess
from decimal import Decimal
P=Path(__file__).resolve().parents[1];D=P/'ap/app_bluetooth/gatt/printer'
class Config(C.Structure):_fields_=[('pitch',C.c_uint32),('direction',C.c_int32),('first',C.c_uint32),('reverse',C.c_uint32)]
class Route(C.Structure):_fields_=[('slot',C.c_uint32),('mask',C.c_uint32),('delay',C.c_uint32),('row',C.c_uint32)]
rows=list(csv.DictReader((P/'tools/data/m0333n_v12_nozzles.csv').open()))
gold=bytearray(118000);preferred=None
for ch in range(3):
 ordered=sorted([r for r in rows if (int(r['domain'])-1)//2==ch],key=lambda r:-Decimal(r['y_um']))
 for n in range(54,62):
  r=ordered[n];slot=(int(r['address'])-1)*4+int(r['section'])-1;mask=1<<(int(r['domain'])-1);delay=(int(Decimal(r['x_um'])*100)-104284+1500)//3000
  length=894 if ch==0 and slot==5 and mask==2 else 895
  for x in range(170+delay,170+delay+length):gold[x*100+slot]|=mask
  if ch==0 and slot==5 and mask==2:preferred=(slot,mask,delay,n)
assert preferred is not None
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'wrapper.c').write_text('''#include "printer_domain_pair.h"
int select(const uint8_t*p,const printer_map_config_t*c,printer_map_route_t*r,uint32_t*n,uint32_t*total){return printer_dp_select(p,c,r,n,total);}
int build(const uint8_t*p,uint8_t*out,uint32_t n,const printer_map_route_t*r,unsigned c){return printer_dp_build(p,out,n,r,c);}
unsigned which(unsigned pass,unsigned rev){return printer_dp_case(pass,rev);}
''')
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-shared','-fPIC','-I'+str(D),str(t/'wrapper.c'),str(D/'printer_mapper.c'),'-o',str(t/'lib.so')],check=True)
 lib=C.CDLL(str(t/'lib.so'));B=C.c_uint8*118000;cfg=Config(3000,-1,0,0);r=Route();dots=C.c_uint32();total=C.c_uint32();base=B.from_buffer_copy(gold)
 lib.select.argtypes=[C.POINTER(C.c_uint8),C.POINTER(Config),C.POINTER(Route),C.POINTER(C.c_uint32),C.POINTER(C.c_uint32)];lib.build.argtypes=[C.POINTER(C.c_uint8),C.POINTER(C.c_uint8),C.c_uint32,C.POINTER(Route),C.c_uint]
 assert not lib.select(base,C.byref(cfg),C.byref(r),C.byref(dots),C.byref(total));assert (r.slot,r.mask,r.delay,r.row)==preferred and dots.value==894
 canonical={}
 for rev in [0,1]:
  seq=[]
  for p in range(8):
   case=lib.which(p,rev);seq.append(case);out=B();assert not lib.build(base,out,118000,C.byref(r),case)
   if case in [0,7]:assert bytes(out)==gold;continue
   addr=2 if case<=3 else 5;mask=1+(case-1)%3;expected=bytearray(118000)
   for x in range(1180):
    if gold[x*100+5]&2:expected[x*100+(addr-1)*4+1]=mask
   assert bytes(out)==expected;assert sum(v.bit_count() for v in expected)==894*mask.bit_count()
   if rev:assert bytes(out)==canonical[case]
   else:canonical[case]=bytes(out)
  assert seq==([0,1,2,3,4,5,6,7] if not rev else [0,6,5,4,3,2,1,7]);assert bytes(base)==gold
 assert lib.build(base,B(),117999,C.byref(r),1);assert lib.build(base,B(),118000,C.byref(r),8);assert lib.build(base,base,118000,C.byref(r),1)
 invalid=bytearray(gold)
 for i in range(len(invalid)):invalid[i]&=~2
 assert lib.select(B.from_buffer_copy(invalid),C.byref(cfg),C.byref(r),C.byref(dots),C.byref(total))
print('PASS: independent CSV source, preferred known D2 trace, exact active columns at both addresses/S2, D1/D2/03 bits and totals, all other bytes zero, original controls, reverse order and data invariance, bounds/missing-D2 rejection')

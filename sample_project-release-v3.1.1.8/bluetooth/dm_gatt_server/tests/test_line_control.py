"""Keep original long-line pixels; remove all but one selected nozzle per color."""
from pathlib import Path
from decimal import Decimal
import ctypes as C,csv,subprocess,tempfile
P=Path(__file__).resolve().parents[1];D=P/'ap/app_bluetooth/gatt/printer'
class Config(C.Structure):_fields_=[('pitch',C.c_uint32),('direction',C.c_int32),('first',C.c_uint32),('reverse',C.c_uint32)]
class Route(C.Structure):_fields_=[('slot',C.c_uint32),('mask',C.c_uint32),('delay',C.c_uint32),('row',C.c_uint32)]
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'wrapper.c').write_text('''#include "printer_line_control.h"
int select(const uint8_t*p,const printer_map_config_t*c,printer_map_route_t*r,uint32_t*n,uint32_t*total){return printer_lc_select(p,c,r,n,total);}
int build(const uint8_t*p,uint8_t*out,uint32_t n,unsigned pass,const printer_map_route_t*r){return printer_lc_build(p,out,n,pass,r);}
''')
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-shared','-fPIC','-I'+str(D),str(t/'wrapper.c'),str(D/'printer_mapper.c'),'-o',str(t/'lib.so')],check=True)
 lib=C.CDLL(str(t/'lib.so'));B=C.c_uint8*118000
 lib.select.argtypes=[C.POINTER(C.c_uint8),C.POINTER(Config),C.POINTER(Route),C.POINTER(C.c_uint32),C.POINTER(C.c_uint32)]
 lib.build.argtypes=[C.POINTER(C.c_uint8),C.POINTER(C.c_uint8),C.c_uint32,C.c_uint,C.POINTER(Route)]
 rows=list(csv.DictReader((P/'tools/data/m0333n_v12_nozzles.csv').open()));cfg=Config(3000,-1,0,0);gold=bytearray(118000);expected=[]
 for ch in range(3):
  physical=sorted([r for r in rows if (int(r['domain'])-1)//2==ch],key=lambda r:-Decimal(r['y_um']))
  for n in range(92,99):
   r=physical[n];slot=(int(r['address'])-1)*4+int(r['section'])-1;mask=1<<(int(r['domain'])-1);delay=(int(Decimal(r['x_um'])*100)-104284+1500)//3000
   for x in range(170+delay,170+delay+895):gold[x*100+slot]|=mask
   if n==95:expected.append((slot,mask,delay,n))
 base=B.from_buffer_copy(gold);chosen=(Route*3)();counts=(C.c_uint32*3)();total=C.c_uint32()
 assert not lib.select(base,C.byref(cfg),chosen,counts,C.byref(total));assert list(counts)==[895]*3 and total.value==18795
 for ch,r in enumerate(chosen):assert (r.slot,r.mask,r.delay,r.row)==expected[ch]
 for p in range(5):
  out=B();assert not lib.build(base,out,118000,p,chosen)
  if p in [0,4]:assert bytes(out)==gold
  else:
   g=bytearray(118000);r=chosen[p-1]
   for x in range(1180):g[x*100+r.slot]=gold[x*100+r.slot]&r.mask
   assert bytes(out)==g and sum(v.bit_count() for v in g)==895
   assert max(sum(v.bit_count() for v in g[x*100:(x+1)*100]) for x in range(1180))==1
  assert bytes(base)==gold
 assert lib.build(base,B(),117999,0,chosen);assert lib.build(base,B(),118000,5,chosen);assert lib.build(base,base,118000,1,chosen)
 for data in [bytearray(118000),bytearray([0xC0])+bytearray(117999),bytearray([0x3F]*6)+bytearray(117994)]:assert lib.select(B.from_buffer_copy(data),C.byref(cfg),chosen,counts,C.byref(total))
 # A 16-dot diagnostic-like pattern is rejected before any hardware is involved.
 sparse=bytearray(118000)
 for ch in range(3):
  slot,mask,_,_=expected[ch]
  for x in range(16):sparse[x*100+slot]|=mask
 assert lib.select(B.from_buffer_copy(sparse),C.byref(cfg),chosen,counts,C.byref(total))
print('PASS: independent CSV long-line fixture; central original nozzle selection, original CMY controls byte-identical, single passes are exact895-dot subsets with max1 bit/column, unchanged source and input/bounds/sparse rejection')

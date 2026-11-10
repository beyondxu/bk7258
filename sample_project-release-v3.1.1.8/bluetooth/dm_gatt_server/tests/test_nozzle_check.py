"""Independent CSV geometry vs every actual serialized diagnostic bit: all 600 nozzles."""
from pathlib import Path
import ctypes as C,csv,subprocess,tempfile
from decimal import Decimal
P=Path(__file__).resolve().parents[1];D=P/'ap/app_bluetooth/gatt/printer'
class Config(C.Structure):_fields_=[('pitch',C.c_uint32),('direction',C.c_int32),('first',C.c_uint32),('reverse',C.c_uint32)]
class Route(C.Structure):_fields_=[('slot',C.c_uint32),('mask',C.c_uint32),('delay',C.c_uint32),('row',C.c_uint32)]
def test_all():
 with tempfile.TemporaryDirectory() as tmp:
  t=Path(tmp);(t/'wrapper.c').write_text('''#include "printer_nozzle_check.h"
int validate(const printer_map_config_t*c){return printer_nc_validate(c);}
int build(uint8_t*p,uint32_t n,unsigned ch,unsigned g,const printer_map_config_t*c){return printer_nc_build(p,n,ch,g,c);}
int route(unsigned ch,unsigned n,const printer_map_config_t*c,printer_map_route_t*r){return printer_nc_route(ch,n,c,r);}
''')
  subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-shared','-fPIC','-I'+str(D),str(t/'wrapper.c'),str(D/'printer_mapper.c'),'-o',str(t/'lib.so')],check=True)
  lib=C.CDLL(str(t/'lib.so'));lib.validate.argtypes=[C.POINTER(Config)];lib.build.argtypes=[C.POINTER(C.c_uint8),C.c_uint32,C.c_uint,C.c_uint,C.POINTER(Config)];lib.route.argtypes=[C.c_uint,C.c_uint,C.POINTER(Config),C.POINTER(Route)]
  reference={(r['color'],int(r['physical_row'])):r for r in csv.DictReader((P/'tests/nozzle-check/nozzle-map.csv').open())}
  assert len(reference)==600
  records=list(csv.DictReader((P/'tools/data/m0333n_v12_nozzles.csv').open()))
  for direction in [-1,1]:
   cfg=Config(3000,direction,0,0);assert not lib.validate(C.byref(cfg));seen=set();total=0
   xvals={int(r['domain']):int(Decimal(r['x_um'])*100) for r in records};offsets={d:(x-xvals[6]+1500)//3000 for d,x in xvals.items()}
   for ch in range(3):
    rows=sorted([r for r in records if (int(r['domain'])-1)//2==ch],key=lambda r:-Decimal(r['y_um']))
    assert len(rows)==len({r['y_um'] for r in rows})==200
    for g in range(4):
     buf=(C.c_uint8*118000)();assert not lib.build(buf,118000,ch,g,C.byref(cfg));gold=bytearray(118000)
     for i,r in enumerate(rows[g*50:(g+1)*50]):
      addr,sec,dom=map(int,(r['address'],r['section'],r['domain']));key=(addr,sec,dom);assert key not in seen;seen.add(key)
      delay=offsets[dom] if direction==-1 else offsets[1]-offsets[dom];first=80+i*20+delay
      assert sec==g+1
      if direction==-1:
       ref=reference[('CMY'[ch],g*50+i+1)]
       assert (int(ref['address']),int(ref['section']),int(ref['domain']),int(ref['first_column']),int(ref['last_column']),int(ref['requested_dots']))==(addr,sec,dom,first,first+15,16)
      route=Route();assert not lib.route(ch,g*50+i,C.byref(cfg),C.byref(route))
      assert (route.slot,route.mask,route.delay,route.row)==((addr-1)*4+sec-1,1<<(dom-1),delay,g*50+i)
      for x in range(first,first+16):gold[x*100+route.slot]|=route.mask
     assert bytes(buf)==gold
     counts=[sum(int(v).bit_count() for v in gold[x*100:(x+1)*100]) for x in range(1180)]
     assert max(counts)==1 and sum(counts)==800;total+=sum(counts)
   assert len(seen)==600 and total==9600
  cfg=Config(3000,-1,0,0);buf=(C.c_uint8*118000)();route=Route()
  assert lib.build(buf,117999,0,0,C.byref(cfg));assert lib.build(buf,118000,3,0,C.byref(cfg));assert lib.build(buf,118000,0,4,C.byref(cfg));assert lib.build(None,118000,0,0,C.byref(cfg));assert lib.route(0,200,C.byref(cfg),C.byref(route))
  for cfg in [Config(3000,-1,1,0),Config(3000,-1,0,1),Config(0,-1,0,0)]:assert lib.validate(C.byref(cfg))
 print('PASS: exact independent CSV serialization, all 600 unique address/section/domain combinations including rows193..200, 16 dots each, max one bit/column, both X directions, bounds rejection')
if __name__=='__main__':test_all()

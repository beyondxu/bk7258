"""Independent CSV serialization/dose check for all600 maintenance nozzles."""
from pathlib import Path
import ctypes as C,csv,subprocess,tempfile
from decimal import Decimal
P=Path(__file__).resolve().parents[1];D=P/'ap/app_bluetooth/gatt/printer'
class Config(C.Structure):_fields_=[('pitch',C.c_uint32),('direction',C.c_int32),('first',C.c_uint32),('reverse',C.c_uint32)]
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'test.c').write_text('''#include "printer_maintenance_pattern.h"
int build(uint8_t*p,uint32_t bytes,unsigned mode,unsigned ch,unsigned g,unsigned phase,const printer_map_config_t*c){return printer_maint_build(p,bytes,mode,ch,g,phase,c);}
int valid(const printer_map_config_t*c){return printer_nc_validate(c);}
void reverse_row(uint8_t*p){printer_maint_reverse(p);}
''')
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-shared','-fPIC','-I'+str(D),str(t/'test.c'),str(D/'printer_mapper.c'),'-o',str(t/'test.so')],check=True)
 lib=C.CDLL(str(t/'test.so'));buf_t=C.c_uint8*118000
 lib.build.argtypes=[C.POINTER(C.c_uint8),C.c_uint32,C.c_uint,C.c_uint,C.c_uint,C.c_uint,C.POINTER(Config)]
 lib.valid.argtypes=[C.POINTER(Config)];lib.reverse_row.argtypes=[C.POINTER(C.c_uint8)]
 records=list(csv.DictReader((P/'tools/data/m0333n_v12_nozzles.csv').open()))
 for direction in [-1,1]:
  cfg=Config(4388,direction,0,0);assert not lib.valid(C.byref(cfg))
  xv={int(r['domain']):int(Decimal(r['x_um'])*100) for r in records}
  offsets={d:(v-xv[6]+cfg.pitch//2)//cfg.pitch for d,v in xv.items()}
  for mode in [0,1]:
   totals=[0,0,0];seen=set()
   for ch in range(3):
    rows=sorted([r for r in records if (int(r['domain'])-1)//2==ch],key=lambda r:-Decimal(r['y_um']))
    for group in range(4):
     for phase in range(3 if mode else 1):
      buf=buf_t();assert not lib.build(buf,118000,mode,ch,group,phase,C.byref(cfg))
      gold=bytearray(118000);dose=32 if mode and phase==1 else 16
      for n,r in enumerate(rows[group*50:(group+1)*50]):
       addr,sec,dom=map(int,(r['address'],r['section'],r['domain']));delay=offsets[dom] if direction==-1 else offsets[1]-offsets[dom]
       first=([40,420,800][phase]+n*5 if mode else 80+n*20)+delay
       assert first>=0 and first+dose<=1180
       for x in range(first,first+dose):gold[x*100+(addr-1)*4+sec-1]|=1<<(dom-1)
       if phase==0:seen.add((addr,sec,dom))
      assert bytes(buf)==gold
      assert sum(v.bit_count() for v in gold)==50*dose
      assert max(sum(v.bit_count() for v in gold[x*100:(x+1)*100]) for x in range(1180))<=8
      totals[phase]+=50*dose
      lib.reverse_row(buf);assert bytes(buf)==b''.join(gold[x*100:(x+1)*100] for x in reversed(range(1180)))
      lib.reverse_row(buf);assert bytes(buf)==gold
   assert len(seen)==600
   assert totals==([9600,19200,9600] if mode else [9600,0,0])
 cfg=Config(4388,-1,0,0);buf=buf_t()
 for mode,ch,g,phase in [(2,0,0,0),(0,3,0,0),(1,0,4,0),(1,0,0,3)]:assert lib.build(buf,118000,mode,ch,g,phase,C.byref(cfg))
 assert lib.build(buf,117999,0,0,0,0,C.byref(cfg));assert lib.build(None,118000,0,0,0,0,C.byref(cfg))
print('PASS independent CSV:600 unique nozzles,16/32/16 doses, <=8 peak bits, region bounds, both X directions and mirrored travel')

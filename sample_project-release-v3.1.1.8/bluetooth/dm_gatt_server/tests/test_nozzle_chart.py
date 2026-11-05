"""Independent vendor-coordinate inverse of every nozzle chart output bit."""
from pathlib import Path
import ctypes as C
import csv,importlib.util,unittest
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('map_audit',root/'tools/map_audit.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class Chart(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.mapper=m.Mapper();cls.func=cls.mapper.lib.printer_map_nozzle_chart
  cls.func.argtypes=[C.POINTER(C.c_uint8),C.c_uint32,C.c_uint32,C.c_uint32,C.POINTER(m.Config)]
  with (root/'tools/data/m0333n_v12_nozzles.csv').open() as f:geometry=list(csv.DictReader(f))
  ys=sorted({float(r['y_um']) for r in geometry},reverse=True)
  cls.inverse={((int(r['address'])-1)*4+int(r['section'])-1,int(r['domain'])-1):(float(r['x_um']),ys.index(float(r['y_um']))) for r in geometry}
  cls.xmin=min(x for x,y in cls.inverse.values());cls.xmax=max(x for x,y in cls.inverse.values())
 def test_every_used_nozzle_has_exact_twelve_positions(self):
  for direction in [-1,1]:
   for mask in [1,2,4,7]:
    cfg=self.mapper.config();cfg.x_pitch_centi_um=3000;cfg.x_direction=direction
    out=(C.c_uint8*118000)();self.assertEqual(self.func(out,len(out),1180,mask,C.byref(cfg)),0)
    actual=set();nozzles=set();sections=[0,0,0,0]
    for x in range(1180):
     for slot in range(100):
      value=out[x*100+slot];self.assertFalse(value&0xC0)
      for bit in range(6):
       if not(value&(1<<bit)):continue
       ch=bit//2;nx,row=self.inverse[slot,bit];self.assertLess(row,192)
       offset=round((nx-self.xmin)/30)
       delay=offset if direction==-1 else round((self.xmax-self.xmin)/30)-offset
       actual.add((ch,row,x-delay));nozzles.add((ch,row));sections[slot%4]+=1
       self.assertGreaterEqual(x,200);self.assertLessEqual(x,999)
    expected={(ch,row,200+ch*250+row%8*14+dx) for ch in range(3) if mask&(1<<ch) for row in range(192) for dx in range(12)}
    self.assertEqual(actual,expected);self.assertEqual(len(nozzles),mask.bit_count()*192)
    self.assertEqual(sections,[600*mask.bit_count(),600*mask.bit_count(),600*mask.bit_count(),504*mask.bit_count()])
 def test_invalid_config_and_capacity_do_not_write(self):
  for size,count,mask in [(10,1180,7),(118000,1180,0),(118000,1180,8),(118000,200,7)]:
   cfg=self.mapper.config();cfg.x_pitch_centi_um=3000;cfg.x_direction=-1
   out=(C.c_uint8*118000)(*[255]*118000)
   self.assertNotEqual(self.func(out,size,count,mask,C.byref(cfg)),0);self.assertEqual(bytes(out),b'\xff'*118000)
if __name__=='__main__':unittest.main()

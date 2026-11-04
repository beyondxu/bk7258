"""Verify three separated color rows and coordinate compensation."""
from pathlib import Path
import ctypes as C
import csv,importlib.util,unittest
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('map_audit',root/'tools/map_audit.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class ColorRows(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.mapper=m.Mapper();cls.func=cls.mapper.lib.printer_map_color_rows_pattern
  cls.func.argtypes=[C.POINTER(C.c_uint8),C.c_uint32,C.c_uint32,C.c_uint32,C.POINTER(m.Config)]
  with (root/'tools/data/m0333n_v12_nozzles.csv').open() as f:
   cls.geometry={(int(r['address']),int(r['section']),int(r['domain'])):(float(r['x_um']),float(r['y_um'])) for r in csv.DictReader(f)}
 def test_exact_three_rows_all_masks_and_directions(self):
  for direction,delays in [(-1,[59,30,0]),(1,[2,31,61])]:
   cfg=self.mapper.config();cfg.x_pitch_centi_um=3000;cfg.x_direction=direction
   for mask in range(1,8):
    out=(C.c_uint8*118000)();self.assertEqual(self.func(out,len(out),1180,mask,C.byref(cfg)),0)
    expected=bytearray(len(out));positions=[]
    for ch,row in enumerate([95,135,175]):
     if not(mask&(1<<ch)):continue
     route=self.mapper.route(ch,row,cfg);self.assertEqual(route.slot,[73,46,15][ch]);self.assertEqual(route.mask,1<<(2*ch+1));self.assertEqual(route.delay_columns,delays[ch])
     for first in [300,500,700]:
      for x in range(first,first+150):expected[(x+delays[ch])*100+route.slot]=route.mask
     nx,ny=self.geometry[(route.slot//4+1,route.slot%4+1,2*ch+2)]
     positions.append((direction*delays[ch]*30+nx,ny))
    self.assertEqual(bytes(out),expected)
    for ch in range(3):self.assertEqual(sum(bool(v&(1<<(ch*2+1))) for v in out),450 if mask&(1<<ch) else 0)
    # 30um quantization leaves <1 column X residual; Y separation is vendor-defined.
    self.assertLess(max(x for x,y in positions)-min(x for x,y in positions),30)
    if mask==7:
     self.assertAlmostEqual(positions[0][1]-positions[1][1],1693.33,delta=0.15)
     self.assertAlmostEqual(positions[1][1]-positions[2][1],1693.33,delta=0.15)
 def test_bounds_reject_without_partial_write(self):
  cfg=self.mapper.config();cfg.x_pitch_centi_um=3000;cfg.x_direction=-1
  for size,count,mask in [(100,1180,7),(118000,1180,0),(118000,1180,8),(118000,900,7)]:
   out=(C.c_uint8*118000)(*[255]*118000)
   self.assertNotEqual(self.func(out,size,count,mask,C.byref(cfg)),0)
   self.assertEqual(bytes(out),bytes([255])*118000)
if __name__=='__main__':unittest.main()

"""Verify 3-color pattern, compensation, bounds and physical alignment."""
from pathlib import Path
import ctypes as C
import csv,importlib.util,unittest
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('map_audit',root/'tools/map_audit.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class Alignment(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.mapper=m.Mapper();cls.func=cls.mapper.lib.printer_map_alignment_pattern
  cls.func.argtypes=[C.POINTER(C.c_uint8),C.c_uint32,C.c_uint32,C.c_uint32,C.c_uint32,C.POINTER(m.Config)]
  with (root/'tools/data/m0333n_v12_nozzles.csv').open() as f:
   cls.geometry={(int(r['address']),int(r['section']),int(r['domain'])):(float(r['x_um']),float(r['y_um'])) for r in csv.DictReader(f)}
 def test_every_mask_both_directions_and_exact_dots(self):
  for direction,delays in [(1,[1,22,43]),(-1,[42,21,0])]:
   cfg=self.mapper.config();cfg.x_direction=direction
   for mask in range(1,8):
    out=(C.c_uint8*118000)();self.assertEqual(self.func(out,len(out),1180,mask,175,C.byref(cfg)),0)
    expected=bytearray(len(out));physical=[]
    for ch in range(3):
     if not(mask&(1<<ch)):continue
     route=self.mapper.route(ch,175,cfg);self.assertEqual(route.slot,15);self.assertEqual(route.delay_columns,delays[ch])
     for first in [300,500,700]:
      for x in range(first,first+150):expected[(x+delays[ch])*100+15]|=1<<(2*ch+1)
     nx,ny=self.geometry[(4,4,2*ch+2)];physical.append((direction*delays[ch]*42.33+nx,ny))
    self.assertEqual(bytes(out),expected)
    for ch in range(3):self.assertEqual(sum(bool(v&(1<<(ch*2+1))) for v in out),450 if mask&(1<<ch) else 0)
    self.assertLess(max(x for x,y in physical)-min(x for x,y in physical),0.2)
    self.assertEqual(len({y for x,y in physical}),1)
 def test_reject_invalid_and_no_partial_buffer_write(self):
  for size,count,mask,row,pitch in [(100,1180,7,175,4233),(118000,1180,0,175,4233),(118000,1180,8,175,4233),(118000,1180,7,192,4233),(118000,1180,7,175,1000),(118000,850,7,175,4233)]:
   out=(C.c_uint8*118000)(*[255]*118000);cfg=self.mapper.config();cfg.x_pitch_centi_um=pitch
   self.assertNotEqual(self.func(out,size,count,mask,row,C.byref(cfg)),0)
   self.assertEqual(bytes(out),bytes([255])*118000)
if __name__=='__main__':unittest.main()

"""Compare reversed chart against baseline: exact column relocation, same 100 nozzle plans."""
from pathlib import Path
import sys,ctypes as C,unittest
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from map_audit import Mapper
class Reverse(unittest.TestCase):
 def test_exact_relocation_for_every_address_and_nozzle(self):
  m=Mapper();cfg=m.config();cfg.x_pitch_centi_um=3000;cfg.x_direction=-1
  fn=m.lib.printer_map_section_paper_order
  fn.argtypes=[C.POINTER(C.c_uint8),C.c_uint32,C.c_uint32,C.c_uint32,C.c_uint32,C.c_void_p]
  for sec in range(4):
   base=(C.c_uint8*118000)();rev=(C.c_uint8*118000)()
   self.assertEqual(fn(base,len(base),1180,sec,0,C.byref(cfg)),0)
   self.assertEqual(fn(rev,len(rev),1180,sec,1,C.byref(cfg)),0)
   expected=bytearray(118000)
   for x in range(1180):
    for slot in range(100):
     if base[x*100+slot]:
      tile=(x-160)//180;within=(x-160)%180
      self.assertIn(tile,range(5));self.assertLess(within,96)
      expected[(160+(4-tile)*180+within)*100+slot]=base[x*100+slot]
   self.assertEqual(bytes(rev),expected)
   self.assertEqual(sum(v.bit_count() for v in rev),2400)
   for slot in range(100):
    self.assertEqual(sum(bool(base[x*100+slot]) for x in range(1180)),sum(bool(rev[x*100+slot]) for x in range(1180)))
   # Physical column load envelope remains identical although addresses change.
   self.assertEqual([sum(base[x*100+j].bit_count() for j in range(100)) for x in range(1180)],
                    [sum(rev[x*100+j].bit_count() for j in range(100)) for x in range(1180)])
  out=(C.c_uint8*118000)(*[255]*118000)
  self.assertNotEqual(fn(out,len(out),1180,0,2,C.byref(cfg)),0);self.assertEqual(bytes(out),b'\xff'*118000)
  m.tmp.cleanup()
if __name__=='__main__':unittest.main()

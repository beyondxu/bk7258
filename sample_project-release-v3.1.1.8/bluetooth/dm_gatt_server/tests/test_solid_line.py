"""Verify generated single-nozzle diagnostics against actual portable mapper."""
from pathlib import Path
import ctypes as C
import importlib.util
import unittest
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('map_audit',root/'tools/map_audit.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class SolidLine(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.mapper=m.Mapper();cls.func=cls.mapper.lib.printer_map_solid_line
  cls.func.argtypes=[C.POINTER(C.c_uint8),C.c_uint32,C.c_uint32,C.c_uint32,C.c_uint32,C.c_uint32,C.c_uint32,C.POINTER(m.Config)]
 def test_single_nozzle_exact_600_columns_and_zero_surroundings(self):
  for ch,mask in [(0,2),(1,8),(2,32)]:
   out=(C.c_uint8*118000)(*[255]*118000);cfg=self.mapper.config()
   self.assertEqual(self.func(out,len(out),1180,300,899,ch,175,C.byref(cfg)),0)
   expected=bytearray(118000)
   for x in range(300,900):expected[x*100+15]=mask
   self.assertEqual(bytes(out),expected)
   self.assertEqual(sum(v.bit_count() for v in out),600)
 def test_bounds_and_invalid_arguments(self):
  out=(C.c_uint8*118000)();cfg=self.mapper.config()
  for size,count,first,last,ch,row in [(100,1180,300,899,2,175),(118000,1180,900,300,2,175),(118000,1180,300,1180,2,175),(118000,1180,300,899,3,175),(118000,1180,300,899,2,192)]:
   self.assertNotEqual(self.func(out,size,count,first,last,ch,row,C.byref(cfg)),0)
if __name__=='__main__':unittest.main()

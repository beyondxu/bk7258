"""Validate bounded plans using the same C parser/mapper used on the board."""
from pathlib import Path
import ctypes as C
import importlib.util
import unittest
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('map_audit',root/'tools/map_audit.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class Capture(C.Structure):
    _fields_=[('columns',C.POINTER(C.c_uint8))]+[(n,C.c_uint32) for n in ['buffer_bytes','column_limit','target_y','first_row','last_row','channel_mask','min_column','max_column','found']]+[('selected_dots',C.c_uint32*3),('first_active',C.c_uint32),('last_active',C.c_uint32)]
class TestCapture(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mapper=m.Mapper()
        cls.mapper.lib.printer_map_bin_capture.argtypes=[C.POINTER(C.c_uint8),C.c_uint32,C.POINTER(m.Config),C.POINTER(m.Stats),C.POINTER(Capture),m.ALLOC,m.FREE,m.YIELD]
    def blob(self,x=230):
        w=1180;body=bytearray()
        for ch in range(3):
            body.extend(bytes.fromhex('1b253600001b56')+bytes([ch+1])+b'\x1b%2'+w.to_bytes(2,'big'))
            plane=bytearray(w*24);plane[x*24+175//8]=0x80>>(175%8)
            # Another row must never leak into this diagnostic.
            plane[x*24+10//8]|=0x80>>(10%8)
            body.extend(plane)
        return bytes(body+b'\x0c')
    def capture(self,blob,mask=1,target=0):
        mapper=self.mapper;cfg=mapper.config();stats=m.Stats();out=(C.c_uint8*(1180*100))()
        cap=Capture();cap.columns=out;cap.buffer_bytes=len(out);cap.column_limit=1180
        cap.target_y=target;cap.first_row=cap.last_row=175;cap.channel_mask=mask;cap.min_column=200;cap.max_column=999
        src=(C.c_uint8*len(blob)).from_buffer_copy(blob)
        code=mapper.lib.printer_map_bin_capture(src,len(blob),C.byref(cfg),C.byref(stats),C.byref(cap),mapper.allocate,mapper.release,mapper.yield_cpu)
        return code,cap,bytes(out)
    def test_each_color_only_and_exact_route(self):
        for ch in range(3):
            code,cap,out=self.capture(self.blob(),1<<ch)
            self.assertEqual(code,0)
            expected=[0,0,0];expected[ch]=1;self.assertEqual(list(cap.selected_dots),expected)
            route=self.mapper.route(ch,175,self.mapper.config())
            reference=bytearray(len(out));reference[(230+route.delay_columns)*100+route.slot]=route.mask
            self.assertEqual(out,reference)
    def test_reject_bounds_missing_blank_and_malformed_tail(self):
        self.assertEqual(self.capture(self.blob(0))[0],-5)
        self.assertEqual(self.capture(self.blob(1179))[0],-5)
        self.assertEqual(self.capture(self.blob(),target=192)[0],-5)
        self.assertNotEqual(self.capture(self.blob()+b'bad')[0],0)
        self.assertEqual(self.capture(self.blob(),mask=8)[0],-1)
if __name__=='__main__':unittest.main()

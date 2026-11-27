"""Decode actual C output back to source coordinates: no omitted/duplicate dots."""
from pathlib import Path
import ctypes as C
import sys, unittest, random
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from page_preview import Renderer, Page
from map_audit import Config, Route, YIELD

class Interlace(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.r=Renderer(); cls.fn=cls.r.lib.printer_page_render_interlaced_band
  u=C.c_uint32; p=C.POINTER(C.c_uint8)
  cls.fn.argtypes=[C.POINTER(Page),u,u,u,p,u,p,u,C.POINTER(u),YIELD]
  cls.r.lib.printer_map_route.argtypes=[u,u,C.POINTER(Config),C.POINTER(Route)]
 def check_image(self,height,dense=False,first=0):
  width=73; stride=(height+7)//8
  data=(C.c_uint8*(3*width*stride))(); rng=random.Random(height)
  expected=set()
  for ch in range(3):
   for x in range(width):
    for y in range(height):
     if dense or rng.randrange(17)==0:
      data[ch*width*stride+x*stride+y//8]|=0x80>>(y%8); expected.add((ch,x,y))
  cfg=Config(4388,-1,first,0)
  page=Page();page.planes=data;page.width=width;page.height=height;page.stride=stride;page.cfg=cfg
  out=(C.c_uint8*118000)(); scratch=(C.c_uint8*(width*72))(); dots=(C.c_uint32*3)()
  seen=set();sections=set(); counts=[0]*3; assignments={}
  body_sections=[0]*4
  for origin in range(0,height,24):
   reverse=(origin//24)%2
   self.assertEqual(self.fn(C.byref(page),origin,24,reverse,out,len(out),scratch,len(scratch),dots,YIELD(0)),0)
   scan=[0]*3; decoded=0;rcfg=Config(4388,1 if reverse else -1,first,0)
   for ch in range(3):
    for row in range(192):
     rt=Route();self.assertEqual(self.r.lib.printer_map_route(ch,row,C.byref(rcfg),C.byref(rt)),0)
     for dx in range(1180):
      if out[dx*100+rt.slot]&rt.mask:
       x=dx-rt.delay_columns
       if reverse:x=width-1-x
       y=origin+row;point=(ch,x,y)
       self.assertIn(point,expected);self.assertNotIn(point,seen)
       seen.add(point);scan[ch]+=1;decoded+=1;sections.add(rt.slot%4)
       if y>=168:body_sections[rt.slot%4]+=1
       if ch==0:assignments[x,y]=row//24
       elif (0,x,y) in expected:self.assertEqual(row//24,assignments[x,y])
   self.assertEqual(scan,list(dots))
   self.assertEqual(decoded,sum(int(v).bit_count() for v in out))
   counts=[a+b for a,b in zip(counts,scan)]
  self.assertEqual(seen,expected)
  self.assertEqual(counts,[sum(c==ch for c,x,y in expected) for ch in range(3)])
  if height>=192:self.assertEqual(sections,{0,1,2,3})
  if dense and height>192:
   # Old (x+y)%8 mask is100% identical along a down-left diagonal.
   # Decode actual output to check absence of that coherent grating.
   pairs=[assignments[x,y]==assignments[x+1,y-1]
          for x in range(width-1) for y in range(169,height)]
   self.assertGreater(sum(pairs)/len(pairs),0.06)
   self.assertLess(sum(pairs)/len(pairs),0.22)
   mean=sum(body_sections)/4
   for n in body_sections:self.assertLess(abs(n-mean)/mean,0.06,body_sections)
 def test_boundaries_sparse_and_both_scan_directions(self):
  for height in [1,23,24,25,167,168,169,192,217,401]:
   with self.subTest(height=height):self.check_image(height)
 def test_dense_and_last_eight_nozzle_offset(self):
  self.check_image(289,True);self.check_image(289,True,8)
 def test_reject_invalid_without_output_mutation(self):
  page=Page();data=(C.c_uint8*12)();page.planes=data;page.width=1;page.height=24;page.stride=3;page.cfg=Config(4388,-1,0,0)
  out=(C.c_uint8*118000)(*([165]*118000));scratch=(C.c_uint8*72)();dots=(C.c_uint32*3)()
  for origin,rows,reverse,ry in [(0,23,0,0),(1,24,0,0),(0,24,2,0),(0,24,0,1)]:
   page.cfg.reverse_y=ry
   self.assertNotEqual(self.fn(C.byref(page),origin,rows,reverse,out,len(out),scratch,len(scratch),dots,YIELD(0)),0)
   self.assertEqual(bytes(out),bytes([165])*118000)
if __name__=='__main__':unittest.main()

"""Actual C full-page parser/layout/render: boundaries, all pixels, failures."""
from pathlib import Path
import sys,unittest,ctypes as C
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from page_preview import Renderer,get_bit
from map_audit import Config,ALLOC

def blob(points,tail=b''):
 body=bytearray()
 for sy in range(0,1536,192):
  for ch in range(3):
   body.extend(bytes.fromhex('1b253600001b56')+bytes([ch+1])+b'\x1b%2'+(1180).to_bytes(2,'big'))
   data=bytearray(1180*24)
   for color,x,y in points:
    if color==ch and sy<=y<sy+192:data[x*24+(y-sy)//8]|=0x80>>((y-sy)%8)
   body.extend(data)
  if sy<1344:body.extend(b'\x1b%5\xc0')
 return bytes(body+b'\x0c'+tail)
class FullPage(unittest.TestCase):
 @classmethod
 def setUpClass(cls):cls.renderer=Renderer()
 def test_complete_image_edges_resize_and_600_nozzle_routing(self):
  points={(ch,x,y) for ch in range(3) for x in [0,1,590,1178,1179] for y in [0,1,191,192,193,738,1474,1475]}
  # Many rows/sections, not just the three diagnostic nozzles.
  points|={(ch,400+ch,y) for ch in range(3) for y in range(1476)}
  code,p=self.renderer.prepare(blob(points));self.assertEqual(code,0)
  try:
   self.assertEqual((p.width,p.height,p.swaths,p.max_delay),(1119,992,6,61))
   expected={(ch,x*p.width//1180,y*p.height//1476) for ch,x,y in points}
   for ch in range(3):
    self.assertEqual(p.source_dots[ch],sum(c==ch for c,x,y in points))
    self.assertEqual(p.output_dots[ch],sum(c==ch for c,x,y in expected))
   for ch,x,y in expected:self.assertTrue(get_bit(p,ch,x,y))
   totals=[0,0,0]
   for strip in range(p.swaths):
    columns,dots=self.renderer.render(p,strip)
    self.assertTrue(all(v<64 for v in columns))
    actual=[sum(bool(v&(1<<bit)) for v in columns for bit in [2*ch,2*ch+1]) for ch in range(3)]
    self.assertEqual(actual,dots)
    totals=[a+b for a,b in zip(totals,actual)]
   self.assertEqual(totals,list(p.output_dots))
   self.assertLess(abs(p.width*.03/(p.height*.04233)-1180/1476),.001)
  finally:self.renderer.close(p)
 def test_malformed_padding_and_allocation_reject_before_page_exists(self):
  for bad in [blob(set())[:-1],blob(set(),b'bad'),blob({(0,0,1476)}),b'']:
   code,p=self.renderer.prepare(bad);self.assertNotEqual(code,0);self.assertFalse(p.planes)
  original=self.renderer.allocate;self.renderer.allocate=ALLOC(lambda n:None)
  try:
   code,p=self.renderer.prepare(blob(set()));self.assertEqual(code,-2);self.assertFalse(p.planes)
  finally:self.renderer.allocate=original
 def test_whole_page_transforms_preserve_all_source_marks(self):
  for flags in [1,2,3]:
   code,p=self.renderer.prepare(blob({(0,0,0),(1,1179,1475),(2,590,738)}));self.assertEqual(code,0)
   try:
    before={(ch,x,y) for ch in range(3) for x in [0,558,559,1118] for y in [0,495,496,991] if get_bit(p,ch,x,y)}
    self.assertEqual(len(before),3)
    self.assertEqual(self.renderer.lib.printer_page_transform(C.byref(p),flags,self.renderer.yield_cpu),0)
    for ch,x,y in before:self.assertTrue(get_bit(p,ch,p.width-1-x if flags&1 else x,p.height-1-y if flags&2 else y))
    self.assertEqual(self.renderer.lib.printer_page_transform(C.byref(p),flags,self.renderer.yield_cpu),0)
    for ch,x,y in before:self.assertTrue(get_bit(p,ch,x,y))
   finally:self.renderer.close(p)
 def test_color_groups_recombine_exactly_without_source_changes(self):
  source=bytes(i%64 for i in range(118000));a=(C.c_uint8*len(source)).from_buffer_copy(source)
  b=(C.c_uint8*len(source))();c=(C.c_uint8*len(source))()
  self.assertEqual(self.renderer.lib.printer_page_color_columns(a,b,len(a),1),0)
  self.assertEqual(self.renderer.lib.printer_page_color_columns(a,c,len(a),6),0)
  self.assertEqual(bytes(a),source)
  self.assertEqual(bytes(x|y for x,y in zip(b,c)),source)
  self.assertFalse(any(v&60 for v in b));self.assertFalse(any(v&3 for v in c))
  self.assertNotEqual(self.renderer.lib.printer_page_color_columns(a,a,len(a),1),0)
  self.assertNotEqual(self.renderer.lib.printer_page_color_columns(a,b,len(a),8),0)
  a[700]=128;b[:]=[255]*len(b)
  self.assertNotEqual(self.renderer.lib.printer_page_color_columns(a,b,len(a),1),0)
  self.assertEqual(bytes(b),b'\xff'*len(b))
 def test_blank_page_and_last_padding_remain_blank(self):
  code,p=self.renderer.prepare(blob(set()));self.assertEqual(code,0)
  try:
   self.assertEqual(list(p.output_dots),[0,0,0]);out,dots=self.renderer.render(p,5)
   self.assertFalse(any(out));self.assertEqual(dots,[0,0,0])
  finally:self.renderer.close(p)
if __name__=='__main__':unittest.main()

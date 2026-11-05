#!/usr/bin/env python3
"""Run the firmware's actual full-page layout on a native BIN; export physical-aspect PNG."""
import argparse,ctypes as C,json,subprocess,tempfile,struct,zlib
from pathlib import Path
from map_audit import Config,ALLOC,FREE,YIELD
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/'ap/app_bluetooth/gatt/printer'
class Page(C.Structure):
 _fields_=[('planes',C.POINTER(C.c_uint8))]+[(n,C.c_uint32) for n in ['width','height','stride','swaths','max_delay']]+[('source_dots',C.c_uint32*3),('output_dots',C.c_uint32*3),('cfg',Config)]
class Renderer:
 def __init__(self):
  self.tmp=tempfile.TemporaryDirectory();so=Path(self.tmp.name)/'page.so'
  subprocess.run(['cc','-std=c99','-O2','-Wall','-Wextra','-Werror','-fPIC','-shared',str(SRC/'printer_mapper.c'),str(SRC/'printer_page.c'),'-o',str(so)],check=True)
  self.lib=C.CDLL(str(so));p=C.POINTER(C.c_uint8)
  self.lib.printer_page_prepare.argtypes=[p,C.c_uint32,C.POINTER(Config),C.POINTER(Page),ALLOC,FREE,YIELD]
  self.lib.printer_page_release.argtypes=[C.POINTER(Page),FREE]
  self.lib.printer_page_render.argtypes=[C.POINTER(Page),C.c_uint32,p,C.c_uint32,p,C.c_uint32,C.POINTER(C.c_uint32),YIELD]
  self.lib.printer_page_transform.argtypes=[C.POINTER(Page),C.c_uint32,YIELD]
  libc=C.CDLL(None);self.allocate=ALLOC(('malloc',libc));self.release=FREE(('free',libc));self.yield_cpu=YIELD(0)
 def prepare(self,blob,cfg=None):
  cfg=cfg or Config(3000,-1,0,0);page=Page();data=(C.c_uint8*len(blob)).from_buffer_copy(blob)
  code=self.lib.printer_page_prepare(data,len(blob),C.byref(cfg),C.byref(page),self.allocate,self.release,self.yield_cpu)
  return code,page
 def close(self,page):self.lib.printer_page_release(C.byref(page),self.release)
 def render(self,page,strip):
  columns=(C.c_uint8*118000)();scratch=(C.c_uint8*(page.width*72))();dots=(C.c_uint32*3)()
  code=self.lib.printer_page_render(C.byref(page),strip,columns,len(columns),scratch,len(scratch),dots,self.yield_cpu)
  if code:raise ValueError(f'render: {code}')
  return bytes(columns),list(dots)
def get_bit(page,ch,x,y):
 return bool(page.planes[ch*page.width*page.stride+x*page.stride+y//8] & (0x80>>(y%8)))
def png(page):
 height=round(page.height*4233/page.cfg.x_pitch_centi_um);rows=bytearray()
 for dy in range(height):
  y=dy*page.height//height;rows.append(0)
  for x in range(page.width):rows.extend(0 if get_bit(page,ch,x,y) else 255 for ch in range(3))
 def chunk(tag,data):return struct.pack('>I',len(data))+tag+data+struct.pack('>I',zlib.crc32(tag+data))
 return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>2I5B',page.width,height,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b'')
def main():
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('bin',type=Path);a.add_argument('--out',type=Path,default=Path('page-preview.png'));args=a.parse_args()
 r=Renderer();code,p=r.prepare(args.bin.read_bytes())
 if code:raise SystemExit(f'PAGE_REJECT: {code}')
 try:
  args.out.write_bytes(png(p))
  print(json.dumps(dict(width=p.width,height=p.height,swaths=p.swaths,max_delay=p.max_delay,source_dots=list(p.source_dots),output_dots=list(p.output_dots),size_mm=[p.width*.03,p.height*.04233],preview=str(args.out)),indent=2))
 finally:r.close(p)
if __name__=='__main__':main()

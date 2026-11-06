#!/usr/bin/env python3
"""C/D2 paper check reference; labels and expanded Y spacing are reference only."""
from pathlib import Path
import csv,ctypes as C,argparse
from PIL import Image,ImageDraw
from map_audit import Mapper
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--reverse',action='store_true');args=parser.parse_args()
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/('tests/page-images/section-paper-reverse' if args.reverse else 'tests/page-images/section-paper');OUT.mkdir(exist_ok=True)
with (ROOT/'tools/data/m0333n_v12_nozzles.csv').open() as f:records=list(csv.DictReader(f))
m=Mapper();cfg=m.config();cfg.x_pitch_centi_um=3000;cfg.x_direction=-1
fn=m.lib.printer_map_section_paper_order;fn.argtypes=[C.POINTER(C.c_uint8),C.c_uint32,C.c_uint32,C.c_uint32,C.c_uint32,C.c_void_p]
im=Image.new('RGB',(1180,1080),'white');draw=ImageDraw.Draw(im);legend=[]
for sec in range(4):
 top=50+sec*260
 draw.text((20,top-35),f'S{sec+1} / Cyan D2 / 25 nozzles / reverse={int(args.reverse)}. A=address; labels not printed.',fill='black')
 out=(C.c_uint8*118000)();assert fn(out,len(out),1180,sec,int(args.reverse),C.byref(cfg))==0
 rows=sorted([r for r in records if r['domain']=='2' and int(r['section'])==sec+1],key=lambda r:-float(r['y_um']))
 for i,r in enumerate(rows):
  tile=4-i%5 if args.reverse else i%5
  slot=(int(r['address'])-1)*4+sec;x=160+tile*180;y=top+i*8
  assert all(out[c*100+slot]==2 for c in range(x,x+96))
  draw.line((x,y,x+95,y),fill=(0,120,180),width=2)
  draw.text((x+100,y-5),'A'+r['address'],fill='black')
  legend.append([sec+1,tile+1,i//5+1,r['address'],2,r['y_um'],slot])
im.save(OUT/'reference.png')
with (OUT/'address-legend.csv').open('w') as f:
 w=csv.writer(f);w.writerow(['section_top_to_bottom','column_left_to_right','line_top_to_bottom','address','domain','physical_y_um','slot_0based']);w.writerows(legend)
m.tmp.cleanup();print(OUT)

#!/usr/bin/env python3
"""Reference for the generated 192-row nozzle check; labels are NOT printed by firmware."""
from pathlib import Path
import ctypes as C,csv
from PIL import Image,ImageDraw
from map_audit import Mapper
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'tests/page-images/nozzle-lines';OUT.mkdir(exist_ok=True)
mapper=Mapper();cfg=mapper.config();cfg.x_pitch_centi_um=3000;cfg.x_direction=-1
im=Image.new('RGB',(1180,1040),'white');draw=ImageDraw.Draw(im)
legend=[]
fn=mapper.lib.printer_map_nozzle_lines
fn.argtypes=[C.POINTER(C.c_uint8),C.c_uint32,C.c_uint32,C.c_uint32,C.c_void_p]
for ch,(name,color) in enumerate([('Cyan',(0,130,180)),('Magenta',(200,0,120)),('Yellow',(185,160,0))]):
 top=45+ch*340
 draw.text((30,top-35),f'{name}: 192 used rows; 6 blocks x 32 lines. Reference labels only.',fill='black')
 out=(C.c_uint8*118000)();assert fn(out,len(out),1180,ch,C.byref(cfg))==0
 inverse={}
 for row in range(192):
  route=mapper.route(ch,row,cfg);inverse[(route.slot,route.mask)]=(row,route.delay_columns)
  legend.append([name,row+1,row%6+1,row//6+1,route.slot//4+1,route.slot%4+1,route.mask.bit_length()])
 for index,value in enumerate(out):
  for bit in range(2*ch,2*ch+2):
   if value&(1<<bit):
    row,delay=inverse[(index%100,1<<bit)]
    draw.point((index//100-delay,top+row*1.5),fill=color)
 for block in range(6):
  x=120+160*block
  draw.text((x,top-17),f'B{block+1}: {block+1}, {block+7} ... {block+187}',fill='black')
  for k in [0,7,15,23,31]:
   row=k*6+block
   draw.text((x+101,top+row*1.5-4),str(row+1),fill='black')
im.save(OUT/'reference.png')
with (OUT/'row-legend.csv').open('w') as f:
 w=csv.writer(f);w.writerow(['color','used_row_1based','block_left_to_right','line_top_to_bottom','address','section','domain']);w.writerows(legend)
mapper.tmp.cleanup()
print(OUT)

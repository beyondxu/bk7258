#!/usr/bin/env python3
"""Actual APP source PNG comparisons; never changes printer firmware or substitutes pixels."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont,ImageChops
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'tests/page-images/comparison';OUT.mkdir(exist_ok=True)
W,H=1180,1476
font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',60)
black=(0,0,0);white=(255,255,255)
a=Image.new('RGB',(W,H),white);d=ImageDraw.Draw(a)
d.rectangle((80,110,1100,1320),outline=black,width=12)
d.text((410,145),'BOLD',font=font,fill=black)
for i,y in enumerate([300,480,660,840,1020,1200],1):
 d.text((110,y-40),str(i),font=font,fill=black)
 d.line((220,y,1060,y),fill=black,width=12)
d.line((260,260,1020,1260),fill=black,width=12)
a.save(OUT/'02-bold-grid.png')
b=Image.new('RGB',(W,H),white);d=ImageDraw.Draw(b)
for i,(label,color) in enumerate([('C',(0,255,255)),('M',(255,0,255)),('Y',(255,255,0))]):
 x=55+i*370
 d.rectangle((x,120,x+330,1260),outline=color,width=12)
 d.text((x+135,150),label,font=font,fill=color)
 for y in [300,470,640,810,980,1150]:d.line((x+25,y,x+305,y),fill=color,width=12)
 d.line((x+35,260,x+290,1200),fill=color,width=12)
b.save(OUT/'03-cmy-same-shapes.png')
shift=72
c=Image.new('RGB',(W,H),white);c.paste(b,(0,shift));c.save(OUT/'04-cmy-shift-down.png')
# Confirm identical underlying shapes translated without clipping; PNGs are RGB, opaque.
assert ImageChops.difference(b.crop((0,H-shift,W,H)),Image.new('RGB',(W,shift),white)).getbbox() is None
assert ImageChops.difference(b.crop((0,0,W,H-shift)),c.crop((0,shift,W,H))).getbbox() is None
for path in sorted(OUT.glob('*.png')):
 im=Image.open(path);assert im.size==(W,H) and im.mode=='RGB'
 print(path.name,path.stat().st_size,'bytes',im.size,im.mode)

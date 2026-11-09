#!/usr/bin/env python3
"""Split the existing BOLD frame at identical source coordinates; no firmware change."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageChops
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'tests/page-images/frame-parts';OUT.mkdir(exist_ok=True)
W,H=1180,1476;white=(255,255,255);black=(0,0,0);cyan=(0,255,255)
def blank():return Image.new('RGB',(W,H),white)
# Exactly match Pillow's 12-pixel inward rectangle used by G1, including its corners.
full=blank();ImageDraw.Draw(full).rectangle((80,110,1100,1320),outline=black,width=12)
horizontal=blank();dh=ImageDraw.Draw(horizontal)
dh.rectangle((80,110,1100,121),fill=black);dh.rectangle((80,1309,1100,1320),fill=black)
vertical=blank();dv=ImageDraw.Draw(vertical)
dv.rectangle((80,122,91,1308),fill=black);dv.rectangle((1089,122,1100,1308),fill=black)
# Component masks are disjoint; their union equals the original frame exactly.
assert ImageChops.difference(ImageChops.darker(horizontal,vertical),full).getbbox() is None
assert ImageChops.difference(ImageChops.lighter(horizontal,vertical),blank()).getbbox() is None
blue=blank();ImageDraw.Draw(blue).rectangle((80,110,1100,1320),outline=cyan,width=12)
for name,im in [('F1-full-frame.png',full),('F2-horizontal-edges.png',horizontal),('F3-vertical-edges.png',vertical),('F4-cyan-frame.png',blue)]:
 im.save(OUT/name);print(name,(OUT/name).stat().st_size)

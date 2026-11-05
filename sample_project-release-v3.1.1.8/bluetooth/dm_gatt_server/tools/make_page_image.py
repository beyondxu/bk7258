#!/usr/bin/env python3
"""Generate a low-coverage full-page structure/calibration fixture, not a photo."""
from pathlib import Path
import struct,zlib
W,H=1180,1476
FONT={'T':['11111','00100','00100','00100','00100','00100','00100'],'O':['01110','10001','10001','10001','10001','10001','01110'],'P':['11110','10001','10001','11110','10000','10000','10000'],'E':['11111','10000','10000','11110','10000','10000','11111'],'N':['10001','11001','11001','10101','10011','10011','10001'],'D':['11110','10001','10001','10001','10001','10001','11110'],'1':['00100','01100','00100','00100','00100','00100','01110'],'2':['01110','10001','00001','00010','00100','01000','11111'],'3':['11110','00001','00001','01110','00001','00001','11110'],'4':['00010','00110','01010','10010','11111','00010','00010'],'5':['11111','10000','10000','11110','00001','00001','11110'],'6':['01110','10000','10000','11110','10001','10001','01110']}
def main():
 pix=bytearray(b'\xff'*(W*H*3))
 def rect(x0,y0,x1,y1,color=(0,0,0)):
  for y in range(max(0,y0),min(H,y1)):
   for x in range(max(0,x0),min(W,x1)):pix[(y*W+x)*3:(y*W+x)*3+3]=bytes(color)
 def text(s,x,y,scale=8):
  for ch in s:
   for row,line in enumerate(FONT[ch]):
    for col,value in enumerate(line):
     if value=='1':rect(x+col*scale,y+row*scale,x+(col+1)*scale,y+(row+1)*scale)
   x+=6*scale
 rect(80,80,1100,83);rect(80,1393,1100,1396);rect(80,80,83,1396);rect(1097,80,1100,1396)
 text('TOP',520,100);text('END',520,1310)
 for i,y in enumerate([220,420,620,820,1020,1220],1):
  text(str(i),110,y-28)
  rect(210,y,1060,y+2)
 for y in range(260,1280):
  x=260+(y-260)*700//1020;rect(x,y,x+3,y+1)
 rect(220,190,440,210,(0,255,255));rect(500,190,720,210,(255,0,255));rect(780,190,1000,210,(255,255,0))
 rows=b''.join(b'\0'+pix[y*W*3:(y+1)*W*3] for y in range(H))
 def chunk(t,d):return struct.pack('>I',len(d))+t+d+struct.pack('>I',zlib.crc32(t+d))
 out=Path(__file__).resolve().parents[1]/'tests/page-images/01-full-page-grid.png'
 out.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>2I5B',W,H,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b''))
 print(out)
if __name__=='__main__':main()

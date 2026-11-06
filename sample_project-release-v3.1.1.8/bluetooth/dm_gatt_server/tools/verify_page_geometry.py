#!/usr/bin/env python3
"""Decode actual emitted columns using appendix-B coordinates, independently of C row LUT.
Host-only audit. Synthetic PNG conversion is not a capture of the phone's BIN.
"""
import argparse,csv,json
from decimal import Decimal
from pathlib import Path
from PIL import Image
from page_preview import Renderer,get_bit
ROOT=Path(__file__).resolve().parents[1]

def fixture_bin(path):
 image=Image.open(path).convert('RGB')
 if image.size!=(1180,1476):raise ValueError('fixture must be 1180x1476')
 pix=image.load();out=bytearray()
 for sy in range(0,1536,192):
  for ch in range(3):
   out.extend(bytes.fromhex('1b253600001b56')+bytes([ch+1])+b'\x1b%2'+(1180).to_bytes(2,'big'))
   for x in range(1180):
    col=bytearray(24)
    for row in range(192):
     if sy+row<1476 and pix[x,sy+row][ch]<128:col[row//8]|=0x80>>(row%8)
    out.extend(col)
  if sy<1344:out.extend(b'\x1b%5\xc0')
 return bytes(out+b'\x0c')

def coordinates():
 records=list(csv.DictReader((ROOT/'tools/data/m0333n_v12_nozzles.csv').open()))
 lookup={}
 ys=sorted({Decimal(r['y_um']) for r in records},reverse=True)
 assert len(ys)==200
 for r in records:
  domain=int(r['domain'])-1;slot=(int(r['address'])-1)*4+int(r['section'])-1
  key=(slot,domain);assert key not in lookup
  lookup[key]=(domain//2,ys.index(Decimal(r['y_um'])),int(Decimal(r['x_um'])*100))
 assert len(lookup)==600
 return lookup

def audit(blob,out):
 lookup=coordinates();renderer=Renderer();code,page=renderer.prepare(blob)
 if code:raise ValueError(f'prepare failed: {code}')
 try:
  expected={(ch,x,y) for ch in range(3) for x in range(page.width) for y in range(page.height) if get_bit(page,ch,x,y)}
  decoded=set();duplicate=0;sections=[[0]*4 for _ in range(3)]
  origin=min(v[2] for v in lookup.values())
  for strip in range(page.swaths):
   columns,_=renderer.render(page,strip)
   for index,value in enumerate(columns):
    if value&0xc0:raise ValueError('invalid domain bits')
    col,slot=divmod(index,100)
    for domain in range(6):
     if not value&(1<<domain):continue
     ch,row,physical_x=lookup[slot,domain]
     # Independent CSV X displacement; firmware baseline moves in direction -1.
     x=col-(physical_x-origin+page.cfg.x_pitch_centi_um//2)//page.cfg.x_pitch_centi_um
     y=strip*192+row
     if not 0<=x<page.width or not 0<=y<page.height:raise ValueError(f'out of page: {ch,x,y}')
     point=(ch,x,y);duplicate+=point in decoded;decoded.add(point);sections[ch][slot%4]+=1
  missing=expected-decoded;extra=decoded-expected
  report=dict(input='synthetic_fixture_or_supplied_bin_not_hardware_capture',width=page.width,height=page.height,swaths=page.swaths,
              expected_bits=len(expected),decoded_bits=len(decoded),missing=len(missing),extra=len(extra),duplicates=duplicate,
              section_dots_CMY=sections,nozzle_rows=192,y_pitch_um=42.33,nominal_feed_mm=192*.04233,
              five_feed_nominal_mm=5*192*.04233,
              note='Does not prove physical nozzles fire, feed calibration, CSV transcription, or pin wiring.')
  out.mkdir(parents=True,exist_ok=True)
  # Export reconstructed columns at physical aspect, rather than source raster.
  image=Image.new('RGB',(page.width,page.height),'white');pix=image.load()
  for ch,x,y in decoded:
   color=list(pix[x,y]);color[ch]=0;pix[x,y]=tuple(color)
  image.resize((page.width,round(page.height*42.33/30)),Image.Resampling.NEAREST).save(out/'decoded-columns.png')
  (out/'geometry-report.json').write_text(json.dumps(report,indent=2)+'\n')
  print(json.dumps(report,indent=2))
  if missing or extra or duplicate:raise AssertionError('column geometry mismatch')
 finally:renderer.close(page)

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 group=parser.add_mutually_exclusive_group(required=True)
 group.add_argument('--bin',type=Path);group.add_argument('--fixture',type=Path)
 parser.add_argument('--out',type=Path,required=True);args=parser.parse_args()
 audit(args.bin.read_bytes() if args.bin else fixture_bin(args.fixture),args.out)
if __name__=='__main__':main()

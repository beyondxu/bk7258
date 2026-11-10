#!/usr/bin/env python3
"""Expected diagnostic geometry from the independent specification CSV.
Labels are reference annotations, never additional firing data.
"""
from pathlib import Path
from decimal import Decimal
import csv
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
P=Path(__file__).resolve().parents[1]
def generate():
 records=list(csv.DictReader((P/'tools/data/m0333n_v12_nozzles.csv').open()))
 out=P/'tests/nozzle-check';out.mkdir(exist_ok=True)
 names=['scan','color','group','physical_row','address','section','domain','domain_nozzle_num','slot','mask_hex','first_column','last_column','requested_dots','x_um','y_um']
 planned=[]
 for ch,color in enumerate('CMY'):
  rows=sorted([r for r in records if (int(r['domain'])-1)//2==ch],key=lambda r:-Decimal(r['y_um']))
  for n,r in enumerate(rows):
   group,i=divmod(n,50);delay=(int(Decimal(r['x_um'])*100)-104284+1500)//3000;first=80+i*20+delay
   planned.append(dict(scan=ch*4+group+1,color=color,group=group+1,physical_row=n+1,address=r['address'],section=r['section'],domain=r['domain'],domain_nozzle_num=r['domain_nozzle_num'],slot=(int(r['address'])-1)*4+int(r['section'])-1,mask_hex=f"0x{1<<(int(r['domain'])-1):02X}",first_column=first,last_column=first+15,requested_dots=16,x_um=r['x_um'],y_um=r['y_um']))
 with (out/'nozzle-map.csv').open('w',newline='') as f:
  w=csv.DictWriter(f,fieldnames=names);w.writeheader();w.writerows(planned)
 fig,ax=plt.subplots(figsize=(9,10));colors={'C':'#009dbb','M':'#d33183','Y':'#b8a300'}
 band_stride=2*3170*30/11700
 for ch,color in enumerate('CMY'):
  base=ch*band_stride
  for n in range(200):
   x=(80+(n%50)*20)*.030;y=base+n*.04233
   ax.plot([x,x+15*.030],[y,y],color=colors[color],lw=1.1)
  for g in range(4):
   y=base+(g*50+24.5)*.04233
   ax.text(1.6,y,f'{color}  G{g+1}\n{g*50+1}-{(g+1)*50}',va='center',ha='right',fontsize=8,color=colors[color])
  ax.text(18,base-1,f'{color}: 200 nozzles, four passes',ha='center',fontsize=10)
 ax.set_xlim(0,34);ax.set_ylim(43,-2);ax.set_aspect('equal');ax.set_xlabel('Logical X (nominal mm; physical orientation may be reversed)');ax.set_ylabel('Row-order / feed distance (nominal mm)')
 ax.set_title('NOZZLE CHECK — expected complete pattern\nLabels are reference only; they are NOT printed',fontsize=12)
 ax.grid(alpha=.12);fig.tight_layout()
 fig.savefig(out/'expected.png',dpi=230);fig.savefig(out/'expected.svg');plt.close(fig)
 print('Wrote 600-row nozzle-map.csv and expected.png/svg')
if __name__=='__main__':generate()

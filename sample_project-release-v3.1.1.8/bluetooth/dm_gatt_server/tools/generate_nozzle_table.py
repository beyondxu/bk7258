#!/usr/bin/env python3
"""Generate/verify the C row lookup from M0333N V1.2 appendix-B CSV."""
import argparse
import csv
from decimal import Decimal
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]

def generate():
    with (ROOT/'tools/data/m0333n_v12_nozzles.csv').open() as f:
        records=list(csv.DictReader(f))
    keys={(int(r['address']),int(r['section']),int(r['domain'])) for r in records}
    assert len(records)==len(keys)==600
    assert keys=={(a,s,d) for a in range(1,26) for s in range(1,5) for d in range(1,7)}
    rows=sorted((r for r in records if r['domain'] in ['1','2']),key=lambda r:-Decimal(r['y_um']))
    assert len({r['y_um'] for r in rows})==200
    text='/* M0333N V1.2 appendix B: descending physical Y, D1/D2 pair.\n * Other CMY pairs have identical Y/address/section layout.\n * Generated from tools/data/m0333n_v12_nozzles.csv; do not sort by address. */\n'
    for name,values in [('map_row_slot',[(int(r['address'])-1)*4+int(r['section'])-1 for r in rows]),
                        ('map_row_pair_bit',[int(r['domain'])-1 for r in rows])]:
        text+=f'static const uint8_t {name}[200] = {{\n'
        for i in range(0,200,20):text+='    '+', '.join(map(str,values[i:i+20]))+',\n'
        text+='};\n'
    offsets=[]
    for d in range(1,7):
        xs={Decimal(r['x_um']) for r in records if int(r['domain'])==d}
        assert len(xs)==1
        offsets.append(int(xs.pop()*100))
    text+='/* Physical X in 0.01 um, D1..D6. */\nstatic const uint32_t map_domain_x[6] = {'+', '.join(map(str,offsets))+'};\n'
    return text

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true');args=parser.parse_args()
    target=ROOT/'ap/app_bluetooth/gatt/printer/printer_nozzle_table.h'
    expected=generate()
    if args.check:
        assert target.read_text()==expected,'Generated C table differs from CSV'
        print('PASS: 600 coordinates and generated C table agree')
    else:target.write_text(expected)
if __name__=='__main__':main()

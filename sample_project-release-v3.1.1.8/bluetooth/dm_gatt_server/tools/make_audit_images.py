#!/usr/bin/env python3
"""Generate exact PNG test fixtures using only the Python standard library."""
import argparse
from pathlib import Path
import struct
import zlib

W,H=1180,1476

def png(points):
    rows=bytearray()
    for y in range(H):
        row=bytearray(b'\xff'*(W*3))
        for x in points.get(y,[]): row[x*3:x*3+3]=b'\0\0\0'
        rows.append(0)
        rows.extend(row)
    def chunk(tag,data):
        return struct.pack('>I',len(data))+tag+data+struct.pack('>I',zlib.crc32(tag+data))
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>2I5B',W,H,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b'')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('directory',nargs='?',type=Path,default=Path('audit-images'))
    args=p.parse_args();args.directory.mkdir(parents=True,exist_ok=True)
    samples={'01-white.png':{},'02-black-dot-1px.png':{738:[590]},
             '03-black-horizontal-1px.png':{738:list(range(200,980))},
             '04-black-vertical-1px.png':{y:[590] for y in range(400,1076)}}
    for name,points in samples.items():
        (args.directory/name).write_bytes(png(points))
        print(name,'black source pixels:',sum(map(len,points.values())))
if __name__=='__main__':main()

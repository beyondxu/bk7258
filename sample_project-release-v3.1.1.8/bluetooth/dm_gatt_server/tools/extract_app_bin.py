#!/usr/bin/env python3
"""Recover one exact APP BIN from audit UART output, verifying offsets/size/CRC."""
import argparse,re,zlib
from pathlib import Path

def extract_last_bin(text):
    # Serial monitor timestamps can split a hex field between two nibbles.
    # Join only this recognized UI wrapper; offsets/size/CRC still verify bytes.
    wrapped=re.compile(r'(\[APP_BIN_HEX\] offset=[0-9A-Fa-f]{8} data=)([0-9A-Fa-f]{1,127})\r?\n\[\d{2}:\d{2}:\d{2}\.\d{3}\]收←◆([0-9A-Fa-f]+)(?=\r?\n|$)')
    def join(m):
        data=m[2]+m[3]
        return m[1]+data if len(data)<=128 and len(data)%2==0 else m[0]
    while True:
        clean=wrapped.sub(join,text)
        if clean==text:break
        text=clean
    current=None; result=None
    for line in text.splitlines():
        begin=re.search(r'\[APP_BIN_BEGIN\] bytes=(\d+) exported=(\d+) crc32=([0-9A-Fa-f]{8}) complete=([01])',line)
        if begin:
            if current is not None:raise ValueError('incomplete earlier APP BIN block')
            size,exported,crc,complete=begin.groups()
            if complete!='1' or size!=exported:raise ValueError('export was truncated; no exact BIN available')
            current=(int(size),int(crc,16),bytearray());continue
        chunk=re.search(r'\[APP_BIN_HEX\] offset=([0-9A-Fa-f]{8}) data=([0-9A-Fa-f]+)',line)
        if chunk:
            if current is None:raise ValueError('chunk without APP_BIN_BEGIN')
            size,crc,data=current
            if int(chunk[1],16)!=len(data):raise ValueError('missing, repeated or out-of-order chunk')
            data.extend(bytes.fromhex(chunk[2]))
            if len(data)>size:raise ValueError('too many bytes')
        end=re.search(r'\[APP_BIN_END\] exported=(\d+) crc32=([0-9A-Fa-f]{8})',line)
        if end:
            if current is None:raise ValueError('end without begin')
            size,crc,data=current
            if len(data)!=size or int(end[1])!=size or int(end[2],16)!=crc or zlib.crc32(data)!=crc:
                raise ValueError('APP BIN size/CRC mismatch')
            result=bytes(data);current=None
    if current is not None:raise ValueError('missing APP_BIN_END')
    if result is None:raise ValueError('no complete APP BIN in log')
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('log',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args();data=extract_last_bin(a.log.read_text(errors='replace'));a.output.write_bytes(data)
    print(f'PASS bytes={len(data)} crc32={zlib.crc32(data):08X} output={a.output}')
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Build a DN20 maintenance frame for a BLE write tool; does not send it."""
import argparse
import struct

def command(mode: str) -> bytes:
    values={'check':b'\0','paper-clean':b'\1','clean':b'\2'}
    param=values[mode]
    return b'\x1d\0'+struct.pack('<HBBB H',5+len(param),6,8,1,len(param))+param+b'\0'*4+b'\x1e'

if __name__=='__main__':
    parser=argparse.ArgumentParser(description='Generate a nozzle check/maintenance request; check/paper-clean require paper, clean uses the confirmed waste pad.')
    parser.add_argument('mode',choices=['check','paper-clean','clean'])
    args=parser.parse_args()
    print(command(args.mode).hex(' ').upper())

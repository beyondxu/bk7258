#!/usr/bin/env python3
"""Compile/run the SAME portable C nozzle mapper used in BK receive-only audit.
Requires Python3 and a host C compiler. Does not drive hardware.
"""
import argparse
import ctypes as C
import json
from pathlib import Path
import subprocess
import tempfile

PROJECT = Path(__file__).resolve().parents[1]
SOURCE = PROJECT/'ap/app_bluetooth/gatt/printer/printer_mapper.c'
class Config(C.Structure):
    _fields_=[('x_pitch_centi_um',C.c_uint32),('x_direction',C.c_int32),('first_nozzle',C.c_uint32),('reverse_y',C.c_uint32)]
class Route(C.Structure):
    _fields_=[('slot',C.c_uint32),('mask',C.c_uint32),('delay_columns',C.c_uint32),('nozzle_row',C.c_uint32)]
class Stats(C.Structure):
    _fields_=[('swaths',C.c_uint32),('input_dots',C.c_uint32*3),('output_dots',C.c_uint32*3),
              ('output_columns',C.c_uint32),('max_width',C.c_uint32),('max_delay',C.c_uint32),
              ('columns_crc32',C.c_uint32),('decoded_bytes',C.c_uint32)]
ALLOC=C.CFUNCTYPE(C.c_void_p,C.c_size_t)
FREE=C.CFUNCTYPE(None,C.c_void_p)
YIELD=C.CFUNCTYPE(None)

class Mapper:
    def __init__(self):
        self.tmp=tempfile.TemporaryDirectory()
        so=Path(self.tmp.name)/'mapper.so'
        subprocess.run(['cc','-std=c99','-O2','-Wall','-Wextra','-Werror','-fPIC','-shared',str(SOURCE),'-o',str(so)],check=True)
        self.lib=C.CDLL(str(so))
        self.lib.printer_map_default_config.argtypes=[C.POINTER(Config)]
        self.lib.printer_map_route.argtypes=[C.c_uint32,C.c_uint32,C.POINTER(Config),C.POINTER(Route)]
        self.lib.printer_map_route.restype=C.c_int
        self.lib.printer_map_column.argtypes=[C.POINTER(C.c_uint8),C.c_uint32,C.c_uint32,C.POINTER(Config),C.POINTER(C.c_uint8)]
        self.lib.printer_map_column.restype=C.c_int
        self.lib.printer_map_bin_dry_run.argtypes=[C.POINTER(C.c_uint8),C.c_uint32,C.POINTER(Config),C.POINTER(Stats),ALLOC,FREE,YIELD]
        self.lib.printer_map_bin_dry_run.restype=C.c_int
        libc=C.CDLL(None)
        self.allocate=ALLOC(('malloc',libc));self.release=FREE(('free',libc));self.yield_cpu=YIELD(0)
    def config(self):
        cfg=Config();self.lib.printer_map_default_config(C.byref(cfg));return cfg
    def route(self,ch,row,cfg):
        route=Route();code=self.lib.printer_map_route(ch,row,C.byref(cfg),C.byref(route))
        if code:raise ValueError(f'Invalid route: {code}')
        return route
    def column(self,planes,width,x,cfg):
        src=(C.c_uint8*len(planes)).from_buffer_copy(planes);out=(C.c_uint8*100)()
        code=self.lib.printer_map_column(src,width,x,C.byref(cfg),out)
        if code:raise ValueError(f'Invalid column: {code}')
        return bytes(out)
    def dry_run(self,blob,cfg):
        src=(C.c_uint8*len(blob)).from_buffer_copy(blob);stats=Stats()
        code=self.lib.printer_map_bin_dry_run(src,len(blob),C.byref(cfg),C.byref(stats),self.allocate,self.release,self.yield_cpu)
        return code,stats

def summary(code,stats,cfg):
    return {'result':code,'configuration':{name:getattr(cfg,name) for name,_ in Config._fields_},
            'swaths':stats.swaths,'input_dots':list(stats.input_dots),'output_dots':list(stats.output_dots),
            'output_columns':stats.output_columns,'max_width':stats.max_width,'max_delay':stats.max_delay,
            'columns_crc32':f'{stats.columns_crc32:08X}' if not code else None,
            'decoded_bytes':stats.decoded_bytes,
            'physical_firing_enabled':False}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('bin',type=Path)
    p.add_argument('--direction',type=int,choices=[-1,1],default=1)
    p.add_argument('--pitch',type=int,default=4233,help='candidate X pitch in 0.01um')
    p.add_argument('--first-nozzle',type=int,default=0)
    p.add_argument('--reverse-y',action='store_true')
    args=p.parse_args();mapper=Mapper();cfg=mapper.config()
    cfg.x_direction=args.direction;cfg.x_pitch_centi_um=args.pitch;cfg.first_nozzle=args.first_nozzle;cfg.reverse_y=args.reverse_y
    code,stats=mapper.dry_run(args.bin.read_bytes(),cfg)
    print(json.dumps(summary(code,stats,cfg),indent=2))
    if code:raise SystemExit(1)
if __name__=='__main__':main()

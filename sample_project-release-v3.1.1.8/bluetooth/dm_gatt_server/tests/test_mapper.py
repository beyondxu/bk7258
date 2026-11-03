from pathlib import Path
import collections
import csv
import ctypes as C
import importlib.util
import random
import struct
import unittest
import zlib

ROOT=Path(__file__).resolve().parents[1]
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
m=module('map_audit',ROOT/'tools/map_audit.py')

class TestMapper(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mapper=m.Mapper()
        with (ROOT/'tools/data/m0333n_v12_nozzles.csv').open() as f:
            cls.geometry={(int(r['address']),int(r['section']),int(r['domain'])):
                          (float(r['x_um']),float(r['y_um'])) for r in csv.DictReader(f)}
        cls.rows=sorted({y for x,y in cls.geometry.values()},reverse=True)
        assert len(cls.geometry)==600 and len(cls.rows)==200
    def test_every_route_against_all_physical_coordinates(self):
        for first,reverse,direction in [(0,0,1),(4,0,1),(8,1,1),(0,0,-1)]:
            cfg=self.mapper.config();cfg.first_nozzle=first;cfg.reverse_y=reverse;cfg.x_direction=direction
            used=set()
            for ch in range(3):
                for row in range(192):
                    route=self.mapper.route(ch,row,cfg)
                    self.assertEqual(route.mask.bit_count(),1)
                    domain=route.mask.bit_length()
                    self.assertIn(domain,[ch*2+1,ch*2+2])
                    address,section=route.slot//4+1,route.slot%4+1
                    x,y=self.geometry[(address,section,domain)]
                    nr=199-first-row if reverse else first+row
                    self.assertEqual(y,self.rows[nr])
                    key=(route.slot,domain);self.assertNotIn(key,used);used.add(key)
            self.assertEqual(len(used),576)
    def test_cmy_alignment_and_both_column_directions(self):
        for direction in [1,-1]:
            cfg=self.mapper.config();cfg.x_direction=direction
            for row in range(192):
                physical=[]
                for ch in range(3):
                    route=self.mapper.route(ch,row,cfg)
                    domain=route.mask.bit_length()
                    nx,ny=self.geometry[(route.slot//4+1,route.slot%4+1,domain)]
                    physical.append((direction*route.delay_columns*42.33+nx,ny))
                self.assertLess(max(x for x,y in physical)-min(x for x,y in physical),0.2)
                self.assertEqual(len({y for x,y in physical}),1)
    def test_random_pixels_inverse_through_independent_coordinate_table(self):
        random.seed(7258);w=4
        for direction in [1,-1]:
            cfg=self.mapper.config();cfg.x_direction=direction
            expected={(ch,x,y) for ch in range(3) for x in range(w) for y in range(192) if random.randrange(9)==0}
            planes=bytearray(w*72)
            for ch,x,y in expected:planes[ch*w*24+x*24+y//8] |= 0x80>>(y%8)
            actual=set();output_count=0
            # Inverse table is read from vendor coordinates, not the generated C row table.
            inverse={((address-1)*4+section-1,domain-1):(x,self.rows.index(y))
                     for (address,section,domain),(x,y) in self.geometry.items()}
            max_x=max(x for x,y in self.geometry.values());min_x=min(x for x,y in self.geometry.values())
            for col in range(w+43):
                result=self.mapper.column(planes,w,col,cfg)
                for slot,data in enumerate(result):
                    self.assertEqual(data&0xC0,0)
                    for bit in range(6):
                        if data&(1<<bit):
                            nx,row=inverse[(slot,bit)]
                            offset=round((nx-min_x)/42.33)
                            delay=43-offset if direction==1 else offset
                            actual.add((bit//2,col-delay,row));output_count+=1
            self.assertEqual(actual,expected)
            self.assertEqual(output_count,len(expected))
    def test_192_row_payload_boundaries_and_conservation(self):
        w=3;raw=bytearray();expected=[0,0,0]
        for swath in range(2):
            for ch in range(3):
                raw.extend(bytes.fromhex('1b253600001b56')+bytes([ch+1])+b'\x1b%2'+w.to_bytes(2,'big'))
                plane=bytearray(w*24)
                for y in [0,7,8,191]:plane[24+y//8]|=0x80>>(y%8)
                raw.extend(plane);expected[ch]+=4
            if swath==0:raw.extend(b'\x1b%5\xc0')
        raw.append(12)
        cfg=self.mapper.config();code,s=self.mapper.dry_run(bytes(raw),cfg)
        self.assertEqual(code,0);self.assertEqual(s.swaths,2)
        self.assertEqual(list(s.input_dots),expected);self.assertEqual(list(s.output_dots),expected)
        self.assertEqual(s.output_columns,2*(w+43))
        # Independent geometric reference for the exact serialized column CRC.
        target_rows={ch:sorted([(y,address,section,domain,x)
                     for (address,section,domain),(x,y) in self.geometry.items()
                     if domain in [ch*2+1,ch*2+2]],reverse=True) for ch in range(3)}
        expected_crc=0
        for swath in range(2):
            columns=[bytearray(100) for _ in range(w+43)]
            for ch in range(3):
                for row in [0,7,8,191]:
                    physical_y,address,section,domain,nx=target_rows[ch][row]
                    delay=round((2863.17-nx)/42.33)
                    columns[1+delay][(address-1)*4+section-1] |= 1<<(domain-1)
            for x,column in enumerate(columns):
                expected_crc=zlib.crc32(struct.pack('<II',swath*192,x)+column,expected_crc)
        self.assertEqual(s.columns_crc32,expected_crc)
        for blob in [bytes(raw[:-2]),bytes(raw[:-1]),bytes(raw)+b'junk',b'\x1b%6\0\1\x0c']:
            code,s=self.mapper.dry_run(blob,cfg);self.assertNotEqual(code,0)
    def test_compressed_and_raw_fingerprints_match(self):
        w=2;raw=bytearray()
        for ch in range(3):
            raw.extend(bytes.fromhex('1b253600001b56')+bytes([ch+1])+b'\x1b%2'+w.to_bytes(2,'big'))
            raw.extend(bytes([0x80])+bytes(w*24-1))
        raw.append(12)
        packed=b''.join(bytes([len(raw[i:i+127])-1])+raw[i:i+127] for i in range(0,len(raw),127))
        pre=bytes.fromhex('1b100304011b100305')+bytes(16)+bytes.fromhex('1b253901')
        cfg=self.mapper.config();one,s=self.mapper.dry_run(bytes(raw),cfg);two,t=self.mapper.dry_run(pre+packed,cfg)
        self.assertEqual((one,two),(0,0));self.assertEqual(bytes(s),bytes(t))
    def test_invalid_config_and_null_args(self):
        cfg=self.mapper.config();cfg.first_nozzle=9
        with self.assertRaises(ValueError):self.mapper.route(0,0,cfg)
        cfg=self.mapper.config()
        for ch,row in [(3,0),(0,192)]:
            with self.assertRaises(ValueError):self.mapper.route(ch,row,cfg)
        code,s=self.mapper.dry_run(b'\x0c',cfg);self.assertNotEqual(code,0)

if __name__=='__main__':unittest.main(verbosity=2)

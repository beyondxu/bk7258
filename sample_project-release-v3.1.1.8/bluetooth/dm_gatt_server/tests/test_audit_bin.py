from pathlib import Path
import importlib.util
import unittest
import zlib

path=Path(__file__).resolve().parents[1]/'tools/audit_bin.py'
spec=importlib.util.spec_from_file_location('audit_bin',path)
a=importlib.util.module_from_spec(spec);spec.loader.exec_module(a)

def stream(points,w=2):
    out=bytearray()
    for ch in (1,2,3):
        out.extend(bytes.fromhex('1b253600001b56')+bytes([ch])+b'\x1b%2'+w.to_bytes(2,'big'))
        payload=bytearray(w*24)
        for color,x,y in points:
            if ch==color: payload[x*24+y//8] |= 0x80>>(y%8)
        out.extend(payload)
    return bytes(out)+b'\x0c'

class TestAuditBin(unittest.TestCase):
    def test_white_and_single_black_pixel(self):
        summary,_=a.decode(stream([]))
        self.assertEqual([c['dots'] for c in summary['channels'].values()],[0,0,0])
        summary,points=a.decode(stream([(ch,1,8) for ch in (1,2,3)]))
        self.assertEqual(summary['cmy_same_coordinate_intersection'],1)
        for ch in (1,2,3):self.assertEqual(points[ch],{(1,8)})
    def test_horizontal_and_vertical(self):
        for dots in [[(ch,x,100) for ch in (1,2,3) for x in range(4)],
                     [(ch,2,y) for ch in (1,2,3) for y in range(192)]]:
            summary,points=a.decode(stream(dots,w=4))
            for ch in (1,2,3):self.assertEqual(points[ch],{(x,y) for c,x,y in dots if c==ch})
    def test_native_compression_and_preamble(self):
        raw=stream([(1,1,0)])
        # Valid native literal runs, retaining ESC sequences in their payload.
        packed=b''.join(bytes([len(raw[i:i+127])-1])+raw[i:i+127] for i in range(0,len(raw),127))
        pre=bytes.fromhex('1b100304011b100305')+bytes(16)
        one,_=a.decode(raw)
        two,_=a.decode(pre+bytes.fromhex('1b253901')+packed)
        self.assertEqual(one['channels'],two['channels'])
        self.assertTrue(two['compressed'])
    def test_binary_esc_is_not_command(self):
        raw=bytearray(stream([]));raw[13:17]=bytes.fromhex('1b253236')
        summary,_=a.decode(bytes(raw))
        self.assertEqual(len(summary['blocks']),3)
    def test_log_crc_offsets_and_complete_requirement(self):
        raw=stream([(1,0,0)]);crc=zlib.crc32(raw)
        begin=f'[AUDIT_BEGIN] len={len(raw)} crc32={crc:08X} dump_len={len(raw)}\n'
        lines=''.join(f'ap0:D(100):[AUDIT_HEX] {i:08X} {raw[i:i+32].hex()}\n' for i in range(0,len(raw),32))
        end=f'[AUDIT_END] len={len(raw)} dump_len={len(raw)} COMPLETE\n'
        self.assertEqual(a.from_log(begin+lines+end),raw)
        for bad in [begin+lines,begin+lines.replace('00000020','00000021')+end,
                    begin+lines+end.replace('COMPLETE','TRUNCATED')]:
            with self.assertRaises(ValueError):a.from_log(bad)
    def test_unsupported_or_truncated_rejected(self):
        for raw in [stream([])[:-2],stream([])[:-1],b'\x1b%1\0\x02\x0c',b'\x1b%6\0\x01\x0c']:
            with self.assertRaises(ValueError):a.decode(raw)
        with self.assertRaises(ValueError):a.native_unpack(bytes([0x82]))
        with self.assertRaises(ValueError):a.native_unpack(bytes([2,0]))

if __name__=='__main__':unittest.main(verbosity=2)

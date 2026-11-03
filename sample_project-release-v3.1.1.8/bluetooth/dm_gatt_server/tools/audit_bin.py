#!/usr/bin/env python3
"""Reconstruct BK audit logs, compare APK BIN, decode native photo Color1 data.
No third-party packages. Unknown commands fail closed rather than scanning ESC.
"""
import argparse
import hashlib
import json
import re
import struct
import zlib
from pathlib import Path

LIMIT = 16 * 1024 * 1024


def from_log(text):
    starts = list(re.finditer(r'\[AUDIT_BEGIN\] len=(\d+) crc32=([0-9A-Fa-f]{8}) dump_len=(\d+)', text))
    if not starts:
        raise ValueError('No AUDIT_BEGIN in log')
    start = starts[-1]
    length, crc, dumped = int(start[1]), int(start[2], 16), int(start[3])
    body = text[start.end():]
    end = re.search(r'\[AUDIT_END\] len=(\d+) dump_len=(\d+) (COMPLETE|TRUNCATED)', body)
    if not end or end[3] != 'COMPLETE' or dumped != length or int(end[1]) != length or int(end[2]) != length:
        raise ValueError('Incomplete/truncated audit capture; do not treat this as a complete BIN')
    if length > LIMIT:
        raise ValueError('BIN too large')
    data = bytearray()
    for match in re.finditer(r'\[AUDIT_HEX\] ([0-9A-Fa-f]{8}) ([0-9A-Fa-f]+)', body[:end.start()]):
        if int(match[1], 16) != len(data):
            raise ValueError(f'Missing, duplicated or reordered UART record at {len(data):08X}')
        data.extend(bytes.fromhex(match[2]))
    if len(data) != length or zlib.crc32(data) != crc:
        raise ValueError('UART reconstruction length/CRC does not match BK saved buffer')
    return bytes(data)


def native_unpack(data):
    # Matches native dnDataDecodingByTiff conventions, not arbitrary TIFF files.
    out = bytearray()
    pos = 0
    while pos < len(data):
        flag = data[pos]
        pos += 1
        if flag <= 0x7E:
            count = flag + 1
            if pos + count > len(data):
                raise ValueError('Truncated native literal run')
            out.extend(data[pos:pos+count])
            pos += count
        elif flag >= 0x82:
            if pos == len(data):
                raise ValueError('Truncated native repeat run')
            out.extend(bytes([data[pos]]) * (257 - flag))
            pos += 1
        # Native decoder skips 0x7F/0x80/0x81 control bytes.
        if len(out) > LIMIT:
            raise ValueError('Native expansion exceeds limit')
    return bytes(out)


def decode(data):
    original = data
    meta = {}
    if data.startswith(bytes.fromhex('1b100304')):
        if len(data) < 25 or data[5:9] != bytes.fromhex('1b100305'):
            raise ValueError('Malformed native BIN preamble')
        meta['print_times'] = data[4]
        meta['native_statistics_raw'] = list(struct.unpack_from('<4I', data, 9))
        data = data[25:]
    compressed = data.startswith(bytes.fromhex('1b253901'))
    if compressed:
        data = native_unpack(data[4:])
    points = {1: set(), 2: set(), 3: set()}
    pos = y = width = height = 0
    channel = None
    blocks = []
    page_end = False
    while pos < len(data):
        if data[pos] == 12:
            pos += 1
            page_end = True
            if pos != len(data):
                raise ValueError('Trailing bytes after page end; multiple pages not supported')
            break
        if data[pos:pos+2] == b'\x1bV':
            if pos + 3 > len(data) or data[pos+2] not in points:
                raise ValueError('Unsupported/truncated color mode (only CMY Color1 supported)')
            channel = data[pos+2]
            pos += 3
            continue
        if data[pos:pos+2] == b'\x1b\r':
            if pos+3 > len(data):
                raise ValueError('Truncated DPI command')
            meta['dpi_command_raw'] = data[pos+2]
            pos += 3
            continue
        if data[pos:pos+2] != b'\x1b%' or pos+3 > len(data):
            raise ValueError(f'Unknown/truncated command at decoded offset 0x{pos:X}')
        command = data[pos+2]
        if command == 0x35:
            if pos+4 > len(data):
                raise ValueError('Truncated feed command')
            y += data[pos+3]
            pos += 4
            continue
        if command == 0x36:
            if pos+5 > len(data):
                raise ValueError('Truncated horizontal command')
            raw_x = int.from_bytes(data[pos+3:pos+5], 'big')
            if raw_x:
                raise ValueError(f'Nonzero horizontal parameter {raw_x}: unit requires validation; no guessed coordinates')
            pos += 5
            continue
        if command != 0x32 or pos+5 > len(data) or channel is None:
            raise ValueError(f'Unsupported/truncated image command at 0x{pos:X}: 0x{command:02X}')
        w = int.from_bytes(data[pos+3:pos+5], 'big')
        if not w or w > 4096 or y+192 > 8192:
            raise ValueError('Unsupported image dimensions')
        start = pos + 5
        end = start + w*24
        if end > len(data):
            raise ValueError('Truncated 192-row color payload')
        for index, byte in enumerate(data[start:end]):
            if not byte:
                continue
            x, packed_y = divmod(index, 24)
            for bit in range(8):
                if byte & (0x80 >> bit):
                    points[channel].add((x, y+packed_y*8+bit))
        blocks.append({'channel': channel, 'width': w, 'y': y, 'bytes': w*24})
        width, height = max(width, w), max(height, y+192)
        pos = end
    if not page_end:
        raise ValueError('No page end: truncated stream or unsupported layout')
    names = {1: 'C', 2: 'M', 3: 'Y'}
    channels = {}
    for ch, dots in points.items():
        bounds = None
        if dots:
            xs, ys = zip(*dots)
            bounds = [min(xs), min(ys), max(xs), max(ys)]
        channels[names[ch]] = {'dots': len(dots), 'bounds_xyxy': bounds,
                               'first_32_points': [list(p) for p in sorted(dots, key=lambda p: (p[1],p[0]))[:32]]}
    summary = {**meta, 'length': len(original), 'crc32': f'{zlib.crc32(original):08X}',
               'sha256': hashlib.sha256(original).hexdigest(), 'compressed': compressed,
               'decoded_length': len(data), 'width': width, 'height_including_padding': height,
               'channels': channels, 'blocks': blocks,
               'cmy_same_coordinate_intersection': len(points[1] & points[2] & points[3])}
    return summary, points


def png_rgb(w, h, rgb):
    def chunk(tag, payload):
        return struct.pack('>I', len(payload)) + tag + payload + struct.pack('>I', zlib.crc32(tag + payload))
    scanlines = b''.join(b'\0' + rgb[y*w*3:(y+1)*w*3] for y in range(h))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>2I5B', w, h, 8, 2, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(scanlines, 9)) + chunk(b'IEND', b''))


def previews(directory, summary, points):
    directory.mkdir(parents=True, exist_ok=True)
    w, h = summary['width'], summary['height_including_padding']
    if w*h > 8_000_000:
        raise ValueError('Preview dimensions too large')
    rgb = bytearray(b'\xff' * (w*h*3))
    for ch, dots in points.items():
        mask = bytearray((w+7)//8*h)
        for x, y in dots:
            rgb[(y*w+x)*3+ch-1] = 0
            mask[y*((w+7)//8)+x//8] |= 0x80 >> (x%8)
        (directory/f'{"CMY"[ch-1]}.pbm').write_bytes(f'P4\n{w} {h}\n'.encode()+mask)
    (directory/'composite.ppm').write_bytes(f'P6\n{w} {h}\n255\n'.encode()+rgb)
    (directory/'composite.png').write_bytes(png_rgb(w, h, rgb))
    (directory/'summary.json').write_text(json.dumps(summary, indent=2), encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path, help='BK UART .txt log or SDK .bin')
    parser.add_argument('--log', action='store_true')
    parser.add_argument('--apk-bin', type=Path, help='SDK BIN sent by APK, for exact byte comparison')
    parser.add_argument('--out', type=Path, default=Path('audit-output'))
    args = parser.parse_args()
    text = args.input.read_text(encoding='utf-8', errors='replace') if args.log else ''
    data = from_log(text) if args.log else args.input.read_bytes()
    sender = None
    if args.log:
        last_begin = text.rfind('[AUDIT_BEGIN]')
        starts = list(re.finditer(r'\[AUDIT_START\] task=(\d+) expected_len=(\d+) expected_crc32=([0-9A-Fa-f]{8})', text[:last_begin]))
        if starts:
            match = starts[-1]
            sender = {'task_id': int(match[1]), 'length': int(match[2]), 'crc32': match[3].upper()}
            sender['matches_saved_bin'] = sender['length'] == len(data) and int(sender['crc32'], 16) == zlib.crc32(data)
            print('APK DECLARED LENGTH/CRC vs BK SAVED:', 'MATCH' if sender['matches_saved_bin'] else 'MISMATCH')
        else:
            print('APK length/CRC metadata unavailable: include AUDIT_START to check sender metadata.')
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out/'bk_received.bin').write_bytes(data)
    if args.apk_bin:
        apk = args.apk_bin.read_bytes()
        if apk != data:
            n = next((i for i, pair in enumerate(zip(apk,data)) if pair[0]!=pair[1]), min(len(apk),len(data)))
            raise ValueError(f'APK/BK BIN mismatch at offset {n}; APK len={len(apk)}, BK len={len(data)}')
        print('APK/BK BIN: BYTE-FOR-BYTE IDENTICAL')
    summary, points = decode(data)
    if sender is not None:
        summary['apk_declared'] = sender
    previews(args.out, summary, points)
    print(json.dumps(summary, indent=2))
    print('Preview: CMY subtractive logical raster; no physical nozzle mapping is applied.')

if __name__ == '__main__':
    main()

"""Minimal PNG reader (no dependency): 8-bit greyscale, RGB, palette or RGBA, not interlaced.

    rows, (w, h) = read_png(path)   # rows of (r, g, b, a)
"""
import struct
import zlib


def read_png(path):
    data = open(path, "rb").read()
    assert data[:8] == b"\x89PNG\r\n\x1a\n", path
    pos, idat, plte, trns = 8, b"", None, None
    while pos < len(data):
        n, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + n]
        pos += 12 + n
        if kind == b"IHDR":
            w, h, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", body)
            assert depth == 8 and not interlace, "%s: unsupported PNG (depth %d)" % (path, depth)
        elif kind == b"PLTE":
            plte = [tuple(body[i:i + 3]) for i in range(0, n, 3)]
        elif kind == b"tRNS":
            trns = body
        elif kind == b"IDAT":
            idat += body
    bpp = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ctype]
    raw = zlib.decompress(idat)
    stride = w * bpp
    prev, rows = bytearray(stride), []
    for y in range(h):
        f = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if f == 1:
                line[i] = (line[i] + a) & 255
            elif f == 2:
                line[i] = (line[i] + b) & 255
            elif f == 3:
                line[i] = (line[i] + (a + b) // 2) & 255
            elif f == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        prev = line
        if ctype == 6:
            rows.append([tuple(line[x * 4:x * 4 + 4]) for x in range(w)])
        elif ctype == 2:
            rows.append([tuple(line[x * 3:x * 3 + 3]) + (255,) for x in range(w)])
        elif ctype == 3:
            rows.append([plte[v] + ((trns[v] if trns and v < len(trns) else 255),) for v in line])
        elif ctype == 4:
            rows.append([(line[x * 2],) * 3 + (line[x * 2 + 1],) for x in range(w)])
        else:
            rows.append([(v, v, v, 255) for v in line])
    return rows, (w, h)

"""Minimal GIF reader (no dependency): returns the animation as full RGBA frames.

    frames, (w, h) = read_gif(path)   # frames = list of rows of (r, g, b, a)
"""


def _lzw(data, min_size):
    clear, end = 1 << min_size, (1 << min_size) + 1
    size, table, out, prev = min_size + 1, None, [], None
    bits, nbits, pos = 0, 0, 0

    def reset():
        return [[i] for i in range(clear)] + [[], []]

    table = reset()
    while True:
        while nbits < size:
            if pos >= len(data):
                return out
            bits |= data[pos] << nbits
            nbits += 8
            pos += 1
        code = bits & ((1 << size) - 1)
        bits >>= size
        nbits -= size
        if code == clear:
            table, size, prev = reset(), min_size + 1, None
            continue
        if code == end:
            return out
        if prev is None:
            entry = table[code]
        elif code < len(table):
            entry = table[code]
            table.append(prev + entry[:1])
        else:
            entry = prev + prev[:1]
            table.append(entry)
        out.extend(entry)
        prev = entry
        if len(table) == (1 << size) and size < 12:
            size += 1


def _colors(data, pos, n):
    return [tuple(data[pos + i * 3:pos + i * 3 + 3]) for i in range(n)], pos + n * 3


def read_gif(path):
    data = open(path, "rb").read()
    assert data[:3] == b"GIF", path
    w, h = data[6] | data[7] << 8, data[8] | data[9] << 8
    flags, pos = data[10], 13
    gct = None
    if flags & 0x80:
        gct, pos = _colors(data, pos, 2 << (flags & 7))
    canvas = [[(0, 0, 0, 0)] * w for _ in range(h)]
    frames, transparent, disposal = [], None, 0
    while pos < len(data):
        b = data[pos]
        if b == 0x3B:
            break
        if b == 0x21:
            label = data[pos + 1]
            pos += 2
            if label == 0xF9:
                packed = data[pos + 1]
                disposal = (packed >> 2) & 7
                transparent = data[pos + 4] if packed & 1 else None
            while data[pos]:
                pos += data[pos] + 1
            pos += 1
            continue
        if b == 0x2C:
            x0, y0 = data[pos + 1] | data[pos + 2] << 8, data[pos + 3] | data[pos + 4] << 8
            fw, fh = data[pos + 5] | data[pos + 6] << 8, data[pos + 7] | data[pos + 8] << 8
            packed = data[pos + 9]
            pos += 10
            table = gct
            if packed & 0x80:
                table, pos = _colors(data, pos, 2 << (packed & 7))
            interlaced = packed & 0x40
            min_size = data[pos]
            pos += 1
            blob = bytearray()
            while data[pos]:
                blob += data[pos + 1:pos + 1 + data[pos]]
                pos += data[pos] + 1
            pos += 1
            idx = _lzw(blob, min_size)
            rows = list(range(fh))
            if interlaced:
                rows = list(range(0, fh, 8)) + list(range(4, fh, 8)) + list(range(2, fh, 4)) + list(range(1, fh, 2))
            before = [r[:] for r in canvas]
            for i, ry in enumerate(rows):
                for x in range(fw):
                    k = i * fw + x
                    if k >= len(idx):
                        break
                    c = idx[k]
                    if c != transparent and 0 <= y0 + ry < h and 0 <= x0 + x < w:
                        canvas[y0 + ry][x0 + x] = table[c] + (255,)
            frames.append([r[:] for r in canvas])
            if disposal == 2:
                for ry in range(fh):
                    for x in range(fw):
                        if 0 <= y0 + ry < h and 0 <= x0 + x < w:
                            canvas[y0 + ry][x0 + x] = (0, 0, 0, 0)
            elif disposal == 3:
                canvas = before
            transparent, disposal = None, 0
            continue
        raise ValueError("bad GIF block %02x at %d" % (b, pos))
    return frames, (w, h)

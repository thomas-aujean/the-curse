"""Renders a room dumped by tests/decor_test.c as the engine would draw it (without sprites).

    wsl python3 tools/art/render_room.py out/room_3.txt out/room_3.png

Far background (res/far.png) behind, at the camera's top-left, then the room plane: res/castle.png
tiles (CASTLE_FLIP mirrors them) or res/decor.png tiles on empty cells.
"""
import os
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(__file__))
from pngread import read_png  # noqa: E402

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
RES = os.path.join(ROOT, "res")


def tiles(path):
    px, (w, h) = read_png(path)
    return [[row[(i % (w // 8)) * 8:(i % (w // 8)) * 8 + 8] for row in px[(i // (w // 8)) * 8:(i // (w // 8)) * 8 + 8]]
            for i in range((w // 8) * (h // 8))]


def main(src, out):
    lines = open(src).read().split("\n")
    w, h = map(int, lines[0].split())
    cells = [[tuple(map(int, c.split(":"))) for c in line.split()] for line in lines[1:1 + h]]
    castle, decor = tiles(os.path.join(RES, "castle.png")), tiles(os.path.join(RES, "decor.png"))
    far, (fw, fh) = read_png(os.path.join(RES, "far.png"))
    W, H = w * 8, h * 8
    img = [[far[max(0, y - max(0, H - fh))][x % fw][:3] for x in range(W)] for y in range(H)]   # horizon at the bottom
    for ty in range(h):
        for tx in range(w):
            art, dec = cells[ty][tx]
            flip = art & 0x8000
            art &= 0x7FFF
            t = castle[art] if art else decor[dec] if dec else None
            if t is None:
                continue
            for y in range(8):
                for x in range(8):
                    p = t[y][7 - x if flip else x]
                    if p[3] >= 128:
                        img[ty * 8 + y][tx * 8 + x] = p[:3]
    raw = b"".join(b"\0" + bytes(c for p in row for c in p) for row in img)

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)
    with open(out, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])

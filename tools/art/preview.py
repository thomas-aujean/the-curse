"""Scaled preview of a generated sheet: wsl python3 tools/art/preview.py ninja|props OUT.png [scale]"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from png import write_indexed  # noqa: E402

which, out = sys.argv[1], sys.argv[2]
scale = int(sys.argv[3]) if len(sys.argv) > 3 else 5
mod = __import__(which)
img = mod.sheet()
pal = list(mod.PAL1 if which == "ninja" else mod.PAL2)
pal[0] = (0x50, 0x40, 0x60)   # visible background instead of transparency
big = [[c for c in row for _ in range(scale)] for row in img for _ in range(scale)]
write_indexed(out, big, pal)

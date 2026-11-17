"""Imports the PixelLab chests: art_in/props/<name>.png (32 x 32 canvas or larger), closed and open.

Each chest becomes one 32 x 32 hardware sprite (4 x 4 tiles), bottom-centred: the closed and open
frames of a chest share one crop, so the lid swings open without the body moving. Order of the frames
(GFX_CHESTS in inc/gfx.h): bronze, bronze open, silver, silver open, gold, gold open.

Colours go to the nearest slot of the shared PAL2 (slots 1..14, see enemies.py).
"""
import os

from knight import dist
from pngread import read_png

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
SRC = os.path.join(ROOT, "art_in", "props")
CHESTS = ["bronze", "silver", "gold"]
CELL = 32


def _load(name):
    rows, (w, h) = read_png(os.path.join(SRC, name + ".png"))
    return {(x, y): rows[y][x][:3] for y in range(h) for x in range(w) if rows[y][x][3] >= 128}


def sheet(pal):
    """Returns the indexed sheet (CELL rows, CELL * 6 columns) for the palette pal."""
    cache = {}

    def index(c):
        if c not in cache:
            cache[c] = min(range(1, 15), key=lambda i: dist(pal[i], c))
        return cache[c]

    img = [[0] * (CELL * 2 * len(CHESTS)) for _ in range(CELL)]
    for k, name in enumerate(CHESTS):
        frames = [_load(name), _load(name + "_open")]
        pts = [p for f in frames for p in f]
        x0, x1 = min(p[0] for p in pts), max(p[0] for p in pts)
        y1 = max(p[1] for p in pts)
        assert x1 - x0 < CELL, "%s: %d px wide" % (name, x1 - x0 + 1)
        dx = (CELL - (x1 - x0 + 1)) // 2 - x0
        dy = CELL - 1 - y1
        for j, f in enumerate(frames):
            ox = (k * 2 + j) * CELL
            for (x, y), c in f.items():
                if 0 <= y + dy < CELL:
                    img[y + dy][ox + x + dx] = index(c)
                else:
                    print("warning: %s frame %d loses a pixel above the cell" % (name, j))
    return img

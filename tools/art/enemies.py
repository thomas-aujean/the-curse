"""Imports the PixelLab enemies (art_in/enemies/*.png, one still image each, facing right).

Sheets: humans in 32 x 48 cells (centre x 16, soles on row 46, like the hero), small creatures in
32 x 32 cells (bottom on row 31). Order is the engine's contract (gfx.h).

PAL2 is shared by the enemies, the props and the coins. Fixed slots, used by drawings made in code
(gfx.c): 1 outline, 2 bone, 3 bone shadow, 4 red, 5 gold, 6 gold shadow, 15 map text. Slots 7..13 are
picked from the enemies' colours; slot 14 is for props only, because the Void form of an enemy is the
same tiles shown with PAL3, and in shadow / highlight mode a sprite pixel 14 or 15 of PAL3 is not a
colour (it lights or darkens what is behind).
"""
import os

from knight import dist, median_cut
from pngread import read_png

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
SRC = os.path.join(ROOT, "art_in", "enemies")

BIG = ["soldat", "archer_b", "garde", "ninja"]    # walker, archer, guard, rival ninja
SMALL = ["spectre", "gelee"]                      # flyer, jelly
FIXED = [(0x10, 0x08, 0x10), (0x00, 0x00, 0x00), (0xCC, 0xCC, 0xAA), (0x88, 0x88, 0x66), (0xEE, 0x22, 0x22),
         (0xEE, 0xBB, 0x22), (0xAA, 0x66, 0x00)]
PROPS_ONLY = (0xE0, 0x70, 0x20)   # slot 14: flame orange
MAP_TEXT = (0xEE, 0xEE, 0x44)


def _load(name):
    px, _ = read_png(os.path.join(SRC, name + ".png"))
    return {(x, y): p[:3] for y, row in enumerate(px) for x, p in enumerate(row) if p[3] > 127}


def _cell(px, w, h, bottom):
    """Centres the drawing on its legs (lower third) and stands it on row `bottom`."""
    ys = [y for _, y in px]
    y0, y1 = min(ys), max(ys)
    legs = [x for x, y in px if y >= y1 - (y1 - y0) // 3]
    cx = (min(legs) + max(legs) + 1) // 2
    return {(x - cx + w // 2, y - y1 + bottom): c for (x, y), c in px.items()
            if 0 <= x - cx + w // 2 < w and 0 <= y - y1 + bottom < h}


def build():
    """Returns (big sheet, small sheet, PAL2, PAL3 for the Void forms)."""
    big = [_cell(_load(n), 32, 48, 46) for n in BIG]
    small = [_cell(_load(n), 32, 32, 31) for n in SMALL]
    # Colours the fixed slots already give are left out of the count: the 7 free slots go to the rest.
    counts = {}
    for px in big + small:
        for c in px.values():
            if min(dist(c, f) for f in FIXED[1:]) > 3000:
                counts[c] = counts.get(c, 0) + 1
    pal = FIXED + sorted(median_cut(counts, 7), key=sum) + [PROPS_ONLY, MAP_TEXT]

    def index(c):
        return min(range(1, 14), key=lambda i: dist(pal[i], c))

    def sheet(cells, w, h):
        return [[index(px[(x, y)]) if (x, y) in px else 0 for px in cells for x in range(w)] for y in range(h)]

    return sheet(big, 32, 48), sheet(small, 32, 32), pal, void_palette(pal)


def void_palette(pal):
    """PAL3: the same slots swallowed by the Void (violet body, cyan where it was red)."""
    out = [pal[0], (0, 0, 0), (0x88, 0x44, 0xCC), (0x44, 0x22, 0x88), (0x44, 0xEE, 0xEE)]
    for r, g, b in pal[5:14]:
        lum = (r * 3 + g * 4 + b * 2) // 9
        if r > 150 and r > g * 2 and r > b * 2:
            out.append((0x44, 0xEE, 0xEE))
        else:
            out.append((min(255, lum * 3 // 4 + 24), lum * 2 // 5, min(255, lum + 40)))
    return out + [(0x08, 0x04, 0x0E), (0x66, 0x44, 0x88)]


def remap_props(props_pal, pal):
    """Index table from the props' own palette (props.py) to the shared PAL2 (slots 1..14)."""
    return [0] + [min(range(1, 15), key=lambda i: dist(pal[i], props_pal[k])) for k in range(1, 16)]

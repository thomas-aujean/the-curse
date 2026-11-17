"""Imports the PixelLab item icons (Thomas, 2026-11-17: one icon per item, known by its look as in
Isaac): art_in/items/<name>.png, 16 x 16, drawn in PAL2's colours (PixelLab object 268344c0, 64
candidates, see docs/pixellab.md).

Each icon is one 16 x 16 hardware sprite (2 x 2 tiles), centred. Order of the upgrades in game_player.h
(ITEM_STONE_HEART ...), at GFX_ITEMS in inc/gfx.h. Colours go to the nearest slot of the shared PAL2
(slots 1..14, see enemies.py).
"""
import os

from knight import dist
from pngread import read_png

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
SRC = os.path.join(ROOT, "art_in", "items")
ITEMS = ["stone_heart", "wind_boots", "raven_wings", "long_blade",
         "lantern", "glass_heart", "lead_boots", "void_blood"]
CELL = 16


def sheet(pal):
    """Returns the indexed sheet (CELL rows, CELL * len(ITEMS) columns) for the palette pal."""
    cache = {}

    def index(c):
        if c not in cache:
            cache[c] = min(range(1, 15), key=lambda i: dist(pal[i], c))
        return cache[c]

    img = [[0] * (CELL * len(ITEMS)) for _ in range(CELL)]
    for k, name in enumerate(ITEMS):
        rows, (w, h) = read_png(os.path.join(SRC, name + ".png"))
        px = {(x, y): rows[y][x][:3] for y in range(h) for x in range(w) if rows[y][x][3] >= 128}
        x0, x1 = min(p[0] for p in px), max(p[0] for p in px)
        y0, y1 = min(p[1] for p in px), max(p[1] for p in px)
        assert x1 - x0 < CELL and y1 - y0 < CELL, name
        dx = (CELL - (x1 - x0 + 1)) // 2 - x0
        dy = (CELL - (y1 - y0 + 1)) // 2 - y0
        for (x, y), c in px.items():
            img[y + dy][k * CELL + x + dx] = index(c)
    return img

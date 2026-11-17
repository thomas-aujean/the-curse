"""The exit door that appears once the boss is down (Thomas, 2026-11-17: the gem in the middle of the
room, taken or not with UP, and a door to the next floor). PixelLab image 9ca208e1 (pixflux 32 x 48):
art_in/door/door.png, a stone arch with a wooden door ajar on golden light.

32 x 48, colours to the nearest slot of the shared PAL2 (enemies.py). Shown as two sprites (4 x 4 then
4 x 2) on the boss's tiles, free once the boss is gone (main.c, drawSprites).
"""
import os

from knight import dist
from pngread import read_png

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
W, H = 32, 48


def sheet(pal):
    rows, (w, h) = read_png(os.path.join(ROOT, "art_in", "door", "door.png"))
    cache = {}

    def index(c):
        if c not in cache:
            cache[c] = min(range(1, 15), key=lambda i: dist(pal[i], c))
        return cache[c]
    px = {(x, y): rows[y][x][:3] for y in range(h) for x in range(w) if rows[y][x][3] >= 128}
    x0 = (min(x for x, _ in px) + max(x for x, _ in px) + 1) // 2 - W // 2
    y1 = max(y for _, y in px)
    img = [[0] * W for _ in range(H)]
    for (x, y), c in px.items():
        tx, ty = x - x0, y - y1 + H - 1
        if 0 <= tx < W and 0 <= ty < H:
            img[ty][tx] = index(c)
    return img

"""The merchant's caravan and the merchant (Thomas, 2026-11-17: "a merchant and his caravan", the
shop entered with UP in front of the caravan). PixelLab: caravan object 7905517f (candidate 1 of 16),
merchant pixflux 44f1530d; art_in/shop/. Colours to the nearest slot of the shared PAL2 (enemies.py).

Both go where the chests' tiles are (gfx.h, GFX_CHESTS), loaded only in the caravan's room and in the
shop, which have no chest. Sheet: the caravan 64 x 64 (8 columns of 8 tiles), then the merchant 32 x 48
(4 columns of 8 tiles, the top 2 empty), in rescomp's COLUMN order; gfx.c cuts them into sprites.
"""
import os

from knight import dist
from pngread import read_png

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
SRC = os.path.join(ROOT, "art_in", "shop")
CARAVAN_W, CARAVAN_H = 64, 64
MERCHANT_W, MERCHANT_H = 32, 48


def _picture(name, w, h, pal):
    """Pixels bottom-centred in a w x h cell, as PAL2 indices."""
    rows, (iw, ih) = read_png(os.path.join(SRC, name))
    cache = {}

    def index(c):
        if c not in cache:
            cache[c] = min(range(1, 15), key=lambda i: dist(pal[i], c))
        return cache[c]
    px = {(x, y): rows[y][x][:3] for y in range(ih) for x in range(iw) if rows[y][x][3] >= 128}
    x0 = (min(x for x, _ in px) + max(x for x, _ in px) + 1) // 2 - w // 2
    y1 = max(y for _, y in px)
    img = [[0] * w for _ in range(h)]
    for (x, y), c in px.items():
        tx, ty = x - x0, y - y1 + h - 1
        if 0 <= tx < w and 0 <= ty < h:
            img[ty][tx] = index(c)
    return img


def sheet(pal):
    caravan = _picture("caravan.png", CARAVAN_W, CARAVAN_H, pal)
    merchant = _picture("merchant.png", MERCHANT_W, MERCHANT_H, pal)
    rows = [[0] * (CARAVAN_W + MERCHANT_W) for _ in range(CARAVAN_H)]
    for y in range(CARAVAN_H):
        rows[y][:CARAVAN_W] = caravan[y]
    for y in range(MERCHANT_H):
        rows[CARAVAN_H - MERCHANT_H + y][CARAVAN_W:] = merchant[y]   # bottom-aligned, top rows empty
    return rows

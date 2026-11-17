"""The key and the padlock (Thomas, 2026-11-17: keys found at random open the locked chest and the
upgrade room's door): PixelLab object c7460c18, candidates 0 (key) and 3 (padlock), art_in/keys/.
16 x 16 each (2 x 2 tiles, PAL2), at GFX_KEY and GFX_PADLOCK (gfx.h).
"""
import os

import shop

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")


def sheet(pal):
    old = shop.SRC
    shop.SRC = os.path.join(ROOT, "art_in", "keys")
    try:
        key = shop._picture("key.png", 16, 16, pal)
        lock = shop._picture("padlock.png", 16, 16, pal)
    finally:
        shop.SRC = old
    return [key[y] + lock[y] for y in range(16)]

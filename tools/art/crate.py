"""The wooden crate, PixelLab object e3ab622b (candidate 3 of 64), art_in/props/crate.png (Thomas,
2026-11-17: the little crates should be nicer and much bigger): 32 x 32, one 4 x 4 sprite (PAL2),
bottom-centred, at GFX_CRATE (gfx.h).
"""
import os

from shop import _picture

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")


def sheet(pal):
    import shop
    old = shop.SRC
    shop.SRC = os.path.join(ROOT, "art_in", "props")
    try:
        return _picture("crate.png", 32, 32, pal)
    finally:
        shop.SRC = old

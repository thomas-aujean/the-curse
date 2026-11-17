"""The horned skull over the way to the boss (Thomas, 2026-11-17: "make the boss's door more obvious,
I am surprised every time"): PixelLab object 7a6a2aa1 (candidate 12), art_in/boss/skull.png.
32 x 32, one 4 x 4 sprite (PAL2), loaded on the boss's tiles in the rooms next to the boss room.
"""
import os

import shop

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")


def sheet(pal):
    old = shop.SRC
    shop.SRC = os.path.join(ROOT, "art_in", "boss")
    try:
        return shop._picture("skull.png", 32, 32, pal)
    finally:
        shop.SRC = old

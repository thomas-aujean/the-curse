"""The RetroMaru logo, shown before the title (Thomas, 2026-11-17): the "quatre-a" sun stacked over
RETROMARU in thickened Michroma (retromaru/logo/quatre-a/C-emp-0-dark.svg, rendered to
art_in/logo/retromaru.png, 1366 x 1107), scaled down by majority vote to the screen and drawn in its
six flat colours. Writes res/logo.png (IMAGE img_logo, PAL0).
"""
import os

from knight import dist
from pngread import read_png

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
COLOURS = [(0x0D, 0x0D, 0x0D), (0xF1, 0xEC, 0xE2), (0x26, 0x95, 0x8C), (0xF2, 0xC4, 0x64),
           (0xF3, 0xA2, 0x62), (0xE9, 0x70, 0x4F)]   # black, cream, teal, yellow, apricot, coral
HEIGHT = 96                                          # on screen (224 px high): small, the sun pixelated


def build():
    rows, (sw, sh) = read_png(os.path.join(ROOT, "art_in", "logo", "retromaru.png"))
    k = sh / HEIGHT
    w, h = int(sw / k), HEIGHT
    img = []
    for y in range(h):
        line = []
        for x in range(w):
            votes = [0] * len(COLOURS)
            for sy in range(int(y * k), int((y + 1) * k)):
                for sx in range(int(x * k), int((x + 1) * k)):
                    p = rows[sy][sx][:3]
                    votes[min(range(len(COLOURS)), key=lambda i: dist(COLOURS[i], p))] += 1
            # The letters' thin strokes win over the black around them as soon as they cover a third.
            best = max(range(len(COLOURS)), key=lambda i: votes[i] * (3 if i else 1))
            line.append(best)
        img.append(line)
    W, H = 320, 224
    out = [[0] * W for _ in range(H)]
    ox, oy = (W - w) // 2, (H - h) // 2
    for y in range(h):
        out[oy + y][ox:ox + w] = img[y]
    return out, COLOURS + [(0, 0, 0)] * 10   # rescomp turns them into Mega Drive colours

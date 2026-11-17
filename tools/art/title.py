"""The title screen (Thomas, 2026-11-17: "the hero seen from behind on top of a cliff, cape in the
wind, the castle down below"): PixelLab image 89aca91e (pixflux 320 x 224), art_in/title/title_a.png.

One 16-colour palette (PAL0) for the whole picture, picked by median cut on Mega Drive colours, with
an ordered dither between neighbouring shades so the sky's gradients do not band. Index 0 is the
darkest colour: shown through as the backdrop (PAL0 colour 0), the same colour.
Writes res/title.png for rescomp (IMAGE img_title, drawn on plane B before the game loads its tiles).
"""
import os

from knight import dist, median_cut
from png import md_color
from pngread import read_png

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]


def build():
    rows, (w, h) = read_png(os.path.join(ROOT, "art_in", "title", "title_a.png"))
    counts = {}
    for r in rows:
        for p in r:
            c = md_color(p[:3])
            counts[c] = counts.get(c, 0) + 1
    pal = sorted({md_color(c) for c in median_cut(counts, 16)}, key=sum)
    while len(pal) < 16:
        pal.append(pal[-1])
    img = []
    for y, r in enumerate(rows):
        line = []
        for x, p in enumerate(r):
            c = p[:3]
            # The two nearest colours, mixed by the Bayer threshold in proportion to the distances.
            order = sorted(range(16), key=lambda i: dist(pal[i], c))
            a, b = order[0], order[1]
            da, db = dist(pal[a], c) ** 0.5, dist(pal[b], c) ** 0.5
            t = da / (da + db) if da + db else 0
            line.append(b if t * 16 > BAYER[y % 4][x % 4] + 4 else a)   # a slight bias to the nearest
        img.append(line)
    return img, pal

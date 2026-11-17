"""The title screen (Thomas, 2026-11-17: "the hero seen from behind on top of a cliff, cape in the
wind, the castle down below"): PixelLab image 89aca91e (pixflux 320 x 224), art_in/title/title_a.png.

One 16-colour palette (PAL0) for the whole picture, picked by median cut on Mega Drive colours, with
an ordered dither between neighbouring shades so the sky's gradients do not band. Index 0 is the
darkest colour: shown through as the backdrop (PAL0 colour 0), the same colour.
Writes res/title.png for rescomp (IMAGE img_title, drawn on plane B before the game loads its tiles).

The cape flutters (Thomas: "can the cape move?"): PixelLab animate_image c1b98c2d on the 96 x 96 corner
around the knight (art_in/title/cape/frame_0..4.png, frame 0 = the picture). Each frame keeps the
picture except where the cape is or was (red pixels), quantized the same way, in res/title_cape.png:
5 frames of 12 x 12 tiles stacked, streamed into one place of video memory in turn (main.c).
"""
import os

from knight import dist, median_cut
from png import md_color
from pngread import read_png

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]
CAPE_X, CAPE_Y, CAPE = 0, 56, 96   # the animated corner (tile aligned)


def _red(p):
    r, g, b = p[:3]
    return r > 60 and r > g * 14 // 10 and r > b * 13 // 10


def _quantize(rows, pal, x0=0, y0=0):
    img = []
    for y, r in enumerate(rows):
        line = []
        for x, p in enumerate(r):
            c = p[:3]
            order = sorted(range(16), key=lambda i: dist(pal[i], c))
            a, b = order[0], order[1]
            da, db = dist(pal[a], c) ** 0.5, dist(pal[b], c) ** 0.5
            t = da / (da + db) if da + db else 0
            line.append(b if t * 16 > BAYER[(y + y0) % 4][(x + x0) % 4] + 4 else a)
        img.append(line)
    return img


def cape_frames(pal):
    """5 frames of the 96 x 96 corner, stacked (96 x 480), as palette indices."""
    rows, _ = read_png(os.path.join(ROOT, "art_in", "title", "title_a.png"))
    base = [r[CAPE_X:CAPE_X + CAPE] for r in rows[CAPE_Y:CAPE_Y + CAPE]]
    out = []
    for k in range(5):
        fr, _ = read_png(os.path.join(ROOT, "art_in", "title", "cape", "frame_%d.png" % k))
        mix = [[fr[y][x] if (_red(base[y][x]) or _red(fr[y][x])) else base[y][x] for x in range(CAPE)]
               for y in range(CAPE)]
        out += _quantize(mix, pal, CAPE_X, CAPE_Y)
    return out


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
    # The two nearest colours, mixed by the Bayer threshold in proportion to the distances.
    return _quantize(rows, pal), pal

"""Zone-1 far background on plane B (512 x 256), in parallax bands (Thomas, 2026-11-17: "the castle is
beautiful, the rest is ugly; parallax with clouds, hills, castle, as much detail as the console takes").

The Mega Drive scrolls plane B line by line (main.c, scrollFar): each band of rows below drifts at its
own fraction of the room's speed. A band only holds its own layer: anything behind a nearer layer is
one flat colour, which looks the same whatever the scroll (the classic trick). Every layer is made of
PixelLab pieces (art_in/far) whose ends slope down, laid side by side and wrapped around 512 px.

Writes res/far_tiles.png (deduplicated tiles, mirrors included), inc/far_bands.h (bands, for main.c)
and inc/far_map.h (the plane map and where each run of tiles goes in video memory, for gfx.c).
"""
import math
import os

from knight import dist
from pngread import read_png

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
FAR = os.path.join(ROOT, "art_in", "far")
W, H = 512, 256
TILE_USER_INDEX = 16   # SGDK 2.11

# (first row, last row + 1, speed in 1/32 of the room's, drift: px per 16 frames)
BANDS = [
    (0, 56, 1, 1),      # high clouds
    (56, 96, 2, 2),     # low clouds
    (96, 176, 3, 0),    # sun, mountains, the castle
    (176, 212, 8, 0),   # hills
    (212, 256, 16, 0),  # forest
]
# Free stretches of video memory for these tiles: (offset from TILE_USER_INDEX, length); see gfx.h.
# The last 16 tiles of video memory (2016..2031) hold the hero's third slash cell (GFX_NINJA_EXT3).
RANGES = [(228, 252), (576, 24), (656, 44), (716, 44), (1960, 56)]

BAYER = [[0, 2], [3, 1]]
SKY = [10, 11, 12, 13, 14]   # PAL0 slots, dark violet -> sun glow


def sky(x, y):
    t = min(1.0, max(0.0, (y - 4) / 172.0))
    pos = t * (len(SKY) - 1)
    i = int(pos)
    if i >= len(SKY) - 1:
        return SKY[-1]
    return SKY[i + 1] if (pos - i) * 4 > BAYER[y % 2][x % 2] + 0.5 else SKY[i]


def _load(name, pal):
    rows, (w, h) = read_png(os.path.join(FAR, name))
    cache = {}

    def index(c):
        if c not in cache:
            cache[c] = min(range(1, 15), key=lambda i: dist(pal[i], c))
        return cache[c]
    px = {(x, y): index(rows[y][x][:3]) for y in range(h) for x in range(w) if rows[y][x][3] >= 128}
    x0, y0 = min(x for x, _ in px), min(y for _, y in px)
    return {(x - x0, y - y0): c for (x, y), c in px.items()}


def _size(px):
    return max(x for x, _ in px) + 1, max(y for _, y in px) + 1


def _paste(img, px, x0, bottom, top_limit, bottom_limit):
    _, h = _size(px)
    for (x, y), c in px.items():
        yy = bottom - (h - 1) + y
        if top_limit <= yy < bottom_limit:
            img[yy][(x0 + x) % W] = c


def compose(pal):
    img = [[sky(x, y) for x in range(W)] for y in range(H)]

    # Clouds, a few per band, drifting.
    clouds = [_load("cloud_%d.png" % i, pal) for i in range(6)]
    for (x, y, k) in [(20, 22, 0), (330, 16, 2),
                      (70, 76, 3), (250, 88, 4)]:
        band = next(b for b in BANDS if b[0] <= y < b[1])
        _paste(img, clouds[k], x, y, band[0], band[1])

    # Sun behind the mountains.
    sx, sy, r = 112, 140, 15
    for y in range(sy - r - 3, sy + r + 3):
        for x in range(sx - r - 3, sx + r + 3):
            d = math.hypot(x - sx, y - sy)
            if d <= r or (d <= r + 2 and (x + y) % 2 == 0):
                img[y][x] = 14

    # Mountains: the ridge and its mirror make one range sloping down at both ends; two ranges overlap.
    m = _load("mountains.png", pal)
    mw, mh = _size(m)
    m = {(x, y): c for (x, y), c in m.items() if 6 <= x < mw - 7}   # cut PixelLab's stray end columns
    m = {(x - 6, y): c for (x, y), c in m.items()}
    mw -= 13
    ridge = dict(m)
    ridge.update({(2 * mw - 1 - x, y): c for (x, y), c in m.items()})
    for x0 in (0, 256):
        _paste(img, ridge, x0 - 40, 175, 96, 176)
    # The castle on its peak.
    castle = {}
    rows, (cw, ch) = read_png(os.path.join(FAR, "castle_1.png"))
    for y in range(ch):
        for x in range(cw):
            p = rows[y][x]
            if p[3] >= 128:
                castle[(x, y)] = {(0x14, 0x0A, 0x14): 10, (0x2E, 0x1A, 0x44): 10, (0x5A, 0x2A, 0x5A): 11,
                                  (0xA0, 0x40, 0x5A): 12, (0xF8, 0xB0, 0x60): 14}[
                    min([(0x14, 0x0A, 0x14), (0x2E, 0x1A, 0x44), (0x5A, 0x2A, 0x5A), (0xA0, 0x40, 0x5A),
                         (0xF8, 0xB0, 0x60)], key=lambda c: dist(c, p[:3]))]
    x0, y0 = min(x for x, _ in castle), min(y for _, y in castle)
    castle = {(x - x0, y - y0): c for (x, y), c in castle.items()}
    # Lower than its picture (Thomas: "it still looks like it floats"): its peak stands behind a dark
    # ridge running the whole width at the bottom of the band. The ridge's colour (10) carries on as the
    # flat colour of the next band, so nothing ends in mid-air at the band's edge.
    _paste(img, castle, 300, 175 + 16, 96, 176)
    ridge = {}
    pieces = [_load("hill_a.png", pal), _load("hill_b.png", pal)]
    for x0, k in ((0, 0), (128, 0), (256, 0), (384, 0)):   # period 128: its tiles repeat
        pw, ph = _size(pieces[k])
        for (x, y), c in pieces[k].items():
            yy = 175 + 4 - (ph - 1) + y         # sunk 4 px: a low ridge
            if yy < 176:
                key = ((x0 + x) % W, yy)
                ridge[key] = c
    for x in range(W):
        ys = [y for (xx, y) in ridge if xx == x]
        if not ys:
            continue
        for y in range(min(ys), 176):
            img[y][x] = ridge.get((x, y), 10)

    # Hills in front of the ridge's flat colour, one step darker than the ridge.
    for y in range(176, 212):
        for x in range(W):
            img[y][x] = 10
    darker = {10: 2, 11: 10, 12: 11}
    hills = [{p: darker.get(c, c) for p, c in _load(n, pal).items()} for n in ("hill_a.png", "hill_b.png")]
    for x0, k in ((0, 0), (150, 1), (310, 0), (390, 1)):
        _paste(img, hills[k], x0, 211, 176, 212)

    # Forest in front of the hills' flat base colour; period 256 to save tiles.
    for y in range(212, 256):
        for x in range(W):
            img[y][x] = 2
    forest = [_load("forest_a.png", pal), _load("forest_b.png", pal)]
    for x0, k in ((0, 0), (110, 1), (256, 0), (366, 1)):
        _paste(img, forest[k], x0, 212 + _size(forest[k])[1] - 1, 212, 256)
    return img


def build(pal):
    """pal: PAL0 (16 colours). Returns (tile sheet rows, tile count, far_bands.h, far_map.h)."""
    img = compose(pal)
    tiles, index, plane = [], {}, []
    for ty in range(H // 8):
        for tx in range(W // 8):
            t = tuple(tuple(img[ty * 8 + y][tx * 8:tx * 8 + 8]) for y in range(8))
            entry = None
            for hf in (0, 1):
                for vf in (0, 1):
                    v = t
                    if hf:
                        v = tuple(r[::-1] for r in v)
                    if vf:
                        v = v[::-1]
                    if v in index:
                        entry = (index[v], hf, vf)
                        break
                if entry:
                    break
            if not entry:
                index[t] = len(tiles)
                tiles.append(t)
                entry = (index[t], 0, 0)
            plane.append(entry)
    total = sum(n for _, n in RANGES)
    assert len(tiles) <= total, "far background: %d tiles for %d places" % (len(tiles), total)

    vram, runs, k = [], [], 0
    for start, n in RANGES:
        take = min(n, len(tiles) - k)
        if take <= 0:
            break
        runs.append((k, TILE_USER_INDEX + start, take))
        vram += [TILE_USER_INDEX + start + i for i in range(take)]
        k += take

    count = len(tiles)
    while len(tiles) % 16:
        tiles.append(tuple(tuple([0] * 8) for _ in range(8)))
    sheet = [[0] * 128 for _ in range(len(tiles) // 16 * 8)]
    for i, t in enumerate(tiles):
        for y in range(8):
            sheet[(i // 16) * 8 + y][(i % 16) * 8:(i % 16) * 8 + 8] = list(t[y])

    bands = ["// Generated by tools/art/far.py: plane B parallax bands (see main.c, scrollFar).",
             "#ifndef FAR_BANDS_H", "#define FAR_BANDS_H", "",
             "#define FAR_BANDS %d" % len(BANDS),
             "static const u16 FAR_BAND_END[FAR_BANDS] = { %s };   // plane row after the band"
             % ", ".join(str(b[1]) for b in BANDS),
             "static const u16 FAR_BAND_SPEED[FAR_BANDS] = { %s };   // 1/32 of the room's speed"
             % ", ".join(str(b[2]) for b in BANDS),
             "static const s16 FAR_BAND_DRIFT[FAR_BANDS] = { %s };   // px per 16 frames"
             % ", ".join(str(b[3]) for b in BANDS),
             "", "#endif", ""]
    m = ["// Generated by tools/art/far.py: the far background's plane map (64 x 32, PAL0, low priority)",
         "// and the runs of res/far_tiles.png in video memory: { first tile, video memory index, count }.",
         "#ifndef FAR_MAP_H", "#define FAR_MAP_H", "",
         "#define FAR_TILE_COUNT %d" % count,
         "#define FAR_RUNS %d" % len(runs),
         "static const u16 FAR_RUN[FAR_RUNS][3] = { %s };" % ", ".join("{ %d, %d, %d }" % r for r in runs),
         "static const u16 FAR_MAP[64 * 32] =", "{"]
    for ty in range(H // 8):
        row = []
        for tx in range(W // 8):
            i, hf, vf = plane[ty * (W // 8) + tx]
            row.append("0x%04X" % ((vf << 12) | (hf << 11) | vram[i]))
        m.append("    " + ", ".join(row) + ",")
    m += ["};", "", "#endif", ""]
    return sheet, count, img, "\n".join(bands), "\n".join(m)

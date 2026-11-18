"""The merchant's shop, inside the caravan (Thomas, 2026-11-18: "the shop should always look the same,
like the inside of the caravan"): one PixelLab picture (pixflux 9a55a9d9, art_in/shop/interior.png,
320 x 224) shown whole on the far plane, with its own PAL0.

The shop is a single screen that never scrolls, so the picture is drawn once, without parallax. In
the shop nothing else uses the room's tiles: no room tiles (the room plane only holds empty tiles),
no decor, no far background. Its tiles take their places in video memory (RANGES) and the room's
come back on the way out (gfx_loadShopRoom).

Writes res/shop_room.png (tiles, deduplicated with mirrors, and the palette) and inc/shop_room.h (the
plane map and where each run of tiles goes in video memory).
"""
import os

from knight import dist, median_cut
from pngread import read_png

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
SRC = os.path.join(ROOT, "art_in", "shop", "interior.png")
# Only the three wares in the shop (Thomas, 2026-11-18: no merchant, nothing else): the odd grey
# object PixelLab left on the floor outside the caravan's wall is painted out (x0, y0, x1, y1).
ERASE = (281, 176, 308, 218)
W, H = 320, 224
HUD_ROWS = 3           # under the window (HUD): never seen, not loaded
TILE_USER_INDEX = 16   # SGDK 2.11
COLOURS = 14           # PAL0 slots 1..14; 0 is the backdrop, 15 the text
# Places of the room's tiles in video memory (offsets from TILE_USER_INDEX, length), see gfx.h: the
# castle tiles, the far background's runs (far.py, RANGES) and the decor's two stretches.
RANGES = [(140, 88), (228, 252), (576, 24), (656, 44), (1960, 56), (1315, 109), (0xD180 // 32 - 16, 116)]


def build(pal0):
    """pal0: the room palette (slots 0 and 15 kept). Returns (tile sheet rows, palette, tile count,
    shop_room.h)."""
    rows, (w, h) = read_png(SRC)
    assert (w, h) == (W, H), "the shop picture must be %d x %d" % (W, H)
    for y in range(ERASE[1], ERASE[3]):
        for x in range(ERASE[0], ERASE[2]):
            rows[y][x] = rows[H - 1][W - 1]   # the black around the caravan
    counts = {}
    for y in range(HUD_ROWS * 8, H):
        for x in range(W):
            c = rows[y][x][:3]
            counts[c] = counts.get(c, 0) + 1
    colours = median_cut(counts, COLOURS)
    pal = [pal0[0]] + colours + [pal0[15]]
    cache = {}

    def index(c):
        if c not in cache:
            cache[c] = 1 + min(range(COLOURS), key=lambda i: dist(colours[i], c))
        return cache[c]

    img = [[index(rows[y][x][:3]) for x in range(W)] for y in range(H)]
    total = sum(n for _, n in RANGES)
    for slack in range(0, 256, 4):
        tiles, plane = _cut(img, colours, slack)
        if len(tiles) <= total:
            break
    assert len(tiles) <= total, "shop picture: %d tiles for %d places" % (len(tiles), total)
    return _output(tiles, plane, pal)


def _cut(img, colours, slack):
    """Tiles of the picture, mirrors shared; a tile within `slack` (summed colour distance of its
    pixels, in 1/1024) of one already kept takes its place, so the picture fits."""
    tiles, known, plane = [], {}, []

    def far_from(a, b):
        d = 0
        for ra, rb in zip(a, b):
            for x, y in zip(ra, rb):
                if x != y:
                    d += dist(colours[x - 1], colours[y - 1]) >> 10 if x and y else 1 << 20
                    if d > slack:
                        return True
        return False
    for ty in range(H // 8):
        for tx in range(W // 8):
            if ty < HUD_ROWS:
                plane.append(None)
                continue
            t = tuple(tuple(img[ty * 8 + y][tx * 8:tx * 8 + 8]) for y in range(8))
            entry = None
            for hf in (0, 1):
                for vf in (0, 1):
                    v = tuple(r[::-1] for r in t) if hf else t
                    v = v[::-1] if vf else v
                    if v in known:
                        entry = (known[v], hf, vf)
                        break
                if entry:
                    break
            if not entry and slack:
                for i, k in enumerate(tiles):
                    if not far_from(t, k):
                        entry = (i, 0, 0)
                        break
            if not entry:
                known[t] = len(tiles)
                tiles.append(t)
                entry = (known[t], 0, 0)
            plane.append(entry)
    return tiles, plane


def _output(tiles, plane, pal):
    """(tile sheet rows, palette, tile count, shop_room.h)."""
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

    m = ["// Generated by tools/art/shop_room.py: the shop's picture on the far plane (40 x 28, PAL0, low",
         "// priority) and the runs of res/shop_room.png in video memory: { first tile, video memory index, count }.",
         "#ifndef SHOP_ROOM_H", "#define SHOP_ROOM_H", "",
         "#define SHOP_ROOM_TILE_COUNT %d" % count,
         "#define SHOP_ROOM_RUNS %d" % len(runs),
         "static const u16 SHOP_ROOM_RUN[SHOP_ROOM_RUNS][3] = { %s };" % ", ".join("{ %d, %d, %d }" % r for r in runs),
         "static const u16 SHOP_ROOM_MAP[40 * 28] =", "{"]
    for ty in range(H // 8):
        row = []
        for tx in range(W // 8):
            e = plane[ty * (W // 8) + tx]
            row.append("0x0000" if e is None else "0x%04X" % ((e[2] << 12) | (e[1] << 11) | vram[e[0]]))
        m.append("    " + ", ".join(row) + ",")
    m += ["};", "", "#endif", ""]
    return sheet, pal, count, "\n".join(m)

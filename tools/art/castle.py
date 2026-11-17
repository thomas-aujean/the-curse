"""Zone-1 castle tiles, from the PixelLab side-scroller tileset (art_in/tiles/castle_tileset.png).

PixelLab gives 16 "Wang" tiles of 16 x 16: one per combination of solid / empty around a block
corner. Rooms are drawn in 16 x 16 blocks (rooms.c) but the engine maps 8 x 8 tiles, so each Wang
tile is cut into its four quarters: an 8 x 8 map tile is one quarter of a block, and its drawing
depends on that block and on the three blocks around the nearest block corner.

Tile order is the engine's contract (gfx.h):
  0                 empty
  1..64             stone [quarter * 16 + bits], quarter = (ty & 1) * 2 + (tx & 1),
                    bits: 1 horizontal neighbour solid, 2 vertical, 4 diagonal, 8 the tile itself
  65..68            cracked wall [quarter]
  69..72            plank alone, left end, middle, right end
"""
import json
import os

from knight import dist, median_cut
from pngread import read_png

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
TILES = os.path.join(ROOT, "art_in", "tiles")
COLOURS = 9   # PAL0 slots 1..9 belong to the room tiles (docs/style.md)

# Per quarter: which Wang corner each of (self, horizontal, vertical, diagonal) is, and where the
# quarter sits in the 16 x 16 Wang tile.
QUARTERS = [
    (("SE", "SW", "NE", "NW"), (8, 8)),   # top-left quarter of a block
    (("SW", "SE", "NW", "NE"), (0, 8)),   # top-right
    (("NE", "NW", "SE", "SW"), (8, 0)),   # bottom-left
    (("NW", "NE", "SW", "SE"), (0, 0)),   # bottom-right
]


def plank_tile(left_end, right_end):
    """Wooden one-way platform, in the old generated palette (remapped by tileset())."""
    rows = []
    for y in range(8):
        row = []
        for x in range(8):
            if y == 0:
                c = 6 if 0 < x < 7 or not (left_end or right_end) else 8
            elif y in (1, 2, 3):
                c = 8 if (x + y * 3) % 7 else 7      # grain
            elif y == 4:
                c = 7
            elif y == 5:
                c = 1
            else:
                c = 0
            if (left_end and x == 0) or (right_end and x == 7):
                c = 1 if y <= 5 else 0
            # Iron bracket under the plank ends.
            if y >= 5 and y <= 7 and ((left_end and x in (1, 2)) or (right_end and x in (5, 6))):
                c = 1 if y < 7 else 0
            row.append(c)
        rows.append(row)
    return rows


def tileset(old_pal, name="castle_tileset", pal=None, extra=None):
    """Returns (sheet rows of palette indices, tile count, the 9 colours of PAL0 slots 1..9).
    old_pal: the generated PAL0, whose colours 1..9 the planks were drawn with.
    pal: the 9 colours to use (default: picked from the tileset); extra: 8 x 8 tiles (indices 0..9) that
    replace the planks (zone 1 outside: the branches, tools/art/outdoor.py)."""
    src = os.path.join(TILES, name)
    px, _ = read_png(src + ".png")
    meta = json.load(open(src + ".json", encoding="utf-8"))["tileset_data"]["tiles"]
    counts = {}
    for row in px:
        for p in row:
            if p[3] > 127:
                counts[p[:3]] = counts.get(p[:3], 0) + 1
    if pal is None:
        pal = sorted(median_cut(counts, COLOURS), key=sum)   # slot 1 = darkest (outline, far silhouettes)

    def index(rgb):
        return 1 + min(range(len(pal)), key=lambda i: dist(pal[i], rgb))

    # In PixelLab's metadata "lower" is the platform material, "upper" the air above it.
    wang = {}
    for t in meta:
        key = tuple(t["corners"][c] == "lower" for c in ("NW", "NE", "SW", "SE"))
        bx, by = t["bounding_box"]["x"], t["bounding_box"]["y"]
        wang[key] = [[index(p[:3]) if p[3] > 127 else 0 for p in row[bx:bx + 16]] for row in px[by:by + 16]]

    tiles = [[[0] * 8 for _ in range(8)]]
    for corners, (ox, oy) in QUARTERS:
        for bits in range(16):
            solid = dict(zip(corners, (bits & 8, bits & 1, bits & 2, bits & 4)))
            src = wang[tuple(bool(solid[c]) for c in ("NW", "NE", "SW", "SE"))]
            tiles.append([row[ox:ox + 8] for row in src[oy:oy + 8]])
    # Cracked wall: the plain inner stone with a thin dark crack ("slightly different", learnable).
    for q in range(4):
        t = [row[:] for row in tiles[1 + q * 16 + 15]]
        for y, x in enumerate([3, 3, 4, 4, 3, 4, 5, 5]):
            t[y][x] = 1
            t[y][x - 1] = min(t[y][x - 1], 2)
        tiles.append(t)
    # Planks keep to the stone's browns: the nearest colour of a wood tone must not be a moss green.
    browns = [i for i in range(len(pal)) if pal[i][0] >= pal[i][1]]
    remap = [0] + [1 + min(browns, key=lambda i: dist(pal[i], old_pal[k])) for k in range(1, 10)]
    if extra is not None:
        tiles += extra
    else:
        for ends in ((True, True), (True, False), (False, False), (False, True)):
            tiles.append([[remap[c] for c in row] for row in plank_tile(*ends)])
    # Deep inside the ground (CASTLE_DEEP): the plain inner quarters in darker colours, for depth.
    def darker(k):
        c = pal[k - 1]
        target = tuple(v * 0.6 for v in c)
        lower = [i for i in range(len(pal)) if sum(pal[i]) < sum(c)] or [k - 1]
        return 1 + min(lower, key=lambda i: dist(pal[i], target))
    dark = [0] + [darker(k) for k in range(1, len(pal) + 1)]
    for q in range(4):
        tiles.append([[dark[c] for c in row] for row in tiles[1 + q * 16 + 15]])
    count = len(tiles)
    while len(tiles) % 16:
        tiles.append([[0] * 8 for _ in range(8)])
    img = [[0] * 128 for _ in range(len(tiles) // 16 * 8)]
    for i, t in enumerate(tiles):
        ox, oy = (i % 16) * 8, (i // 16) * 8
        for y in range(8):
            img[oy + y][ox:ox + 8] = t[y]
    return img, count, pal

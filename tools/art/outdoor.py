"""Zone 1, outside the castle (Thomas, 2026-11-17): earth and grass, branches as one-way platforms,
dead trees, graves and an iron fence. All PixelLab, all in PAL0 slots 1..9.

Room tiles (res/castle.png, contract in inc/gfx.h): the ground's Wang quarters and cracked walls as in
castle.py, then the branch pieces instead of the planks:
  69..73  branch, platform row:   middle A, middle B, end, tip 1, tip 2
  74..78  branch, row under it:   same order
A platform run is middle tiles (A / B by column parity) between two ends; the ends and the tips past
them are the same tiles mirrored on the left. Tips and the row under a branch only go on empty tiles.

Decor (res/decor.png, inc/decor_tiles.h): whole pictures cut into 8 x 8 tiles, deduplicated, drawn
by the engine on empty tiles of the room plane (src/game_decor.c). Tile 0 is empty.
"""
import os

from knight import dist, median_cut
from pngread import read_png

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
ART = os.path.join(ROOT, "art_in")
COLOURS = 9

# name, file; the engine's DECOR_* enum follows this order.
DECOR = [
    ("TREE_A", "trees/tree_a.png"),
    ("TREE_B", "trees/tree_b.png"),
    ("FENCE", "decor/fence.png"),
    ("GRAVE_ROUND", "decor/grave_round.png"),
    ("GRAVE_CROSS", "decor/grave_cross.png"),
    ("GRAVE_BROKEN", "decor/grave_broken.png"),
    ("GRAVE_COLUMN", "decor/grave_column.png"),
    ("GRAVE_SKULL", "decor/grave_skull.png"),
]
BRANCH = "trees/branch.png"
BRANCH_COLUMNS = [16, 24, 40, 48, 56]   # x of middle A, middle B, end, tip 1, tip 2 in the picture


def _load(rel):
    rows, (w, h) = read_png(os.path.join(ART, rel))
    return {(x, y): rows[y][x][:3] for y in range(h) for x in range(w) if rows[y][x][3] >= 128}


GRASS_COLOURS, GROUND_COLOURS = 2, 4


def _green(c):
    return c[1] > c[0] + 8 and c[1] > c[2] + 8


def palette(ground_png):
    """The 9 colours shared by everything outside: 2 for the grass on the ground (green pixels, or
    they lose to the many browns of the earth or stones), 4 for the rest of the ground, 3 for the
    branch and the decor, among their colours that the ground's do not already give (each picture
    weighed the same whatever its size, so a small grave still gets its greys)."""
    grass, rest = {}, {}
    for c in _load(ground_png).values():
        d = grass if _green(c) else rest
        d[c] = d.get(c, 0) + 1
    ground = median_cut(grass, GRASS_COLOURS) + median_cut(rest, GROUND_COLOURS)
    counts = {}
    for px in [_load(BRANCH)] + [_load(f) for _, f in DECOR]:
        n = len(px)
        for c in px.values():
            if min(dist(c, g) for g in ground) > 1200:
                counts[c] = counts.get(c, 0) + 10000.0 / n
    pal = sorted(ground + median_cut(counts, COLOURS - GRASS_COLOURS - GROUND_COLOURS), key=sum)
    GRASS.clear()
    GRASS.update(1 + i for i, c in enumerate(pal) if _green(c))
    return pal


GRASS = set()   # palette indices of the grass greens, filled by palette()


def _indexer(pal):
    cache = {}

    def index(c):
        if c not in cache:
            cache[c] = 1 + min(range(len(pal)), key=lambda i: dist(pal[i], c))
        return cache[c]
    return index


def branch_tiles(pal):
    index = _indexer(pal)
    px = _load(BRANCH)
    top = min(y for _, y in px)
    tiles = []
    for row in (0, 1):
        for x0 in BRANCH_COLUMNS:
            tiles.append([[index(px[(x0 + x, top + row * 8 + y)]) if (x0 + x, top + row * 8 + y) in px else 0
                           for x in range(8)] for y in range(8)])
    return tiles


SINK = 3   # px a picture goes down into the ground, behind the grass: it stands, it does not float


def decor(pal, ground):
    """Returns (sheet rows 128 px wide, tile count, C header).
    ground: the two tiles (palette indices) of a flat ground top, for even and odd columns. Every
    picture gets one more row of cells, on the ground: the ground tile with the foot of the picture
    (SINK px) drawn over it and the grass blades of the ground's two top rows drawn back in front.
    Pictures stand on even columns, so the parity of each of their columns is known here."""
    index = _indexer(pal)
    tiles, seen = [[[0] * 8 for _ in range(8)]], {}
    seen[tuple(tuple(r) for r in tiles[0])] = 0
    kinds = []
    for name, rel in DECOR:
        px = _load(rel)
        x0, x1 = min(x for x, _ in px), max(x for x, _ in px)
        y0, y1 = min(y for _, y in px), max(y for _, y in px)
        w = (x1 - x0 + 8) // 8
        above = (y1 - y0 + 1 - SINK + 7) // 8      # rows of cells over the ground
        h = above + 1
        ox = x0 - (w * 8 - (x1 - x0 + 1)) // 2      # centred
        oy = y1 + 1 - (above * 8 + SINK)            # picture bottom SINK px into the ground row
        cells = []
        for ty in range(h):
            for tx in range(w):
                t = [[index(px[(ox + tx * 8 + x, oy + ty * 8 + y)])
                      if (ox + tx * 8 + x, oy + ty * 8 + y) in px else 0 for x in range(8)] for y in range(8)]
                if ty == above:
                    g = ground[tx & 1]
                    t = [[t[y][x] if y < SINK and t[y][x] else g[y][x] for x in range(8)] for y in range(8)]
                    for y in range(2):   # grass blades in front of the foot, one in two on the second row
                        for x in range(8):
                            if g[y][x] and (y == 0 or (x + y) & 1) and g[y][x] in GRASS:
                                t[y][x] = g[y][x]
                key = tuple(tuple(r) for r in t)
                if key not in seen:
                    seen[key] = len(tiles)
                    tiles.append(t)
                cells.append(seen[key])
        kinds.append((name, w, h, cells))

    count = len(tiles)
    while len(tiles) % 16:
        tiles.append([[0] * 8 for _ in range(8)])
    img = [[0] * 128 for _ in range(len(tiles) // 16 * 8)]
    for i, t in enumerate(tiles):
        for y in range(8):
            img[(i // 16) * 8 + y][(i % 16) * 8:(i % 16) * 8 + 8] = t[y]

    out = ["// Generated by tools/art/outdoor.py: zone-1 decor, cut into 8 x 8 tiles (res/decor.png).",
           "// Each kind: width and height in tiles, then its cells row by row (0 = empty, else decor tile).",
           "#ifndef DECOR_TILES_H", "#define DECOR_TILES_H", "",
           "#define DECOR_TILE_COUNT %d" % count, "",
           "enum { %s, DECOR_KINDS };" % ", ".join("DECOR_" + k[0] for k in kinds), ""]
    out.append("static const u8 DECOR_W[DECOR_KINDS] = { %s };" % ", ".join(str(k[1]) for k in kinds))
    out.append("static const u8 DECOR_H[DECOR_KINDS] = { %s };" % ", ".join(str(k[2]) for k in kinds))
    starts, pos = [], 0
    for k in kinds:
        starts.append(pos)
        pos += len(k[3])
    out.append("static const u16 DECOR_START[DECOR_KINDS] = { %s };" % ", ".join(map(str, starts)))
    out.append("static const u8 DECOR_CELLS[%d] =" % pos)
    out.append("{")
    for name, w, h, cells in kinds:
        out.append("    // %s" % name)
        for r in range(h):
            out.append("    " + ", ".join("%3d" % c for c in cells[r * w:(r + 1) * w]) + ",")
    out += ["};", "", "#endif", ""]
    assert count < 256
    return img, count, "\n".join(out)

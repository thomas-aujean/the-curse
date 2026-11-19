"""World A, the forest at dusk (Thomas, 2026-11-19: the mockup art_in/mockups/monde1_A_foret.png, floors 1-2).

One palette for the whole world (PAL0 slots 1..14, from the mockup's backdrop and the world's pictures),
so the far forest, the ground, the branches and the decor match, as in the mockup. The far plane is cut
in bands of repeated pieces of the backdrop (art_in/forest/far.png, the mockup without its castle, its
knight and its ledge): the whole picture would take 951 tiles, the plane has 376 places. The big
twisted trees that frame the mockup become decor in the rooms.

Same contracts as the other worlds (tools/art/interior.py):
  res/forest.png       room tiles (castle.py order, the branch for the platforms) and PAL0
  res/forest_far_tiles.png, inc/forest_far_map.h   the far plane (64 x 32) and its runs in video memory
  res/forest_decor.png, inc/forest_decor_tiles.h   the decor (DECORF_*)
"""
import os

from knight import dist, median_cut
from pngread import read_png
import outdoor
import interior

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
ART = os.path.join(ROOT, "art_in")
TILESET = "forest_tileset"            # PixelLab sidescroller 1a00c92e
BRANCH = "forest/branch.png"          # PixelLab pixflux 3a835b5e
BRANCH_COLUMNS = [16, 24, 40, 48, 56]
FAR = "forest/far.png"                # PixelLab pixflux 4e520d8a (from the mockup), 320 x 224
# The backdrop's pieces: the sky between the trees' crowns (x, width, rows), repeated with its mirror;
# the mountains and the forest below it, the middle of the picture, repeated with its mirror.
SKY = (128, 64, 0, 56)
LAND = (112, 128, 56, 224)
SUN = (165, 60, 222, 102)   # x0, y0, x1, y1 of the sun in the backdrop
DECOR = [
    ("BIG_TREE", "forest/big_tree.png"),
    ("CRYSTALS", "forest/crystals.png"),
]
COLOURS = 14


def _load(rel):
    rows, (w, h) = read_png(os.path.join(ART, rel))
    return {(x, y): rows[y][x][:3] for y in range(h) for x in range(w) if rows[y][x][3] >= 128}


def palette():
    """14 colours: the backdrop weighed as much as all the rest together (it fills the screen)."""
    counts = {}
    far = _load(FAR)
    for c in far.values():
        counts[c] = counts.get(c, 0) + 1.0
    others = [_load(os.path.join("tiles", TILESET + ".png")), _load(BRANCH)] + [_load(f) for _, f in DECOR]
    for px in others:
        for c in px.values():
            counts[c] = counts.get(c, 0) + len(far) / len(others) / len(px)
    return sorted(median_cut(counts, COLOURS), key=sum)


def branch_tiles(pal):
    return outdoor.branch_tiles(pal, BRANCH, BRANCH_COLUMNS)


def far(pal0):
    """pal0: PAL0 (slots 1..14 the world's). Returns (tile sheet rows, tile count, forest_far_map.h,
    the plane's picture for previews)."""
    rows, (w, h) = read_png(os.path.join(ART, FAR))
    cache = {}

    def index(c):
        if c not in cache:
            cache[c] = 1 + min(range(COLOURS), key=lambda i: dist(pal0[1 + i], c))
        return cache[c]

    def sky(y):
        """The sky's colour on row y: the commonest colour between the trees' crowns, dark ones aside."""
        row = [rows[y][x][:3] for x in range(120, 200) if sum(rows[y][x][:3]) >= 150]
        return index(max(set(row), key=row.count)) if row else index(rows[y][160][:3])

    def piece(spec, y):
        """Row y of a piece repeated with its mirror: (x, width) of the picture, period 2 * width.
        The sun is left out (the mirror would show two): the sky's colour instead."""
        x0, pw, _, _ = spec
        s = sky(y)

        def at(x):
            k = x % (2 * pw)
            sx = x0 + (k if k < pw else 2 * pw - 1 - k)
            c = rows[y][sx][:3]
            if SUN[0] <= sx < SUN[2] and SUN[1] <= y < SUN[3] and sum(c) > 600:
                return s
            if y < LAND[2] + 20 and sum(c) < 60:   # tips of the trees' crowns hanging into the land
                return s
            return index(c)
        return at

    W, H = 512, 256
    top = H - (LAND[3] - SKY[2])   # the plane is 256 rows: more sky on top (the camera's vertical travel)
    img = []
    for y in range(H):
        py = max(0, y - top)   # row of the picture
        if py < SKY[3]:   # the sky: bands of its colours (the crowns stay with the big trees), dithered
            c, n = sky(py), sky(min(py + 2, SKY[3] - 1))   # into the next band over its last two rows
            img.append([n if c != n and (x + y) & 1 else c for x in range(W)])
        else:
            at = piece(LAND, py)
            img.append([at(x) for x in range(W)])
    pal = pal0
    total = sum(n for _, n in interior.RANGES)
    for slack in range(0, 512, 8):
        tiles, plane = interior._cut(img, pal, slack)
        if len(tiles) <= total:
            break
    assert len(tiles) <= total, "forest far plane: %d tiles for %d places" % (len(tiles), total)
    vram, runs, k = [], [], 0
    for start, n in interior.RANGES:
        take = min(n, len(tiles) - k)
        if take <= 0:
            break
        runs.append((k, interior.TILE_USER_INDEX + start, take))
        vram += [interior.TILE_USER_INDEX + start + i for i in range(take)]
        k += take
    count = len(tiles)
    while len(tiles) % 16:
        tiles.append(tuple(tuple([0] * 8) for _ in range(8)))
    sheet = [[0] * 128 for _ in range(len(tiles) // 16 * 8)]
    for i, t in enumerate(tiles):
        for y in range(8):
            sheet[(i // 16) * 8 + y][(i % 16) * 8:(i % 16) * 8 + 8] = list(t[y])
    m = ["// Generated by tools/art/forest.py: world A's far plane (64 x 32, PAL0, low priority) and the runs",
         "// of res/forest_far_tiles.png in video memory: { first tile, video memory index, count }.",
         "#ifndef FOREST_FAR_MAP_H", "#define FOREST_FAR_MAP_H", "",
         "#define FOREST_FAR_SKY_END %d   // plane row where the land band starts" % (top + SKY[3]),
         "#define FOREST_FAR_RUNS %d" % len(runs),
         "static const u16 FOREST_FAR_RUN[FOREST_FAR_RUNS][3] = { %s };" % ", ".join("{ %d, %d, %d }" % r for r in runs),
         "static const u16 FOREST_FAR_MAP[64 * 32] =", "{"]
    for ty in range(H // 8):
        m.append("    " + ", ".join("0x%04X" % ((vf << 12) | (hf << 11) | vram[i])
                                    for i, hf, vf in plane[ty * 64:(ty + 1) * 64]) + ",")
    m += ["};", "", "#endif", ""]
    return sheet, count, chr(10).join(m), img


def decor(pal, ground):
    grass = set(outdoor.GRASS)
    outdoor.GRASS.clear()
    out = outdoor.decor(pal, ground, DECOR, "DECORF", ())
    outdoor.GRASS.update(grass)
    return out

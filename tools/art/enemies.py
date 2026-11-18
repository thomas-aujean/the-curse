"""Imports the PixelLab enemies: key poses in art_in/enemies/poses/<enemy>/<pose>.png, facing right.

Humans are drawn on a 48 x 48 canvas, small creatures on 32 x 32. A pose is rarely as wide as its
canvas, and video memory is short, so every frame is cut into at most four hardware sprites:

    A  4 x 4 tiles at (0, 0)       B  1 or 2 x 4 tiles at (32, 0)
    C  4 x 2 tiles at (0, 32)      D  1 or 2 x 2 tiles at (32, 32)

Only the parts that hold pixels get tiles. A pose up to 32 px wide is slid into A and C alone (24
tiles); a weapon held out adds the strip B / D. The tiles are written in the order the engine loads
them (part after part, each one column by column, as the VDP reads a sprite): inc/enemy_frames.h
tells where each frame starts and where its cell sits around the enemy's feet.

PAL2 is shared by the enemies, the props and the coins. Fixed slots, used by drawings made in code
(gfx.c): 1 outline, 2 bone, 3 bone shadow, 4 red, 5 gold, 6 gold shadow, 15 map text. Slots 7..13 are
picked from the enemies' colours; slot 14 is for props only, because the Void form of an enemy is the
same tiles shown with PAL3, and in shadow / highlight mode a sprite pixel 14 or 15 of PAL3 is not a
colour (it lights or darkens what is behind).
"""
import os

from knight import dist, median_cut
from pngread import read_png

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
SRC = os.path.join(ROOT, "art_in", "enemies", "poses")

# Engine frame -> source image. Order is free: the engine uses the names (EF_* in enemy_frames.h).
HUMANS = [
    ("soldier_walk_a", "soldier/walk_a"), ("soldier_walk_b", "soldier/walk_b"),
    ("soldier_windup", "soldier/windup"), ("soldier_strike", "soldier/strike"),
    ("archer_idle", "archer/walk_a"), ("archer_aim", "archer/aim_sky"), ("archer_release", "archer/release_sky"),
    ("guard_walk_a", "guard/walk_a"), ("guard_walk_b", "guard/walk_b"),
    # Guard (2026-11-17): spear drawn back behind the shoulder (PixelLab state 1822e872), then the
    # thrust with its crescent (slash.py); upwards: state 20f0ab0f. The old "brace" pose is unused.
    ("guard_windup", "guard/windup_back"), ("guard_thrust", "guard/thrust"), ("guard_up", "guard/strike_up"),
    # The ninja is out for now (Thomas, 2026-11-17: "the ninjas are no good"); its art stays in
    # art_in/enemies/poses/ninja.
]
FLOATING = [("spectre_float_a", "spectre/float_a"), ("spectre_float_b", "spectre/float_b"),
            ("spectre_lunge", "spectre/lunge"),
            # Bat (2026-11-17): PixelLab object fe55975b (candidate 10) and its state 454ad00a.
            ("bat_a", "bat/fly_a"), ("bat_b", "bat/fly_b"), ("bat_hang", "bat/hang")]
GROUND_SMALL = [("jelly_squash", "jelly/squash"), ("jelly_stretch", "jelly/stretch")]

FIXED = [(0x10, 0x08, 0x10), (0x00, 0x00, 0x00), (0xCC, 0xCC, 0xAA), (0x88, 0x88, 0x66), (0xEE, 0x22, 0x22),
         (0xEE, 0xBB, 0x22), (0xAA, 0x66, 0x00)]
PROPS_ONLY = (0xE0, 0x70, 0x20)   # slot 14: flame orange
MAP_TEXT = (0xEE, 0xEE, 0x44)
MAX_TILES = 664   # room in video memory after the props (gfx.h: GFX_ENEMIES up to the font)


def _load(path):
    px, _ = read_png(os.path.join(SRC, *path.split("/")) + ".png")
    return {(x, y): p[:3] for y, row in enumerate(px) for x, p in enumerate(row) if p[3] > 127}


def _human(px):
    """(cell pixels, cell x from the enemy's centre, cell y from just below the feet, cell height)."""
    xs, ys = [x for x, _ in px], [y for _, y in px]
    ground = max(ys)
    # Centre on the legs (lower third): a weapon held out must not push the body sideways.
    legs = [x for x, y in px if y >= ground - (ground - min(ys)) // 3]
    cx = (min(legs) + max(legs) + 1) // 2
    if max(xs) - min(xs) + 1 > 48:   # a weapon held far out: its ends go, 24 px kept on each side
        px = {(x, y): c for (x, y), c in px.items() if cx - 24 <= x < cx + 24}
        xs = [x for x, _ in px]
    width = max(xs) - min(xs) + 1
    left = min(xs) - max(0, (32 - width) // 2)
    top = ground - 47
    return {(x - left, y - top): c for (x, y), c in px.items() if y >= top}, left - cx, -48, 48


def _floating(px):
    """Spirits keep their place on the canvas: the poses are drawn around the same point."""
    return px, -16, -32, 32


def _ground_small(px):
    xs, ground = [x for x, _ in px], max(y for _, y in px)
    cx = (min(xs) + max(xs) + 1) // 2
    return ({(x - cx + 16, y - ground + 31): c for (x, y), c in px.items()
             if 0 <= x - cx + 16 < 32 and y - ground + 31 >= 0}, -16, -32, 32)


def _parts(px, height):
    """[(x, y, w, h)] in tiles: the hardware sprites that cover the frame, and the engine's flags."""
    right = max(x for x, _ in px)
    strip = 0 if right < 32 else (right - 32) // 8 + 1   # tiles of the strip right of the 32 px block
    assert strip <= 2, "pose wider than 48 px"
    boxes = [(0, 0, 4, 4), (4, 0, strip, 4), (0, 4, 4, 2), (4, 4, strip, 2)]
    used, flags = [], 0
    for k, (bx, by, bw, bh) in enumerate(boxes):
        if by * 8 < height and any(bx * 8 <= x < (bx + bw) * 8 and by * 8 <= y < (by + bh) * 8 for x, y in px):
            used.append(boxes[k])
            flags |= 1 << k
    return used, flags | (0x10 if strip == 2 else 0)


def _kmeans(counts, free):
    """Refines the free slots: every colour goes to its nearest slot (fixed ones included), and each
    free slot moves to the mean of what it was given. Median cut alone ignores the fixed slots."""
    for _ in range(12):
        slots = FIXED[1:] + free
        acc = [[0, 0, 0, 0] for _ in free]
        for c, n in counts.items():
            i = min(range(len(slots)), key=lambda k: dist(slots[k], c)) - len(FIXED[1:])
            if i >= 0:
                for a in range(3):
                    acc[i][a] += c[a] * n
                acc[i][3] += n
        free = [tuple(round(a[k] / a[3]) for k in range(3)) if a[3] else free[i] for i, a in enumerate(acc)]
    return free


def build():
    """Returns (tile sheet 128 px wide, PAL2, PAL3 for the Void forms, C header)."""
    frames = [(n, _human(_load(p))) for n, p in HUMANS]
    frames += [(n, _floating(_load(p))) for n, p in FLOATING]
    frames += [(n, _ground_small(_load(p))) for n, p in GROUND_SMALL]

    counts = {}
    for _, (px, _, _, _) in frames:
        for c in px.values():
            counts[c] = counts.get(c, 0) + 1
    # Colours the fixed slots already give are left out of the first guess: the 7 free slots go to the rest.
    rest = {c: n for c, n in counts.items() if min(dist(c, f) for f in FIXED[1:]) > 3000}
    pal = FIXED + sorted(_kmeans(counts, median_cut(rest, 7)), key=sum) + [PROPS_ONLY, MAP_TEXT]
    cache = {}

    def index(c):
        if c not in cache:
            cache[c] = min(range(1, 14), key=lambda i: dist(pal[i], c))
        return cache[c]

    tiles, table, names = [], [], []
    for name, (px, dx, dy, height) in frames:
        parts, flags = _parts(px, height)
        names.append("EF_" + name.upper())
        table.append("    { %d, %d, %d, 0x%02X },   /* %s */" % (len(tiles), dx, dy, flags, name))
        for bx, by, bw, bh in parts:
            for tx in range(bx, bx + bw):
                for ty in range(by, by + bh):
                    tiles.append([[index(px[(tx * 8 + x, ty * 8 + y)]) if (tx * 8 + x, ty * 8 + y) in px else 0
                                   for x in range(8)] for y in range(8)])
    assert len(tiles) <= MAX_TILES, "enemies need %d tiles, video memory has %d" % (len(tiles), MAX_TILES)
    count = len(tiles)
    while len(tiles) % 16:
        tiles.append([[0] * 8 for _ in range(8)])
    sheet = [[c for t in tiles[r:r + 16] for c in t[y]] for r in range(0, len(tiles), 16) for y in range(8)]

    header = ("// Generated by tools/art/make_art.py from tools/art/enemies.py: do not edit.\n"
              "#ifndef ENEMY_FRAMES_H\n#define ENEMY_FRAMES_H\n\n"
              "enum\n{\n" + "".join("    %s,\n" % n for n in names) + "    EF_COUNT\n};\n\n"
              "#define ENEMY_TILE_COUNT %d\n\n" % count
              + "// First tile, cell x from the enemy's centre (facing right), cell y from just below its feet,\n"
              "// parts: bit 0 = 4x4 at (0, 0), 1 = strip x4 at (32, 0), 2 = 4x2 at (0, 32), 3 = strip x2 at (32, 32),\n"
              "// bit 4 = the strip is 2 tiles wide (1 otherwise). Tiles follow each other in that order.\n"
              "#define ENEMY_FRAME_TABLE \\\n{ \\\n" + "".join(t + " \\\n" for t in table) + "}\n\n#endif\n")
    return sheet, pal, void_palette(pal), header


def preview():
    """Every frame put back together from its parts, as the engine shows it: (sheet of 48 x 48 cells, PAL2)."""
    sheet, pal, _, header = build()
    rows = [line.split("{")[1].split("}")[0].split(",") for line in header.splitlines() if line.startswith("    { ")]
    out = [[0] * (48 * len(rows)) for _ in range(48)]
    for f, (first, _, _, flags) in enumerate(rows):
        t, flags = int(first), int(flags, 16)
        strip = 2 if flags & 0x10 else 1
        for k, (bx, by, bw, bh) in enumerate([(0, 0, 4, 4), (4, 0, strip, 4), (0, 4, 4, 2), (4, 4, strip, 2)]):
            if not flags & (1 << k):
                continue
            for tx in range(bx, bx + bw):
                for ty in range(by, by + bh):
                    for y in range(8):
                        for x in range(8):
                            out[ty * 8 + y][f * 48 + tx * 8 + x] = sheet[(t // 16) * 8 + y][(t % 16) * 8 + x]
                    t += 1
    return out, pal


def void_palette(pal):
    """PAL3: the same slots swallowed by the Void (violet body, cyan where it was red)."""
    out = [pal[0], (0, 0, 0), (0x88, 0x44, 0xCC), (0x44, 0x22, 0x88), (0x44, 0xEE, 0xEE)]
    for r, g, b in pal[5:14]:
        lum = (r * 3 + g * 4 + b * 2) // 9
        if r > 150 and r > g * 2 and r > b * 2:
            out.append((0x44, 0xEE, 0xEE))
        else:
            out.append((min(255, lum * 3 // 4 + 24), lum * 2 // 5, min(255, lum + 40)))
    return out + [(0x08, 0x04, 0x0E), (0x66, 0x44, 0x88)]


def remap_props(props_pal, pal):
    """Index table from the props' own palette (props.py) to the shared PAL2 (slots 1..14)."""
    return [0] + [min(range(1, 15), key=lambda i: dist(pal[i], props_pal[k])) for k in range(1, 16)]


# Floors further down (Thomas, 2026-11-18: as in Streets of Rage): the same enemies in other colours, and
# stronger. Slots 7..13 (the enemies' own colours) take a tint, each colour keeping its brightness:
# blood red from floor 3, tarnished gold from floor 5. The fixed slots (bone, red, gold) stay.
TIER_TINTS = [(0xE0, 0x30, 0x30), (0xE8, 0xB0, 0x30)]


def tier_palettes(pal):
    """Slots 7..13 of PAL2 for each tier after the first: [[(r, g, b) x 7] x 2]."""
    out = []
    for tint in TIER_TINTS:
        row = []
        for r, g, b in pal[7:14]:
            lum = (r * 3 + g * 4 + b * 2) / 9 / 255
            k = min(1.0, lum * 1.6 + 0.08)
            t = tuple(c * k for c in tint)
            row.append(tuple(min(255, round(o * 0.3 + n * 0.7)) for o, n in zip((r, g, b), t)))
        out.append(row)
    return out


def tier_header(pal):
    """inc/enemy_tiers.h: those colours as Mega Drive colours (0000 BBB0 GGG0 RRR0)."""
    def md(c):
        r, g, b = c
        return ((b >> 5) << 9) | ((g >> 5) << 5) | ((r >> 5) << 1)
    rows = ["{ %s }," % ", ".join("0x%03X" % md(c) for c in row) for row in tier_palettes(pal)]
    return ("// Generated by tools/art/make_art.py from tools/art/enemies.py: do not edit.\n"
            "#ifndef ENEMY_TIERS_H\n#define ENEMY_TIERS_H\n\n"
            "// PAL2 slots 7..13 of the enemies on deeper floors (tier 1, tier 2).\n"
            "#define ENEMY_TIER_TABLE { " + " ".join(rows) + " }\n\n#endif\n")

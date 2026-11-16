"""Imports the PixelLab knight (art_in/knight/<state>/<animation>/frame_NNN.png) into the hero sheet.

Two PixelLab states of the same character: "sheathed" (sword at the hip) for moving around, "sword"
(sword in hand) for the strikes. Every engine frame (ninja.ORDER) is mapped to one exported image;
the slash arcs still come from ninja.py (smear layer only), drawn behind the knight.

One 15-colour palette for the whole hero (PAL1). Reserved slots: 1 darkest (outline), 2-3 tunic blues
(tinted by the Void corruption, gfx.c), 5 red (HUD hearts), 15 light red (HUD text).
"""
import os

from pngread import read_png

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
SRC = os.path.join(ROOT, "art_in", "knight")
W, H, FEET, EXT = 32, 48, 46, 32
PW, PH = 96, 96   # preview cell: the body cell with its surroundings

# Engine frame -> (animation folder, image index[, "air"]).
# Default anchor: the animation's ground line goes on row FEET. "air": the image's own lowest pixel
# does (the engine moves the hero during a jump, the drawing must not rise a second time).
FRAMES = {
    # Running comes from PixelLab's run animation; strikes and jumps are key poses (one PixelLab
    # state each, same 48 x 48 canvas, so the knight keeps his size from pose to pose).
    "rise": ("poses/rise", 0, "air legs"),
    "fall": ("poses/fall", 0, "air legs"),
    "land": ("poses/land", 0, "legs"),
    "slash0": ("poses/windup_wide", 0, "feet"),
    "slash1": ("poses/strike_wide", 0, "legs"),
    "slash2": ("poses/strike_wide", 0, "legs"),
    "slash3": ("poses/strike_wide", 0, "legs"),
    "slash4": ("poses/windup_wide", 0, "feet"),
    "up0": ("poses/windup_wide", 0, "feet"),
    "up1": ("poses/up", 0, "legs"),
    "up2": ("poses/up", 0, "legs"),
    "air0": ("poses/windup_wide", 0, "air feet"),
    "air1": ("poses/strike_wide", 0, "air legs"),
    "air2": ("poses/strike_wide", 0, "air legs"),
    "air3": ("poses/tuck", 0, "air legs"),
    "down0": ("poses/tuck", 0, "air legs"),
    "down1": ("poses/down2", 0, "air"),
    "down2": ("poses/down2", 0, "air"),
    # PixelLab fills its canvas whatever the pose: its kneeling knight is as tall as the standing one.
    # The crouch comes from the crouch animation (drawn at the right size, head lowered); the low
    # strike is brought back to that height ("low": scaled around the feet).
    "crouch": ("sheathed/crouch", 4, "air"),
    "crouch_slash0": ("sheathed/crouch", 4, "air"),
    "crouch_slash1": ("poses/crouch_strike", 0, "legs low"),
    "hurt": ("sheathed/hurt", 2),
}
FRAMES.update({"idle%d" % i: ("sheathed/idle", i) for i in range(4)})
LOW_SCALE = 84           # percent: the low strike at the height of the crouch (38 px for 45)
BREATH_ROW = FEET - 17   # belt line: everything above it moves ("breathe" flag)
FRAMES.update({"run%d" % i: ("sheathed/run", i) for i in range(8)})
FRAMES.update({"flip%d" % i: ("poses/tuck", 0, "air legs") for i in range(4)})

# Blades cut by the 48 px canvas, continued here (source image coordinates).
#   ("copy", x, y0, y1, dx, dy, length): repeat column x (rows y0..y1) along (dx, dy), tapering at the tip.
#   ("up", x0, y, length): a vertical blade above row y, columns x0..x0+3 (outline, light, mid, outline).
#   ("down", x0, y, length): the same below row y (outline, mid, light, outline, as the plunging pose is lit).
BLADES = {
    "poses/crouch_strike": ("copy", 47, 29, 32, 1, 0.34, 10),
    "poses/up": ("up", 26, 0, 18),
    "poses/down2": ("down", 31, 47, 8),
}
STEEL = [(0x05, 0x05, 0x0A), (0xC9, 0xDC, 0xE1), (0x80, 0x94, 0xA4), (0x05, 0x05, 0x0A)]


def _blade(path, fr):
    """Extra {(x, y): rgb} pixels in source coordinates for a blade cut by the canvas."""
    spec = BLADES.get(path)
    if not spec:
        return {}
    out = {}
    if spec[0] == "copy":
        _, x, y0, y1, dx, dy, length = spec
        col = [fr[y][x] for y in range(y0, y1 + 1)]
        for k in range(1, length + 1):
            trim = max(0, k - (length - 2))   # the last pixels narrow to a point
            for i, p in enumerate(col):
                if p[3] > 127 and trim <= i < len(col) - trim:
                    out[(x + k * dx, y0 + i + int(round(k * dy)))] = p[:3]
    elif spec[0] == "right":   # horizontal blade from column x, rows y0..y0+3
        _, x, y0, length = spec
        for k in range(length):
            trim = max(0, k - (length - 3))
            for i, c in enumerate(STEEL):
                if trim <= i < len(STEEL) - trim:
                    out[(x + k, y0 + i)] = c
    elif spec[0] == "down":
        _, x0, y, length = spec
        steel = [STEEL[0], STEEL[2], STEEL[1], STEEL[3]]
        for k in range(1, length + 1):
            trim = max(0, k - (length - 2))
            for i, c in enumerate(steel):
                if trim <= i < len(steel) - trim:
                    out[(x0 + i, y + k)] = c
    else:
        _, x0, y, length = spec
        for k in range(1, length + 1):
            trim = max(0, k - (length - 2))
            for i, c in enumerate(STEEL):
                if trim <= i < len(STEEL) - trim:
                    out[(x0 + i, y - k)] = c
    return out
TEXT_RED = (0xEE, 0x44, 0x44)

_anims = {}


def available():
    return os.path.isdir(os.path.join(SRC, "sheathed", "idle"))


def _anim(path):
    """([frames as rows of RGBA], centre column, ground row) of one exported animation."""
    if path not in _anims:
        d = os.path.join(SRC, *path.split("/"))
        frames = [read_png(os.path.join(d, f))[0] for f in sorted(os.listdir(d)) if f.endswith(".png")]
        ground = max(y for fr in frames for y, r in enumerate(fr) if any(p[3] > 127 for p in r))
        # PixelLab centres the character on its canvas (80 or 64 px wide depending on the animation).
        _anims[path] = (frames, len(frames[0][0]) // 2, ground)
    return _anims[path]


def _pixels(name):
    """{(x, y): rgb} of one engine frame, in body-cell coordinates (centre x 16, soles row FEET)."""
    spec = FRAMES[name]
    frames, cx, ground = _anim(spec[0])
    fr = frames[spec[1]]
    flags = spec[2] if len(spec) > 2 else ""
    if "air" in flags:
        ground = max(y for y, r in enumerate(fr) if any(p[3] > 127 for p in r))
    if "legs" in flags or "feet" in flags:
        # A rotation image is not centred on its canvas: centre it on the legs ("feet": on the boots
        # alone, when a cape hangs as low as the knees).
        rows = 12 if "legs" in flags else 6
        xs = [x for r in fr[ground - rows:ground + 1] for x, p in enumerate(r) if p[3] > 127]
        cx = (min(xs) + max(xs) + 1) // 2
    src = {(x, y): p[:3] for y, r in enumerate(fr) for x, p in enumerate(r) if p[3] > 127}
    src.update(_blade(spec[0], fr))
    px = {(x - cx + W // 2, y - ground + FEET): c for (x, y), c in src.items()}
    if "low" in flags:
        px = _scaled(px, LOW_SCALE)
    if "breathe" in flags:   # chest and head sink by one pixel, the legs stay planted
        px = {(x, y + 1 if y < BREATH_ROW else y): c for (x, y), c in sorted(px.items(), key=lambda e: -e[0][1])
              if y != BREATH_ROW - 1 or (x, BREATH_ROW) not in px}
    return px


def _scaled(px, percent):
    """Nearest-pixel reduction around the point between the feet."""
    out = {}
    for ty in range(-H, H):
        for tx in range(-2 * W, 3 * W):
            src = (W // 2 + round((tx - W // 2) * 100 / percent), FEET - round((FEET - ty) * 100 / percent))
            if src in px:
                out[(tx, ty)] = px[src]
    return out


def dist(a, b):
    return (a[0] - b[0]) ** 2 * 3 + (a[1] - b[1]) ** 2 * 4 + (a[2] - b[2]) ** 2 * 2


def median_cut(counts, n):
    boxes = [list(counts.items())]
    while len(boxes) < n:
        best, axis, spread = None, 0, -1
        for i, box in enumerate(boxes):
            if len(box) < 2:
                continue
            for a in range(3):
                vals = [c[a] for c, _ in box]
                s = max(vals) - min(vals)
                if s > spread:
                    best, axis, spread = i, a, s
        if best is None:
            break
        box = sorted(boxes.pop(best), key=lambda e: e[0][axis])
        total, acc, cut = sum(k for _, k in box), 0, 1
        for j, (_, k) in enumerate(box):
            acc += k
            if acc >= total / 2:
                cut = max(1, min(len(box) - 1, j + 1))
                break
        boxes += [box[:cut], box[cut:]]
    pal = []
    for box in boxes:
        t = sum(k for _, k in box)
        pal.append(tuple(round(sum(c[a] * k for c, k in box) / t) for a in range(3)))
    return pal


def palette(frames):
    counts = {}
    for px in frames.values():
        for c in px.values():
            counts[c] = counts.get(c, 0) + 1
    pal = median_cut(counts, 13)
    pal.sort(key=lambda c: sum(c))
    darkest = pal.pop(0)
    red = max(pal, key=lambda c: c[0] - (c[1] + c[2]) / 2)
    pal.remove(red)
    blues = sorted(pal, key=lambda c: c[2] - c[0], reverse=True)[:2]
    for b in blues:
        pal.remove(b)
    blues.sort(key=lambda c: sum(c))
    rest = pal
    # Slots: 0 transparent, 1 darkest, 2-3 blues, 4 rest[0], 5 red, 6.. rest, 15 text red.
    out = [(0x10, 0x08, 0x10), darkest, blues[0], blues[1], rest[0], red] + rest[1:]
    while len(out) < 15:
        out.append((0, 0, 0))
    return out[:15] + [TEXT_RED]


def nearest(pal, c):
    return min(range(1, 15), key=lambda i: dist(pal[i], c))


def _auto_cell(lost):
    """Where to put one more 32 x 32 extension cell: the position (8 px grid, corners included, so a
    blade tip past a corner is kept) that covers the most pixels still lost."""
    best, where = -1, (W, 0)
    for ex in range(-48, W + 17, 8):
        for ey in range(-40, H, 8):
            n = sum(1 for x, y in lost if ex <= x < ex + EXT and ey <= y < ey + EXT)
            if n > best:
                best, where = n, (ex, ey)
    return where


def _auto_pair(lost):
    """The two extension cells that together cover the most lost pixels."""
    spots = [(ex, ey) for ex in range(-48, W + 17, 8) for ey in range(-40, H, 8)]
    covers = [frozenset(q for q in lost if ex <= q[0] < ex + EXT and ey <= q[1] < ey + EXT) for ex, ey in spots]
    spots = [s for s, c in zip(spots, covers) if c]
    covers = [c for c in covers if c]
    best, pair = -1, [spots[0], spots[0]]
    for i in range(len(spots)):
        for j in range(i, len(spots)):
            n = len(covers[i] | covers[j])
            if n > best:
                best, pair = n, [spots[i], spots[j]]
    return pair


def build(ninja):
    """Every engine frame as indexed pixels: {name: (body cell, [extension cells], [their offsets],
    lost pixel count, {(x, y): index})}, plus the palette."""
    rgb = {name: _pixels(name) for name in ninja.ORDER}
    pal = palette(rgb)
    arc_color = [0] + [nearest(pal, ninja.PAL1[i]) for i in range(1, 16)]
    cache, out = {}, {}
    for name in ninja.ORDER:
        px = {}
        for q, c in rgb[name].items():
            if c not in cache:
                cache[c] = nearest(pal, c)
            px[q] = cache[c]
        # No more code-drawn slash arcs: the side-view PixelLab strikes show the blade themselves.
        offsets = []

        def lost():
            return [q for q in px if not (0 <= q[0] < W and 0 <= q[1] < H)
                    and not any(ex <= q[0] < ex + EXT and ey <= q[1] < ey + EXT for ex, ey in offsets)]

        # The engine draws at most two extension sprites per frame. One cell where it covers the most;
        # if that is not enough, the best pair of cells (one after the other can leave two remnants).
        if len(lost()) > 2:
            offsets.append(_auto_cell(lost()))
            if len(lost()) > 2:
                offsets[:] = _auto_pair([q for q in px if not (0 <= q[0] < W and 0 <= q[1] < H)])
        body = [[px.get((x, y), 0) for x in range(W)] for y in range(H)]
        cells = [[[px.get((ex + x, ey + y), 0) for x in range(EXT)] for y in range(EXT)] for ex, ey in offsets]
        out[name] = (body, cells, offsets, len(lost()), px)
    return out, pal


def apply(ninja):
    """Returns (body sheet, extension sheet, palette, {frame: [(x, y)] extension offsets}, {frame: lost})."""
    frames, pal = build(ninja)
    bodies = [frames[n][0] for n in ninja.ORDER]
    cells = [c for n in ninja.ORDER for c in frames[n][1]] or [[[0] * EXT for _ in range(EXT)]]
    return (ninja.side_by_side(bodies, W), ninja.side_by_side(cells, EXT), pal,
            {n: frames[n][2] for n in ninja.ORDER}, {n: frames[n][3] for n in ninja.ORDER if frames[n][3]})


def preview_sheet(ninja):
    """Every frame as seen in game: what the body cell and the extension cells really show."""
    frames, pal = build(ninja)
    cells = []
    for name in ninja.ORDER:
        body, exts, offsets, _, _ = frames[name]
        cell = [[0] * PW for _ in range(PH)]
        for (ox, oy), src in [((0, 0), body)] + list(zip(offsets, exts)):
            for y, row in enumerate(src):
                for x, c in enumerate(row):
                    if c and 0 <= x + ox + 32 < PW and 0 <= y + oy + 40 < PH:
                        cell[y + oy + 40][x + ox + 32] = c
        cells.append(cell)
    return ninja.side_by_side(cells, PW), pal

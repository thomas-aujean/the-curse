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
    "rise": ("sheathed/jump", 3, "air"),
    "fall": ("sheathed/jump", 5, "air"),
    "land": ("sheathed/jump", 7),
    "slash0": ("sword/slash", 1),
    "slash1": ("sword/slash", 2),
    "slash2": ("sword/slash", 2),
    "slash3": ("sword/slash", 4),
    "slash4": ("sword/slash", 5),
    "up0": ("sword/up", 0),
    "up1": ("sword/up", 1),
    "up2": ("sword/up", 2),
    "air0": ("sword/jump", 3, "air"),
    "air1": ("sword/down", 1, "air"),
    "air2": ("sword/down", 2, "air"),
    "air3": ("sword/jump", 5, "air"),
    "down0": ("sword/jump", 4, "air"),
    "down1": ("sword/down", 1, "air"),
    "down2": ("sword/down", 2, "air"),
    "crouch": ("sheathed/jump", 2),
    "crouch_slash0": ("sword/jump", 2),
    "crouch_slash1": ("sword/jump", 2),
    "hurt": ("sheathed/hurt", 2),
}
# At rest: the calm standing image, breathing made by sinking the upper body one pixel (PixelLab's
# fight-stance idle looked like boxing, Thomas 2026-11-16).
FRAMES.update({"idle%d" % i: ("sheathed/stand", 0, "legs breathe" if i in (1, 2) else "legs") for i in range(4)})
BREATH_ROW = FEET - 17   # belt line: everything above it moves
FRAMES.update({"run%d" % i: ("sheathed/run", i) for i in range(8)})
# No somersault yet (the PixelLab front-flip job failed): the tucked jump image holds the peak.
FRAMES.update({"flip%d" % i: ("sheathed/jump", 4, "air") for i in range(4)})
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
    if "legs" in flags:   # a rotation image is not centred on its canvas: centre it on the legs
        xs = [x for r in fr[ground - 12:ground + 1] for x, p in enumerate(r) if p[3] > 127]
        cx = (min(xs) + max(xs) + 1) // 2
    px = {(x - cx + W // 2, y - ground + FEET): p[:3]
          for y, r in enumerate(fr) for x, p in enumerate(r) if p[3] > 127}
    if "breathe" in flags:   # chest and head sink by one pixel, the legs stay planted
        px = {(x, y + 1 if y < BREATH_ROW else y): c for (x, y), c in sorted(px.items(), key=lambda e: -e[0][1])
              if y != BREATH_ROW - 1 or (x, BREATH_ROW) not in px}
    return px


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
    """Where to put one more 32 x 32 extension cell: on the side that loses the most pixels."""
    groups = {"left": [q for q in lost if q[0] < 0], "right": [q for q in lost if q[0] >= W],
              "top": [q for q in lost if 0 <= q[0] < W and q[1] < 0]}
    side = max(groups, key=lambda k: len(groups[k]))
    top = min(y for _, y in groups[side])
    return {"left": (-EXT, top), "right": (W, top), "top": (0, -EXT)}[side]


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
        pose = ninja.POSES[name]
        offsets = [tuple(o) for o in pose.get("ext", [])]
        if offsets:   # frames with a slash arc: ninja.py's smear layer, behind the knight
            for q, c in ninja.render(pose)[1].items():
                px.setdefault(q, arc_color[c])

        def lost():
            return [q for q in px if not (0 <= q[0] < W and 0 <= q[1] < H)
                    and not any(ex <= q[0] < ex + EXT and ey <= q[1] < ey + EXT for ex, ey in offsets)]

        # The engine draws at most two extension sprites per frame.
        while len(offsets) < 2 and len(lost()) > 2:
            offsets.append(_auto_cell(lost()))
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

"""Imports the PixelLab knight (art_in/knight/*.gif) into the hero sheet.

The knight's animations replace the matching frames of the generated sheet (ninja.py); frames not yet
drawn in PixelLab keep the generated pose, recoloured with the knight's palette so the sheet shares
one 15-colour palette (PAL1). Reserved PAL1 slots: 1 darkest (outline), 2-3 tunic blues (tinted by the
Void corruption, gfx.c), 5 red (HUD hearts), 15 light red (HUD text).
"""
import os

from gif import read_gif

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
SRC = os.path.join(ROOT, "art_in", "knight")
W, H, FEET = 32, 48, 46

# Sheet frame name prefix -> (gif file, frames to keep from it).
ANIMS = {
    "idle": ("idle.gif", [0, 2, 4, 6]),
    "run": ("run.gif", list(range(8))),
}
TEXT_RED = (0xEE, 0x44, 0x44)


def available():
    return any(os.path.exists(os.path.join(SRC, f)) for f, _ in ANIMS.values())


def _dist(a, b):
    return (a[0] - b[0]) ** 2 * 3 + (a[1] - b[1]) ** 2 * 4 + (a[2] - b[2]) ** 2 * 2


def _median_cut(counts, n):
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


def _crop(frames, keep, cx, bottom):
    """Per frame: (32 x 48 body cell, [(32 x 32 extension cell, x, y)]). Pixels left of the body cell
    (the scarf trailing behind) go into one extension cell placed against its left edge."""
    out = []
    for i in keep:
        fr = frames[i]

        def px(x, y):   # pixel at body-cell coordinates, None if empty
            sy, sx = y + bottom - FEET, x + cx - W // 2
            if 0 <= sy < len(fr) and 0 <= sx < len(fr[0]) and fr[sy][sx][3] > 127:
                return fr[sy][sx][:3]
            return None

        cell = [[px(x, y) for x in range(W)] for y in range(H)]
        left = [(x, y) for y in range(H) for x in range(-32, 0) if px(x, y)]
        exts = []
        if left:
            top = max(0, min(min(y for _, y in left), H - 32))
            exts.append(([[px(x - 32, y + top) for x in range(32)] for y in range(32)], -32, top))
        out.append((cell, exts))
    return out


def load():
    """Returns {frame name: 32 x 48 cell of RGB or None}, already aligned (centre x 16, soles row 46)."""
    gifs = {k: read_gif(os.path.join(SRC, f))[0] for k, (f, _) in ANIMS.items()
            if os.path.exists(os.path.join(SRC, f))}
    ref = gifs.get("idle") or next(iter(gifs.values()))
    # Body centre: middle of the legs (lower third) of the first idle frame; soles: lowest pixel.
    rows = [r for r in ref[0] if any(p[3] for p in r)]
    legs = rows[len(rows) * 2 // 3:]
    xs = [x for r in legs for x, p in enumerate(r) if p[3] > 127]
    cx = (min(xs) + max(xs) + 1) // 2
    cells = {}
    for name, frames in gifs.items():
        bottom = max(y for fr in frames for y, r in enumerate(fr) if any(p[3] > 127 for p in r))
        for k, cell in enumerate(_crop(frames, ANIMS[name][1], cx, bottom)):
            cells["%s%d" % (name, k)] = cell
    return cells


def palette(cells):
    counts = {}
    for body, exts in cells.values():
        for cell in [body] + [e[0] for e in exts]:
            for row in cell:
                for c in row:
                    if c:
                        counts[c] = counts.get(c, 0) + 1
    pal = _median_cut(counts, 13)
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
    return min(range(1, 16), key=lambda i: _dist(pal[i], c))


def apply(ninja):
    """Builds the hero sheets from the generated ninja (ninja.py) with the knight's frames swapped in.
    Returns (body sheet, extension sheet, palette, {frame: [(x, y)] extension offsets}, knight frames)."""
    cells = load()
    pal = palette(cells)
    remap = [0] + [nearest(pal, ninja.PAL1[i]) for i in range(1, 16)]

    def index(cell):
        return [[nearest(pal, c) if c else 0 for c in row] for row in cell]

    bodies, ext_cells, offsets = [], [], {}
    for name in ninja.ORDER:
        if name in cells:
            body, exts = cells[name]
            bodies.append(index(body))
            ext_cells += [index(e[0]) for e in exts]
            offsets[name] = [(e[1], e[2]) for e in exts]
        else:
            body, exts = ninja.frame(name)[:2]
            bodies.append([[remap[v] for v in row] for row in body])
            ext_cells += [[[remap[v] for v in row] for row in e] for e in exts]
            offsets[name] = [tuple(o[:2]) for o in ninja.POSES[name].get("ext", [])]
    if not ext_cells:
        ext_cells = [[[0] * 32 for _ in range(32)]]
    return (ninja.side_by_side(bodies, W), ninja.side_by_side(ext_cells, 32), pal, offsets,
            sorted(n for n in cells if n in ninja.ORDER))

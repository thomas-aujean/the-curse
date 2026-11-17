"""The guard's spear crescents (Thomas, 2026-11-17: "a powerful blow in two frames: the spear behind
the back, then a slash moving forward; he can also strike upwards; the same kind of arc"): the
knight's crescent (knight.crescent), in the enemies' colours (PAL2: 2 pale core, 4 red rim), so the
danger reads at a glance. Two drawings, each two hardware sprites:

  side: 32 x 48 (4 x 4 then 4 x 2), in front of the guard: cell x from its centre +2, y from its feet -50
  up:   48 x 32 (4 x 4 then 2 x 4), over its head: cell x from its centre -24, y from its feet -84

Sheet: the side drawing in columns of 6 tiles (4 columns), then the up drawing (6 columns of 4 tiles),
as rescomp's COLUMN order gives them; main.c (drawEnemySlash) cuts them into sprites.
"""
from knight import crescent

CORE, RIM = 2, 4
SIDE_W, SIDE_H, SIDE_X, SIDE_Y = 32, 48, 2, -50
UP_W, UP_H, UP_X, UP_Y = 48, 32, -24, -84
# Crescents in the guard's coordinates (centre x 0, feet y 0): (cx, cy, radius, start, end, stage).
SIDE = (0, -25, 26, 160, 20, 0)
UP = (0, -56, 26, 110, 250, 0)


def _cell(spec, x0, y0, w, h):
    cx, cy = spec[0] - x0, spec[1] - y0
    img = [[0] * w for _ in range(h)]
    for (x, y), c in crescent((cx, cy) + spec[2:]).items():
        if 0 <= x < w and 0 <= y < h:
            img[y][x] = CORE if c[0] > 0xC0 else RIM
    return img


def sheet():
    """48 rows: the side drawing (32 wide) then the up drawing (48 wide, its 32 rows at the top)."""
    side = _cell(SIDE, SIDE_X, SIDE_Y, SIDE_W, SIDE_H)
    up = _cell(UP, UP_X, UP_Y, UP_W, UP_H)
    rows = [[0] * (SIDE_W + UP_W) for _ in range(48)]
    for y in range(SIDE_H):
        rows[y][:SIDE_W] = side[y]
    for y in range(UP_H):
        rows[y][SIDE_W:] = up[y]
    return rows

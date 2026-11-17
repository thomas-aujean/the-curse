"""The archer's arrow (Thomas, 2026-11-17: "their arrow is too small"; it now flies in an arc), drawn
here: 16 x 16, pointing right and up at 0, 22.5, 45, 67.5 and 90 degrees. The engine picks the one
nearest the arrow's flight and mirrors it for the other directions (main.c, arrowFrame).
Colours: PAL2 slots (enemies.py): 1 outline, 3 shaft, 12 head, 4 fletching.
"""
import math

ANGLES = [0, 22.5, 45, 67.5, 90]
CELL = 16


def _frame(deg):
    a = math.radians(deg)
    dx, dy = math.cos(a), -math.sin(a)          # screen y goes down
    px, py = -dy, dx                            # perpendicular
    c = 7.5
    img = {}

    def put(x, y, col, over=True):
        x, y = int(round(x)), int(round(y))
        if 0 <= x < CELL and 0 <= y < CELL and (over or (x, y) not in img):
            img[(x, y)] = col
    for i in range(-11, 12):                    # shaft, 12 px
        t = i / 2.0
        put(c + t * dx, c + t * dy, 3)
    for i in range(7, 13):                      # head: 3 px, two barbs
        t = i / 2.0
        put(c + t * dx, c + t * dy, 12)
    for s in (-1, 1):
        put(c + 4 * dx + s * px, c + 4 * dy + s * py, 12)
    for i in range(-12, -7):                    # fletching, on both sides
        t = i / 2.0
        for s in (-1, 1):
            put(c + (t - 0.5) * dx + s * px, c + (t - 0.5) * dy + s * py, 4)
    outline = {}
    for (x, y) in img:
        for ox, oy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            q = (x + ox, y + oy)
            if q not in img and 0 <= q[0] < CELL and 0 <= q[1] < CELL:
                outline[q] = 1
    img.update(outline)
    return img


def sheet():
    """Indexed sheet: CELL rows, CELL * len(ANGLES) columns."""
    rows = [[0] * (CELL * len(ANGLES)) for _ in range(CELL)]
    for k, deg in enumerate(ANGLES):
        for (x, y), col in _frame(deg).items():
            rows[y][k * CELL + x] = col
    return rows

"""Generates The Curse's zone-1 graphics as indexed PNGs in res/ (see docs/style.md).

    wsl python3 tools/art/make_art.py

Placeholder-quality but coherent pixel art, produced by code so it can be iterated quickly.
Any PNG here can later be replaced by hand-drawn art that follows the same palette and layout.
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from png import write_indexed  # noqa: E402
import ninja  # noqa: E402

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
RES = os.path.join(ROOT, "res")

# PAL0, zone 1 at dusk (docs/style.md). Index 0 = transparent / backdrop.
PAL0 = [
    (0x1A, 0x0F, 0x2A),  # 0  backdrop
    (0x14, 0x0A, 0x14),  # 1  outline
    (0x3A, 0x22, 0x30),  # 2  stone shadow
    (0x6A, 0x3A, 0x3A),  # 3  stone dark
    (0x9A, 0x5A, 0x48),  # 4  stone mid
    (0xC8, 0x8A, 0x5A),  # 5  stone light
    (0xF0, 0xC0, 0x80),  # 6  sunset rim
    (0x4A, 0x2A, 0x1A),  # 7  wood dark
    (0x8A, 0x54, 0x28),  # 8  wood
    (0x4A, 0x6A, 0x3A),  # 9  moss
    (0x2E, 0x1A, 0x44),  # 10 sky dark violet
    (0x5A, 0x2A, 0x5A),  # 11 sky purple
    (0xA0, 0x40, 0x5A),  # 12 sky magenta
    (0xE0, 0x70, 0x40),  # 13 sky orange
    (0xF8, 0xB0, 0x60),  # 14 sun glow
    (0xEE, 0xEE, 0xEE),  # 15 text
]


def h(*v):
    """Small deterministic hash -> 0..255 (stable art from run to run)."""
    x = 2166136261
    for n in v:
        x = ((x ^ (n & 0xFFFFFFFF)) * 16777619) & 0xFFFFFFFF
    x ^= x >> 13
    return x & 0xFF


# ---------------------------------------------------------------- castle tiles

def brick_pixel(gx, gy):
    """Bricks 16 x 8 px, every other row shifted by 8: lit top, dark bottom, mortar lines."""
    row = gy // 8
    sx = gx + (row % 2) * 8
    bx, x, y = sx // 16, sx % 16, gy % 8
    brick = h(bx, row)
    if y == 7 or x == 0:
        return 1 if (x + y) % 3 else 2          # mortar, a little broken
    if y == 0:
        return 5 if brick % 3 else 4             # lit upper edge
    if y == 6:
        return 2                                 # shadow under the brick
    tone = 4 if brick % 4 else 3
    speck = h(gx, gy, 7)
    if speck < 18:
        return 3 if tone == 4 else 2             # pits and chips
    if speck > 245:
        return 5
    return tone


def stone_tile(parity, mask):
    """mask bits: 1 top open, 2 bottom open, 4 left open, 8 right open (edges facing air)."""
    px = []
    for y in range(8):
        row = []
        for x in range(8):
            gx, gy = parity * 8 + x, y
            c = brick_pixel(gx, gy)
            if mask & 1:                          # sunlit top edge, with moss tufts
                if y == 0:
                    c = 9 if h(gx, 11, parity) < 70 else 6
                elif y == 1:
                    c = 9 if h(gx, 12, parity) < 40 else 5
            if mask & 2:                          # underside in shadow
                if y == 7:
                    c = 1
                elif y == 6:
                    c = 2
            if mask & 4:                          # left face catches the low sun
                if x == 0:
                    c = 1 if (mask & 2 and y == 7) else 5
            if mask & 8:                          # right face in shade
                if x == 7:
                    c = 1
                elif x == 6 and c > 2:
                    c = 2
            row.append(c)
        px.append(row)
    return px


def cracked_tile(parity):
    t = stone_tile(parity, 0)
    # A thin dark crack running down the brick: "slightly different", learnable.
    for y, x in enumerate([3, 3, 4, 4, 3, 4, 5, 5]):
        t[y][x] = 1
    t[2][5] = 2
    t[5][2] = 2
    return t


def plank_tile(left_end, right_end):
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


def castle_tileset():
    """Tile order is the engine's contract (gfx.h): 0 empty, 1..32 stone [parity*16 + mask],
    33..34 cracked [parity], 35 plank alone, 36 left end, 37 middle, 38 right end."""
    tiles = [[[0] * 8 for _ in range(8)]]
    for parity in (0, 1):
        for mask in range(16):
            tiles.append(stone_tile(parity, mask))
    tiles += [cracked_tile(0), cracked_tile(1)]
    tiles += [plank_tile(True, True), plank_tile(True, False), plank_tile(False, False), plank_tile(False, True)]
    while len(tiles) % 16:
        tiles.append([[0] * 8 for _ in range(8)])
    width = 16
    img = [[0] * (width * 8) for _ in range(len(tiles) // width * 8)]
    for i, t in enumerate(tiles):
        ox, oy = (i % width) * 8, (i // width) * 8
        for y in range(8):
            for x in range(8):
                img[oy + y][ox + x] = t[y][x]
    return img, 39


# ---------------------------------------------------------------- dusk sky (plane B)

BAYER = [[0, 2], [3, 1]]


def sky_color(x, y, height):
    """Vertical gradient violet -> magenta -> orange with 2x2 ordered dithering between bands."""
    bands = [10, 11, 12, 13, 14]
    t = min(1.0, max(0.0, (y - 10) / (height * 0.80)))
    pos = t * (len(bands) - 1)
    i = int(pos)
    frac = pos - i
    if i >= len(bands) - 1:
        return bands[-1]
    return bands[i + 1] if frac * 4 > BAYER[y % 2][x % 2] + 0.5 else bands[i]


def far_background():
    W, H = 512, 256
    horizon = 200
    img = [[sky_color(x, y, horizon) for x in range(W)] for y in range(H)]

    # Low sun sitting on the horizon (it repeats every 512 px: one per screen-and-a-half).
    sx, sy, r = 352, horizon - 58, 15
    for y in range(sy - r - 3, sy + r + 3):
        for x in range(sx - r - 3, sx + r + 3):
            d = math.hypot(x - sx, y - sy)
            if d <= r:
                img[y][x] = 14
            elif d <= r + 2 and (x + y) % 2 == 0:
                img[y][x] = 14

    # Long thin clouds catching the light (period 256 px to save tiles).
    for k in range(6):
        cy = 70 + k * 18 + h(k, 1) % 10
        cx = h(k, 2) * 2 % 256
        length = 40 + h(k, 3) % 60
        color = 12 if cy < 120 else 13
        for rep in (0, 256):
            for dx in range(length):
                x = (cx + dx + rep) % W
                mid = min(dx, length - dx)
                thick = 1 + (mid > 6) + (mid > 16)
                for dy in range(thick):
                    img[cy + dy][x] = 14 if dy == 0 and mid > 3 else color
                img[cy + thick][x] = 11 if color == 12 else 12    # shaded underside

    # Far hills (purple), then the castle ramparts in silhouette, both 256-px periodic.
    for x in range(W):
        hill = horizon - 8 + int(6 * math.sin(x * 2 * math.pi / 256) + 4 * math.sin(x * 2 * math.pi / 128 + 1))
        for y in range(hill, H):
            img[y][x] = 11
    for x in range(W):
        p = x % 256
        top = horizon + 4
        # Two towers and a gatehouse per period, crenellated walls between.
        for tx0, tw, th in ((24, 22, 70), (120, 34, 52), (196, 18, 84)):
            if tx0 <= p < tx0 + tw:
                top = horizon - th
                if p - tx0 < 3 or tx0 + tw - p <= 3:
                    pass
                elif (p - tx0) // 4 % 2 == 0 and False:
                    pass
        if top == horizon + 4 and (p // 6) % 2 == 0:
            top = horizon - 2                   # crenels
        for y in range(max(0, top), H):
            img[y][x] = 2 if y > top else 1
        # Tower crenellations.
        for tx0, tw, th in ((24, 22, 70), (120, 34, 52), (196, 18, 84)):
            if tx0 <= p < tx0 + tw and (p - tx0) // 4 % 2 == 1:
                for y in range(horizon - th - 5, horizon - th):
                    img[y][x] = 1
    # Lit windows in the towers.
    for tx0, tw, th in ((24, 22, 70), (120, 34, 52), (196, 18, 84)):
        for wy in range(horizon - th + 14, horizon - 6, 18):
            wx = tx0 + tw // 2 - 1
            for rep in (0, 256):
                for y in range(wy, wy + 5):
                    for x in (wx + rep, wx + 1 + rep):
                        img[y][x] = 13 if y > wy else 14
    return img


def main():
    os.makedirs(RES, exist_ok=True)
    tiles, count = castle_tileset()
    write_indexed(os.path.join(RES, "castle.png"), tiles, PAL0)
    write_indexed(os.path.join(RES, "far.png"), far_background(), PAL0)
    write_indexed(os.path.join(RES, "ninja.png"), ninja.sheet(), ninja.PAL1)
    print("castle.png: %d tiles, far.png: 512x256, ninja.png: %d frames" % (count, len(ninja.ORDER)))


if __name__ == "__main__":
    main()

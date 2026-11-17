"""Breakable props, 16 x 16 each, bottom on row 15 (PAL2).

PAL2: 1 outline, 2 bone, 3 bone shadow, 4 red, 5 gold, 6 gold shadow (coins), 7 wood, 8 wood shadow,
9 silver, 10 silver shadow, 11 flame, 12 flame orange, 13 clay, 14 clay shadow, 15 map text.
Characters: '.' empty, '0'-'9' and 'a'-'e' = indices 10-14.
"""

PAL2 = [
    (0x10, 0x08, 0x10), (0x00, 0x00, 0x00), (0xCC, 0xCC, 0xAA), (0x88, 0x88, 0x66), (0xEE, 0x22, 0x22),
    (0xEE, 0xBB, 0x22), (0xAA, 0x66, 0x00), (0x8A, 0x54, 0x28), (0x4A, 0x2A, 0x1A), (0xBB, 0xBB, 0xCC),
    (0x66, 0x66, 0x77), (0xF8, 0xD0, 0x40), (0xE0, 0x70, 0x20), (0xB0, 0x60, 0x40), (0x70, 0x30, 0x20),
    (0xEE, 0xEE, 0x44),
]

JAR = [
    "................",
    "................",
    "................",
    "................",
    ".....111111.....",
    "......1dd1......",
    ".....1ddde1.....",
    "....1dddddde1...",
    "...1dd5d5d5de1..",
    "...1dddddddde1..",
    "...1ddddddddee1.",
    "...1dddddddde1..",
    "....1ddddddee1..",
    ".....1eeeeee1...",
    "......111111....",
    "................",
]

URN = [
    "................",
    "......1111......",
    ".....122221.....",
    "......1221......",
    ".....123321.....",
    "....12222231....",
    "...1222222231...",
    "...1244444431...",
    "...1222222231...",
    "...1222222331...",
    "....12222331....",
    ".....122331.....",
    "......1231......",
    ".....122331.....",
    "....11111111....",
    "................",
]

CRATE = [
    "................",
    "................",
    "................",
    "..111111111111..",
    "..177777777781..",
    "..1788888888a1..",
    "..17877777878a1.",
    "..1778777787781.",
    "..1777877877781.",
    "..1777787777781.",
    "..1777877877781.",
    "..1778777787781.",
    "..17877777787a1.",
    "..1788888888a1..",
    "..111111111111..",
    "................",
]

CANDLE = [
    ".......b........",
    "......bcb.......",
    "......bcb.......",
    ".......1........",
    "......121.......",
    "......121.......",
    "......121.......",
    "......121.......",
    "......131.......",
    ".....19a91......",
    "......1a1.......",
    "......1a1.......",
    "......1a1.......",
    ".....19aa91.....",
    "....1111111.....",
    "................",
]


ORDER = ["jar", "urn", "crate", "candle"]   # chests: chests.py (PixelLab)

ART = {"jar": JAR, "urn": URN, "crate": CRATE, "candle": CANDLE}


def pixel(ch):
    if ch == ".":
        return 0
    if ch.isdigit():
        return int(ch)
    return 10 + "abcde".index(ch)


def sheet():
    """Every prop is moved down so its lowest pixel is on row 15: it stands on the floor."""
    img = [[0] * (16 * len(ORDER)) for _ in range(16)]
    for i, name in enumerate(ORDER):
        art = ART[name]
        last = max(y for y in range(16) if art[y].strip("."))
        art = ["." * 16] * (15 - last) + art[:16 - (15 - last)]
        for y in range(16):
            row = art[y].ljust(16, ".")[:16]
            for x in range(16):
                img[y][i * 16 + x] = pixel(row[x])
    return img

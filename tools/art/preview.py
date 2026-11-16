"""Scaled preview of a generated sheet, on a dusk-coloured background:

    wsl python3 tools/art/preview.py knight|enemies|hero|ninja|props OUT.png [scale] [frames per row] [first] [count]

knight = every frame of the PixelLab knight with its extension cells, as seen in game (96 x 96 cells)
enemies = every enemy pose put back together from its sprites, with the shared PAL2 (48 x 48 cells)
hero  = every frame of the ninja with its slash arcs around it (80 x 88 cells), as seen in game
ninja = the raw 32 x 48 body cells of res/ninja.png
"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from png import write_indexed  # noqa: E402

which, out = sys.argv[1], sys.argv[2]
scale = int(sys.argv[3]) if len(sys.argv) > 3 else 4
per_row = int(sys.argv[4]) if len(sys.argv) > 4 else 9
first = int(sys.argv[5]) if len(sys.argv) > 5 else 0
count = int(sys.argv[6]) if len(sys.argv) > 6 else 999

if which == "knight":
    import knight
    import ninja
    (img, pal), fw = knight.preview_sheet(ninja), knight.PW
elif which == "enemies":
    import enemies
    (img, pal), fw = enemies.preview(), 48
elif which == "hero":
    import ninja
    img, fw, pal = ninja.preview_sheet(), ninja.PW, list(ninja.PAL1)
    for name, lost in ninja.lost_pixels().items():
        print("WARNING %s: %d pixels outside its cells" % (name, lost))
else:
    mod = __import__(which)
    img, fw = mod.sheet(), getattr(mod, "W", 16)
    pal = list(mod.PAL2 if which == "props" else mod.PAL1)
pal[0] = (0x6A, 0x3A, 0x4A)   # visible background instead of transparency (zone-1 stone at dusk)
if which == "hero" and os.environ.get("CELLS"):
    # CELLS=1 tints the 32 x 48 body cell, to check what a pose lets out of it.
    pal.append((0x5A, 0x30, 0x40))
    for y in range(28, 28 + ninja.H):
        for x in range(len(img[0])):
            if 16 <= x % fw < 16 + ninja.W and img[y][x] == 0:
                img[y][x] = 16

fh = len(img)
frames = list(range(len(img[0]) // fw))[first:first + count]
rows = (len(frames) + per_row - 1) // per_row
grid = [[0] * (min(per_row, len(frames)) * fw) for _ in range(rows * fh)]
for k, f in enumerate(frames):
    ox, oy = (k % per_row) * fw, (k // per_row) * fh
    for y in range(fh):
        grid[oy + y][ox:ox + fw] = img[y][f * fw:(f + 1) * fw]
big = [[c for c in row for _ in range(scale)] for row in grid for _ in range(scale)]
write_indexed(out, big, pal, opaque=True)

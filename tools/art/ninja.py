"""The ninja as a jointed puppet, rendered into 24 x 32 frames (feet on row 29, body centred on x 12),
facing right. The extra width leaves room for the headband tails streaming behind.

Each pose gives limb angles; the renderer paints back limbs, torso, front limbs, head and scarf,
then adds a 1-px dark outline. Consistent proportions across every frame, cheap to tweak.
PAL1: 1 outline, 2 suit shadow, 3 suit, 4 skin, 5 scarf, 6 blade, 7 suit highlight, 8 scarf shadow,
9 steel.
"""
import math

W, H = 24, 32
FEET = 29

# Head, facing right (rows top to bottom). '.' = empty.
HEAD = [
    ".37773.",
    "3333377",
    "3344444",
    "3344141",
    "3333333",
    ".33333.",
    "..333..",
]

# (front thigh, front shin, back thigh, back shin) in degrees from vertical, + = forward.
RUN_LEG = [(35, 10), (20, -15), (0, -45), (-20, -65), (-35, -30), (-20, -55), (5, -75), (30, -25)]


def pose_run(i):
    ft, fs = RUN_LEG[i]
    bt, bs = RUN_LEG[(i + 4) % 8]
    bob = {0: 1, 4: 1, 2: -1, 6: -1}.get(i, 0)
    return dict(lean=1, bob=bob, legs=((ft, fs), (bt, bs)),
                arms=((-bt * 0.9, -bt * 0.9 + 70), (-ft * 0.9, -ft * 0.9 + 70)), scarf=i)


POSES = {}
for i in range(8):
    POSES["run%d" % i] = pose_run(i)
# Fighting stance (Street Fighter style): wide, knees bent, guard fist forward, breathing on 4 frames.
for i, bob in enumerate((0, 1, 1, 0)):
    POSES["idle%d" % i] = dict(lean=1, bob=bob + 1, legs=((28, -12), (-26, 8)),
                               arms=((15, 40), (55, 100 - bob * 10)), scarf=i * 2, wind=0.4)
POSES["rise"] = dict(lean=1, bob=-1, legs=((60, -70), (20, -80)), arms=((-40, 10), (-70, -20)), scarf=5)
POSES["peak"] = dict(lean=0, bob=0, legs=((35, -45), (-15, -50)), arms=((60, 80), (-40, 20)), scarf=3)
POSES["fall"] = dict(lean=0, bob=0, legs=((15, -10), (-20, -15)), arms=((100, 110), (-90, -60)), scarf=1)
POSES["slash0"] = dict(lean=-1, bob=0, legs=((25, 0), (-20, 0)), arms=((-150, -110), (-20, 20)), scarf=4)   # drawing
POSES["slash1"] = dict(lean=2, bob=1, legs=((35, 5), (-30, 0)), arms=((90, 90), (-40, 10)), scarf=6,
                       sword=True)
POSES["slash2"] = dict(lean=1, bob=1, legs=((30, 5), (-25, 0)), arms=((135, 140), (-30, 20)), scarf=7,
                       free_hands=True)
POSES["crouch"] = dict(lean=1, bob=0, crouch=True, legs=((85, -10), (40, -60)), arms=((40, 90), (10, 60)),
                       scarf=1)
POSES["hurt"] = dict(lean=-2, bob=0, legs=((20, 10), (-30, -20)), arms=((-60, -30), (-120, -90)), scarf=5,
                     free_hands=True)

ORDER = ["idle%d" % i for i in range(4)] + ["run%d" % i for i in range(8)] + ["rise", "peak", "fall",
                                                                  "slash0", "slash1", "slash2",
                                                                  "crouch", "hurt"]


def stamp(img, x, y, color, thick):
    for dy in range(-(thick // 2), thick - thick // 2):
        for dx in range(-(thick // 2), thick - thick // 2):
            xx, yy = int(round(x)) + dx, int(round(y)) + dy
            if 0 <= xx < W and 0 <= yy < H:
                img[yy][xx] = color


def segment(img, x0, y0, angle, length, color, thick):
    """Draws a limb from (x0, y0) pointing down rotated by angle (deg, + = forward). Returns its end."""
    a = math.radians(angle)
    x1, y1 = x0 + math.sin(a) * length, y0 + math.cos(a) * length
    steps = int(length * 2) + 1
    for s in range(steps + 1):
        t = s / steps
        stamp(img, x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, color, thick)
    return x1, y1


def line(img, a, b, color, thick):
    steps = int(max(abs(b[0] - a[0]), abs(b[1] - a[1])) * 2) + 1
    for s in range(steps + 1):
        t = s / steps
        stamp(img, a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, color, thick)


def reach(img, shoulder, target, upper, lower, color):
    # Elbow placed by the law of cosines, bent downwards (natural for a hand raised behind).
    dx, dy = target[0] - shoulder[0], target[1] - shoulder[1]
    d = min(math.hypot(dx, dy), upper + lower - 0.01)
    base = math.atan2(dy, dx)
    cos_a = (upper * upper + d * d - lower * lower) / (2 * upper * d)
    alpha = math.acos(max(-1.0, min(1.0, cos_a)))
    angle = base - alpha if math.sin(base - alpha) > math.sin(base + alpha) else base + alpha
    elbow = (shoulder[0] + math.cos(angle) * upper, shoulder[1] + math.sin(angle) * upper)
    line(img, shoulder, elbow, color, 2)
    line(img, elbow, target, color, 2)
    stamp(img, target[0], target[1], 4, 1)   # hand on the grip


def render(p):
    img = [[0] * W for _ in range(H)]
    crouch = p.get("crouch", False)
    bob, lean = p["bob"], p["lean"]
    hip_y = (22 if crouch else 18) + bob
    hip_x = 11
    neck_x, neck_y = hip_x + lean, hip_y - (6 if crouch else 7)
    (ft, fs), (bt, bs) = p["legs"]
    (fua, fla), (bua, bla) = p["arms"]
    thigh, shin = (4, 4) if crouch else (5, 6)

    def leg(t, s, color):
        kx, ky = segment(img, hip_x, hip_y, t, thigh, color, 3)
        fx, fy = segment(img, kx, ky, s, shin, color, 2)
        # Foot, pointing forward, clamped onto the ground line.
        fy = min(fy, FEET)
        for dx in range(3):
            stamp(img, fx + dx, fy, 1 if color == 2 else 2, 1)

    def arm(ua, la, color, sword=False):
        sx, sy = neck_x + 1, neck_y + 1
        ex, ey = segment(img, sx, sy, ua, 4, color, 2)
        hx, hy = segment(img, ex, ey, la, 3, color, 2)
        stamp(img, hx, hy, 4, 1)   # hand
        if sword:   # blade held straight out (the slash arc is a separate sprite)
            segment(img, hx, hy - 1, 90, 6, 9, 1)

    # Sword on the back, drawn first so the torso covers its middle: scabbard running down the back,
    # guard behind the shoulder, grip sticking out behind the head (hidden while it is in hand).
    holding = not p.get("sword") and not p.get("free_hands")
    grip = (neck_x - 6, neck_y - 8)
    if not p.get("sword"):
        line(img, (neck_x - 3, neck_y - 2), (neck_x + 2, neck_y + 10), 8, 2)    # scabbard

    leg(bt, bs, 2)
    arm(bua, bla, 2)
    # Torso: shoulders 5 px wide narrowing to the hips, belt in scarf red.
    for y in range(neck_y, hip_y + 1):
        t = (y - neck_y) / max(1, hip_y - neck_y)
        cx = neck_x + (hip_x - neck_x) * t
        half = 3 - t
        for x in range(int(round(cx - half)), int(round(cx + half)) + 1):
            if 0 <= x < W:
                img[y][x] = 5 if y == hip_y - 1 else (7 if x == int(round(cx + half)) and y < hip_y - 2 else 3)
    leg(ft, fs, 3)

    if not p.get("sword"):
        # Hilt sticking out behind the head: steel guard, wrapped grip, pommel.
        line(img, (neck_x - 5, neck_y - 3), (neck_x - 2, neck_y - 4), 9, 1)   # guard
        line(img, (neck_x - 4, neck_y - 4), grip, 2, 1)                       # grip
        stamp(img, grip[0] - 1, grip[1] - 1, 9, 1)                            # pommel
    if holding:
        # Front hand on the grip, ready to draw (drawn before the head: the arm passes behind it).
        reach(img, (neck_x + 1, neck_y + 1), (grip[0] + 1, grip[1] + 2), 5, 5, 3)

    # Head.
    hx0, hy0 = neck_x - 2, neck_y - 7
    for y, row in enumerate(HEAD):
        for x, ch in enumerate(row):
            if ch != "." and 0 <= hy0 + y < H and 0 <= hx0 + x < W:
                img[hy0 + y][hx0 + x] = int(ch)
    # Headband: knot at the back of the head, two long tails streaming behind in the wind. Taut and
    # level when moving, drooping when standing; the wave travels along them from frame to frame.
    wind = p.get("wind", 1.0)
    kx, ky = hx0 - 1, hy0 + 2
    stamp(img, kx, ky, 5, 1)
    for strand, (length, drop) in enumerate(((7, 0.0), (5, 0.6))):
        for k in range(1, length + 1):
            phase = (p["scarf"] + k) % 8
            wave = [0, 1, 1, 0, -1, -1, 0, 1][phase] * (0.5 + 0.5 * wind)
            sag = k * (1 - wind) * 0.8 + k * drop * 0.3
            x = kx - k
            y = ky + strand + wave + sag
            if 0 <= x < W and 0 <= int(round(y)) < H:
                img[int(round(y))][x] = 5 if (k + strand) % 3 else 8
    if not holding:
        arm(fua, fla, 3, p.get("sword", False))

    # Auto outline: empty pixels touching the figure become outline.
    out = [row[:] for row in img]
    for y in range(H):
        for x in range(W):
            if img[y][x]:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                xx, yy = x + dx, y + dy
                if 0 <= xx < W and 0 <= yy < H and img[yy][xx] not in (0, 1):
                    out[y][x] = 1
                    break
    return out


def sheet():
    """All frames side by side (rescomp COLUMN order = Mega Drive sprite tile order per frame)."""
    frames = [render(POSES[name]) for name in ORDER]
    img = [[0] * (W * len(frames)) for _ in range(H)]
    for i, f in enumerate(frames):
        for y in range(H):
            for x in range(W):
                img[y][i * W + x] = f[y][x]
    return img


PAL1 = [
    (0x10, 0x08, 0x10), (0x00, 0x00, 0x00), (0x22, 0x22, 0x55), (0x44, 0x44, 0xAA), (0xEE, 0xAA, 0x88),
    (0xEE, 0x22, 0x22), (0xEE, 0xEE, 0xEE), (0x77, 0x88, 0xDD), (0x99, 0x11, 0x11), (0x99, 0x99, 0xAA),
    (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0xEE, 0x44, 0x44),
]

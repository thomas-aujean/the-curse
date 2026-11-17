"""The hero: a ninja between Shinobi (look) and Kisuke (motion), 32 x 48 frames, facing right.

Original art, drawn by code: a jointed puppet whose limbs are shaded capsules (3 tones per material,
light from the low sun on the left), plus hand-placed pixel clusters for the head, the sword, the
armour and the cloth tails. Every part is separated from what it covers by a dark line, and the whole
figure gets a 1-px outline. See docs/style.md.

Frame layout: hitbox centre on x = CX, soles on row FEET (the outline under them is row FEET + 1).
Strike frames are drawn on a larger canvas: what leaves the 32 x 48 body cell (blade, slash arc) is
cut into 32 x 32 "extension" cells that the engine shows as extra sprites next to the body.
PAL1 indices are named below; the HUD hearts use RED and OUT.
"""
import math

W, H = 32, 48
CX = 16.0          # pixel column boundary the body is centred on
FEET = 46          # last pixel row of the soles
GROUND = FEET + 1.0
EXT = 32           # extension cells are 32 x 32 (one 4 x 4 hardware sprite)
OX, OY, BW, BH = 48, 48, 144, 152   # big canvas: origin of the body cell, size

OUT, N2, N3, SKIN, RED, WHITE, N7, RED_D, STEEL, RIM, SKIN_D, BRZ_D, BRZ, SMEAR, RED_L = range(1, 16)

PAL1 = [
    (0x40, 0x20, 0x60),  # 0  transparent
    (0x00, 0x00, 0x20),  # 1  outline (dark navy, never pure black)
    (0x20, 0x20, 0x60),  # 2  suit shadow
    (0x40, 0x40, 0xA0),  # 3  suit
    (0xE0, 0xA0, 0x80),  # 4  skin
    (0xE0, 0x20, 0x20),  # 5  red: headband, sash (and HUD hearts)
    (0xE0, 0xE0, 0xE0),  # 6  blade edge, slash arc core
    (0x60, 0x80, 0xC0),  # 7  suit light
    (0x80, 0x00, 0x20),  # 8  red shadow
    (0x80, 0x80, 0xA0),  # 9  steel
    (0xA0, 0xC0, 0xE0),  # 10 suit rim light
    (0xA0, 0x60, 0x40),  # 11 skin shadow
    (0x60, 0x40, 0x20),  # 12 leather, bronze shadow
    (0xC0, 0x80, 0x40),  # 13 bronze: guards, sword fittings
    (0x80, 0xC0, 0xE0),  # 14 slash arc trail
    (0xE0, 0x60, 0x40),  # 15 red light (and the boss room on the map)
]

SUIT = (N2, N3, N7)
SUIT_FAR = (N2, N2, N3)        # far limbs: one step darker, so depth reads at a glance
GUARD = (BRZ_D, BRZ, BRZ)
GUARD_FAR = (BRZ_D, BRZ_D, BRZ)
SKIN_R = (SKIN_D, SKIN, SKIN)
SASH = (RED_D, RED, RED_L)

LIGHT = (-0.87, 0.49)          # unit vector towards the low sun: left, slightly below

THIGH, SHIN, UARM, FARM, TORSO = 10.0, 10.0, 6.5, 6.0, 12.0
SOLE = 1.9                     # ankle height above the sole
BLADE = 17.0

CODES = {".": 0, "o": OUT, "d": N2, "n": N3, "l": N7, "h": RIM, "s": SKIN, "k": SKIN_D, "r": RED,
         "R": RED_D, "L": RED_L, "w": WHITE, "g": STEEL, "b": BRZ_D, "B": BRZ}

# Head facing right: hood, headband, eye slit, mask over the nose. Anchor = its centre.
HEAD = [
    "..lnnn..",
    ".lnnnnd.",
    "LrrrrrRR",
    "hlnssss.",
    "lnnskos.",
    "lnnnnddd",
    ".nnnddd.",
    "..nddd..",
]


# ---------------------------------------------------------------- geometry helpers

def vec(angle, length=1.0):
    """Limb direction: 0 = down, 90 = forward (right), 180 = up, -90 = back."""
    a = math.radians(angle)
    return (math.sin(a) * length, math.cos(a) * length)


def add(a, b):
    return (a[0] + b[0], a[1] + b[1])


def mul(a, k):
    return (a[0] * k, a[1] * k)


def rot(p, pivot, deg):
    """Rotates clockwise on screen (y down), i.e. a forward roll when facing right."""
    a = math.radians(deg)
    x, y = p[0] - pivot[0], p[1] - pivot[1]
    return (pivot[0] + x * math.cos(a) - y * math.sin(a), pivot[1] + x * math.sin(a) + y * math.cos(a))


def ik(a, c, l1, l2, bend):
    """Middle joint of a two-bone chain from a to c; bend chooses the side it folds to."""
    dx, dy = c[0] - a[0], c[1] - a[1]
    d = max(0.01, min(math.hypot(dx, dy), l1 + l2 - 0.01))
    base = math.atan2(dy, dx)
    alpha = math.acos(max(-1.0, min(1.0, (l1 * l1 + d * d - l2 * l2) / (2 * l1 * d))))
    ang = base + bend * alpha
    return (a[0] + math.cos(ang) * l1, a[1] + math.sin(ang) * l1)


def cell(p):
    return (int(math.floor(p[0])), int(math.floor(p[1])))


# ---------------------------------------------------------------- canvas and shapes

class Canvas:
    """Body-cell coordinates, with room around the cell for blades and slash arcs."""

    def __init__(self):
        self.px = [[0] * BW for _ in range(BH)]

    def get(self, x, y):
        return self.px[y + OY][x + OX] if -OX <= x < BW - OX and -OY <= y < BH - OY else 0

    def put(self, x, y, c):
        if -OX <= x < BW - OX and -OY <= y < BH - OY:
            self.px[y + OY][x + OX] = c

    def blit(self, shape, sep=True):
        """Paints a shape; where it covers what is already drawn, a dark line separates the two
        (suit shadow on lit cloth, outline colour elsewhere: no pure black inside the figure)."""
        if sep:
            for (x, y) in shape:
                for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                    if (nx, ny) in shape:
                        continue
                    e = self.get(nx, ny)
                    if e:
                        self.put(nx, ny, N2 if e in (N3, N7, RIM) else OUT)
        for (x, y), c in shape.items():
            self.put(x, y, c)

    def dot(self, p, c):
        x, y = cell(p)
        self.put(x, y, c)


def capsule(a, b, ra, rb, ramp, thr=(-0.25, 0.4), band=None):
    """Tapered limb from a to b, shaded like a cylinder under LIGHT. band(t, side) may override the ramp."""
    shape = {}
    dx, dy = b[0] - a[0], b[1] - a[1]
    l2 = dx * dx + dy * dy or 1e-9
    r = max(ra, rb) + 1
    for y in range(int(min(a[1], b[1]) - r) - 1, int(max(a[1], b[1]) + r) + 2):
        for x in range(int(min(a[0], b[0]) - r) - 1, int(max(a[0], b[0]) + r) + 2):
            cx, cy = x + 0.5, y + 0.5
            t = max(0.0, min(1.0, ((cx - a[0]) * dx + (cy - a[1]) * dy) / l2))
            ox, oy = cx - (a[0] + dx * t), cy - (a[1] + dy * t)
            rr = ra + (rb - ra) * t
            if ox * ox + oy * oy > rr * rr:
                continue
            i = (ox * LIGHT[0] + oy * LIGHT[1]) / rr
            rmp = (band(t, i) if band else None) or ramp
            shape[(x, y)] = rmp[2] if i > thr[1] else rmp[0] if i < thr[0] else rmp[1]
    return shape


def line(a, b, color):
    shape = {}
    n = int(max(abs(b[0] - a[0]), abs(b[1] - a[1])) * 2) + 1
    for s in range(n + 1):
        t = s / n
        shape[cell((a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t))] = color
    return shape


def sprite(rows, centre, turns=0):
    """Hand-drawn pixels centred on a point; turns = quarter turns clockwise (somersault)."""
    for _ in range(turns % 4):
        rows = ["".join(rows[len(rows) - 1 - y][x] for y in range(len(rows))) for x in range(len(rows[0]))]
    x0 = int(round(centre[0] - len(rows[0]) / 2.0))
    y0 = int(round(centre[1] - len(rows) / 2.0))
    return {(x0 + x, y0 + y): CODES[ch] for y, row in enumerate(rows) for x, ch in enumerate(row) if ch != "."}


def ribbon(cv, start, length, flow, amp, phase, droop=0.0, freq=55.0, colors=(RED, RED_D, RED_L)):
    """Cloth tail: a chain of 1-px steps streaming along `flow` (screen degrees: 180 = back, 90 = down,
    270 = up), with a wave travelling down it and `droop` degrees of turn per pixel."""
    x, y = start
    pts = []
    for k in range(length):
        a = flow + amp * math.sin(math.radians(phase - k * freq)) * min(1.0, k / 3.0) + droop * k
        a = math.radians(a)
        x += math.cos(a)
        y += math.sin(a)
        pts.append((x, y, a))
    shape = {}
    for k, (x, y, a) in enumerate(pts):
        p = cell((x, y))
        if k < length * 0.75:
            flat = abs(math.cos(a)) >= abs(math.sin(a))
            q = (p[0], p[1] + 1) if flat else (p[0] + 1, p[1])
            if q not in shape:
                shape[q] = colors[1]
        rising = math.sin(a) < -0.25 and math.cos(a) < 0
        shape[p] = colors[2] if rising else colors[0]
    cv.blit(shape)


def arc(layer, centre, radius, tail, lead, width, strong):
    """Slash smear: a crescent swept by the blade from angle `tail` to `lead` (limb degrees), thin and
    pale where the blade was, wide and white where it is. Two tones, no outline."""
    span = lead - tail
    for y in range(int(centre[1] - radius) - 2, int(centre[1] + radius) + 3):
        for x in range(int(centre[0] - radius) - 2, int(centre[0] + radius) + 3):
            dx, dy = x + 0.5 - centre[0], y + 0.5 - centre[1]
            r = math.hypot(dx, dy)
            if r > radius or r < radius - width - 1:
                continue
            ang = math.degrees(math.atan2(dx, dy))        # 0 = down, 90 = forward
            u = (ang - tail) / span
            for turn in (-360, 360):                       # the sweep may cross the +-180 seam
                u2 = (ang + turn - tail) / span
                if 0 <= u2 <= 1:
                    u = u2
            if not 0 <= u <= 1:
                continue
            thick = width * (u ** 0.7)
            outer = radius - (1 - u) * (1 - u) * 2.5      # the tail curls slightly inwards
            depth = outer - r
            if depth < 0 or depth > thick:
                continue
            if u < 0.3 and (x + y) % 2:                   # the oldest part of the sweep breaks up
                continue
            white = depth < thick * (0.55 if strong else 0.3) and u > (0.35 if strong else 0.75)
            layer[(x, y)] = WHITE if white else SMEAR


# ---------------------------------------------------------------- the puppet

def leg_points(hip, spec):
    """spec: ('fk', thigh, shin[, foot]) or ('ik', ankle x from CX, sole height above ground[, foot])."""
    foot = spec[3] if len(spec) > 3 else 0
    if spec[0] == "fk":
        knee = add(hip, vec(spec[1], THIGH))
        ankle = add(knee, vec(spec[2], SHIN))
    else:
        ankle = (CX + spec[1], GROUND - SOLE - spec[2])
        knee = ik(hip, ankle, THIGH, SHIN, -1)
    return knee, ankle, foot


def draw_leg(cv, hip, knee, ankle, foot, far, turn=0.0):
    suit = SUIT_FAR if far else SUIT
    guard = GUARD_FAR if far else GUARD

    def shin_band(t, i):
        # Shin guard: a bronze plate on the lit front of the shin, cloth showing behind it.
        if 0.3 < t < 0.84 and i > -0.35:
            return guard
        return None

    cv.blit(capsule(hip, knee, 3.0, 2.3, suit))
    cv.blit(capsule(knee, ankle, 2.1, 1.5, suit, band=shin_band))
    # Tabi: toes pointing `foot` degrees below the horizontal.
    a = math.radians(foot + turn)
    toe = (ankle[0] + math.cos(a) * 3.4, ankle[1] + math.sin(a) * 3.4 + 0.5)
    heel = (ankle[0] - math.cos(a) * 0.6, ankle[1] + 0.5)
    cv.blit(capsule(heel, toe, 1.45, 1.25, (OUT, N2, N2) if far else (N2, N2, N3)))


def draw_arm(cv, shoulder, elbow, hand, far):
    suit = SUIT_FAR if far else SUIT
    guard = GUARD_FAR if far else GUARD

    def fore_band(t, i):
        if 0.3 < t < 0.88 and i > -0.35:
            return guard
        return None

    cv.blit(capsule(shoulder, elbow, 2.2, 1.6, suit))
    cv.blit(capsule(elbow, hand, 1.6, 1.4, suit, band=fore_band))
    cv.blit(capsule(hand, hand, 1.3, 1.3, (SKIN_D, SKIN_D, SKIN) if far else SKIN_R), sep=False)


def arm_points(shoulder, spec, grip):
    """spec: ('fk', upper, fore) | ('grip',) hand on the sword grip | ('to', x, y[, bend])."""
    if spec[0] == "fk":
        elbow = add(shoulder, vec(spec[1], UARM))
        return elbow, add(elbow, vec(spec[2], FARM))
    target = grip if spec[0] == "grip" else (spec[1], spec[2])
    bend = spec[3] if len(spec) > 3 else -1
    return ik(shoulder, target, UARM, FARM, bend), target


def render(p):
    """Returns (figure canvas, slash-arc layer)."""
    cv = Canvas()
    smear = {}
    lean = p.get("lean", 0)
    curl = p.get("curl", 0)
    spin = p.get("spin", 0)                      # whole-body roll, degrees clockwise
    hip_x = CX + p.get("x", 0)

    # Legs first on a trial hip, to stand the lowest sole on the ground when asked.
    hip = (hip_x, GROUND - p.get("h", 20))
    if p.get("ground"):
        low = max(leg_points(hip, p[k])[1][1] for k in ("near_leg", "far_leg"))
        hip = (hip_x, hip[1] + (GROUND - SOLE - low) - p.get("lift", 0))

    up = vec(180 - (lean - curl / 2.0))          # belly direction
    up2 = vec(180 - (lean + curl / 2.0))         # chest direction
    mid = add(hip, mul(up, 5.4))
    chest = add(mid, mul(up2, 3.4))
    neck = add(mid, mul(up2, TORSO - 5.4))
    back = (up2[1], -up2[0])                     # unit vector out of the back
    shoulder = add(mid, mul(up2, 4.0))
    head_c = add(neck, add(mul(up2, 2.6), p.get("head", (1.2, -1.2))))

    # Sword across the back: the hilt slants out behind the near shoulder.
    sx = add(add(mid, mul(up2, 5.2)), mul(back, 3.6))
    tilt = math.radians(p.get("sword_tilt", 46))
    axis = (up2[0] * math.cos(tilt) + back[0] * math.sin(tilt), up2[1] * math.cos(tilt) + back[1] * math.sin(tilt))
    side = (axis[1], -axis[0])
    pull = p.get("pull", 0)                      # how far the blade is drawn out of the scabbard
    guard_p = add(sx, mul(axis, 1.0 + pull))
    grip = add(guard_p, mul(axis, 3.4))
    pommel = add(guard_p, mul(axis, 6.2))
    tip = add(sx, mul(axis, -10.0))

    pivot = p.get("pivot") or add(hip, mul(up, 4.0))

    def R(pt):
        return rot(pt, pivot, spin) if spin else pt

    quarter = int(round(spin / 90.0)) if spin else 0
    drawn = p.get("blade") is not None

    near = leg_points(hip, p["near_leg"])
    far = leg_points(hip, p["far_leg"])

    # 1. Far leg, far arm.
    draw_leg(cv, R(add(hip, (-0.6, 0))), R(far[0]), R(far[1]), far[2], True, spin)
    f_sh = add(shoulder, (1.0, 0.3))
    f_el, f_hand = arm_points(f_sh, p.get("far_arm", ("fk", 30, 100)), grip)
    draw_arm(cv, R(f_sh), R(f_el), R(f_hand), True)

    # 2. Scabbard (mostly hidden by the back).
    cv.blit(capsule(R(sx), R(tip), 1.25, 1.0, (OUT, BRZ_D, BRZ_D)))

    def draw_hilt():
        if pull:
            cv.blit(line(R(sx), R(guard_p), WHITE))
        cv.blit(line(R(guard_p), R(pommel), RED_D))
        for k in (1.6, 3.4, 5.0):
            cv.dot(R(add(guard_p, mul(axis, k))), BRZ)
        cv.blit(line(R(add(guard_p, mul(side, 1.7))), R(add(guard_p, mul(side, -1.7))), BRZ))
        cv.dot(R(pommel), BRZ)

    # 3. Torso: hips, red sash, chest, the strap of the scabbard across the chest.
    cv.blit(capsule(R(add(hip, mul(up, 0.4))), R(add(hip, mul(up, 2.6))), 3.4, 3.4, SUIT))
    cv.blit(capsule(R(mid), R(chest), 3.8, 4.6, SUIT))
    cv.blit(capsule(R(add(hip, mul(up, 3.5))), R(add(hip, mul(up, 4.7))), 3.7, 3.7, SASH), sep=False)
    s0 = R(add(add(mid, mul(up2, 6.0)), mul(back, 2.4)))
    s1 = R(add(add(mid, mul(up2, 0.4)), mul(back, -3.6)))
    cv.blit(line(s0, s1, BRZ_D), sep=False)

    # 4. Sash knot and its two short tails at the back of the waist.
    knot = R(add(add(hip, mul(up, 4.0)), mul(back, 3.8)))
    st = p.get("sash", (100, 18, 0))
    ribbon(cv, knot, 7, st[0], st[1], st[2], droop=4, colors=(RED, RED_D, RED))
    ribbon(cv, add(knot, (0, 1)), 5, st[0] + 22, st[1], st[2] + 70, droop=5, colors=(RED_D, RED_D, RED))

    # 5. Near leg.
    draw_leg(cv, R(add(hip, (0.6, 0))), R(near[0]), R(near[1]), near[2], False, spin)

    # 6. Sword arm resting on the grip: it passes behind the head, so the face stays readable.
    n_sh = add(shoulder, (-0.6, 0.2))
    n_spec = p.get("near_arm", ("grip",))
    n_el, n_hand = arm_points(n_sh, n_spec, grip)
    if not drawn:
        draw_hilt()

    # 7. Neck and head.
    hc = R(head_c)
    cv.blit(capsule(R(add(neck, mul(up2, -0.5))), hc, 1.9, 1.9, (N2, N2, N3)))
    head_turn = quarter + p.get("head_turn", 0)
    cv.blit(sprite(HEAD, hc, head_turn))

    if n_spec[0] == "grip":
        draw_arm(cv, R(n_sh), R(n_el), R(n_hand), False)

    # 8. Headband knot and tails, streaming with 1-2 frames of lag behind the body.
    kn = add(hc, rot((-4.6, -1.4), (0, 0), 90 * head_turn))
    t = p.get("tails", (150, 20, 0))
    droop = p.get("droop", 1.5)
    if t[0] == "spin":
        # Rolling: the tails wrap around the ball, trailing the knot along its circle.
        phi = math.degrees(math.atan2(kn[1] - pivot[1], kn[0] - pivot[0]))
        t = (phi - 100, t[1], t[2])
        droop = -7.0
    ribbon(cv, kn, 14, t[0], t[1], t[2], droop=droop)
    ribbon(cv, add(kn, (0, 1)), 10, t[0] + 14, t[1] * 1.2, t[2] + 100, droop=droop * 1.6)

    # 9. Near arm when it is busy, and the blade when it is out.
    if drawn:
        hand = R(n_hand)
        d = vec(p["blade"] + spin)
        n = (d[1], -d[0])
        base = add(hand, mul(d, 1.6))
        body = {}
        for k in range(0, int(BLADE * 2)):
            s = k / 2.0
            q = add(add(base, mul(d, s)), mul(n, -0.8 * (s / BLADE) ** 2))   # katana curve
            body[cell(q)] = WHITE
            if s < BLADE - 4:
                body.setdefault(cell(add(q, mul(n, 1.0))), STEEL)
        cv.blit(body)
        cv.blit(line(add(base, mul(n, 1.8)), add(base, mul(n, -1.8)), BRZ))
        cv.blit(line(add(hand, mul(d, -2.8)), hand, RED_D))
        a = p.get("arc")
        if a:   # (tail angle, width, strong): the crescent ends on the blade
            c = R(n_sh)
            tipd = math.hypot(base[0] + d[0] * BLADE - c[0], base[1] + d[1] * BLADE - c[1])
            lead = math.degrees(math.atan2(base[0] + d[0] * BLADE - c[0], base[1] + d[1] * BLADE - c[1]))
            while lead - a[0] > 180:
                lead -= 360
            while lead - a[0] < -180:
                lead += 360
            arc(smear, c, tipd + 0.5, a[0], lead, a[1], a[2])
    if n_spec[0] != "grip":
        draw_arm(cv, R(n_sh), R(n_el), R(n_hand), False)

    return cv, smear


def finish(cv, smear):
    """1-px dark outline around the figure, a rim of sunset light on its left edge, then the slash arc
    slipped underneath. Returns a dict of pixels in body-cell coordinates."""
    out = {}
    for y in range(-OY, BH - OY):
        for x in range(-OX, BW - OX):
            c = cv.get(x, y)
            if c:
                out[(x, y)] = RIM if c == N7 and not cv.get(x - 1, y) else c
            elif cv.get(x + 1, y) or cv.get(x - 1, y) or cv.get(x, y + 1) or cv.get(x, y - 1):
                out[(x, y)] = OUT
    for q, c in smear.items():
        out.setdefault(q, c)
    return out


# ---------------------------------------------------------------- poses

POSES = {}
ORDER = []


def pose(name, **kw):
    POSES[name] = kw
    ORDER.append(name)


# Fighting stance: low, weight between both legs, sword hand on the grip, guard fist forward.
for i, (dh, dl) in enumerate(((0.0, 0), (0.5, 1), (1.0, 2), (0.5, 1))):
    pose("idle%d" % i, h=19.0 - dh, lean=12 + dl, near_leg=("ik", -6.0, 0), far_leg=("ik", 7.0, 0),
         far_arm=("fk", 38, 112 + dl * 2), tails=(160, 14, i * 90), droop=2.5, sash=(110, 10, i * 90))

# Run: leaning far forward, long strides, free arm trailing, tails streaming flat behind.
RUN_LEG = [(52, 14, -18), (26, -22, 0), (-8, -42, 10), (-36, -56, 50),
           (-30, -104, 75), (8, -98, 60), (50, -52, 30), (66, -2, -10)]
for i in range(8):
    nt, ns, nf = RUN_LEG[i]
    ft, fs, ff = RUN_LEG[(i + 4) % 8]
    swing = math.sin(math.radians(i * 45))
    pose("run%d" % i, ground=True, lift=(2 if i in (3, 7) else 0), lean=33, x=-1,
         near_leg=("fk", nt, ns, nf), far_leg=("fk", ft, fs, ff),
         far_arm=("fk", -58 + swing * 14, -72 + swing * 10), tails=(182, 14, i * 90), droop=0.3,
         sash=(176, 14, i * 90 + 40))

# Jump: stretched on the way up, a forward somersault over the top, open on the way down, and one
# frame of deep landing. Cloth lags: it still points down at the top and up after landing.
pose("rise", h=24, lean=8, near_leg=("fk", 72, -18, 40), far_leg=("fk", -6, -22, 62),
     far_arm=("fk", -34, -14), tails=(112, 10, 0), droop=-0.5, sash=(96, 8, 0))
for i in range(4):
    pose("flip%d" % i, h=20, x=-4, lean=35, curl=60, spin=i * 90, pivot=(CX + 0.5, GROUND - 23),
         near_leg=("fk", 118, 10, 30), far_leg=("fk", 104, -6, 30), far_arm=("fk", 75, 20),
         head=(0.4, 1.0), head_turn=1, tails=("spin", 6, i * 90), sash=(200 + i * 90, 6, 0))
pose("fall", h=23, lean=-4, near_leg=("fk", 36, -26, 42), far_leg=("fk", -12, -38, 55),
     far_arm=("fk", -138, -160), tails=(258, 12, 0), droop=-1.0, sash=(250, 10, 0))
pose("land", h=11, lean=38, curl=20, near_leg=("ik", 6, 0), far_leg=("ik", -8, 0),
     far_arm=("to", CX + 10, GROUND - 3, -1), tails=(236, 14, 90), droop=-2.0, sash=(240, 10, 0))

# Side slash: coil, lunge with the blade through the target, follow through to the ground, sheathe.
STRIKE_EXT = [(32, -8), (32, 24)]
pose("slash0", h=17.5, lean=-5, x=-1, near_leg=("ik", -7.5, 0), far_leg=("ik", 6.5, 0), pull=3,
     far_arm=("fk", 24, 78), tails=(140, 12, 0), sash=(100, 10, 0))
pose("slash1", h=15.5, lean=28, near_leg=("ik", -11, 0, 20), far_leg=("ik", 10, 0),
     near_arm=("fk", 84, 88), blade=80, arc=(150, 7.0, True), ext=STRIKE_EXT,
     far_arm=("fk", -70, -84), tails=(190, 16, 0), droop=0.5, sash=(180, 12, 0))
pose("slash2", h=14.5, lean=36, near_leg=("ik", -11, 0, 25), far_leg=("ik", 10.5, 0),
     near_arm=("fk", 52, 44), blade=44, arc=(104, 5.0, False), ext=STRIKE_EXT,
     far_arm=("fk", -80, -96), tails=(170, 16, 120), droop=1.0, sash=(165, 12, 90))
pose("slash3", h=16.5, lean=16, near_leg=("ik", -9, 0), far_leg=("ik", 9, 0), pull=6.5,
     far_arm=("fk", 10, 60), tails=(150, 14, 200), droop=2.0, sash=(120, 10, 90))
pose("slash4", h=17.2, lean=13, near_leg=("ik", -7.5, 0), far_leg=("ik", 8.2, 0), pull=3,
     far_arm=("fk", 30, 100), tails=(158, 14, 270), droop=2.5, sash=(112, 10, 180))

# Upward slash: sink, then stretch up on the toes, the arc passing over the head.
UP_EXT = [(-16, -24), (16, -24)]
pose("up0", h=15, lean=6, near_leg=("ik", -6.5, 0), far_leg=("ik", 6.5, 0), pull=3,
     far_arm=("fk", 12, 46), tails=(150, 12, 0), sash=(104, 10, 0))
pose("up1", h=21, lean=-3, near_leg=("ik", -4, 0), far_leg=("ik", 5, 0),
     near_arm=("fk", 166, 176), blade=186, arc=(116, 7.0, True), ext=UP_EXT,
     far_arm=("fk", -30, -10), tails=(110, 12, 0), droop=-0.5, sash=(92, 8, 0))
pose("up2", h=20.5, lean=-8, near_leg=("ik", -4.5, 0), far_leg=("ik", 5, 0),
     near_arm=("fk", 196, 208), blade=226, arc=(160, 5.0, False), ext=UP_EXT,
     far_arm=("fk", -10, 30), tails=(120, 12, 120), droop=0.0, sash=(96, 8, 90))

# In the air: side slash with the legs gathered, and the plunging slash under the feet.
pose("air0", h=22, lean=-2, near_leg=("fk", 50, -40, 40), far_leg=("fk", 10, -50, 50), pull=3,
     far_arm=("fk", 30, 80), tails=(200, 12, 0), droop=0.5, sash=(190, 10, 0))
pose("air1", h=22, lean=22, near_leg=("fk", 30, -70, 50), far_leg=("fk", -20, -80, 60),
     near_arm=("fk", 84, 88), blade=80, arc=(150, 7.0, True), ext=STRIKE_EXT,
     far_arm=("fk", -70, -84), tails=(196, 16, 0), droop=0.5, sash=(184, 12, 0))
pose("air2", h=22, lean=30, near_leg=("fk", 40, -60, 50), far_leg=("fk", -10, -85, 60),
     near_arm=("fk", 52, 44), blade=44, arc=(104, 5.0, False), ext=STRIKE_EXT,
     far_arm=("fk", -80, -96), tails=(180, 16, 120), droop=0.8, sash=(170, 12, 90))
pose("air3", h=22.5, lean=4, near_leg=("fk", 40, -30, 42), far_leg=("fk", -8, -40, 55), pull=6,
     far_arm=("fk", -60, -30), tails=(230, 12, 200), droop=-0.5, sash=(220, 10, 90))
DOWN_EXT = [(4, 40)]
pose("down0", h=18, lean=6, near_leg=("fk", 84, -50, 40), far_leg=("fk", 56, -70, 50), pull=5,
     far_arm=("fk", 40, 100), tails=(230, 12, 0), droop=-0.5, sash=(220, 10, 0))
pose("down1", h=15, lean=14, near_leg=("fk", 96, -40, 40), far_leg=("fk", 70, -64, 50),
     near_arm=("fk", 22, 8), blade=2, arc=(52, 6.0, True), ext=DOWN_EXT,
     far_arm=("fk", -120, -150), tails=(262, 14, 0), droop=-0.5, sash=(255, 10, 0))
pose("down2", h=15, lean=18, near_leg=("fk", 92, -44, 40), far_leg=("fk", 66, -68, 50),
     near_arm=("fk", -8, -22), blade=-36, arc=(16, 4.5, False), ext=DOWN_EXT,
     far_arm=("fk", -110, -140), tails=(266, 14, 120), droop=-0.5, sash=(258, 10, 90))

# Crouch, and its low slash.
CROUCH = dict(h=9, lean=30, curl=12, near_leg=("ik", 5, 0), far_leg=("ik", -7, 0, 0))
pose("crouch", far_arm=("fk", 44, 116), tails=(160, 10, 0), droop=3.0, sash=(120, 8, 0), **CROUCH)
pose("crouch_slash0", far_arm=("fk", 30, 90), pull=3, tails=(150, 10, 0), droop=3.0, sash=(110, 8, 0), **CROUCH)
pose("crouch_slash1", near_arm=("fk", 86, 90), blade=88, arc=(119, 4.5, True), ext=[(32, 16)],
     far_arm=("fk", -60, -80), tails=(188, 12, 0), droop=0.5, sash=(176, 8, 0), **CROUCH)

# Hit: thrown backwards, arms and cloth flung forward.
pose("hurt", h=21, lean=-22, x=-1, near_leg=("fk", 42, 12, 25), far_leg=("fk", 16, -24, 35),
     near_arm=("fk", 72, 44), far_arm=("fk", 96, 62), head=(0.2, -1.0), tails=(338, 16, 0), droop=1.0,
     sash=(350, 12, 0))


# ---------------------------------------------------------------- sheets

_cache = {}


def frame(name):
    """(body cell rows, [extension cell rows], pixels that fall outside every cell, all pixels)."""
    if name not in _cache:
        p = POSES[name]
        px = finish(*render(p))
        exts = p.get("ext", [])
        body = [[0] * W for _ in range(H)]
        cells = [[[0] * EXT for _ in range(EXT)] for _ in exts]
        lost = 0
        for (x, y), c in px.items():
            if 0 <= x < W and 0 <= y < H:
                body[y][x] = c
                continue
            for k, (ex, ey) in enumerate(exts):
                if ex <= x < ex + EXT and ey <= y < ey + EXT:
                    cells[k][y - ey][x - ex] = c
                    break
            else:
                lost += 1
        _cache[name] = (body, cells, lost, px)
    return _cache[name]


def side_by_side(cells, w):
    img = [[0] * (w * len(cells)) for _ in range(len(cells[0]))]
    for i, f in enumerate(cells):
        for y, row in enumerate(f):
            img[y][i * w:(i + 1) * w] = row
    return img


def sheet():
    """All body cells side by side, 32 x 48 each (rescomp cuts the sheet by columns; the engine regroups
    the tiles of a frame into its two hardware sprites, see gfx.c)."""
    return side_by_side([frame(name)[0] for name in ORDER], W)


def ext_sheet():
    """All extension cells side by side, 32 x 32 each, in frame order."""
    return side_by_side([c for name in ORDER for c in frame(name)[1]], EXT)


def lost_pixels():
    return {name: frame(name)[2] for name in ORDER if frame(name)[2]}


PW, PH = 80, 88   # preview cell: the body cell with its surroundings


def preview_sheet():
    cells = []
    for name in ORDER:
        px = frame(name)[3]
        cells.append([[px.get((x - 16, y - 28), 0) for x in range(PW)] for y in range(PH)])
    return side_by_side(cells, PW)


def header(offsets=None):
    """inc/ninja_frames.h: frame names and where each frame's extension cells go."""
    lines = ["// Generated by tools/art/make_art.py from tools/art/ninja.py: do not edit.",
             "#ifndef NINJA_FRAMES_H", "#define NINJA_FRAMES_H", "", "enum", "{"]
    lines += ["    NINJA_%s," % name.upper() for name in ORDER]
    lines += ["    NINJA_FRAME_COUNT", "};", "",
              "// Per frame: number of 32 x 32 extension cells, index of the first one in the extension",
              "// sheet, then their offsets from the top-left of the 32 x 48 body cell (facing right).",
              "#define NINJA_EXT_TABLE \\", "{ \\"]
    first = 0
    for name in ORDER:
        e = offsets[name] if offsets else POSES[name].get("ext", [])
        xs = [e[k][0] if k < len(e) else 0 for k in range(3)]
        ys = [e[k][1] if k < len(e) else 0 for k in range(3)]
        lines.append("    { %d, %d, { %d, %d, %d }, { %d, %d, %d } }, /* %s */ \\" % ((len(e), first) + tuple(xs) + tuple(ys) + (name,)))
        first += len(e)
    lines += ["}", "", "#endif", ""]
    return "\n".join(lines)

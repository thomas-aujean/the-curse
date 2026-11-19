#include "game_player.h"

// Shinobi: fast, high jump, little defence. Values tuned at 60 Hz (1/256 px per frame).
typedef struct
{
    s16 runMax, accel, turn, decel, airAccel, airDecel;
    s16 gravity, jumpVel, jumpCut, maxFall;
    u8 coyote, buffer, dropFrames, attackFrames, attackFrom, attackTo;
    u8 invulFrames, hurtFrames;
    s16 knockX, knockY, bounceVel, recoil;
    u16 lightMax;
} Tuning;

static const Tuning NTSC =
{
    // Scaled x1.25 for the 32 x 48 hero (Thomas, 2026-11-16: same speed looked slow on a bigger body),
    // then the whole game a little faster (2026-11-17): speeds x1.12-1.15, gravity x1.25 so the jumps
    // keep their height and take less time.
    .runMax = 1000,    // 3.9 px/frame
    .accel = 104,      // full speed in ~10 frames
    .turn = 207,       // turning back bites harder than starting
    .decel = 138,
    .airAccel = 69,
    .airDecel = 12,
    .gravity = 125,
    .jumpVel = 2530,   // v^2 / 2g = 100 px
    .jumpCut = 716,    // releasing C caps the rise speed: short hops
    .maxFall = 2150,
    .coyote = 6,       // can still jump a few frames after leaving a ledge
    .buffer = 6,       // a jump pressed just before landing is not lost
    .dropFrames = 10,
    // Hollow Knight-like slash (Thomas, 2026-11-17: "where timing really counts"): a short wind-up, the
    // crescent hits for 4 frames only, then a recovery where no new slash can start: a missed blow costs.
    // The crescent shows only while the blow hits (Thomas: "shorter, for a more precise timing").
    .attackFrames = 16,
    .attackFrom = 3,   // hitbox active on frames [from, to) of the slash
    .attackTo = 7,
    .invulFrames = 90,  // 1.5 s of blinking after a hit
    .hurtFrames = 14,
    .knockX = 500,
    .knockY = 858,
    .bounceVel = 1430,  // ~40 px hop after a downward hit
    .recoil = 716,      // a side slash that lands pushes the hero back a little
    .lightMax = 20 * 60, // 20 s of protection inside the Void
};

static Tuning t;
static u8 isPal;

void player_setRegion(u8 pal)
{
    t = NTSC;
    isPal = pal;
    if (!pal) return;
    // 50 Hz: speeds x 6/5, accelerations x 36/25, durations x 5/6. Load-time divisions only.
    t.runMax = NTSC.runMax * 6 / 5;
    t.jumpVel = NTSC.jumpVel * 6 / 5;
    t.jumpCut = NTSC.jumpCut * 6 / 5;
    t.maxFall = NTSC.maxFall * 6 / 5;
    t.accel = NTSC.accel * 36 / 25;
    t.turn = NTSC.turn * 36 / 25;
    t.decel = NTSC.decel * 36 / 25;
    t.airAccel = NTSC.airAccel * 36 / 25;
    t.airDecel = NTSC.airDecel * 36 / 25;
    t.gravity = NTSC.gravity * 36 / 25;
    t.coyote = NTSC.coyote * 5 / 6;
    t.buffer = NTSC.buffer * 5 / 6;
    t.dropFrames = NTSC.dropFrames * 5 / 6;
    t.attackFrames = NTSC.attackFrames * 5 / 6;
    t.attackFrom = NTSC.attackFrom * 5 / 6;
    t.attackTo = NTSC.attackTo * 5 / 6;
    t.invulFrames = NTSC.invulFrames * 5 / 6;
    t.hurtFrames = NTSC.hurtFrames * 5 / 6;
    t.knockX = NTSC.knockX * 6 / 5;
    t.knockY = NTSC.knockY * 6 / 5;
    t.bounceVel = NTSC.bounceVel * 6 / 5;
    t.recoil = NTSC.recoil * 6 / 5;
    t.lightMax = 20 * 50;
}

void player_init(Player *p, s16 px, s16 py)
{
    p->x = (s32) px << 8;
    p->y = (s32) py << 8;
    p->vx = p->vy = 0;
    p->onGround = p->facingLeft = p->crouching = p->jumpHeld = 0;
    p->coyote = p->jumpBuffer = p->dropTimer = p->attackTimer = p->attackDir = 0;
    p->prevPad = 0;
    p->attackId = 0;
    p->maxHp = p->hp = 6;
    p->grey = 0;
    p->invulnerable = p->hurtTimer = 0;
    p->light = p->lightMax = t.lightMax;
    p->corrupted = 0;
    p->owned = 0;
    p->runBonus = p->jumpBonus = 0;
    p->reachBonus = p->airJumps = p->airJumpsLeft = 0;
    p->lightDrain = 1;
    p->damageBonus = 0;
    p->coins = 0;
    p->keys = 0;
}

void player_place(Player *p, s16 px, s16 py)
{
    p->x = (s32) px << 8;
    p->y = (s32) py << 8;
    p->vx = p->vy = 0;
    p->onGround = p->crouching = p->jumpHeld = 0;
    p->coyote = p->jumpBuffer = p->dropTimer = p->attackTimer = 0;
    p->hurtTimer = 0;
    p->light = p->lightMax;
    p->corrupted = 0;
}

static s16 scaleSpeed(s16 v)
{
    return isPal ? v * 6 / 5 : v;
}

void player_takeItem(Player *p, u8 item)
{
    switch (item)
    {
        case ITEM_HEART:
            p->hp += 2;
            break;
        case ITEM_STONE_HEART:   // a grey heart, as Isaac's soul hearts (Thomas, 2026-11-19)
            p->grey += 2;
            if (p->maxHp + p->grey > MAX_HEARTS * 2) p->grey = MAX_HEARTS * 2 - p->maxHp;
            break;
        case ITEM_WIND_BOOTS:
            p->runBonus += scaleSpeed(112);
            break;
        case ITEM_RAVEN_WINGS:
            p->airJumps = 1;
            break;
        case ITEM_LONG_BLADE:
            p->reachBonus += 10;
            break;
        case ITEM_LANTERN:
            p->lightMax += t.lightMax / 2;
            p->light = p->lightMax;
            break;
        case ITEM_GLASS_HEART:
            if (p->maxHp > 2) p->maxHp -= 2;
            p->damageBonus++;
            break;
        case ITEM_LEAD_BOOTS:
            p->runBonus -= scaleSpeed(112);
            break;
        case ITEM_VOID_BLOOD:
            p->lightDrain = 2;
            break;
    }
    if (item >= ITEM_FIRST_UPGRADE && item < ITEM_COUNT) p->owned |= 1 << (item - ITEM_FIRST_UPGRADE);
    if (p->hp > p->maxHp) p->hp = p->maxHp;
}

const char *player_itemName(u8 item)
{
    static const char *const NAMES[ITEM_COUNT] =
    {
        "", "COEUR",
        "COEUR DE PIERRE", "BOTTES DE VENT", "AILES DE CORBEAU", "LAME LONGUE",
        "LANTERNE", "COEUR DE VERRE", "BOTTES DE PLOMB", "SANG DU NEANT",
    };
    return item < ITEM_COUNT ? NAMES[item] : "";
}

// Isaac-style: the items are known by their look, a short line hints at what one does.
const char *player_itemPhrase(u8 item)
{
    static const char *const PHRASES[ITEM_COUNT] =
    {
        "", "LA VIE REVIENT",
        "LOURD MAIS SOLIDE", "LE VENT TE PORTE", "PRENDS TON ENVOL", "LA LAME S'ALLONGE",
        "LA NUIT RECULE", "FRAGILE ET FEROCE", "CHAQUE PAS PESE", "LE NEANT A SOIF",
    };
    return item < ITEM_COUNT ? PHRASES[item] : "";
}

static u8 solidAt(const Room *r, s16 px, s16 py)
{
    return room_tileAt(r, px, py) == TILE_SOLID;
}

// Is any tile of the vertical segment [top, bottom] at column px solid? Steps of one tile.
static u8 solidColumn(const Room *r, s16 px, s16 top, s16 bottom)
{
    for (s16 py = top; py < bottom; py += ROOM_TILE)
        if (solidAt(r, px, py)) return 1;
    return solidAt(r, px, bottom);
}

static u8 rowBlocks(const Room *r, s16 left, s16 right, s16 py, u8 withPlatforms)
{
    for (s16 px = left;; px += ROOM_TILE)
    {
        if (px > right) px = right;
        u8 tile = room_tileAt(r, px, py);
        if (tile == TILE_SOLID || (withPlatforms && tile == TILE_PLATFORM)) return 1;
        if (px == right) return 0;
    }
}

static s16 approach(s16 v, s16 target, s16 step)
{
    if (v < target) return v + step > target ? target : v + step;
    if (v > target) return v - step < target ? target : v - step;
    return v;
}

static void moveX(Player *p, const Room *r, s16 h)
{
    p->x += p->vx;
    s16 px = p->x >> 8, top = (p->y >> 8) - h, bottom = (p->y >> 8) - 1;
    if (p->vx > 0 && solidColumn(r, px + PLAYER_W / 2 - 1, top, bottom))
    {
        px = ((px + PLAYER_W / 2 - 1) & ~7) - PLAYER_W / 2;
        p->x = (s32) px << 8;
        p->vx = 0;
    }
    else if (p->vx < 0 && solidColumn(r, px - PLAYER_W / 2, top, bottom))
    {
        px = ((px - PLAYER_W / 2) & ~7) + ROOM_TILE + PLAYER_W / 2;
        p->x = (s32) px << 8;
        p->vx = 0;
    }
}

static void moveY(Player *p, const Room *r, s16 h)
{
    s16 oldFeet = (p->y >> 8) - 1;
    p->y += p->vy;
    s16 px = p->x >> 8, py = p->y >> 8;
    s16 left = px - PLAYER_W / 2, right = px + PLAYER_W / 2 - 1;

    p->onGround = 0;
    if (p->vy >= 0)
    {
        s16 feet = py - 1;
        // A platform only catches feet that were above its top row on the previous frame.
        u8 platforms = !p->dropTimer && (oldFeet >> 3) < (feet >> 3);
        if (rowBlocks(r, left, right, feet, 0) || (platforms && rowBlocks(r, left, right, feet, 1)))
        {
            py = feet & ~7;
            p->y = (s32) py << 8;
        }
        // Resting on the pixel row just below the body (a platform only by its top row). Probing
        // every frame keeps onGround stable while gravity adds sub-pixel speed.
        if (rowBlocks(r, left, right, py, !p->dropTimer && !(py & 7)))
        {
            p->y = (s32) py << 8;
            p->vy = 0;
            p->onGround = 1;
        }
    }
    else
    {
        s16 head = py - h;
        if (rowBlocks(r, left, right, head, 0))
        {
            // A PAL jump starts above 8 px per frame: the head may be more than one tile into the
            // ceiling, so step down tile by tile until it is out (2 steps at most).
            head = (head & ~7) + ROOM_TILE;
            while (head < py && rowBlocks(r, left, right, head, 0)) head += ROOM_TILE;
            p->y = (s32) (head + h) << 8;
            p->vy = 0;
        }
    }
}

void player_update(Player *p, const Room *r, u16 pad)
{
    if (p->invulnerable) p->invulnerable--;
    if (p->hurtTimer)
    {
        p->hurtTimer--;
        pad = 0;   // knocked back: no control
    }
    u16 pressed = pad & ~p->prevPad;
    p->prevPad = pad;
    p->upPressed = (pressed & PAD_UP) != 0;

    if (p->dropTimer) p->dropTimer--;
    if (p->attackTimer) p->attackTimer--;
    if (p->jumpBuffer) p->jumpBuffer--;
    if (p->coyote) p->coyote--;
    if (p->onGround)
    {
        p->coyote = t.coyote;
        p->airJumpsLeft = p->airJumps;
    }

    if (pressed & PAD_ATTACK && !p->attackTimer)
    {
        p->attackTimer = t.attackFrames;
        p->attackId++;
        // Direction is locked for the whole slash. Down only in the air (on the ground it is a crouch).
        p->attackDir = (pad & PAD_UP) ? ATTACK_UP : ((pad & PAD_DOWN) && !p->onGround) ? ATTACK_DOWN : ATTACK_SIDE;
    }

    // Crouch; standing back up needs headroom.
    u8 wantCrouch = p->onGround && (pad & PAD_DOWN);
    if (wantCrouch) p->crouching = 1;
    else if (p->crouching)
    {
        s16 px = p->x >> 8, py = p->y >> 8;
        if (!rowBlocks(r, px - PLAYER_W / 2, px + PLAYER_W / 2 - 1, py - PLAYER_H, 0)) p->crouching = 0;
    }

    // Jump (buffered), or drop through a one-way platform with DOWN + C.
    if (pressed & PAD_JUMP) p->jumpBuffer = t.buffer;
    if (p->jumpBuffer && (pad & PAD_DOWN) && p->onGround)
    {
        s16 px = p->x >> 8, py = p->y >> 8;
        // Standing half on the plank, half on the stone beside it (a trapdoor is only 32 px wide):
        // slide onto the plank instead of refusing the drop.
        if (room_tileAt(r, px, py) == TILE_PLATFORM)
            for (s16 shift = 1; shift <= PLAYER_W / 2 && rowBlocks(r, px - PLAYER_W / 2, px + PLAYER_W / 2 - 1, py, 0); shift++)
            {
                if (!rowBlocks(r, px - shift - PLAYER_W / 2, px - shift + PLAYER_W / 2 - 1, py, 0)) px -= shift;
                else if (!rowBlocks(r, px + shift - PLAYER_W / 2, px + shift + PLAYER_W / 2 - 1, py, 0)) px += shift;
                else continue;
                p->x = (s32) px << 8;
                p->vx = 0;
            }
        if (!rowBlocks(r, px - PLAYER_W / 2, px + PLAYER_W / 2 - 1, py, 0))
        {
            p->dropTimer = t.dropFrames;
            p->jumpBuffer = 0;
            p->onGround = 0;
            p->coyote = 0;
            p->crouching = 0;
        }
    }
    u8 grounded = p->onGround || p->coyote;
    if (p->jumpBuffer && !p->crouching && (grounded || p->airJumpsLeft))
    {
        if (!grounded) p->airJumpsLeft--;   // Raven wings: a second jump in the air
        p->vy = -(t.jumpVel + p->jumpBonus);
        p->jumpBuffer = 0;
        p->coyote = 0;
        p->onGround = 0;
        p->jumpHeld = 1;
    }
    if (p->jumpHeld && !(pad & PAD_JUMP))
    {
        if (p->vy < -t.jumpCut) p->vy = -t.jumpCut;
        p->jumpHeld = 0;
    }
    if (p->vy >= 0) p->jumpHeld = 0;

    // Run.
    s16 dir = 0;
    if (!p->crouching)
    {
        if (pad & PAD_LEFT) dir = -1;
        else if (pad & PAD_RIGHT) dir = 1;
    }
    if (dir)
    {
        p->facingLeft = dir < 0;
        s16 a = p->onGround ? ((p->vx && (p->vx < 0) != (dir < 0)) ? t.turn : t.accel) : t.airAccel;
        s16 runMax = t.runMax + p->runBonus;
        p->vx = approach(p->vx, dir * runMax, a);
    }
    else p->vx = approach(p->vx, 0, p->onGround ? t.decel : t.airDecel);

    // Gravity.
    p->vy += t.gravity;
    if (p->vy > t.maxFall) p->vy = t.maxFall;

    s16 h = p->crouching ? PLAYER_CROUCH_H : PLAYER_H;
    moveX(p, r, h);
    moveY(p, r, h);
}

u8 player_attackActive(const Player *p)
{
    u8 elapsed = t.attackFrames - p->attackTimer;
    return p->attackTimer && elapsed >= t.attackFrom && elapsed < t.attackTo;
}

u8 player_attackTimer(const Player *p)
{
    return p->attackTimer;
}

// Slash hitboxes follow the drawn crescents (tools/art/knight.py, ARCS) and start inside the body, so an
// enemy pressed against the hero is still hit.
void player_attackBox(const Player *p, s16 *x0, s16 *y0, s16 *x1, s16 *y1)
{
    s16 px = p->x >> 8, py = p->y >> 8;
    if (p->attackDir == ATTACK_UP)
    {
        // Over the head, from one shoulder to the other.
        *x0 = px - 27;
        *x1 = px + 26;
        *y0 = py - 62;
        *y1 = py - 36;
    }
    else if (p->attackDir == ATTACK_DOWN)
    {
        *x0 = px - 25;
        *x1 = px + 24;
        *y0 = py - 8;
        *y1 = py + 14;
    }
    else
    {
        // Standing, the crescent sweeps from above the head down to the feet (low enemies get hit).
        s16 reach = (p->crouching ? 28 : 32) + p->reachBonus;
        *y0 = py - (p->crouching ? 34 : 58);
        *y1 = py - 2;
        *x0 = p->facingLeft ? px - reach : px - 4;
        *x1 = p->facingLeft ? px + 3 : px + reach - 1;
    }
}
void player_boostUp(Player *p, u8 height)
{
    // v^2 = 2 g h (1/256 px units: v^2 = 2 * gravity * h * 256), integer square root.
    u32 v2 = (u32) 512 * t.gravity * height, lo = 0, hi = 4095;
    while (lo < hi)
    {
        u32 mid = (lo + hi + 1) / 2;
        if (mid * mid <= v2) lo = mid;
        else hi = mid - 1;
    }
    p->vy = lo < (u32) t.jumpVel ? -(s16) lo : -t.jumpVel;
    p->jumpHeld = 0;
    p->onGround = 0;
}

void player_recoil(Player *p)
{
    p->vx = p->facingLeft ? t.recoil : -t.recoil;
}

void player_bounce(Player *p)
{
    p->vy = -t.bounceVel;
    p->jumpHeld = 0;
    p->onGround = 0;
}

void player_hurt(Player *p, u8 damage, u8 fromRight)
{
    if (p->invulnerable || p->hp <= 0) return;
    s8 d = damage + p->corrupted;   // the pact's price: one more half heart per hit
    s8 g = d < p->grey ? d : p->grey;   // the grey hearts go first
    p->grey -= g;
    p->hp -= d - g;
    if (p->hp < 0) p->hp = 0;
    p->invulnerable = t.invulFrames;
    p->hurtTimer = t.hurtFrames;
    p->vx = fromRight ? -t.knockX : t.knockX;
    p->vy = -t.knockY;
    p->onGround = 0;
    p->crouching = 0;
    p->attackTimer = 0;
}


void player_updateLight(Player *p, u8 inVoid)
{
    if (inVoid)
    {
        p->light = p->light > p->lightDrain ? p->light - p->lightDrain : 0;
        if (!p->light) p->corrupted = 1;
    }
    else
    {
        // Refills twice as fast as it drains; the Void lets go once a quarter is back.
        p->light += 2;
        if (p->light > p->lightMax) p->light = p->lightMax;
        if (p->light >= p->lightMax / 4) p->corrupted = 0;
    }
}

u8 player_slashDamage(const Player *p)
{
    s8 d = (p->corrupted ? 2 : 1) + p->damageBonus;
    return d < 1 ? 1 : d;
}
u8 player_attackPhase(const Player *p)
{
    u8 elapsed = t.attackFrames - p->attackTimer;
    return elapsed < t.attackFrom ? 0 : elapsed < t.attackTo ? 1 : 2;
}

u8 player_attackStep(const Player *p)
{
    u8 elapsed = t.attackFrames - p->attackTimer;
    if (elapsed < t.attackFrom) return 0;
    if (elapsed < t.attackTo) return elapsed < (t.attackFrom + t.attackTo) / 2 ? 1 : 2;   // the crescent
    return elapsed < (t.attackTo + t.attackFrames) / 2 ? 3 : 4;                            // recovery
}

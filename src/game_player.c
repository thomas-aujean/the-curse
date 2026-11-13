#include "game_player.h"

// Shinobi: fast, high jump, little defence. Values tuned at 60 Hz (1/256 px per frame).
typedef struct
{
    s16 runMax, accel, turn, decel, airAccel, airDecel;
    s16 gravity, jumpVel, jumpCut, maxFall;
    u8 coyote, buffer, dropFrames, attackFrames, attackFrom, attackTo;
} Tuning;

static const Tuning NTSC =
{
    .runMax = 704,     // 2.75 px/frame
    .accel = 72,       // full speed in ~10 frames
    .turn = 144,       // turning back bites harder than starting
    .decel = 96,
    .airAccel = 48,
    .airDecel = 8,
    .gravity = 80,
    .jumpVel = 1810,   // v^2 / 2g = 80 px (10 tiles): "saute haut"
    .jumpCut = 512,    // releasing C caps the rise speed: short hops
    .maxFall = 1536,
    .coyote = 6,       // can still jump a few frames after leaving a ledge
    .buffer = 6,       // a jump pressed just before landing is not lost
    .dropFrames = 10,
    .attackFrames = 14,
    .attackFrom = 2,   // hitbox active on frames [from, to) of the slash
    .attackTo = 8,
};

static Tuning t;

void player_setRegion(u8 pal)
{
    t = NTSC;
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
}

void player_init(Player *p, s16 px, s16 py)
{
    p->x = (s32) px << 8;
    p->y = (s32) py << 8;
    p->vx = p->vy = 0;
    p->onGround = p->facingLeft = p->crouching = p->jumpHeld = 0;
    p->coyote = p->jumpBuffer = p->dropTimer = p->attackTimer = 0;
    p->prevPad = 0;
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
            p->y = (s32) (((head & ~7) + ROOM_TILE + h)) << 8;
            p->vy = 0;
        }
    }
}

void player_update(Player *p, const Room *r, u16 pad)
{
    u16 pressed = pad & ~p->prevPad;
    p->prevPad = pad;

    if (p->dropTimer) p->dropTimer--;
    if (p->attackTimer) p->attackTimer--;
    if (p->jumpBuffer) p->jumpBuffer--;
    if (p->coyote) p->coyote--;
    if (p->onGround) p->coyote = t.coyote;

    if (pressed & PAD_ATTACK && !p->attackTimer) p->attackTimer = t.attackFrames;

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
        if (!rowBlocks(r, px - PLAYER_W / 2, px + PLAYER_W / 2 - 1, py, 0))
        {
            p->dropTimer = t.dropFrames;
            p->jumpBuffer = 0;
            p->onGround = 0;
            p->coyote = 0;
            p->crouching = 0;
        }
    }
    if (p->jumpBuffer && (p->onGround || p->coyote) && !p->crouching)
    {
        p->vy = -t.jumpVel;
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
        p->vx = approach(p->vx, dir * t.runMax, a);
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

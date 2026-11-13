#include "game_enemy.h"

typedef struct
{
    s16 speed, chase;    // 1/256 px per frame; walker chase = 0: never charges; boss chase = dash speed
    s16 range;           // px: how close the player must be to be noticed
    s8 hp;
    u8 damage;
} EnemyStats;

// [type][voidForm], NTSC values.
static const EnemyStats STATS[ENEMY_TYPES][2] =
{
    { { 0 }, { 0 } },
    { { 154, 0, 0, 2, 1 }, { 256, 640, 128, 4, 2 } },        // walker: patrols; Void form charges
    { { 192, 0, 112, 1, 1 }, { 384, 0, 160, 2, 2 } },        // flyer: homes in on the player
    { { 192, 1024, 0, 16, 2 }, { 256, 1280, 0, 24, 3 } },    // boss: dash, leap, fan of shots
    { { 0, 768, 200, 2, 1 }, { 0, 384, 240, 3, 2 } },        // archer: chase = arrow / spore speed
    { { 384, 640, 160, 2, 1 }, { 512, 1024, 200, 3, 2 } },   // ninja: chase = shuriken / lunge speed
    { { 128, 0, 160, 3, 1 }, { 256, 0, 200, 6, 2 } },        // guard
    { { 256, 0, 160, 2, 1 }, { 448, 0, 200, 2, 2 } },        // jelly
    { { 320, 0, 160, 1, 1 }, { 512, 0, 200, 1, 1 } },        // small jelly
};

static s16 speedScale6, gravity, maxFall, knockback, leapVel, shotSpeed, shotSpread;
static u8 hurtFrames, deathFrames, bossIdle, bossWarn;
static u8 archerDelay, archerVoidDelay, ninjaDelay, ninjaVoidDelay, jellyDelay;
static s16 ninjaJump, jellyJump, homingAccel;
static u8 guardTurn;
static u16 sporeLife;

void enemies_setRegion(u8 isPal)
{
    // 50 Hz: speeds x 6/5, accelerations x 36/25, durations x 5/6 (same rule as the player).
    gravity = isPal ? 80 * 36 / 25 : 80;
    maxFall = isPal ? 1536 * 6 / 5 : 1536;
    knockback = isPal ? 512 * 6 / 5 : 512;
    leapVel = isPal ? 1700 * 6 / 5 : 1700;
    shotSpeed = isPal ? 512 * 6 / 5 : 512;
    shotSpread = isPal ? 144 * 6 / 5 : 144;
    hurtFrames = isPal ? 10 * 5 / 6 : 10;
    deathFrames = isPal ? 16 * 5 / 6 : 16;
    bossIdle = isPal ? 50 * 5 / 6 : 50;
    bossWarn = isPal ? 36 * 5 / 6 : 36;
    speedScale6 = isPal ? 6 : 5;
    archerDelay = isPal ? 90 * 5 / 6 : 90;
    archerVoidDelay = isPal ? 120 * 5 / 6 : 120;
    ninjaDelay = isPal ? 70 * 5 / 6 : 70;
    ninjaVoidDelay = isPal ? 45 * 5 / 6 : 45;
    jellyDelay = isPal ? 50 * 5 / 6 : 50;
    ninjaJump = isPal ? 1500 * 6 / 5 : 1500;
    jellyJump = isPal ? 900 * 6 / 5 : 900;
    homingAccel = isPal ? 24 * 36 / 25 : 24;
    sporeLife = isPal ? 240 * 5 / 6 : 240;
    guardTurn = isPal ? 40 * 5 / 6 : 40;
}

static s16 speedOf(s16 v)
{
    return v * speedScale6 / 5;
}

void enemy_size(u8 type, s16 *w, s16 *h)
{
    static const u8 SIZE[ENEMY_TYPES][2] =
    {
        { 0, 0 }, { 14, 22 }, { 14, 12 }, { 28, 44 }, { 14, 22 }, { 12, 28 }, { 16, 22 }, { 14, 10 }, { 8, 7 },
    };
    *w = SIZE[type][0];
    *h = SIZE[type][1];
}

void enemies_clear(EnemySet *s)
{
    for (u8 i = 0; i < MAX_ENEMIES; i++) s->e[i].type = ENEMY_NONE;
    for (u8 i = 0; i < MAX_SHOTS; i++) s->shots[i].active = 0;
}

Enemy *enemies_add(EnemySet *s, u8 type, s16 px, s16 py, u8 voidForm)
{
    for (u8 i = 0; i < MAX_ENEMIES; i++)
    {
        Enemy *e = &s->e[i];
        if (e->type != ENEMY_NONE) continue;
        e->type = type;
        e->x = e->homeX = (s32) px << 8;
        e->y = e->homeY = (s32) py << 8;
        e->vx = e->vy = 0;
        e->facingLeft = i & 1;
        e->hurtTimer = e->deathTimer = 0;
        e->lastHitId = 0xFF;
        e->looted = 0;
        e->timer = i * 37;   // desynchronise the bobbing
        e->state = BOSS_IDLE;
        e->step = 0;
        e->stateTimer = type == ENEMY_BOSS ? bossIdle : 30 + i * 11;   // staggered first actions
        e->voidForm = 0;
        e->hp = STATS[type][0].hp;
        if (voidForm) enemy_setVoidForm(e, 1);
        return e;
    }
    return 0;
}

void enemies_setVoid(EnemySet *s, u8 voidForm)
{
    for (u8 i = 0; i < MAX_ENEMIES; i++) enemy_setVoidForm(&s->e[i], voidForm);
}

void enemy_setVoidForm(Enemy *e, u8 voidForm)
{
    if (e->type == ENEMY_NONE || e->deathTimer || e->voidForm == voidForm) return;
    // Keep the damage already taken.
    s8 lost = STATS[e->type][e->voidForm].hp - e->hp;
    e->voidForm = voidForm;
    e->hp = STATS[e->type][voidForm].hp - lost;
    if (e->hp < 1) e->hp = 1;
}

static s32 absS32(s32 v)
{
    return v < 0 ? -v : v;
}

static s16 approach(s16 v, s16 target, s16 step)
{
    if (v < target) return v + step > target ? target : v + step;
    if (v > target) return v - step < target ? target : v - step;
    return v;
}

static u8 isSolid(const Room *r, s16 x, s16 y)
{
    return room_tileAt(r, x, y) == TILE_SOLID;
}

// Walking body: horizontal move blocked by walls, gravity, landing on solids and platforms.
// Returns bit 0 = hit a wall, bit 1 = on the ground.
static u8 moveBody(Enemy *e, const Room *r, s16 w, s16 h)
{
    u8 result = 0;
    s16 py = e->y >> 8;
    e->x += e->vx;
    s16 px = e->x >> 8;
    s16 front = px + (e->vx > 0 ? w / 2 : -w / 2);
    if (e->vx && (isSolid(r, front, py - 4) || isSolid(r, front, py - h + 2)))
    {
        e->x -= e->vx;
        px = e->x >> 8;
        e->vx = 0;
        result |= 1;
    }

    s16 oldFeet = py - 1;
    e->vy += gravity;
    if (e->vy > maxFall) e->vy = maxFall;
    e->y += e->vy;
    py = e->y >> 8;
    s16 feet = py - 1;
    if (e->vy < 0)
    {
        if (isSolid(r, px, py - h))
        {
            e->y = (s32) ((((py - h) & ~7) + 8 + h)) << 8;
            e->vy = 0;
        }
        return result;
    }
    u8 t = room_tileAt(r, px, feet);
    if (t == TILE_SOLID || (t == TILE_PLATFORM && (oldFeet >> 3) < (feet >> 3)))
    {
        e->y = (s32) (feet & ~7) << 8;
        e->vy = 0;
        result |= 2;
    }
    else if (!(py & 7) && room_tileAt(r, px, py) != TILE_EMPTY)
    {
        e->vy = 0;   // resting on the row below
        result |= 2;
    }
    return result;
}

static void updateWalker(Enemy *e, const EnemyStats *st, const Room *r, const Player *p)
{
    s16 w, h;
    enemy_size(e->type, &w, &h);
    s16 px = e->x >> 8, py = e->y >> 8;

    if (!e->hurtTimer)
    {
        s16 speed = speedOf(st->speed);
        s32 dx = (p->x - e->x) >> 8, dy = (p->y - e->y) >> 8;
        if (st->chase && absS32(dy) < 24 && absS32(dx) < st->range)
        {
            e->facingLeft = dx < 0;
            speed = speedOf(st->chase);
        }
        s16 dir = e->facingLeft ? -1 : 1;
        s16 ahead = px + dir * (w / 2 + 1);
        u8 ground = room_tileAt(r, ahead, py);
        // Turn back at walls and ledges (never walk off a platform).
        if (isSolid(r, ahead, py - 4) || isSolid(r, ahead, py - h + 2) || ground == TILE_EMPTY)
        {
            e->facingLeft = !e->facingLeft;
            e->vx = 0;
        }
        else e->vx = dir * speed;
    }
    else e->vx = approach(e->vx, 0, 32);
    moveBody(e, r, w, h);
}

static void updateFlyer(Enemy *e, const EnemyStats *st, const Player *p)
{
    s16 speed = speedOf(st->speed);
    s16 accel = speed / 8 + 1;
    s32 tx, ty;
    s32 dx = (p->x - e->x) >> 8, dy = ((p->y - (16 << 8)) - e->y) >> 8;   // aim at the chest

    if (absS32(dx) < st->range && absS32(dy) < st->range)
    {
        tx = dx;
        ty = dy;
    }
    else
    {
        // Idle: bob around the spawn point (triangle wave, no sine table needed).
        u16 phase = e->timer & 63;
        s16 bob = phase < 32 ? phase - 16 : 48 - phase;
        tx = (e->homeX - e->x) >> 8;
        ty = ((e->homeY - e->y) >> 8) + bob;
    }
    if (!e->hurtTimer)
    {
        e->vx = approach(e->vx, tx > 2 ? speed : tx < -2 ? -speed : 0, accel);
        e->vy = approach(e->vy, ty > 2 ? speed : ty < -2 ? -speed : 0, accel);
        if (tx) e->facingLeft = tx < 0;
    }
    else
    {
        e->vx = approach(e->vx, 0, 32);
        e->vy = approach(e->vy, 0, 32);
    }
    e->x += e->vx;
    e->y += e->vy;   // spirits drift through inner walls (kept inside the room by enemies_update)
}

static void fireShots(EnemySet *s, const Enemy *e, u8 count, u8 damage)
{
    s16 dir = e->facingLeft ? -1 : 1;
    u8 fired = 0;
    for (u8 i = 0; i < MAX_SHOTS && fired < count; i++)
    {
        Shot *sh = &s->shots[i];
        if (sh->active) continue;
        sh->active = 1;
        sh->x = e->x + ((s32) (dir * 16) << 8);
        sh->y = e->y - (28 << 8);
        sh->vx = dir * shotSpeed;
        sh->vy = ((s16) fired * 2 - (count - 1)) * shotSpread / 2;   // fan, centred on the horizon
        sh->damage = damage;
        sh->kind = SHOT_ORB;
        sh->life = 0xFFFF;
        fired++;
    }
}

// Guardian of the gem: idle, then cycles dash (announced by flashing), leap onto the player,
// fan of shots. The Void form dashes faster and fires more shots.
static void updateBoss(EnemySet *s, Enemy *e, const EnemyStats *st, const Room *r, const Player *p)
{
    s16 w, h;
    enemy_size(e->type, &w, &h);
    s32 dx = (p->x - e->x) >> 8;
    s16 dir = e->facingLeft ? -1 : 1;
    u8 moved;

    switch (e->state)
    {
        case BOSS_IDLE:
            e->facingLeft = dx < 0;
            e->vx = approach(e->vx, (dx < 0 ? -1 : 1) * speedOf(st->speed), 16);
            moveBody(e, r, w, h);
            if (--e->stateTimer) break;
            e->step = (e->step + 1) % 3;
            if (e->step == 0)
            {
                e->state = BOSS_DASH_WARN;
                e->stateTimer = bossWarn;
            }
            else if (e->step == 1)
            {
                e->state = BOSS_LEAP;
                e->vy = -leapVel;
                // Land roughly on the player: about 40 frames of air, so vx ~ dx * 256 / 40.
                s32 vx = dx * 6;
                s16 maxVx = speedOf(st->chase);
                e->vx = vx > maxVx ? maxVx : vx < -maxVx ? -maxVx : vx;
            }
            else
            {
                fireShots(s, e, e->voidForm ? 5 : 3, st->damage);
                e->stateTimer = bossIdle;
            }
            break;

        case BOSS_DASH_WARN:
            e->vx = 0;
            moveBody(e, r, w, h);
            if (--e->stateTimer == 0)
            {
                e->state = BOSS_DASH;
                e->vx = dir * speedOf(st->chase);
            }
            break;

        case BOSS_DASH:
            e->vx = dir * speedOf(st->chase);
            if (moveBody(e, r, w, h) & 1)
            {
                e->state = BOSS_IDLE;
                e->stateTimer = bossIdle;
            }
            break;

        case BOSS_LEAP:
            moved = moveBody(e, r, w, h);
            if ((moved & 2) && e->vy == 0)
            {
                e->vx = 0;
                e->state = BOSS_IDLE;
                e->stateTimer = bossIdle;
            }
            break;
    }
}

static Shot *newShot(EnemySet *s)
{
    for (u8 i = 0; i < MAX_SHOTS; i++)
        if (!s->shots[i].active) return &s->shots[i];
    return 0;
}

static void faceAndStand(Enemy *e, const Room *r, const Player *p)
{
    s16 w, h;
    enemy_size(e->type, &w, &h);
    e->facingLeft = p->x < e->x;
    e->vx = approach(e->vx, 0, 32);
    moveBody(e, r, w, h);
}

// Archer: stands still and shoots when the player is roughly level with it.
// Void bulb: spits a slow spore that steers towards the player for a few seconds.
static void updateArcher(EnemySet *s, Enemy *e, const EnemyStats *st, const Room *r, const Player *p)
{
    faceAndStand(e, r, p);
    if (e->hurtTimer || --e->stateTimer) return;
    e->stateTimer = e->voidForm ? archerVoidDelay : archerDelay;
    s32 dx = (p->x - e->x) >> 8, dy = (p->y - e->y) >> 8;
    if (absS32(dx) > st->range || (!e->voidForm && absS32(dy) > 40)) return;
    Shot *sh = newShot(s);
    if (!sh) return;
    s16 dir = e->facingLeft ? -1 : 1;
    sh->active = 1;
    sh->x = e->x + ((s32) (dir * 8) << 8);
    sh->y = e->y - (14 << 8);
    sh->vx = dir * speedOf(st->chase);
    sh->vy = 0;
    sh->damage = st->damage;
    sh->kind = e->voidForm ? SHOT_SPORE : SHOT_ARROW;
    sh->life = e->voidForm ? sporeLife : 0xFFFF;
}

// Ninja: closes in, then every so often leaps at the player; the normal one throws a shuriken
// as it leaps, the Void hunter lunges twice as fast instead.
static void updateNinja(EnemySet *s, Enemy *e, const EnemyStats *st, const Room *r, const Player *p)
{
    s16 w, h;
    enemy_size(e->type, &w, &h);
    s32 dx = (p->x - e->x) >> 8, dy = (p->y - e->y) >> 8;
    u8 grounded = e->vy == 0;
    if (!e->hurtTimer && grounded)
    {
        e->facingLeft = dx < 0;
        s16 dir = e->facingLeft ? -1 : 1;
        // Keep a fighting distance of about 48 px.
        s16 target = absS32(dx) > st->range ? 0 : absS32(dx) > 48 ? dir * speedOf(st->speed) : -dir * speedOf(st->speed) / 2;
        e->vx = approach(e->vx, target, 48);
        if (--e->stateTimer == 0 && absS32(dx) < st->range)
        {
            e->stateTimer = e->voidForm ? ninjaVoidDelay : ninjaDelay;
            e->vy = -ninjaJump;
            e->vx = dir * (e->voidForm ? speedOf(st->chase) : speedOf(st->speed));
            Shot *sh = e->voidForm ? 0 : newShot(s);
            if (sh)
            {
                s32 vy = dy * 3;
                sh->active = 1;
                sh->x = e->x;
                sh->y = e->y - (18 << 8);
                sh->vx = dir * speedOf(st->chase);
                sh->vy = vy > 512 ? 512 : vy < -512 ? -512 : vy;
                sh->damage = st->damage;
                sh->kind = SHOT_SHURIKEN;
                sh->life = 0xFFFF;
            }
        }
        else if (!e->stateTimer) e->stateTimer = 20;
    }
    else if (e->hurtTimer) e->vx = approach(e->vx, 0, 32);
    moveBody(e, r, w, h);
}

// Guard: walks slowly towards the player, turning back at ledges. Blocking is in enemies_update.
static void updateGuard(Enemy *e, const EnemyStats *st, const Room *r, const Player *p)
{
    s16 w, h;
    enemy_size(e->type, &w, &h);
    s16 px = e->x >> 8, py = e->y >> 8;
    if (!e->hurtTimer)
    {
        // Turning around takes a moment: that is the window to strike its back.
        s32 dx = (p->x - e->x) >> 8;
        u8 behind = absS32(dx) < st->range && (dx < 0) != e->facingLeft;
        if (!behind) e->stateTimer = guardTurn;
        else if (--e->stateTimer == 0)
        {
            e->facingLeft = !e->facingLeft;
            e->stateTimer = guardTurn;
        }
        s16 dir = e->facingLeft ? -1 : 1;
        s16 ahead = px + dir * (w / 2 + 1);
        if (isSolid(r, ahead, py - 4) || room_tileAt(r, ahead, py) == TILE_EMPTY) e->vx = 0;
        else e->vx = dir * speedOf(st->speed);
    }
    else e->vx = approach(e->vx, 0, 32);
    moveBody(e, r, w, h);
}

// Jelly: hops towards the player.
static void updateJelly(Enemy *e, const EnemyStats *st, const Room *r, const Player *p)
{
    s16 w, h;
    enemy_size(e->type, &w, &h);
    u8 grounded = e->vy == 0;
    if (grounded && !e->hurtTimer)
    {
        e->vx = approach(e->vx, 0, 64);
        if (--e->stateTimer == 0)
        {
            e->stateTimer = jellyDelay;
            e->facingLeft = p->x < e->x;
            e->vy = e->type == ENEMY_JELLY_SMALL ? -jellyJump * 3 / 4 : -jellyJump;
            e->vx = (e->facingLeft ? -1 : 1) * speedOf(st->speed);
        }
    }
    moveBody(e, r, w, h);
}

static void steerSpore(Shot *sh, const Player *p)
{
    s32 dx = (p->x - sh->x) >> 8, dy = ((p->y - (14 << 8)) - sh->y) >> 8;
    sh->vx = approach(sh->vx, dx > 0 ? 384 : -384, homingAccel);
    sh->vy = approach(sh->vy, dy > 0 ? 384 : -384, homingAccel);
}

static u8 overlap(s16 ax0, s16 ay0, s16 ax1, s16 ay1, s16 bx0, s16 by0, s16 bx1, s16 by1)
{
    return ax0 <= bx1 && bx0 <= ax1 && ay0 <= by1 && by0 <= ay1;
}

static void updateShots(EnemySet *s, const Room *r, Player *p, s16 px0, s16 py0, s16 px1, s16 py1)
{
    for (u8 i = 0; i < MAX_SHOTS; i++)
    {
        Shot *sh = &s->shots[i];
        if (!sh->active) continue;
        if (sh->kind == SHOT_SPORE) steerSpore(sh, p);
        if (sh->life != 0xFFFF && --sh->life == 0)
        {
            sh->active = 0;
            continue;
        }
        sh->x += sh->vx;
        sh->y += sh->vy;
        s16 x = sh->x >> 8, y = sh->y >> 8;
        if (x < 0 || y < 0 || x >= r->w * ROOM_TILE || y >= r->h * ROOM_TILE || isSolid(r, x, y))
        {
            sh->active = 0;
            continue;
        }
        if (overlap(x - 3, y - 3, x + 3, y + 3, px0, py0, px1, py1))
        {
            player_hurt(p, sh->damage, sh->vx < 0);
            sh->active = 0;
        }
    }
}

void enemies_update(EnemySet *s, const Room *r, Player *p)
{
    s->blocked = 0;
    s16 ax0 = 0, ay0 = 0, ax1 = 0, ay1 = 0;
    u8 slashing = player_attackActive(p);
    if (slashing) player_attackBox(p, &ax0, &ay0, &ax1, &ay1);

    s16 ppx = p->x >> 8, ppy = p->y >> 8;
    s16 ph = p->crouching ? PLAYER_CROUCH_H : PLAYER_H;
    s16 px0 = ppx - PLAYER_W / 2, px1 = ppx + PLAYER_W / 2 - 1, py0 = ppy - ph, py1 = ppy - 1;

    for (u8 i = 0; i < MAX_ENEMIES; i++)
    {
        Enemy *e = &s->e[i];
        if (e->type == ENEMY_NONE) continue;
        if (e->deathTimer)
        {
            if (--e->deathTimer == 0) e->type = ENEMY_NONE;
            continue;
        }
        const EnemyStats *st = &STATS[e->type][e->voidForm];
        e->timer++;
        if (e->hurtTimer) e->hurtTimer--;
        if (e->type == ENEMY_WALKER) updateWalker(e, st, r, p);
        else if (e->type == ENEMY_FLYER) updateFlyer(e, st, p);
        else if (e->type == ENEMY_BOSS) updateBoss(s, e, st, r, p);
        else if (e->type == ENEMY_ARCHER) updateArcher(s, e, st, r, p);
        else if (e->type == ENEMY_NINJA) updateNinja(s, e, st, r, p);
        else if (e->type == ENEMY_GUARD) updateGuard(e, st, r, p);
        else updateJelly(e, st, r, p);

        s16 w, h;
        enemy_size(e->type, &w, &h);
        // Everyone stays inside the room's outer walls (spirits drift through inner walls only).
        s32 minX = (s32) (16 + w / 2) << 8, maxX = (s32) (r->w * ROOM_TILE - 16 - w / 2) << 8;
        s32 minY = (s32) (16 + h) << 8, maxY = (s32) (r->h * ROOM_TILE - 16) << 8;
        if (e->x < minX) { e->x = minX; e->vx = 0; }
        if (e->x > maxX) { e->x = maxX; e->vx = 0; }
        if (e->y < minY) { e->y = minY; e->vy = 0; }
        if (e->y > maxY) { e->y = maxY; e->vy = 0; }
        s16 ex = e->x >> 8, ey = e->y >> 8;
        s16 x0 = ex - w / 2, x1 = ex + w / 2 - 1, y0 = ey - h, y1 = ey - 1;

        if (slashing && e->lastHitId != p->attackId && overlap(ax0, ay0, ax1, ay1, x0, y0, x1, y1))
        {
            e->lastHitId = p->attackId;
            if (e->type == ENEMY_GUARD)
            {
                // The shield covers the front against side slashes; the Void colossus also shrugs
                // off blows from above or below: only its back is vulnerable.
                u8 playerInFront = (p->x < e->x) == e->facingLeft;
                if (playerInFront && (p->attackDir == ATTACK_SIDE || e->voidForm))
                {
                    s->blocked = 1;
                    p->vx = p->x < e->x ? -knockback : knockback;   // pushed back by the shield
                    continue;
                }
            }
            e->hurtTimer = hurtFrames;
            if (e->type != ENEMY_BOSS)   // the boss is too heavy to be pushed around
            {
                e->vx = p->facingLeft ? -knockback : knockback;
                if (p->attackDir == ATTACK_UP) e->vy = -knockback;
                if (p->attackDir == ATTACK_DOWN) e->vy = knockback;
            }
            if (p->attackDir == ATTACK_DOWN) player_bounce(p);
            e->hp -= player_slashDamage(p);
            if (e->hp <= 0) e->deathTimer = e->type == ENEMY_BOSS ? deathFrames * 4 : deathFrames;
            else if (e->type == ENEMY_JELLY)
            {
                // Splits into small jellies (three in the Void) that jump apart.
                e->deathTimer = 1;
                e->looted = 1;   // the small ones carry the coins
                u8 parts = e->voidForm ? 3 : 2;
                for (u8 k = 0; k < parts; k++)
                {
                    Enemy *n = enemies_add(s, ENEMY_JELLY_SMALL, ex - 8 + k * 8, ey, e->voidForm);
                    if (!n) break;
                    n->vy = -jellyJump / 2;
                    n->vx = (k * 2 - (parts - 1)) * 256;
                    n->stateTimer = jellyDelay;
                    n->lastHitId = p->attackId;   // not hit again by the same slash
                }
            }
            continue;
        }
        if (overlap(px0, py0, px1, py1, x0, y0, x1, y1))
            player_hurt(p, st->damage, ex > ppx);
    }
    updateShots(s, r, p, px0, py0, px1, py1);
}

u8 enemies_alive(const EnemySet *s)
{
    u8 n = 0;
    for (u8 i = 0; i < MAX_ENEMIES; i++)
        if (s->e[i].type != ENEMY_NONE && !s->e[i].deathTimer) n++;
    return n;
}

s8 enemies_bossSlot(const EnemySet *s)
{
    for (u8 i = 0; i < MAX_ENEMIES; i++)
        if (s->e[i].type == ENEMY_BOSS && !s->e[i].deathTimer) return i;
    return -1;
}

u8 enemies_blocked(EnemySet *s)
{
    return s->blocked;
}

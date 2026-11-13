#include "game_enemy.h"

typedef struct
{
    s16 speed, chase;    // 1/256 px per frame; chase = 0: never charges
    s16 range;           // px: how close the player must be to be noticed
    s8 hp;
    u8 damage;
} EnemyStats;

// [type][voidForm], NTSC values.
static const EnemyStats STATS[3][2] =
{
    { { 0 }, { 0 } },
    { { 154, 0, 0, 2, 1 }, { 256, 640, 128, 4, 2 } },     // walker: patrols; Void form charges
    { { 192, 0, 112, 1, 1 }, { 384, 0, 160, 2, 2 } },     // flyer: homes in on the player
};

static s16 speedScale6, gravity, maxFall, knockback;
static u8 hurtFrames, deathFrames, pal;

void enemies_setRegion(u8 isPal)
{
    // 50 Hz: speeds x 6/5, accelerations x 36/25, durations x 5/6 (same rule as the player).
    pal = isPal;
    gravity = isPal ? 80 * 36 / 25 : 80;
    maxFall = isPal ? 1536 * 6 / 5 : 1536;
    knockback = isPal ? 512 * 6 / 5 : 512;
    hurtFrames = isPal ? 10 * 5 / 6 : 10;
    deathFrames = isPal ? 16 * 5 / 6 : 16;
    speedScale6 = isPal ? 6 : 5;
}

static s16 speedOf(s16 v)
{
    return v * speedScale6 / 5;
}

void enemy_size(u8 type, s16 *w, s16 *h)
{
    *w = type == ENEMY_WALKER ? 14 : 14;
    *h = type == ENEMY_WALKER ? 22 : 12;
}

void enemies_clear(EnemySet *s)
{
    for (u8 i = 0; i < MAX_ENEMIES; i++) s->e[i].type = ENEMY_NONE;
}

void enemies_add(EnemySet *s, u8 type, s16 px, s16 py, u8 voidForm)
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
        e->timer = i * 37;   // desynchronise the bobbing
        e->voidForm = 0;
        e->hp = STATS[type][0].hp;
        if (voidForm) enemies_setVoid(s, 1);
        return;
    }
}

void enemies_setVoid(EnemySet *s, u8 voidForm)
{
    for (u8 i = 0; i < MAX_ENEMIES; i++) enemy_setVoidForm(&s->e[i], voidForm);
}

void enemy_setVoidForm(Enemy *e, u8 voidForm)
{
    {
        if (e->type == ENEMY_NONE || e->deathTimer || e->voidForm == voidForm) return;
        // Keep the damage already taken.
        s8 lost = STATS[e->type][e->voidForm].hp - e->hp;
        e->voidForm = voidForm;
        e->hp = STATS[e->type][voidForm].hp - lost;
        if (e->hp < 1) e->hp = 1;
    }
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

    e->x += e->vx;
    px = e->x >> 8;
    s16 front = px + (e->vx > 0 ? w / 2 : -w / 2);
    if (e->vx && isSolid(r, front, py - 4))
    {
        e->x -= e->vx;
        e->vx = 0;
    }

    s16 oldFeet = py - 1;
    e->vy += gravity;
    if (e->vy > maxFall) e->vy = maxFall;
    e->y += e->vy;
    py = e->y >> 8;
    s16 feet = py - 1;
    u8 t = room_tileAt(r, px, feet);
    if (e->vy >= 0 && (t == TILE_SOLID || (t == TILE_PLATFORM && (oldFeet >> 3) < (feet >> 3))))
    {
        e->y = (s32) (feet & ~7) << 8;
        e->vy = 0;
    }
    else if (e->vy >= 0 && !(py & 7) && room_tileAt(r, px, py) != TILE_EMPTY)
        e->vy = 0;   // resting on the row below
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
    e->y += e->vy;   // spirits drift through walls
}

static u8 overlap(s16 ax0, s16 ay0, s16 ax1, s16 ay1, s16 bx0, s16 by0, s16 bx1, s16 by1)
{
    return ax0 <= bx1 && bx0 <= ax1 && ay0 <= by1 && by0 <= ay1;
}

void enemies_update(EnemySet *s, const Room *r, Player *p)
{
    s16 ax0 = 0, ay0 = 0, ax1 = 0, ay1 = 0;
    u8 slashing = player_attackActive(p);
    if (slashing) player_attackBox(p, &ax0, &ay0, &ax1, &ay1);

    s16 ppx = p->x >> 8, ppy = p->y >> 8;
    s16 ph = p->crouching ? PLAYER_CROUCH_H : PLAYER_H;

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
        else updateFlyer(e, st, p);

        s16 w, h;
        enemy_size(e->type, &w, &h);
        s16 ex = e->x >> 8, ey = e->y >> 8;
        s16 x0 = ex - w / 2, x1 = ex + w / 2 - 1, y0 = ey - h, y1 = ey - 1;

        if (slashing && e->lastHitId != p->attackId && overlap(ax0, ay0, ax1, ay1, x0, y0, x1, y1))
        {
            e->lastHitId = p->attackId;
            e->hurtTimer = hurtFrames;
            e->vx = p->facingLeft ? -knockback : knockback;
            if (p->attackDir == ATTACK_UP) e->vy = -knockback;
            if (p->attackDir == ATTACK_DOWN)
            {
                e->vy = knockback;
                player_bounce(p);
            }
            e->hp -= player_slashDamage(p);
            if (e->hp <= 0) e->deathTimer = deathFrames;
            continue;
        }
        if (overlap(ppx - PLAYER_W / 2, ppy - ph, ppx + PLAYER_W / 2 - 1, ppy - 1, x0, y0, x1, y1))
            player_hurt(p, st->damage, ex > ppx);
    }
}

u8 enemies_alive(const EnemySet *s)
{
    u8 n = 0;
    for (u8 i = 0; i < MAX_ENEMIES; i++)
        if (s->e[i].type != ENEMY_NONE && !s->e[i].deathTimer) n++;
    return n;
}

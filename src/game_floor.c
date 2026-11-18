#include "game_floor.h"

// Binding of Isaac style floor: rooms grow from the start as a tree (a new room may only touch
// its parent), so every adjacency is a door and dead ends are natural spots for special rooms.
// Secret rooms follow fixed rules the player can learn:
//   - secret: the empty cell touching the most rooms (never the boss);
//   - super secret: an empty cell touching exactly one regular room (never the boss).

static const s8 DX[4] = { -1, 1, 0, 0 };
static const s8 DY[4] = { 0, 0, -1, 1 };
static const u8 DOOR_BIT[4] = { DOOR_LEFT, DOOR_RIGHT, DOOR_UP, DOOR_DOWN };

// Own xorshift instead of SGDK's random(): a seed must rebuild the same floor (shared seeds, codes).
static u16 rng;

static u16 rnd(void)
{
    rng ^= rng << 7;
    rng ^= rng >> 9;
    rng ^= rng << 8;
    return rng;
}

static u16 rndRange(u16 n)
{
    return rnd() % n;
}

static u8 inside(s16 x, s16 y)
{
    return x >= 0 && x < FLOOR_W && y >= 0 && y < FLOOR_H;
}

static u8 neighbours(const Floor *f, s16 x, s16 y)
{
    u8 n = 0;
    for (u8 d = 0; d < 4; d++)
    {
        s16 nx = x + DX[d], ny = y + DY[d];
        if (inside(nx, ny) && f->type[ny][nx] != ROOM_NONE) n++;
    }
    return n;
}

static u8 touches(const Floor *f, s16 x, s16 y, u8 type)
{
    for (u8 d = 0; d < 4; d++)
    {
        s16 nx = x + DX[d], ny = y + DY[d];
        if (inside(nx, ny) && f->type[ny][nx] == type) return 1;
    }
    return 0;
}

static void clear(Floor *f)
{
    for (u8 y = 0; y < FLOOR_H; y++)
        for (u8 x = 0; x < FLOOR_W; x++)
        {
            f->type[y][x] = ROOM_NONE;
            f->doors[y][x] = 0;
            f->hidden[y][x] = 0;
        }
}

static u8 grow(Floor *f, u8 target)
{
    u8 qx[FLOOR_W * FLOOR_H], qy[FLOOR_W * FLOOR_H];
    u8 head = 0, tail = 0;

    clear(f);
    f->type[f->startY][f->startX] = ROOM_START;
    f->roomCount = 1;
    qx[tail] = f->startX;
    qy[tail++] = f->startY;

    while (head < tail && f->roomCount < target)
    {
        u8 x = qx[head], y = qy[head++];
        u8 first = rndRange(4);
        for (u8 i = 0; i < 4 && f->roomCount < target; i++)
        {
            u8 d = (first + i) & 3;
            s16 nx = x + DX[d], ny = y + DY[d];
            if (!inside(nx, ny) || f->type[ny][nx] != ROOM_NONE) continue;
            if (neighbours(f, nx, ny) > 1) continue;   // keeps the layout a tree
            if (rnd() & 1) continue;
            f->type[ny][nx] = ROOM_NORMAL;
            f->roomCount++;
            qx[tail] = nx;
            qy[tail++] = ny;
        }
    }
    return f->roomCount == target;
}

// Breadth-first distances over regular rooms (secrets are behind walls, not on the path).
static void distances(const Floor *f, u8 fromX, u8 fromY, u8 dist[FLOOR_H][FLOOR_W])
{
    u8 qx[FLOOR_W * FLOOR_H], qy[FLOOR_W * FLOOR_H];
    u8 head = 0, tail = 0;

    for (u8 y = 0; y < FLOOR_H; y++)
        for (u8 x = 0; x < FLOOR_W; x++) dist[y][x] = NO_DIST;
    dist[fromY][fromX] = 0;
    qx[tail] = fromX;
    qy[tail++] = fromY;
    while (head < tail)
    {
        u8 x = qx[head], y = qy[head++];
        for (u8 d = 0; d < 4; d++)
        {
            s16 nx = x + DX[d], ny = y + DY[d];
            if (!inside(nx, ny) || dist[ny][nx] != NO_DIST) continue;
            u8 t = f->type[ny][nx];
            if (t == ROOM_NONE || t == ROOM_SECRET || t == ROOM_SUPER_SECRET) continue;
            dist[ny][nx] = dist[y][x] + 1;
            qx[tail] = nx;
            qy[tail++] = ny;
        }
    }
}

// Picks a random cell among those with the given score; returns 0 if none.
static u8 pickCell(const u8 score[FLOOR_H][FLOOR_W], u8 wanted, u8 *ox, u8 *oy)
{
    u8 count = 0;
    for (u8 y = 0; y < FLOOR_H; y++)
        for (u8 x = 0; x < FLOOR_W; x++)
            if (score[y][x] == wanted) count++;
    if (!count) return 0;
    u8 k = rndRange(count);
    for (u8 y = 0; y < FLOOR_H; y++)
        for (u8 x = 0; x < FLOOR_W; x++)
            if (score[y][x] == wanted && k-- == 0)
            {
                *ox = x;
                *oy = y;
                return 1;
            }
    return 0;
}

static u8 placeSpecialRooms(Floor *f, u8 needShop)
{
    u8 dist[FLOOR_H][FLOOR_W];
    u8 score[FLOOR_H][FLOOR_W];
    u8 deadEnds = 0, x, y;

    distances(f, f->startX, f->startY, dist);

    // Boss: the farthest dead end from the start.
    u8 best = 0;
    for (y = 0; y < FLOOR_H; y++)
        for (x = 0; x < FLOOR_W; x++)
        {
            score[y][x] = 0;
            if (f->type[y][x] == ROOM_NORMAL && neighbours(f, x, y) == 1)
            {
                deadEnds++;
                score[y][x] = dist[y][x];
                if (dist[y][x] > best) best = dist[y][x];
            }
        }
    if (deadEnds < (needShop ? 3 : 2)) return 0;
    pickCell(score, best, &x, &y);
    f->type[y][x] = ROOM_BOSS;

    // Upgrade, shop: random among the remaining dead ends, never next to the start room (Thomas,
    // 2026-11-18: the start stays neutral, no caravan nor padlock in it).
    static const u8 OTHERS[2] = { ROOM_UPGRADE, ROOM_SHOP };
    for (u8 i = 0; i < 2; i++)
    {
        for (y = 0; y < FLOOR_H; y++)
            for (x = 0; x < FLOOR_W; x++)
                score[y][x] = f->type[y][x] == ROOM_NORMAL && neighbours(f, x, y) == 1 && !touches(f, x, y, ROOM_START);
        if (!pickCell(score, 1, &x, &y)) return i == 1 && !needShop;   // only the shop can be missing
        f->type[y][x] = OTHERS[i];
    }
    return 1;
}

static void placeSecrets(Floor *f)
{
    u8 score[FLOOR_H][FLOOR_W];
    u8 x, y, best = 0;

    for (y = 0; y < FLOOR_H; y++)
        for (x = 0; x < FLOOR_W; x++)
        {
            score[y][x] = 0;
            if (f->type[y][x] != ROOM_NONE || touches(f, x, y, ROOM_BOSS) || touches(f, x, y, ROOM_START)
                || touches(f, x, y, ROOM_SHOP)) continue;   // the shop is a closed caravan
            score[y][x] = neighbours(f, x, y);
            if (score[y][x] > best) best = score[y][x];
        }
    if (best && pickCell(score, best, &x, &y)) f->type[y][x] = ROOM_SECRET;

    // Exactly one neighbour, and it must be a regular room (not the boss, not the secret room).
    for (y = 0; y < FLOOR_H; y++)
        for (x = 0; x < FLOOR_W; x++)
            score[y][x] = f->type[y][x] == ROOM_NONE && neighbours(f, x, y) == 1
                && !touches(f, x, y, ROOM_BOSS) && !touches(f, x, y, ROOM_SECRET) && !touches(f, x, y, ROOM_START)
                && !touches(f, x, y, ROOM_SHOP);
    if (pickCell(score, 1, &x, &y)) f->type[y][x] = ROOM_SUPER_SECRET;
}

static void linkRooms(Floor *f)
{
    for (u8 y = 0; y < FLOOR_H; y++)
        for (u8 x = 0; x < FLOOR_W; x++)
        {
            u8 t = f->type[y][x];
            if (t == ROOM_NONE) continue;
            for (u8 d = 0; d < 4; d++)
            {
                s16 nx = x + DX[d], ny = y + DY[d];
                if (!inside(nx, ny)) continue;
                u8 n = f->type[ny][nx];
                if (n == ROOM_NONE) continue;
                u8 secret = t == ROOM_SECRET || t == ROOM_SUPER_SECRET
                    || n == ROOM_SECRET || n == ROOM_SUPER_SECRET;
                if (secret) f->hidden[y][x] |= DOOR_BIT[d];
                else f->doors[y][x] |= DOOR_BIT[d];
            }
        }
}

void floor_generate(Floor *f, u16 seed, u8 depth)
{
    u8 target = 8 + depth * 2;
    if (target > 20) target = 20;

    f->seed = seed;
    rng = seed ? seed : 1;   // xorshift never leaves 0
    f->startX = FLOOR_W / 2;
    f->startY = FLOOR_H / 2;

    // Retry until the tree has enough dead ends; after many tries accept a floor without a shop.
    for (u16 tries = 0;; tries++)
        if (grow(f, target) && placeSpecialRooms(f, tries < 300)) break;

    placeSecrets(f);
    linkRooms(f);

    // The Void closes in on the boss, who guards the gem: reach him before the circle closes.
    for (u8 y = 0; y < FLOOR_H; y++)
        for (u8 x = 0; x < FLOOR_W; x++)
            if (f->type[y][x] == ROOM_BOSS)
            {
                f->voidX = x;
                f->voidY = y;
            }
}

u16 floor_voidDist2(const Floor *f, u8 x, u8 y)
{
    s16 dx = ((s16) x - f->voidX) * VOID_UNIT_X;
    s16 dy = ((s16) y - f->voidY) * VOID_UNIT_Y;
    return dx * dx + dy * dy;
}

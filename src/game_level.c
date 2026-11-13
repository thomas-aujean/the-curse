#include "game_level.h"

void level_buildRoom(Room *r, const RoomTemplate *t, u8 doors)
{
    room_clear(r, t->wBlocks * 2, t->hBlocks * 2);
    for (u8 by = 0; by < t->hBlocks; by++)
        for (u8 bx = 0; bx < t->wBlocks; bx++)
        {
            char c = t->rows[by][bx];
            u8 top = TILE_EMPTY, bottom = TILE_EMPTY;
            u8 closed = (c == 'L' && !(doors & DOOR_LEFT)) || (c == 'R' && !(doors & DOOR_RIGHT))
                || (c == 'U' && !(doors & DOOR_UP)) || (c == 'D' && !(doors & DOOR_DOWN));
            if (c == '#' || closed) top = bottom = TILE_SOLID;
            else if (c == '-') top = TILE_PLATFORM;
            else if (c == 'D' && (by == 0 || t->rows[by - 1][bx] != 'D')) top = TILE_PLATFORM;

            u8 tx = bx * 2, ty = by * 2;
            r->tiles[ty][tx] = r->tiles[ty][tx + 1] = top;
            r->tiles[ty + 1][tx] = r->tiles[ty + 1][tx + 1] = bottom;
        }
}

const RoomTemplate *level_templateFor(const Level *lv, u8 x, u8 y)
{
    u8 type = lv->floor.type[y][x];
    // Rooms holding an item use the small room: the item always sits in its middle, on the floor.
    if (type == ROOM_START || type == ROOM_UPGRADE || type == ROOM_SHOP || type == ROOM_SECRET
        || type == ROOM_SUPER_SECRET)
        return &ROOM_TEMPLATES[0];
    if (type == ROOM_BOSS) return &ROOM_TEMPLATES[ROOM_TEMPLATE_COUNT - 1];
    // Stable per seed and cell: coming back to a room gives the same layout.
    u16 h = lv->floor.seed ^ (u16) ((x * 31 + y * 17) * 0x9E37);
    h ^= h >> 7;
    h ^= h << 5;
    return &ROOM_TEMPLATES[h % ROOM_TEMPLATE_RANDOM];
}

u8 level_doors(const Level *lv, u8 x, u8 y)
{
    return lv->floor.doors[y][x] | (lv->floor.hidden[y][x] & lv->opened[y][x]);
}

// Bounding box of a door marker, in blocks.
static void findDoor(const RoomTemplate *t, char c, u8 *x0, u8 *y0, u8 *x1, u8 *y1)
{
    *x0 = *y0 = 0xFF;
    *x1 = *y1 = 0;
    for (u8 by = 0; by < t->hBlocks; by++)
        for (u8 bx = 0; bx < t->wBlocks; bx++)
            if (t->rows[by][bx] == c)
            {
                if (bx < *x0) *x0 = bx;
                if (by < *y0) *y0 = by;
                if (bx > *x1) *x1 = bx;
                if (by > *y1) *y1 = by;
            }
}

static void setPos(Player *p, s16 px, s16 py)
{
    p->x = (s32) px << 8;
    p->y = (s32) py << 8;
}

static u8 fps = 60;

static const u8 DIR_BITS[4] = { DOOR_LEFT, DOOR_RIGHT, DOOR_UP, DOOR_DOWN };
static const char DIR_MARKER[4] = { 'L', 'R', 'U', 'D' };

// Which item a room holds: fixed per seed and cell, so the same floor always gives the same items.
static u8 itemFor(const Level *lv, u8 x, u8 y)
{
    u8 type = lv->floor.type[y][x];
    if (type == ROOM_SECRET) return ITEM_HEART;
    if (type != ROOM_UPGRADE && type != ROOM_SUPER_SECRET) return ITEM_NONE;
    u16 h = lv->floor.seed * 13 + x * 7 + y * 3 + type;
    h ^= h >> 5;
    return ITEM_FIRST_UPGRADE + h % UPGRADE_COUNT;
}

// enteredBy: the door of the new room the player comes through (0 = level start).
static void enterRoom(Level *lv, Player *p, u8 x, u8 y, u8 enteredBy)
{
    const RoomTemplate *t = level_templateFor(lv, x, y);
    lv->cellX = x;
    lv->cellY = y;
    lv->visited[y][x] = 1;
    level_buildRoom(&lv->room, t, level_doors(lv, x, y));

    // Walls hiding a secret room: still solid, drawn cracked, broken by a slash.
    lv->crackedCount = 0;
    u8 hidden = lv->floor.hidden[y][x] & ~lv->opened[y][x];
    for (u8 d = 0; d < 4; d++)
        if (hidden & DIR_BITS[d])
        {
            u8 x0, y0, x1, y1;
            findDoor(t, DIR_MARKER[d], &x0, &y0, &x1, &y1);
            Cracked *c = &lv->cracked[lv->crackedCount++];
            c->dir = DIR_BITS[d];
            c->x0 = x0 * 16;
            c->y0 = y0 * 16;
            c->x1 = (x1 + 1) * 16 - 1;
            c->y1 = (y1 + 1) * 16 - 1;
        }

    // Enemies come back each time, unless the room was cleared. Never in the start room.
    // Their form is set every frame from their position (level_update).
    enemies_clear(&lv->enemies);
    if (!lv->cleared[y][x] && lv->floor.type[y][x] != ROOM_START)
        for (u8 by = 0; by < t->hBlocks; by++)
            for (u8 bx = 0; bx < t->wBlocks; bx++)
            {
                char c = t->rows[by][bx];
                if (c == 'w') enemies_add(&lv->enemies, ENEMY_WALKER, bx * 16 + 8, (by + 1) * 16, 0);
                if (c == 'f') enemies_add(&lv->enemies, ENEMY_FLYER, bx * 16 + 8, by * 16 + 14, 0);
                if (c == 'B') enemies_add(&lv->enemies, ENEMY_BOSS, bx * 16 + 8, (by + 1) * 16, 0);
            }

    // Item on its altar, in the middle of the small room, until taken.
    lv->itemId = lv->taken[y][x] ? ITEM_NONE : itemFor(lv, x, y);
    lv->itemX = t->wBlocks * 8;
    lv->itemY = (t->hBlocks - 2) * 16;

    lv->bossFight = lv->floor.type[y][x] == ROOM_BOSS && !lv->cleared[y][x];
    if (lv->bossFight)
    {
        lv->voidRunning = 0;                   // the fight stops the Void
        lv->bossStrong = lv->voidRadiusFx == 0;
        lv->lockTimer = fps;
    }

    u8 x0, y0, x1, y1;
    s16 w = t->wBlocks * 16, h = t->hBlocks * 16;
    switch (enteredBy)
    {
        case DOOR_LEFT:
            findDoor(t, 'L', &x0, &y0, &x1, &y1);
            setPos(p, 16 + PLAYER_W / 2 + 2, (y1 + 1) * 16);
            break;
        case DOOR_RIGHT:
            findDoor(t, 'R', &x0, &y0, &x1, &y1);
            setPos(p, w - 16 - PLAYER_W / 2 - 2, (y1 + 1) * 16);
            break;
        case DOOR_UP:      // falling in from the room above
            findDoor(t, 'U', &x0, &y0, &x1, &y1);
            setPos(p, (x0 + x1 + 1) * 8, 16 + PLAYER_H + 1);
            break;
        case DOOR_DOWN:    // coming up from below: a boost lands the player on the platform over the hole
            findDoor(t, 'D', &x0, &y0, &x1, &y1);
            setPos(p, (x0 + x1 + 1) * 8, h);
            player_boostUp(p);
            break;
        default:           // start of the floor: keeps health and upgrades
            player_place(p, w / 2 - 32, h - 32);
            break;
    }
}

void level_enter(Level *lv, Player *p, u8 x, u8 y)
{
    enterRoom(lv, p, x, y, 0);
}

void level_setRegion(u8 pal)
{
    fps = pal ? 50 : 60;
}

// Each cell of the floor map owns a 2 x 2 screen slot of the castle (the largest room size):
// the Void circle lives in these world coordinates.
static s16 cellOriginX(u8 x) { return x * CELL_W_PX; }
static s16 cellOriginY(u8 y) { return y * CELL_H_PX; }

static u32 dist2(s16 dx, s16 dy)
{
    // 16 x 16 -> 32-bit products: the 68000 does them in one MULS instead of a libgcc call.
    return (u32) ((s32) dx * dx) + (u32) ((s32) dy * dy);
}

static u16 isqrt(u32 v)
{
    u16 lo = 0, hi = 0xFFFF;
    while (lo < hi)
    {
        u16 mid = lo + (hi - lo + 1) / 2;
        if ((u32) mid * mid <= v) lo = mid;
        else hi = mid - 1;
    }
    return lo;
}

void level_start(Level *lv, Player *p, u16 seed, u8 depth)
{
    floor_generate(&lv->floor, seed, depth);
    for (u8 y = 0; y < FLOOR_H; y++)
        for (u8 x = 0; x < FLOOR_W; x++)
            lv->visited[y][x] = lv->cleared[y][x] = lv->opened[y][x] = lv->taken[y][x] = 0;
    lv->gemActive = 0;
    lv->bossFight = 0;

    // The circle closes on the centre of the boss room. It starts just around the farthest room
    // corner and reaches zero after VOID_SECONDS.
    u8 bx = lv->floor.voidX, by = lv->floor.voidY;
    const RoomTemplate *bt = level_templateFor(lv, bx, by);
    lv->voidCX = cellOriginX(bx) + bt->wBlocks * 8;
    lv->voidCY = cellOriginY(by) + bt->hBlocks * 8;
    u32 far = 0;
    for (u8 y = 0; y < FLOOR_H; y++)
        for (u8 x = 0; x < FLOOR_W; x++)
        {
            u8 type = lv->floor.type[y][x];
            if (type == ROOM_NONE || type == ROOM_SECRET || type == ROOM_SUPER_SECRET) continue;
            const RoomTemplate *t = level_templateFor(lv, x, y);
            s16 x0 = cellOriginX(x) - lv->voidCX, y0 = cellOriginY(y) - lv->voidCY;
            s16 x1 = x0 + t->wBlocks * 16, y1 = y0 + t->hBlocks * 16;
            s16 fx = -x0 > x1 ? x0 : x1, fy = -y0 > y1 ? y0 : y1;
            u32 d = dist2(fx, fy);
            if (d > far) far = d;
        }
    u32 radius = isqrt(far) + 32;
    lv->voidRadiusFx = lv->voidStartFx = radius << 8;
    u32 frames = (u32) VOID_SECONDS * fps;
    lv->voidRate = ((radius << 8) + frames - 1) / frames;   // rounded up: never later than planned
    lv->voidRunning = 1;

    enterRoom(lv, p, lv->floor.startX, lv->floor.startY, 0);
}

u8 level_pointInVoid(const Level *lv, s16 px, s16 py)
{
    s16 r = lv->voidRadiusFx >> 8;
    return dist2(cellOriginX(lv->cellX) + px - lv->voidCX, cellOriginY(lv->cellY) + py - lv->voidCY)
        > (u32) r * r;
}

u8 level_cellInVoid(const Level *lv, u8 x, u8 y)
{
    const RoomTemplate *t = level_templateFor(lv, x, y);
    s16 r = lv->voidRadiusFx >> 8;
    return dist2(cellOriginX(x) + t->wBlocks * 8 - lv->voidCX, cellOriginY(y) + t->hBlocks * 8 - lv->voidCY)
        > (u32) r * r;
}

u8 level_crackedAt(const Level *lv, s16 px, s16 py)
{
    for (u8 i = 0; i < lv->crackedCount; i++)
    {
        const Cracked *c = &lv->cracked[i];
        if (px >= c->x0 && px <= c->x1 && py >= c->y0 && py <= c->y1) return 1;
    }
    return 0;
}

// A slash on a cracked wall opens the passage on both sides.
static u8 breakWalls(Level *lv, const Player *p)
{
    if (!player_attackActive(p) || !lv->crackedCount) return 0;
    s16 ax0, ay0, ax1, ay1;
    player_attackBox(p, &ax0, &ay0, &ax1, &ay1);
    for (u8 i = 0; i < lv->crackedCount; i++)
    {
        Cracked *c = &lv->cracked[i];
        if (ax0 > c->x1 || c->x0 > ax1 || ay0 > c->y1 || c->y0 > ay1) continue;
        u8 x = lv->cellX, y = lv->cellY;
        lv->opened[y][x] |= c->dir;
        if (c->dir == DOOR_LEFT) lv->opened[y][x - 1] |= DOOR_RIGHT;
        if (c->dir == DOOR_RIGHT) lv->opened[y][x + 1] |= DOOR_LEFT;
        if (c->dir == DOOR_UP) lv->opened[y - 1][x] |= DOOR_DOWN;
        if (c->dir == DOOR_DOWN) lv->opened[y + 1][x] |= DOOR_UP;
        *c = lv->cracked[--lv->crackedCount];
        level_buildRoom(&lv->room, level_templateFor(lv, x, y), level_doors(lv, x, y));
        return 1;
    }
    return 0;
}

static u8 nearPlayer(s16 x, s16 y, s16 ppx, s16 ppy)
{
    return x > ppx - 16 && x < ppx + 16 && y > ppy - PLAYER_H - 8 && y < ppy + 8;
}

u8 level_update(Level *lv, Player *p)
{
    if (lv->voidRunning)
    {
        lv->voidRadiusFx = lv->voidRadiusFx > lv->voidRate ? lv->voidRadiusFx - lv->voidRate : 0;
        if (!lv->voidRadiusFx) lv->voidRunning = 0;
    }

    s16 ppx = p->x >> 8, ppy = p->y >> 8;
    lv->roomInVoid = level_pointInVoid(lv, ppx, ppy - PLAYER_H / 2);   // where the player stands
    player_updateLight(p, lv->roomInVoid);
    for (u8 i = 0; i < MAX_ENEMIES; i++)
    {
        Enemy *e = &lv->enemies.e[i];
        if (e->type == ENEMY_BOSS) enemy_setVoidForm(e, lv->bossStrong);
        else if (e->type != ENEMY_NONE) enemy_setVoidForm(e, level_pointInVoid(lv, e->x >> 8, (e->y >> 8) - 8));
    }
    enemies_update(&lv->enemies, &lv->room, p);
    if (!enemies_alive(&lv->enemies)) lv->cleared[lv->cellY][lv->cellX] = 1;

    u8 result = 0;
    if (breakWalls(lv, p)) result |= LEVEL_REDRAW;

    if (lv->itemId != ITEM_NONE && nearPlayer(lv->itemX, lv->itemY - 8, ppx, ppy))
    {
        player_takeItem(p, lv->itemId);
        lv->lastItem = lv->itemId;
        lv->itemId = ITEM_NONE;
        lv->taken[lv->cellY][lv->cellX] = 1;
        result |= LEVEL_ITEM;
    }

    if (lv->bossFight)
    {
        if (lv->lockTimer && --lv->lockTimer == 0)
        {
            // Lock the doors once the player is well inside (never closes on the player).
            level_buildRoom(&lv->room, level_templateFor(lv, lv->cellX, lv->cellY), 0);
            result |= LEVEL_REDRAW;
        }
        s8 slot = enemies_bossSlot(&lv->enemies);
        if (slot < 0)
        {
            // Boss down: the gem drops where it fell.
            lv->bossFight = 0;
            for (u8 i = 0; i < MAX_ENEMIES; i++)
                if (lv->enemies.e[i].type == ENEMY_BOSS)
                {
                    lv->gemX = lv->enemies.e[i].x;
                    lv->gemY = lv->enemies.e[i].y - (24 << 8);
                }
            lv->gemVy = 0;
            lv->gemActive = 1;
        }
    }
    if (lv->gemActive)
    {
        s16 gx = lv->gemX >> 8, gy = lv->gemY >> 8;
        if (room_tileAt(&lv->room, gx, gy) == TILE_EMPTY)
        {
            lv->gemVy += 64;
            lv->gemY += lv->gemVy;
        }
        else
        {
            lv->gemY = (s32) (gy & ~7) << 8;
            lv->gemVy = 0;
        }
        if (nearPlayer(gx, gy, ppx, ppy))
        {
            lv->gemActive = 0;
            return result | LEVEL_GEM;
        }
    }
    if (result & LEVEL_REDRAW) return result;

    s16 px = p->x >> 8, cy = (p->y >> 8) - PLAYER_H / 2;
    s16 w = lv->room.w * ROOM_TILE, h = lv->room.h * ROOM_TILE;
    u8 x = lv->cellX, y = lv->cellY, doors = level_doors(lv, x, y);

    if (px < 0 && (doors & DOOR_LEFT)) enterRoom(lv, p, x - 1, y, DOOR_RIGHT);
    else if (px >= w && (doors & DOOR_RIGHT)) enterRoom(lv, p, x + 1, y, DOOR_LEFT);
    else if (cy < 0 && (doors & DOOR_UP)) enterRoom(lv, p, x, y - 1, DOOR_DOWN);
    else if (cy >= h && (doors & DOOR_DOWN)) enterRoom(lv, p, x, y + 1, DOOR_UP);
    else return result;
    return result | LEVEL_REDRAW;
}

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
    if (type == ROOM_START) return &ROOM_TEMPLATES[0];
    if (type == ROOM_BOSS) return &ROOM_TEMPLATES[3];
    // Stable per seed and cell: coming back to a room gives the same layout.
    u16 h = lv->floor.seed ^ (u16) ((x * 31 + y * 17) * 0x9E37);
    h ^= h >> 7;
    h ^= h << 5;
    return &ROOM_TEMPLATES[h % ROOM_TEMPLATE_COUNT];
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

// enteredBy: the door of the new room the player comes through (0 = level start).
static void enterRoom(Level *lv, Player *p, u8 x, u8 y, u8 enteredBy)
{
    const RoomTemplate *t = level_templateFor(lv, x, y);
    lv->cellX = x;
    lv->cellY = y;
    lv->visited[y][x] = 1;
    level_buildRoom(&lv->room, t, lv->floor.doors[y][x]);

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
        default:
            player_init(p, w / 2, h - 32);
            break;
    }
}

static u8 fps = 60;

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
        for (u8 x = 0; x < FLOOR_W; x++) lv->visited[y][x] = lv->cleared[y][x] = 0;

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
    lv->voidRadiusFx = radius << 8;
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
        if (e->type != ENEMY_NONE) enemy_setVoidForm(e, level_pointInVoid(lv, e->x >> 8, (e->y >> 8) - 8));
    }
    enemies_update(&lv->enemies, &lv->room, p);
    if (!enemies_alive(&lv->enemies)) lv->cleared[lv->cellY][lv->cellX] = 1;

    s16 px = p->x >> 8, cy = (p->y >> 8) - PLAYER_H / 2;
    s16 w = lv->room.w * ROOM_TILE, h = lv->room.h * ROOM_TILE;
    u8 x = lv->cellX, y = lv->cellY, doors = lv->floor.doors[y][x];

    if (px < 0 && (doors & DOOR_LEFT)) enterRoom(lv, p, x - 1, y, DOOR_RIGHT);
    else if (px >= w && (doors & DOOR_RIGHT)) enterRoom(lv, p, x + 1, y, DOOR_LEFT);
    else if (cy < 0 && (doors & DOOR_UP)) enterRoom(lv, p, x, y - 1, DOOR_DOWN);
    else if (cy >= h && (doors & DOOR_DOWN)) enterRoom(lv, p, x, y + 1, DOOR_UP);
    else return 0;
    return 1;
}
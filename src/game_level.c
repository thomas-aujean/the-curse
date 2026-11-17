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
    // Weighted like Isaac's rooms: the medium ones are common, the 4 x 3 keep is the rare landmark of a
    // floor, the treasure room and the ambush rarer still. Order of ROOM_TEMPLATES (rooms.c).
    static const u8 ROOM_WEIGHT[ROOM_TEMPLATE_COUNT] =
    {
        8, 10, 8, 8, 6, 4, 5, 2,   // small, corridor, shaft, hall, long, deep, great, keep
        8, 1, 10, 2, 8, 7, 6,      // pillars, treasure, stairs, ambush, zigzag, tower, bridges
        0,                         // the boss arena, never drawn at random
    };
    u16 total = 0;
    for (u8 i = 0; i < ROOM_TEMPLATE_COUNT; i++) total += ROOM_WEIGHT[i];
    u16 roll = h % total;
    u8 i = 0;
    while (roll >= ROOM_WEIGHT[i]) roll -= ROOM_WEIGHT[i++];
    return &ROOM_TEMPLATES[i];
}

u8 level_doors(const Level *lv, u8 x, u8 y)
{
    // The shop only has its opening on the left (out to the caravan); the caravan's room loses its
    // door to the shop.
    u8 secret = lv->floor.hidden[y][x] & lv->opened[y][x];
    if (x == lv->shopX && y == lv->shopY) return lv->shopExit | secret;
    u8 d = lv->floor.doors[y][x] | secret;
    if (x == lv->carX && y == lv->carY)
        d &= ~(lv->shopX < x ? DOOR_LEFT : lv->shopX > x ? DOOR_RIGHT : lv->shopY < y ? DOOR_UP : DOOR_DOWN);
    if (x == lv->lockX && y == lv->lockY && !lv->lockOpen) d &= ~lv->lockDir;   // a wall with a padlock
    return d;
}

static u16 lootRandom(Level *lv);
static u8 hasPropSpot(const Level *lv, u8 x, u8 y);
static u8 levelMessage(Level *lv, u8 message);

// Plain rooms (not the start, the shop's caravan room, nor the room before the upgrade room).
static u8 plainRoom(const Level *lv, u8 x, u8 y)
{
    return lv->floor.type[y][x] == ROOM_NORMAL && !(x == lv->carX && y == lv->carY) && !(x == lv->lockX && y == lv->lockY);
}

// This floor's two key rooms and its locked chest (a room whose layout has a chest spot), at random;
// the upgrade room's door gets its padlock.
static void pickKeyRooms(Level *lv)
{
    lv->upX = lv->upY = lv->lockX = lv->lockY = 0xFF;
    lv->lockOpen = 0;
    lv->keysTaken = 0;
    lv->lockHitId = 0xFF;
    for (u8 y = 0; y < FLOOR_H; y++)
        for (u8 x = 0; x < FLOOR_W; x++)
            if (lv->floor.type[y][x] == ROOM_UPGRADE)
            {
                u8 d = lv->floor.doors[y][x];
                lv->upX = x;
                lv->upY = y;
                lv->lockX = d & DOOR_LEFT ? x - 1 : d & DOOR_RIGHT ? x + 1 : x;
                lv->lockY = d & DOOR_UP ? y - 1 : d & DOOR_DOWN ? y + 1 : y;
                lv->lockDir = d & DOOR_LEFT ? DOOR_RIGHT : d & DOOR_RIGHT ? DOOR_LEFT : d & DOOR_UP ? DOOR_DOWN : DOOR_UP;
            }
    u8 count = 0;
    for (u8 y = 0; y < FLOOR_H; y++)
        for (u8 x = 0; x < FLOOR_W; x++)
            if (plainRoom(lv, x, y)) count++;
    lv->keyX[0] = lv->keyX[1] = lv->lockedChestX = 0xFF;
    lv->keyY[0] = lv->keyY[1] = lv->lockedChestY = 0xFF;
    if (!count) return;
    for (u8 k = 0, tries = 0; k < 3 && tries < 40; k++, tries++)   // a few retries, never forever
    {
        u8 pick = lootRandom(lv) % count, n = 0;
        for (u8 y = 0; y < FLOOR_H; y++)
            for (u8 x = 0; x < FLOOR_W; x++)
                if (plainRoom(lv, x, y) && n++ == pick)
                {
                    if (k == 1 && x == lv->keyX[0] && y == lv->keyY[0] && count > 1) { k--; continue; }   // two rooms
                    if (k < 2) { lv->keyX[k] = x; lv->keyY[k] = y; }
                    else if (hasPropSpot(lv, x, y)) { lv->lockedChestX = x; lv->lockedChestY = y; }
                    else k--;   // another room for the chest (most layouts have a spot)
                }
    }
}

// A spot on the lowest ground of the room (solid under hw px on each side, `clear` px free above),
// as near the middle (shifted by `bias` px) as possible. (x, y): centre, top of the ground.
static void findGround(const Level *lv, s16 hw, s16 clear, s16 bias, s16 *x, s16 *y)
{
    s16 w = lv->room.w * ROOM_TILE, h = lv->room.h * ROOM_TILE;
    *x = w / 2;
    *y = h - 2 * ROOM_TILE;
    for (s16 gy = h - ROOM_TILE; gy > 64; gy -= ROOM_TILE)
        for (s16 k = 0; k < w / 8; k++)
        {
            s16 cx = w / 2 + bias + ((k & 1) ? -1 : 1) * ((k + 1) / 2) * 8;
            if (cx < hw + 20 || cx > w - hw - 20) continue;
            u8 ok = 1;
            for (s16 dx = -hw; dx <= hw && ok; dx += 4)
            {
                if (room_tileAt(&lv->room, cx + dx, gy) != TILE_SOLID) ok = 0;
                for (s16 dy = ROOM_TILE; dy <= clear && ok; dy += ROOM_TILE)
                    if (room_tileAt(&lv->room, cx + dx, gy - dy) != TILE_EMPTY) ok = 0;
            }
            if (ok)
            {
                *x = cx;
                *y = gy;
                return;
            }
        }
}

// Where the caravan stands in its room: room for its 64 px.
static void placeCaravan(Level *lv)
{
    findGround(lv, 28, 56, 0, &lv->caravanX, &lv->caravanY);
}

// Does the layout of this room have a spot for a jar or a chest?
static u8 hasPropSpot(const Level *lv, u8 x, u8 y)
{
    const RoomTemplate *t = level_templateFor(lv, x, y);
    for (u8 by = 0; by < t->hBlocks; by++)
        for (u8 bx = 0; bx < t->wBlocks; bx++)
            if (t->rows[by][bx] == 'o' || t->rows[by][bx] == 'c') return 1;
    return 0;
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

static u8 nearPlayer(s16 x, s16 y, s16 ppx, s16 ppy);

static const u8 DIR_BITS[4] = { DOOR_LEFT, DOOR_RIGHT, DOOR_UP, DOOR_DOWN };
static const char DIR_MARKER[4] = { 'L', 'R', 'U', 'D' };

// An upgrade not yet placed on this floor (Thomas: never the same one twice in a floor), from a
// random start; any once all of them are out.
static u8 dealItem(Level *lv, u16 h)
{
    u8 start = h % UPGRADE_COUNT;
    for (u8 k = 0; k < UPGRADE_COUNT; k++)
    {
        u8 i = (start + k) % UPGRADE_COUNT;
        if (lv->itemsDealt & (1 << i)) continue;
        lv->itemsDealt |= 1 << i;
        return ITEM_FIRST_UPGRADE + i;
    }
    return ITEM_FIRST_UPGRADE + start;
}

// Altars and shop dealt when the floor starts: fixed per seed, so the same floor gives the same items.
static void dealFloorItems(Level *lv)
{
    lv->itemsDealt = 0;
    for (u8 y = 0; y < FLOOR_H; y++)
        for (u8 x = 0; x < FLOOR_W; x++)
        {
            u8 type = lv->floor.type[y][x];
            u16 h = lv->floor.seed * 13 + x * 7 + y * 3 + type;
            h ^= h >> 5;
            lv->roomItem[y][x] = type == ROOM_UPGRADE || type == ROOM_SUPER_SECRET ? dealItem(lv, h) : ITEM_NONE;
            if (type == ROOM_SHOP)
            {
                lv->shopWares[0] = dealItem(lv, h);
                lv->shopWares[1] = dealItem(lv, h >> 3);
            }
        }
}

// Which item a room holds.
static u8 itemFor(const Level *lv, u8 x, u8 y)
{
    return lv->floor.type[y][x] == ROOM_SECRET ? ITEM_HEART : lv->roomItem[y][x];
}

// enteredBy: the door of the new room the player comes through (0 = level start).
static void enterRoom(Level *lv, Player *p, u8 x, u8 y, u8 enteredBy)
{
    const RoomTemplate *t = level_templateFor(lv, x, y);
    lv->cellX = x;
    lv->cellY = y;
    lv->visited[y][x] = 1;
    level_buildRoom(&lv->room, t, level_doors(lv, x, y));
    if (x == lv->carX && y == lv->carY) placeCaravan(lv);
    u8 noChest = (x == lv->carX && y == lv->carY) || (x == lv->shopX && y == lv->shopY);   // their tiles hold the caravan
    u8 lockedHere = x == lv->lockedChestX && y == lv->lockedChestY, lockedPlaced = 0;
    if (x == lv->lockX && y == lv->lockY)
    {
        u8 x0, y0, x1, y1;
        findDoor(t, lv->lockDir == DOOR_LEFT ? 'L' : lv->lockDir == DOOR_RIGHT ? 'R' : lv->lockDir == DOOR_UP ? 'U' : 'D',
                 &x0, &y0, &x1, &y1);
        lv->padlockX = (x0 + x1 + 1) * 8;
        lv->padlockY = (y0 + y1 + 1) * 8;
    }

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

    // Enemies come back each time, except the ones killed (Thomas: the dead stay dead). Never in the
    // start room nor in the shop. Their form is set every frame from their position (level_update).
    enemies_clear(&lv->enemies);
    for (u8 i = 0; i < MAX_ENEMIES; i++) lv->spawnOf[i] = 0xFF;
    if (!lv->cleared[y][x] && lv->floor.type[y][x] != ROOM_START && lv->floor.type[y][x] != ROOM_SHOP)
    {
        u8 n = 0;   // place of the enemy in the template
        for (u8 by = 0; by < t->hBlocks; by++)
            for (u8 bx = 0; bx < t->wBlocks; bx++)
            {
                char c = t->rows[by][bx];
                u8 type = c == 'w' ? ENEMY_WALKER : c == 'f' ? ENEMY_FLYER : c == 'B' ? ENEMY_BOSS
                        : c == 'a' ? ENEMY_ARCHER : c == 'g' ? ENEMY_GUARD   // 'n' (ninja): out for now
                        : c == 'j' ? ENEMY_JELLY : c == 'b' ? ENEMY_BAT : ENEMY_NONE;
                if (type == ENEMY_NONE) continue;
                u8 k = n++;
                if (k < 16 && (lv->killed[y][x] & (1 << k))) continue;
                Enemy *e = enemies_add(&lv->enemies, type, bx * 16 + 8, type == ENEMY_FLYER || type == ENEMY_BAT ? by * 16 + 14 : (by + 1) * 16, 0);
                if (e) lv->spawnOf[e - lv->enemies.e] = k;
            }
    }

    // Item on its altar, in the middle of the small room, until taken.
    lv->itemId = lv->taken[y][x] ? ITEM_NONE : itemFor(lv, x, y);
    lv->itemX = t->wBlocks * 8;
    lv->itemY = (t->hBlocks - 2) * 16;

    // Breakable jars ('o'), unless already broken; coins left on the floor are lost.
    for (u8 i = 0; i < MAX_PICKUPS; i++) lv->pickups[i].type = PICKUP_NONE;
    lv->propCount = 0;
    for (u8 by = 0; by < t->hBlocks; by++)
        for (u8 bx = 0; bx < t->wBlocks; bx++)
            if ((t->rows[by][bx] == 'o' || t->rows[by][bx] == 'c') && lv->propCount < MAX_PROPS)
            {
                Prop *pr = &lv->props[lv->propCount];
                // Varied props, fixed per seed and room; chests: bronze 70 %, silver 25 %, gold 5 %.
                u16 roll = (lv->floor.seed + x * 37 + y * 101 + lv->propCount * 13) * 0x9E37;
                roll ^= roll >> 8;
                // Chest spots: the floor's locked chest in its room, otherwise a chest one time in two
                // (bronze 70 %, silver 30 %), else a crate (Thomas: not a chest in every room).
                if (t->rows[by][bx] == 'o' || noChest) pr->type = PROP_JAR + (roll & 3);
                else if (lockedHere && !lockedPlaced) { pr->type = PROP_CHEST_GOLD; lockedPlaced = 1; }
                else
                {
                    u8 r100 = roll % 100;
                    pr->type = r100 < 50 ? PROP_CRATE : r100 < 85 ? PROP_CHEST_BRONZE : PROP_CHEST_SILVER;
                }
                pr->alive = !(lv->propsBroken[y][x] & (1 << lv->propCount));
                pr->x = bx * 16 + 8;
                pr->y = (by + 1) * 16;
                lv->propCount++;
            }
    // No chest spot left for the locked chest: it takes a jar's place.
    if (lockedHere && !lockedPlaced && lv->propCount) lv->props[0].type = PROP_CHEST_GOLD;
    // This floor's keys lie on the ground of their rooms until picked up.
    for (u8 k = 0; k < 2; k++)
        if (x == lv->keyX[k] && y == lv->keyY[k] && !(lv->keysTaken & (1 << k)))
            for (u8 i = 0; i < MAX_PICKUPS; i++)
                if (lv->pickups[i].type == PICKUP_NONE)
                {
                    Pickup *pk = &lv->pickups[i];
                    s16 kx, ky;
                    findGround(lv, 6, 24, k ? 48 : -48, &kx, &ky);
                    pk->type = PICKUP_KEY;
                    pk->itemId = k;
                    pk->price = 0;
                    pk->x = (s32) kx << 8;
                    pk->y = (s32) ky << 8;
                    pk->vx = pk->vy = 0;
                    break;
                }

    // Shop: a heart and two upgrades on display, with their price.
    if (lv->floor.type[y][x] == ROOM_SHOP)
        for (u8 slot = 0; slot < 3; slot++)
        {
            if (lv->shopBought[y][x] & (1 << slot)) continue;
            Pickup *pk = &lv->pickups[slot];
            pk->type = PICKUP_ITEM;
            pk->itemId = slot == 0 ? ITEM_HEART : lv->shopWares[slot - 1];
            pk->price = slot == 0 ? SHOP_HEART_PRICE : SHOP_ITEM_PRICE;
            static const s8 SLOT_X[3] = { -96, -48, 48 };   // the middle is the trapdoor and its arrow
            pk->x = (s32) (t->wBlocks * 8 + SLOT_X[slot]) << 8;
            pk->y = (s32) ((t->hBlocks - 2) * 16) << 8;
            pk->vx = pk->vy = 0;
        }
    lv->shopChanged = 1;
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

// Each cell of the floor map owns a 4 x 3 screen slot of the castle (the largest room size):
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
            lv->visited[y][x] = lv->cleared[y][x] = lv->opened[y][x] = lv->taken[y][x] = lv->killed[y][x] = 0,
            lv->propsBroken[y][x] = lv->shopBought[y][x] = 0;
    lv->rng = seed | 1;
    dealFloorItems(lv);
    // The shop and the room its only door leads to (the caravan's).
    lv->shopX = lv->shopY = lv->carX = lv->carY = 0xFF;
    for (u8 y = 0; y < FLOOR_H; y++)
        for (u8 x = 0; x < FLOOR_W; x++)
            if (lv->floor.type[y][x] == ROOM_SHOP)
            {
                u8 d = lv->floor.doors[y][x];
                lv->shopX = x;
                lv->shopY = y;
                lv->carX = d & DOOR_LEFT ? x - 1 : d & DOOR_RIGHT ? x + 1 : x;
                lv->carY = d & DOOR_UP ? y - 1 : d & DOOR_DOWN ? y + 1 : y;
                lv->shopExit = lv->floor.hidden[y][x] & DOOR_LEFT ? DOOR_RIGHT : DOOR_LEFT;
            }
    pickKeyRooms(lv);
    lv->gemActive = lv->gemTaken = lv->exitOpen = 0;
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
    // A crouching slash also strikes the ground under the feet (Thomas: a cracked floor under a low
    // platform could not be reached, the jump for a downward slash landed on the platform).
    s16 fx = p->x >> 8, fy = p->y >> 8;
    u8 low = p->crouching && p->onGround;
    for (u8 i = 0; i < lv->crackedCount; i++)
    {
        Cracked *c = &lv->cracked[i];
        u8 floorHit = low && c->dir == DOOR_DOWN && fx + 11 >= c->x0 && fx - 12 <= c->x1 && fy + 10 >= c->y0 && fy <= c->y1;
        if (!floorHit && (ax0 > c->x1 || c->x0 > ax1 || ay0 > c->y1 || c->y0 > ay1)) continue;
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

static u16 lootRandom(Level *lv)
{
    lv->rng ^= lv->rng << 7;
    lv->rng ^= lv->rng >> 9;
    lv->rng ^= lv->rng << 8;
    return lv->rng;
}

static void spawnPickup(Level *lv, u8 type, s16 x, s16 y, s16 vx)
{
    for (u8 i = 0; i < MAX_PICKUPS; i++)
    {
        Pickup *pk = &lv->pickups[i];
        if (pk->type != PICKUP_NONE) continue;
        pk->type = type;
        pk->price = 0;
        pk->x = (s32) x << 8;
        pk->y = (s32) y << 8;
        pk->vx = vx;
        pk->vy = -640;   // small pop upwards
        return;
    }
}

// Coins burst out, spread left and right; sometimes a half heart.
static void dropLoot(Level *lv, s16 x, s16 y, u8 coins, u8 heartPercent)
{
    for (u8 i = 0; i < coins; i++) spawnPickup(lv, PICKUP_COIN, x, y, ((s16) i * 2 - (coins - 1)) * 96);
    if (lootRandom(lv) % 100 < heartPercent) spawnPickup(lv, PICKUP_HEART, x, y, 0);
}

// Now and then a key falls out too (percent).
static void dropKey(Level *lv, s16 x, s16 y, u8 percent)
{
    if (lootRandom(lv) % 100 >= percent) return;
    for (u8 i = 0; i < MAX_PICKUPS; i++)
        if (lv->pickups[i].type == PICKUP_NONE)
        {
            spawnPickup(lv, PICKUP_KEY, x, y, 0);
            lv->pickups[i].itemId = 0xFF;
            return;
        }
}

static void lootEnemies(Level *lv)
{
    for (u8 i = 0; i < MAX_ENEMIES; i++)
    {
        Enemy *e = &lv->enemies.e[i];
        if (e->type == ENEMY_NONE || !e->deathTimer || e->looted) continue;
        e->looted = 1;
        if (lv->spawnOf[i] < 16) lv->killed[lv->cellY][lv->cellX] |= 1 << lv->spawnOf[i];
        s16 x = e->x >> 8, y = (e->y >> 8) - 8;
        // Risk pays: Void forms always drop, and twice as much.
        if (e->type == ENEMY_BOSS) dropLoot(lv, x, y, e->voidForm ? 10 : 5, 0);
        else if (e->voidForm) dropLoot(lv, x, y, 2, 15);
        else if (lootRandom(lv) % 100 < 60) dropLoot(lv, x, y, 1, 5);
        if (e->type != ENEMY_BOSS) dropKey(lv, x, y, e->voidForm ? 6 : 3);
    }
}

static u8 breakProps(Level *lv, Player *p)
{
    if (!player_attackActive(p)) return 0;
    u8 result = 0;
    s16 ax0, ay0, ax1, ay1;
    player_attackBox(p, &ax0, &ay0, &ax1, &ay1);
    for (u8 i = 0; i < lv->propCount; i++)
    {
        Prop *pr = &lv->props[i];
        // Chests are drawn 32 x 32 (small ones about 26 x 22, the gold one 30 x 28), props 16 x 16.
        s16 hw = pr->type == PROP_CHEST_GOLD || pr->type == PROP_CRATE ? 13 : pr->type >= PROP_CHEST_BRONZE ? 11 : 6;
        s16 h = pr->type == PROP_CHEST_GOLD ? 26 : pr->type == PROP_CRATE ? 27 : pr->type >= PROP_CHEST_BRONZE ? 20 : 14;
        if (!pr->alive || ax0 > pr->x + hw || pr->x - hw > ax1 || ay0 > pr->y - 1 || pr->y - h > ay1) continue;
        // The locked chest: a key opens it (and is spent), without one the blade rings on the padlock.
        if (pr->type == PROP_CHEST_GOLD)
        {
            if (!p->keys)
            {
                if (lv->lockHitId != p->attackId)
                {
                    lv->lockHitId = p->attackId;
                    EnemySet *s = &lv->enemies;
                    s->hitX = pr->x;
                    s->hitY = pr->y - 14;
                    s->hitTimer = 8;
                    s->hitBlocked = 1;
                    result |= levelMessage(lv, MSG_NEED_KEY);
                }
                continue;
            }
            p->keys--;
        }
        pr->alive = 0;
        lv->propsBroken[lv->cellY][lv->cellX] |= 1 << i;
        u8 inVoid = level_pointInVoid(lv, pr->x, pr->y - 8);
        u8 coins, heart;
        switch (pr->type)
        {
            case PROP_CANDLE: coins = 1; heart = 5; break;
            case PROP_CRATE: coins = 1 + lootRandom(lv) % 2; heart = 30; break;
            case PROP_URN: coins = 2 + lootRandom(lv) % 3; heart = 10; break;
            case PROP_CHEST_BRONZE: coins = 3 + lootRandom(lv) % 3; heart = 10; break;
            case PROP_CHEST_SILVER: coins = 6 + lootRandom(lv) % 4; heart = 30; break;
            case PROP_CHEST_GOLD: coins = 3; heart = 0; break;
            default: coins = 1 + lootRandom(lv) % 3; heart = 20; break;
        }
        if (inVoid) coins *= 2;   // richer in the Void
        dropLoot(lv, pr->x, pr->y - 8, coins, inVoid ? heart + 15 : heart);
        dropKey(lv, pr->x, pr->y - 8, pr->type == PROP_CHEST_BRONZE || pr->type == PROP_CHEST_SILVER ? 20
                                      : pr->type == PROP_CRATE ? 8 : pr->type == PROP_CHEST_GOLD ? 0 : 3);
        if (pr->type == PROP_CHEST_GOLD)
        {
            // A gold chest holds an upgrade, popping out like the coins.
            for (u8 k = 0; k < MAX_PICKUPS; k++)
            {
                Pickup *pk = &lv->pickups[k];
                if (pk->type != PICKUP_NONE) continue;
                pk->type = PICKUP_ITEM;
                pk->itemId = dealItem(lv, lootRandom(lv));
                pk->price = 0;
                pk->x = (s32) pr->x << 8;
                pk->y = (s32) (pr->y - 16) << 8;
                pk->vx = pk->vy = 0;
                break;
            }
        }
    }
    return result;
}

// A short message, at most once every two seconds (standing on a heart would spam it).
static u8 levelMessage(Level *lv, u8 message)
{
    if (lv->messageCooldown) return 0;
    lv->message = message;
    lv->messageCooldown = fps * 2;
    return LEVEL_MESSAGE;
}

// Pickups fall and bounce on floors and platforms; touching one takes it (wares: UP, if affordable).
static u8 updatePickups(Level *lv, Player *p, s16 ppx, s16 ppy)
{
    u8 result = 0;
    for (u8 i = 0; i < MAX_PICKUPS; i++)
    {
        Pickup *pk = &lv->pickups[i];
        if (pk->type == PICKUP_NONE) continue;
        if (pk->type != PICKUP_ITEM)
        {
            pk->vy += 64;
            if (pk->vy > 1024) pk->vy = 1024;
            pk->x += pk->vx;
            s16 nx = pk->x >> 8;
            if (room_tileAt(&lv->room, nx, (pk->y >> 8) - 4) == TILE_SOLID)
            {
                pk->x -= pk->vx;
                pk->vx = -pk->vx / 2;
            }
            s16 oldY = pk->y >> 8;
            pk->y += pk->vy;
            s16 gy = pk->y >> 8;
            u8 t = room_tileAt(&lv->room, pk->x >> 8, gy);
            if (pk->vy > 0 && (t == TILE_SOLID || (t == TILE_PLATFORM && oldY <= (gy & ~7))))
            {
                pk->y = (s32) (gy & ~7) << 8;
                pk->vy = pk->vy > 384 ? -pk->vy / 3 : 0;   // one small bounce
                pk->vx = pk->vx / 2;
            }
        }
        s16 x = pk->x >> 8, y = pk->y >> 8;
        if (!nearPlayer(x, y - 4, ppx, ppy)) continue;
        if (pk->type == PICKUP_COIN) p->coins++;
        else if (pk->type == PICKUP_KEY)
        {
            if (p->keys < 9) p->keys++;
            if (pk->itemId < 2) lv->keysTaken |= 1 << pk->itemId;
            result |= LEVEL_KEY;
        }
        else if (pk->type == PICKUP_HEART)
        {
            if (p->hp >= p->maxHp)   // left on the floor for later, like Isaac
            {
                result |= levelMessage(lv, MSG_FULL_HEALTH);
                continue;
            }
            p->hp++;
        }
        else
        {
            // A ware is bought with UP, not by walking past it: the shop is entered from the side,
            // across its wares.
            if (!p->upPressed)
            {
                if (lv->message == MSG_BUY && lv->messageItem != pk->itemId) lv->messageCooldown = 0;   // next ware
                result |= levelMessage(lv, MSG_BUY);
                lv->messageItem = pk->itemId;
                continue;
            }
            if (p->coins < pk->price)
            {
                result |= levelMessage(lv, MSG_NO_MONEY);
                continue;
            }
            p->coins -= pk->price;
            player_takeItem(p, pk->itemId);
            lv->lastItem = pk->itemId;
            lv->shopBought[lv->cellY][lv->cellX] |= 1 << i;
            lv->shopChanged = 1;
            result |= LEVEL_ITEM;
        }
        pk->type = PICKUP_NONE;
    }
    return result;
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
    if (lv->messageCooldown) lv->messageCooldown--;
    lootEnemies(lv);
    result |= breakProps(lv, p);
    result |= updatePickups(lv, p, ppx, ppy);

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
            // Boss down: the gem falls from high up in the middle of the room, the exit door stands
            // on the floor a quarter of the room further right.
            s16 w = lv->room.w * ROOM_TILE;
            lv->bossFight = 0;
            lv->gemX = (s32) (w / 2) << 8;
            lv->gemY = (s32) (3 * ROOM_TILE) << 8;
            lv->gemVy = 0;
            lv->gemActive = 1;
            lv->exitOpen = 1;
            lv->exitX = w * 3 / 4;
            lv->exitY = 3 * ROOM_TILE;
            while (lv->exitY < lv->room.h * ROOM_TILE && room_tileAt(&lv->room, lv->exitX, lv->exitY) != TILE_SOLID)
                lv->exitY += ROOM_TILE;
        }
    }
    if (lv->gemActive)
    {
        s16 gx = lv->gemX >> 8, gy = lv->gemY >> 8;
        if (room_tileAt(&lv->room, gx, gy) != TILE_SOLID)   // through the branches, down to the ground
        {
            if (lv->gemVy < 1536) lv->gemVy += 64;   // at most 6 px a frame: never through a floor
            lv->gemY += lv->gemVy;
        }
        else
        {
            lv->gemY = (s32) (gy & ~7) << 8;
            lv->gemVy = 0;
        }
        if (nearPlayer(gx, gy, ppx, ppy))
        {
            if (p->upPressed)
            {
                lv->gemActive = 0;
                lv->gemTaken = 1;
                return result | LEVEL_GEM;
            }
            result |= levelMessage(lv, MSG_TAKE);
        }
    }
    if (lv->exitOpen && ppx > lv->exitX - 14 && ppx < lv->exitX + 14 && ppy > lv->exitY - 24 && ppy <= lv->exitY + 2)
    {
        if (p->upPressed) return result | LEVEL_EXIT;
        result |= levelMessage(lv, MSG_EXIT);
    }
    if (result & LEVEL_REDRAW) return result;

    s16 px = p->x >> 8, cy = (p->y >> 8) - PLAYER_H / 2;
    s16 w = lv->room.w * ROOM_TILE, h = lv->room.h * ROOM_TILE;
    u8 x = lv->cellX, y = lv->cellY, doors = level_doors(lv, x, y);

    // The upgrade room's padlocked door: walking into it with a key opens it (the key is spent).
    if (x == lv->lockX && y == lv->lockY && !lv->lockOpen && ppx > lv->padlockX - 28 && ppx < lv->padlockX + 28
        && ppy > lv->padlockY - 16 && ppy < lv->padlockY + 48)
    {
        if (p->keys)
        {
            p->keys--;
            lv->lockOpen = 1;
            level_buildRoom(&lv->room, level_templateFor(lv, x, y), level_doors(lv, x, y));
            lv->message = MSG_UNLOCKED;
            lv->messageCooldown = fps * 2;
            return result | LEVEL_REDRAW | LEVEL_MESSAGE;
        }
        result |= levelMessage(lv, MSG_NEED_KEY);
    }
    // The caravan: UP in front of its door enters the shop; the shop's opening leads back out to it.
    if (x == lv->carX && y == lv->carY && ppx > lv->caravanX - 20 && ppx < lv->caravanX + 20
        && ppy > lv->caravanY - 24 && ppy <= lv->caravanY + 2)
    {
        if (p->upPressed)
        {
            enterRoom(lv, p, lv->shopX, lv->shopY, lv->shopExit);
            return result | LEVEL_REDRAW;
        }
        result |= levelMessage(lv, MSG_ENTER);
    }
    if (x == lv->shopX && y == lv->shopY && (lv->shopExit == DOOR_LEFT ? px < 0 : px >= w))
    {
        enterRoom(lv, p, lv->carX, lv->carY, 0);
        setPos(p, lv->caravanX, lv->caravanY);
        return result | LEVEL_REDRAW;
    }

    if (px < 0 && (doors & DOOR_LEFT)) enterRoom(lv, p, x - 1, y, DOOR_RIGHT);
    else if (px >= w && (doors & DOOR_RIGHT)) enterRoom(lv, p, x + 1, y, DOOR_LEFT);
    else if (cy < 0 && (doors & DOOR_UP)) enterRoom(lv, p, x, y - 1, DOOR_DOWN);
    else if (cy >= h && (doors & DOOR_DOWN)) enterRoom(lv, p, x, y + 1, DOOR_UP);
    else return result;
    return result | LEVEL_REDRAW;
}

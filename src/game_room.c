#include "game_room.h"

typedef struct
{
    u8 x0, y0, x1, y1, tile;   // inclusive tile rectangle
} RoomRect;

// Test room for the controls: a jump-height ledge, a head-bump ceiling, one-way platforms.
static const RoomRect TEST_ROOM[] =
{
    { 0, 0, 39, 1, TILE_SOLID },     // ceiling
    { 0, 0, 1, 27, TILE_SOLID },     // left wall
    { 38, 0, 39, 27, TILE_SOLID },   // right wall
    { 0, 25, 39, 27, TILE_SOLID },   // floor
    { 15, 18, 21, 19, TILE_SOLID },  // low ceiling: 40 px of room above the floor
    { 27, 22, 32, 24, TILE_SOLID },  // step, 24 px high
    { 30, 14, 37, 15, TILE_SOLID },  // high ledge, 64 px above the step
    { 4, 20, 11, 20, TILE_PLATFORM },
    { 12, 14, 22, 14, TILE_PLATFORM },
    { 3, 9, 10, 9, TILE_PLATFORM },
};

void room_buildTest(Room *r)
{
    for (u8 y = 0; y < ROOM_H; y++)
        for (u8 x = 0; x < ROOM_W; x++) r->tiles[y][x] = TILE_EMPTY;

    for (u8 i = 0; i < sizeof(TEST_ROOM) / sizeof(TEST_ROOM[0]); i++)
    {
        const RoomRect *rc = &TEST_ROOM[i];
        for (u8 y = rc->y0; y <= rc->y1; y++)
            for (u8 x = rc->x0; x <= rc->x1; x++) r->tiles[y][x] = rc->tile;
    }
}

u8 room_tileAt(const Room *r, s16 px, s16 py)
{
    if (px < 0 || py < 0) return TILE_SOLID;
    s16 tx = px >> 3, ty = py >> 3;
    if (tx >= ROOM_W || ty >= ROOM_H) return TILE_SOLID;
    return r->tiles[ty][tx];
}

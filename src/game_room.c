#include "game_room.h"

typedef struct
{
    u8 x0, y0, x1, y1, tile;   // inclusive tile rectangle
} RoomRect;

// Test room, 2 x 2 screens: bottom floor with a step, a low ceiling and a tall pillar;
// a climb of platforms on the right; an upper floor on a shelf, reached from the climb.
static const RoomRect TEST_ROOM[] =
{
    { 0, 0, 79, 1, TILE_SOLID },      // ceiling
    { 0, 0, 1, 55, TILE_SOLID },      // left wall
    { 78, 0, 79, 55, TILE_SOLID },    // right wall
    { 0, 53, 79, 55, TILE_SOLID },    // floor

    // Bottom floor.
    { 18, 50, 23, 52, TILE_SOLID },   // step, 24 px
    { 28, 46, 34, 47, TILE_SOLID },   // low ceiling: 40 px of room above the floor
    { 40, 47, 47, 47, TILE_PLATFORM },
    { 60, 44, 62, 52, TILE_SOLID },   // pillar, 72 px: needs an almost full jump

    // Climb on the right.
    { 66, 46, 73, 46, TILE_PLATFORM },
    { 52, 41, 59, 41, TILE_PLATFORM },
    { 64, 35, 71, 35, TILE_PLATFORM },
    { 50, 29, 57, 29, TILE_PLATFORM },
    { 56, 23, 64, 23, TILE_PLATFORM },
    { 66, 14, 77, 15, TILE_SOLID },   // top-right ledge, 72 px above the last platform

    // Shelf and upper floor.
    { 2, 36, 45, 37, TILE_SOLID },
    { 8, 30, 15, 30, TILE_PLATFORM },
    { 20, 25, 27, 25, TILE_PLATFORM },
    { 32, 20, 41, 20, TILE_PLATFORM },
    { 2, 18, 10, 19, TILE_SOLID },    // top-left ledge
    { 16, 9, 24, 9, TILE_PLATFORM },
    { 38, 4, 40, 19, TILE_SOLID },    // hanging column splitting the top
};

void room_clear(Room *r, u8 w, u8 h)
{
    r->w = w;
    r->h = h;
    for (u8 y = 0; y < ROOM_MAX_H; y++)
        for (u8 x = 0; x < ROOM_MAX_W; x++) r->tiles[y][x] = TILE_EMPTY;
}

void room_fill(Room *r, u8 x0, u8 y0, u8 x1, u8 y1, u8 tile)
{
    for (u8 y = y0; y <= y1; y++)
        for (u8 x = x0; x <= x1; x++) r->tiles[y][x] = tile;
}

void room_buildTest(Room *r)
{
    room_clear(r, 80, 56);
    for (u8 i = 0; i < sizeof(TEST_ROOM) / sizeof(TEST_ROOM[0]); i++)
    {
        const RoomRect *rc = &TEST_ROOM[i];
        room_fill(r, rc->x0, rc->y0, rc->x1, rc->y1, rc->tile);
    }
}

u8 room_tileAt(const Room *r, s16 px, s16 py)
{
    s16 tx = px >> 3, ty = py >> 3;
    if (tx >= 0 && ty >= 0 && tx < r->w && ty < r->h) return r->tiles[ty][tx];

    // Outside: solid, except beyond an open door (empty border tile) so the player can walk out.
    if (tx < 0) tx = 0;
    if (ty < 0) ty = 0;
    if (tx >= r->w) tx = r->w - 1;
    if (ty >= r->h) ty = r->h - 1;
    return r->tiles[ty][tx] == TILE_EMPTY ? TILE_EMPTY : TILE_SOLID;
}

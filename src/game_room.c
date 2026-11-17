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

static u8 tileOr(const Room *r, s16 tx, s16 ty)
{
    if (tx < 0 || ty < 0 || tx >= r->w || ty >= r->h) return TILE_SOLID;   // walls go on
    return r->tiles[ty][tx];
}

// Branch piece on the platform row for tile (tx, ty): 0 middle A, 1 middle B, 2 end, 3 tip 1, 4 tip 2,
// + BRANCH_LEFT on the left side (mirrored), or -1. A run of platform tiles is a branch: middle tiles
// between two ends, and the twigs (tips) carry on over the two empty tiles past each end.
#define BRANCH_LEFT 8
static s8 branchPiece(const Room *r, s16 tx, s16 ty)
{
    u8 t = tileOr(r, tx, ty);
    if (t == TILE_PLATFORM)
    {
        u8 left = tileOr(r, tx - 1, ty) == TILE_PLATFORM, right = tileOr(r, tx + 1, ty) == TILE_PLATFORM;
        if (left && right) return tx & 1;
        return left ? 2 : 2 + BRANCH_LEFT;   // alone: a short branch, its end towards the right
    }
    if (t != TILE_EMPTY) return -1;
    if (tileOr(r, tx - 1, ty) == TILE_PLATFORM) return 3;
    if (tileOr(r, tx + 1, ty) == TILE_PLATFORM) return 3 + BRANCH_LEFT;
    if (tileOr(r, tx - 1, ty) == TILE_EMPTY && tileOr(r, tx - 2, ty) == TILE_PLATFORM) return 4;
    if (tileOr(r, tx + 1, ty) == TILE_EMPTY && tileOr(r, tx + 2, ty) == TILE_PLATFORM) return 4 + BRANCH_LEFT;
    return -1;
}

static u16 branchTile(u16 base, s8 piece)
{
    return base + (piece & 7) + (piece & BRANCH_LEFT ? CASTLE_FLIP : 0);
}

u16 room_artTile(const Room *r, s16 tx, s16 ty)
{
    if (tx < 0 || ty < 0 || tx >= r->w || ty >= r->h) return 0;
    u8 t = r->tiles[ty][tx];
    if (t == TILE_PLATFORM) return branchTile(CASTLE_BRANCH, branchPiece(r, tx, ty));
    u8 quarter = (ty & 1) << 1 | (tx & 1);
    s16 nx = (tx & 1) ? tx + 1 : tx - 1, ny = (ty & 1) ? ty + 1 : ty - 1;   // towards the block corner
    u8 bits = (tileOr(r, nx, ty) == TILE_SOLID) | (tileOr(r, tx, ny) == TILE_SOLID) << 1
        | (tileOr(r, nx, ny) == TILE_SOLID) << 2 | (t == TILE_SOLID) << 3;
    if (bits == 15)
    {
        // Inner ground. Half the blocks (fixed per position) show the mirror image, so the 16 x 16
        // pattern does not repeat; far from the air (3 tiles every way) it gets darker, for depth.
        u16 bx = tx >> 1, by = ty >> 1, h = (bx * 7 + by * 13) ^ (bx * by) ^ (bx >> 2);
        u16 flip = h & 1 ? CASTLE_FLIP : 0;
        u8 q = h & 1 ? quarter ^ 1 : quarter;
        u8 deep = tileOr(r, tx - 3, ty) == TILE_SOLID && tileOr(r, tx + 3, ty) == TILE_SOLID
            && tileOr(r, tx, ty - 3) == TILE_SOLID && tileOr(r, tx, ty + 3) == TILE_SOLID
            && tileOr(r, tx - 2, ty - 2) == TILE_SOLID && tileOr(r, tx + 2, ty - 2) == TILE_SOLID
            && tileOr(r, tx - 2, ty + 2) == TILE_SOLID && tileOr(r, tx + 2, ty + 2) == TILE_SOLID;
        return (deep ? CASTLE_DEEP + q : CASTLE_STONE + q * 16 + 15) | flip;
    }
    if (bits) return CASTLE_STONE + quarter * 16 + bits;
    s8 piece = branchPiece(r, tx, ty);
    if (piece >= 0) return branchTile(CASTLE_BRANCH, piece);
    if (tileOr(r, tx, ty - 1) != TILE_SOLID && (piece = branchPiece(r, tx, ty - 1)) >= 0)
        return branchTile(CASTLE_BRANCH_LOW, piece);
    return 0;
}

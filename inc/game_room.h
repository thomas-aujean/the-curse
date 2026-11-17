#ifndef GAME_ROOM_H
#define GAME_ROOM_H

#include "game_floor.h"   // integer types (SGDK or PC)

// A room can be larger than the screen (40 x 28 tiles in H40): the camera scrolls inside it.
// Up to 4 x 3 screens. One byte per tile (13 KB of the 64 KB of RAM): the collision code reads it
// directly, several times per frame and per character.
#define ROOM_MAX_W 160
#define ROOM_MAX_H 84
#define ROOM_TILE 8
#define SCREEN_W_TILES 40
#define SCREEN_H_TILES 28

enum
{
    TILE_EMPTY,
    TILE_SOLID,
    TILE_PLATFORM,   // one-way: solid only when landing on it from above
};

typedef struct
{
    u8 w, h;   // in tiles
    u8 tiles[ROOM_MAX_H][ROOM_MAX_W];
} Room;

void room_clear(Room *r, u8 w, u8 h);
void room_fill(Room *r, u8 x0, u8 y0, u8 x1, u8 y1, u8 tile);   // inclusive tile rectangle
void room_buildTest(Room *r);

// Room art (res/castle.png, tools/art/castle.py and outdoor.py): every map tile is a quarter of a
// 16 x 16 block, drawn from the solid neighbours around the nearest block corner (grassy top, shaded
// underside, faces, corners; empty tiles next to the ground get the grass hanging over). Platforms are
// branches: middle pieces between two ends, twigs over the two empty tiles past each end, and the
// underside on the row below. Indices into the room tileset:
#define CASTLE_STONE       1        // + quarter * 16 + bits (1 horizontal neighbour solid, 2 vertical, 4 diagonal, 8 self)
#define CASTLE_CRACKED     65       // + quarter (drawn by main.c: walls hiding a secret room)
#define CASTLE_BRANCH      69       // + piece: middle A, middle B, end, tip 1, tip 2 (right side; mirrored on the left)
#define CASTLE_BRANCH_LOW  74       // + piece: the same, on the row under the platform
#define CASTLE_BRANCH_HIGH 79       // + piece: the same, on the row above (twigs)
#define CASTLE_DEEP        84       // + quarter: inner ground far from the air, darker
#define CASTLE_TILES       88
#define CASTLE_FLIP        0x8000   // mirror the tile
u16 room_artTile(const Room *r, s16 tx, s16 ty);   // index | CASTLE_FLIP, 0 = empty

// Outside the room counts as solid, except beyond an open door.
u8 room_tileAt(const Room *r, s16 px, s16 py);

#endif

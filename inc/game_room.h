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

// Outside the room counts as solid, except beyond an open door.
u8 room_tileAt(const Room *r, s16 px, s16 py);

#endif

#ifndef GAME_ROOM_H
#define GAME_ROOM_H

#include "game_floor.h"   // integer types (SGDK or PC)

// One room = one screen in H40 mode: 40 x 28 tiles of 8 x 8 pixels.
#define ROOM_W 40
#define ROOM_H 28
#define ROOM_TILE 8

enum
{
    TILE_EMPTY,
    TILE_SOLID,
    TILE_PLATFORM,   // one-way: solid only when landing on it from above
};

typedef struct
{
    u8 tiles[ROOM_H][ROOM_W];
} Room;

void room_buildTest(Room *r);

// Outside the room counts as solid, so the player can never leave the grid.
u8 room_tileAt(const Room *r, s16 px, s16 py);

#endif

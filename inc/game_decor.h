#ifndef GAME_DECOR_H
#define GAME_DECOR_H

#include "game_room.h"

// Zone-1 decor (dead trees, graves, iron fence): whole pictures drawn on empty tiles of the room plane,
// standing on the ground, never on a platform. No collision. Placed once per room from a seed, so a
// room looks the same every time it is entered.
#define MAX_DECOR 16

void decor_place(const Room *r, u16 seed);
u8 decor_tileAt(s16 tx, s16 ty);   // decor tile (1..DECOR_TILE_COUNT - 1), 0 = none
u8 decor_count(void);

// What the room plane shows at (tx, ty): room art (room_artTile: index | CASTLE_FLIP) or the decor
// (DECOR_TILE | decor tile), on empty tiles and on the ground under its foot. 0 = nothing.
#define DECOR_TILE 0x4000
u16 decor_planeTile(const Room *r, s16 tx, s16 ty);

#endif

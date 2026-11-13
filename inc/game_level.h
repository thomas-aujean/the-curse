#ifndef GAME_LEVEL_H
#define GAME_LEVEL_H

#include "game_floor.h"
#include "game_room.h"
#include "game_player.h"
#include "game_enemy.h"

// A level = a generated floor whose cells are filled with hand-made room templates.
// The floor map only knows which cell connects to which; each room can be 1 to 2 x 2 screens.

typedef struct
{
    u8 wBlocks, hBlocks;       // 16 x 16 blocks
    const char *const *rows;
} RoomTemplate;

#define ROOM_TEMPLATE_COUNT 4
extern const RoomTemplate ROOM_TEMPLATES[ROOM_TEMPLATE_COUNT];

typedef struct
{
    Floor floor;
    Room room;
    u8 cellX, cellY;                   // current room on the floor map
    u8 visited[FLOOR_H][FLOOR_W];
    u8 cleared[FLOOR_H][FLOOR_W];      // all enemies killed: the room stays empty
    EnemySet enemies;
    u16 voidRadius;                    // safe-zone radius (floor_voidDist2 units)
    u8 roomInVoid;
} Level;

void level_start(Level *lv, Player *p, u16 seed, u8 depth);

// Builds a room from a template; doors is the DOOR_* mask of open doors. Exposed for tests.
void level_buildRoom(Room *r, const RoomTemplate *t, u8 doors);
const RoomTemplate *level_templateFor(const Level *lv, u8 x, u8 y);

// Updates the enemies of the current room, then moves to the neighbouring room when the player has
// left through a door. Returns 1 when the room changed (the screen must be redrawn).
u8 level_update(Level *lv, Player *p);

// The Void shrank: returns 1 when the current room just fell into it (enemies transformed).
u8 level_setVoidRadius(Level *lv, u16 radius);
u16 level_startRadius(const Level *lv);   // radius that covers the whole floor

#endif

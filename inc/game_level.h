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
    s16 voidCX, voidCY;                // Void circle centre, castle pixels (the boss room centre)
    u32 voidRadiusFx;                  // safe-zone radius, 1/256 px
    u32 voidRate;                      // shrink per frame, 1/256 px
    u8 voidRunning;
    u8 roomInVoid;                     // the player stands outside the circle
} Level;

void level_start(Level *lv, Player *p, u16 seed, u8 depth);

// Builds a room from a template; doors is the DOOR_* mask of open doors. Exposed for tests.
void level_buildRoom(Room *r, const RoomTemplate *t, u8 doors);
const RoomTemplate *level_templateFor(const Level *lv, u8 x, u8 y);

// Updates the enemies of the current room, then moves to the neighbouring room when the player has
// left through a door. Returns 1 when the room changed (the screen must be redrawn).
u8 level_update(Level *lv, Player *p);

#define CELL_W_PX 640     // each map cell owns a 2 x 2 screen slot of the castle
#define CELL_H_PX 448
#ifndef VOID_SECONDS     // tools/replay.ps1 -Flags "-DVOID_SECONDS=20" to watch it quickly
#define VOID_SECONDS 300  // from the first room to a circle closed on the boss
#endif

void level_setRegion(u8 pal);
u8 level_pointInVoid(const Level *lv, s16 px, s16 py);   // room pixels, current room
u8 level_cellInVoid(const Level *lv, u8 x, u8 y);        // room centre, for the map

#endif

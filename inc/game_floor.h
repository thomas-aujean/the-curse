#ifndef GAME_FLOOR_H
#define GAME_FLOOR_H

// Floor generator: pure logic, no VDP access, so it also compiles on PC (tests/floor_test.c).
#ifdef PC_TEST
#include <stdint.h>
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
#else
#include <types.h>
#endif

#define FLOOR_W 9
#define FLOOR_H 8

enum
{
    ROOM_NONE,
    ROOM_NORMAL,
    ROOM_START,
    ROOM_BOSS,          // the boss guards the gem
    ROOM_UPGRADE,       // one guaranteed upgrade per floor
    ROOM_SHOP,
    ROOM_SECRET,        // empty cell touching the most rooms
    ROOM_SUPER_SECRET,  // empty cell touching a single room
};

#define DOOR_LEFT  1
#define DOOR_RIGHT 2
#define DOOR_UP    4
#define DOOR_DOWN  8

#define NO_DIST 0xFF

typedef struct
{
    u8 type[FLOOR_H][FLOOR_W];
    u8 doors[FLOOR_H][FLOOR_W];    // visible doors (DOOR_* bits)
    u8 hidden[FLOOR_H][FLOOR_W];   // breakable walls leading to/from secret rooms
    u8 roomCount;                  // regular rooms (secrets excluded)
    u8 startX, startY;
    u8 voidX, voidY;               // centre of the Void circle (the boss room): the safe zone shrinks onto it
    u16 seed;
} Floor;

// Load-time only (uses modulo and retries, never call it inside the frame loop).
void floor_generate(Floor *f, u16 seed, u8 depth);

// Squared distance from the Void centre to a room centre, in VOID_UNIT_X/Y units (rooms are wider than tall).
// A room is outside the safe zone when this exceeds radius * radius.
#define VOID_UNIT_X 10
#define VOID_UNIT_Y 7
u16 floor_voidDist2(const Floor *f, u8 x, u8 y);

#endif

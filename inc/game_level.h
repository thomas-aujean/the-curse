#ifndef GAME_LEVEL_H
#define GAME_LEVEL_H

#include "game_floor.h"
#include "game_room.h"
#include "game_player.h"
#include "game_enemy.h"

// A level = a generated floor whose cells are filled with hand-made room templates.
// The floor map only knows which cell connects to which; each room can be 1 to 4 x 3 screens.

typedef struct
{
    u8 wBlocks, hBlocks;       // 16 x 16 blocks
    const char *const *rows;
} RoomTemplate;

#define ROOM_TEMPLATE_COUNT 9
#define ROOM_TEMPLATE_RANDOM 8   // the first ones fill ordinary rooms; the last is the boss arena
extern const RoomTemplate ROOM_TEMPLATES[ROOM_TEMPLATE_COUNT];

#define MAX_PICKUPS 10
#define SHOP_HEART_PRICE 3
#define SHOP_ITEM_PRICE 12
#define MAX_PROPS 8        // one bit each in propsBroken

enum { PICKUP_NONE, PICKUP_COIN, PICKUP_HEART, PICKUP_ITEM, PICKUP_KEY };

typedef struct
{
    u8 type;              // PICKUP_*
    u8 itemId;            // PICKUP_ITEM: what is sold; PICKUP_KEY: which floor key (0, 1) or 0xFF (a drop)
    u8 price;             // coins; 0 = free
    s32 x, y;             // 1/256 px, y = bottom
    s16 vx, vy;
} Pickup;

// Breakable props, then Rogue Legacy style chests (opened, not destroyed).
enum { PROP_JAR, PROP_URN, PROP_CRATE, PROP_CANDLE, PROP_CHEST_BRONZE, PROP_CHEST_SILVER, PROP_CHEST_GOLD };

typedef struct
{
    u8 type;              // PROP_*
    u8 alive;             // chests: closed
    s16 x, y;             // room pixels, centre of the bottom
} Prop;

typedef struct
{
    s16 x0, y0, x1, y1;   // room pixels, inclusive
    u8 dir;               // DOOR_* of the hidden passage
} Cracked;

typedef struct
{
    Floor floor;
    Room room;
    u8 cellX, cellY;                   // current room on the floor map
    u8 visited[FLOOR_H][FLOOR_W];
    u8 cleared[FLOOR_H][FLOOR_W];      // all enemies killed: the room stays empty
    u16 killed[FLOOR_H][FLOOR_W];      // bit per enemy of the room's template: killed, never back
    u8 spawnOf[MAX_ENEMIES];           // enemy slot -> its place in the template (0xFF: not from it)
    u8 opened[FLOOR_H][FLOOR_W];       // hidden passages broken open (DOOR_* bits, both sides)
    u8 taken[FLOOR_H][FLOOR_W];        // the room's item was picked up
    u8 roomItem[FLOOR_H][FLOOR_W];     // upgrade on the altar of each upgrade / super secret room
    u8 shopWares[2];                   // the shop's two upgrades (one shop per floor)
    u16 itemsDealt;                    // bit per upgrade already placed on this floor: no twice
    Cracked cracked[4];                // cracked walls of the current room
    u8 crackedCount;
    u8 itemId, lastItem;               // item on the altar of the current room (ITEM_*)
    s16 itemX, itemY;                  // room pixels, y = floor under the item
    Pickup pickups[MAX_PICKUPS];       // coins, half hearts, shop wares
    Prop props[MAX_PROPS];             // breakable jars of the current room
    u8 propCount;
    u8 propsBroken[FLOOR_H][FLOOR_W];  // bit per jar, so broken jars stay broken
    u8 shopBought[FLOOR_H][FLOOR_W];   // bit per shop slot
    u8 shopChanged;                    // a ware was bought: redraw the price tags
    // The merchant's caravan (Thomas, 2026-11-17): the shop has no door to the floor any more; its
    // caravan stands in the neighbouring room (carX, carY), UP in front of it enters the shop, the
    // shop's left opening leads back out in front of the caravan.
    u8 shopX, shopY, carX, carY;       // 0xFF: no shop on this floor
    u8 shopExit;                       // DOOR_LEFT, or DOOR_RIGHT when a secret wall takes the left
    // Keys (Thomas, 2026-11-17): two lie in rooms of each floor, others drop by chance; one opens the
    // locked chest (one per floor, it holds an upgrade), one the upgrade room's door.
    u8 keyX[2], keyY[2];               // the rooms holding this floor's two keys
    u8 keysTaken;                      // bit per floor key picked up
    u8 lockedChestX, lockedChestY;     // the room of the locked chest
    u8 upX, upY, lockX, lockY, lockDir; // upgrade room, the room before its door, that door (DOOR_*)
    u8 lockOpen;                       // the upgrade room's door was unlocked
    s16 padlockX, padlockY;            // in the room before it: the padlock on the door (px, centre)
    u8 lockHitId;                      // the slash that last knocked on the locked chest
    s16 caravanX, caravanY;            // in the caravan's room: centre of its door, ground (px)
    u8 message, messageCooldown;       // MSG_* to show (LEVEL_MESSAGE)
    u8 messageItem;                    // MSG_BUY: the ware the player stands at
    u16 rng;
    EnemySet enemies;
    s16 voidCX, voidCY;                // Void circle centre, castle pixels (the boss room centre)
    u32 voidRadiusFx;                  // safe-zone radius, 1/256 px
    u32 voidStartFx;                   // radius at the start of the floor
    u32 voidRate;                      // shrink per frame, 1/256 px
    u8 voidRunning;
    u8 roomInVoid;                     // the player stands outside the circle

    // Boss room: the Void stops when the player walks in, the doors lock a second later.
    u8 bossFight;                      // the boss of this floor is alive and fighting
    u8 bossStrong;                     // reached after the circle closed: Void form
    u8 lockTimer;
    // Boss down (Thomas, 2026-11-17): the gem falls in the middle of the room and is taken (or not)
    // with UP; a door appears, UP in front of it goes to the next floor.
    u8 gemActive;                      // waiting on the floor of the boss room
    u8 gemTaken;                       // taken on this floor
    s32 gemX, gemY;                    // 1/256 px, y = bottom
    s16 gemVy;
    u8 exitOpen;                       // the door to the next floor is there
    s16 exitX, exitY;                  // room px: centre, bottom
} Level;

// level_update result bits
#define LEVEL_REDRAW 1      // new room, or the doors just locked: redraw the room
#define LEVEL_EXIT   2      // UP in front of the exit door: the floor is won
#define LEVEL_GEM    16     // the gem was taken
#define LEVEL_KEY    32     // a key was picked up
#define LEVEL_ITEM   4      // an item was picked up (lastItem): show its name
#define LEVEL_MESSAGE 8     // show lv->message

enum { MSG_NONE, MSG_FULL_HEALTH, MSG_NO_MONEY, MSG_BUY, MSG_TAKE, MSG_EXIT, MSG_ENTER, MSG_NEED_KEY, MSG_UNLOCKED };
#define MERCHANT_X_FROM_RIGHT 40       // the merchant stands this far from the shop's right edge

void level_start(Level *lv, Player *p, u16 seed, u8 depth);
void level_enter(Level *lv, Player *p, u8 x, u8 y);   // jump straight into a room (tests, debug)

// Builds a room from a template; doors is the DOOR_* mask of open doors. Exposed for tests.
void level_buildRoom(Room *r, const RoomTemplate *t, u8 doors);
const RoomTemplate *level_templateFor(const Level *lv, u8 x, u8 y);

// Updates the enemies of the current room, then moves to the neighbouring room when the player has
// left through a door. Returns 1 when the room changed (the screen must be redrawn).
u8 level_update(Level *lv, Player *p);

#define CELL_W_PX 1280    // each map cell owns a 4 x 3 screen slot of the castle (the largest room)
#define CELL_H_PX 672
#ifndef VOID_SECONDS     // tools/replay.ps1 -Flags "-DVOID_SECONDS=20" to watch it quickly
#define VOID_SECONDS 300  // from the first room to a circle closed on the boss
#endif

void level_setRegion(u8 pal);
u8 level_pointInVoid(const Level *lv, s16 px, s16 py);   // room pixels, current room
u8 level_cellInVoid(const Level *lv, u8 x, u8 y);        // room centre, for the map
u8 level_doors(const Level *lv, u8 x, u8 y);             // visible doors + secret passages opened
u8 level_crackedAt(const Level *lv, s16 px, s16 py);     // inside a cracked wall (drawing)

#endif

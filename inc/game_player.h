#ifndef GAME_PLAYER_H
#define GAME_PLAYER_H

#include "game_room.h"

#ifdef PC_TEST
#define PAD_UP    0x0001
#define PAD_DOWN  0x0002
#define PAD_LEFT  0x0004
#define PAD_RIGHT 0x0008
#define PAD_A     0x0040
#define PAD_B     0x0010
#define PAD_C     0x0020
#else
#include <joy.h>
#define PAD_UP    BUTTON_UP
#define PAD_DOWN  BUTTON_DOWN
#define PAD_LEFT  BUTTON_LEFT
#define PAD_RIGHT BUTTON_RIGHT
#define PAD_A     BUTTON_A
#define PAD_B     BUTTON_B
#define PAD_C     BUTTON_C
#endif

// Controls: C jump (release early = shorter jump), B slash (UP + B: upward; DOWN + B in the air: downward,
// bouncing off what it hits), DOWN crouch,
// DOWN + C drop through a platform.
#define PAD_JUMP   PAD_C
#define PAD_ATTACK PAD_B

enum { ATTACK_SIDE, ATTACK_UP, ATTACK_DOWN };

// Items: a heart in secret rooms, upgrades (good or bad, like Isaac) on altars.
enum
{
    ITEM_NONE,
    ITEM_HEART,
    ITEM_FIRST_UPGRADE,
    ITEM_STONE_HEART = ITEM_FIRST_UPGRADE,   // +1 heart
    ITEM_WIND_BOOTS,                         // runs faster
    ITEM_RAVEN_WINGS,                        // double jump
    ITEM_LONG_BLADE,                         // longer side slash
    ITEM_LANTERN,                            // light lasts 50 % longer
    ITEM_GLASS_HEART,                        // -1 heart, slash +1 damage
    ITEM_LEAD_BOOTS,                         // runs slower
    ITEM_VOID_BLOOD,                         // light drains twice as fast
    ITEM_COUNT
};
#define UPGRADE_COUNT (ITEM_COUNT - ITEM_FIRST_UPGRADE)

// The hero is drawn 32 x 48; his box is slimmer and lower than the drawing, as in most action games.
#define PLAYER_W 12
#define PLAYER_H 40
#define PLAYER_CROUCH_H 24

// Positions in 1/256 pixel, speeds in 1/256 pixel per frame (no FPU on the 68000).
typedef struct
{
    s32 x, y;            // x = centre of the hitbox, y = just below the feet
    s16 vx, vy;
    u8 onGround, facingLeft, crouching, jumpHeld;
    u8 coyote, jumpBuffer, dropTimer, attackTimer, attackDir;   // attackDir: ATTACK_*
    u8 attackId;         // changes at every slash, so an enemy is hit once per slash
    s8 hp, maxHp;        // half hearts
    u8 invulnerable;     // frames of blinking after a hit
    u8 hurtTimer;        // frames of knockback without control
    u16 light;           // protection against the Void, in frames: drains inside it, refills outside
    u16 lightMax;
    // Upgrades (kept from floor to floor)
    s16 runBonus, jumpBonus;   // 1/256 px per frame, already scaled for the region
    u8 reachBonus;             // px added to the side slash
    u8 airJumps, airJumpsLeft;
    u8 lightDrain;             // light lost per frame inside the Void
    s8 damageBonus;
    u16 coins;
    u8 corrupted;        // light ran out: touched by the Void (pact: hits harder, takes more)
    u16 prevPad;
    u8 upPressed;        // UP pressed this frame: buys the ware the player stands at (shop)
} Player;

// Per-frame constants: NTSC values, scaled for 50 Hz so a jump has the same height and duration.
void player_setRegion(u8 pal);
void player_init(Player *p, s16 px, s16 py);    // new game: full health, no upgrades
void player_place(Player *p, s16 px, s16 py);   // new floor: keeps health and upgrades
void player_takeItem(Player *p, u8 item);
const char *player_itemName(u8 item);
const char *player_itemPhrase(u8 item);   // said when it is taken (21 characters at most)
void player_update(Player *p, const Room *r, u16 pad);
void player_boostUp(Player *p);
void player_bounce(Player *p);   // downward slash hit something: small hop
void player_hurt(Player *p, u8 damage, u8 fromRight);   // ignored while invulnerable

// Light protection: call once per frame with whether the current room is in the Void.
void player_updateLight(Player *p, u8 inVoid);
u8 player_slashDamage(const Player *p);   // full-jump speed upwards, e.g. when entering a room from below
u8 player_attackActive(const Player *p);
u8 player_attackTimer(const Player *p);
u8 player_attackPhase(const Player *p);   // 0 wind-up, 1 strike, 2 recovery (animation)
u8 player_attackStep(const Player *p);    // 0 wind-up, 1-2 strike, 3-4 recovery (animation frames)

// Slash hitbox in room pixels (inclusive), valid while player_attackActive().
void player_attackBox(const Player *p, s16 *x0, s16 *y0, s16 *x1, s16 *y1);

#endif

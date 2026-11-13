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

#define PLAYER_W 12
#define PLAYER_H 28
#define PLAYER_CROUCH_H 18

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
    u8 corrupted;        // light ran out: touched by the Void (pact: hits harder, takes more)
    u16 prevPad;
} Player;

// Per-frame constants: NTSC values, scaled for 50 Hz so a jump has the same height and duration.
void player_setRegion(u8 pal);
void player_init(Player *p, s16 px, s16 py);
void player_update(Player *p, const Room *r, u16 pad);
void player_boostUp(Player *p);
void player_bounce(Player *p);   // downward slash hit something: small hop
void player_hurt(Player *p, u8 damage, u8 fromRight);   // ignored while invulnerable

// Light protection: call once per frame with whether the current room is in the Void.
void player_updateLight(Player *p, u8 inVoid);
u16 player_lightMax(void);
u8 player_slashDamage(const Player *p);   // full-jump speed upwards, e.g. when entering a room from below
u8 player_attackActive(const Player *p);
u8 player_attackTimer(const Player *p);

// Slash hitbox in room pixels (inclusive), valid while player_attackActive().
void player_attackBox(const Player *p, s16 *x0, s16 *y0, s16 *x1, s16 *y1);

#endif

#ifndef GAME_ENEMY_H
#define GAME_ENEMY_H

#include "game_room.h"
#include "game_player.h"

// Each enemy has two forms: normal inside the safe zone, "Void" form (faster, tougher, meaner)
// when its room has been swallowed by the Void.

#define MAX_ENEMIES 12   // the largest rooms (4 x 3 screens) need that many

enum
{
    ENEMY_NONE,
    ENEMY_WALKER,       // patrols; Void: charges
    ENEMY_FLYER,        // homes in; Void: faster
    ENEMY_BOSS,         // guardian of the gem
    ENEMY_ARCHER,       // shoots arrows; Void: bulb spitting homing spores
    ENEMY_NINJA,        // leaps and throws shurikens; Void: lunging hunter
    ENEMY_GUARD,        // shield blocks frontal slashes; Void: only the back is vulnerable
    ENEMY_JELLY,        // splits in two when hit; Void: in three
    ENEMY_JELLY_SMALL,
    ENEMY_TYPES
};

enum { SHOT_ORB, SHOT_ARROW, SHOT_SHURIKEN, SHOT_SPORE };

#define MAX_SHOTS 8

typedef struct
{
    u8 type;
    u8 voidForm;
    s32 x, y;            // 1/256 px; x = centre, y = just below the feet (like the player)
    s32 homeX, homeY;    // flyers drift around their spawn point
    s16 vx, vy;
    s8 hp;
    u8 facingLeft;
    u8 hurtTimer;        // frozen and flashing after a hit
    u8 deathTimer;       // > 0 while the death puff plays, then the slot is freed
    u8 lastHitId;        // the same slash never hits twice
    u8 looted;           // its coins were dropped
    u16 timer;
    u8 state;            // boss: current attack (BOSS_*)
    u8 step;             // boss: position in the attack cycle
    u16 stateTimer;
    u8 pose;             // ENEMY_POSE_*: what the body is doing, for the display
    u8 attackTimer;      // soldier, guard: blow in progress then rest; archer: bow just released
} Enemy;

// Poses every enemy shares; the display maps them to drawings (main.c).
// Moving: A and B alternate with the distance travelled (flyers: with time; jelly: A on the ground,
// B in the air). WINDUP announces the attack, STRIKE is the attack itself.
enum { ENEMY_POSE_A, ENEMY_POSE_B, ENEMY_POSE_WINDUP, ENEMY_POSE_STRIKE, ENEMY_POSES };

enum { BOSS_IDLE, BOSS_DASH_WARN, BOSS_DASH, BOSS_LEAP, BOSS_SHOOT };

typedef struct
{
    u8 active;
    s32 x, y;            // centre, 1/256 px
    s16 vx, vy;
    u8 damage;
    u8 kind;             // SHOT_*
    u16 life;            // frames left (spores fade out)
} Shot;

typedef struct
{
    Enemy e[MAX_ENEMIES];
    Shot shots[MAX_SHOTS];
    u8 blocked;
} EnemySet;

void enemies_setRegion(u8 pal);
void enemies_clear(EnemySet *s);
Enemy *enemies_add(EnemySet *s, u8 type, s16 px, s16 py, u8 voidForm);   // 0 when full
void enemies_setVoid(EnemySet *s, u8 voidForm);   // everyone transforms
void enemy_setVoidForm(Enemy *e, u8 voidForm);    // keeps the damage already taken

// Moves the enemies, applies the player's slash to them and their contact damage to the player.
void enemies_update(EnemySet *s, const Room *r, Player *p);

u8 enemies_alive(const EnemySet *s);   // still fighting (death puffs excluded)
u8 enemies_blocked(EnemySet *s);       // a shield blocked a slash this frame (sound later)
void enemy_size(u8 type, s16 *w, s16 *h);

// Slot of the boss if one is still fighting, -1 otherwise.
s8 enemies_bossSlot(const EnemySet *s);

#endif

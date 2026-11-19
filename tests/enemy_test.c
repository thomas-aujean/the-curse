// PC test of enemies and combat.
//   wsl gcc -std=c99 -Wall -Wextra -DPC_TEST -Iinc tests/enemy_test.c src/game_enemy.c src/game_player.c src/game_room.c -o out/enemy_test && out/enemy_test
#include <stdio.h>
#include "game_enemy.h"

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static Room room;
static EnemySet set;

static void buildRoom(void)
{
    room_clear(&room, 40, 28);
    room_fill(&room, 0, 0, 39, 1, TILE_SOLID);
    room_fill(&room, 0, 0, 1, 27, TILE_SOLID);
    room_fill(&room, 38, 0, 39, 27, TILE_SOLID);
    room_fill(&room, 0, 25, 39, 27, TILE_SOLID);
    room_fill(&room, 10, 18, 19, 18, TILE_PLATFORM);   // x 80..159, top y = 144
}

static void step(Player *p, u16 pad, int frames)
{
    while (frames--)
    {
        player_update(p, &room, pad);
        enemies_update(&set, &room, p);
    }
}

int main(void)
{
    buildRoom();
    player_setRegion(0);
    enemies_setRegion(0);
    Player p;

    // A walker never walks off its platform, in either form.
    for (int form = 0; form < 2; form++)
    {
        enemies_clear(&set);
        enemies_add(&set, ENEMY_WALKER, 120, 144, form);
        player_init(&p, 300, 200);
        int minX = 999, maxX = 0, fell = 0;
        for (int f = 0; f < 600; f++)
        {
            enemies_update(&set, &room, &p);
            int x = set.e[0].x >> 8, y = set.e[0].y >> 8;
            if (x < minX) minX = x;
            if (x > maxX) maxX = x;
            if (y != 144) fell = 1;
        }
        CHECK(!fell && minX >= 80 && maxX <= 160 && maxX - minX > 40,
              "walker form %d patrol: x %d..%d fell %d", form, minX, maxX, fell);
    }

    // Two slashes kill a normal walker; the same slash never hits twice.
    enemies_clear(&set);
    enemies_add(&set, ENEMY_WALKER, 140, 200, 0);
    set.e[0].facingLeft = 1;
    player_init(&p, 120, 200);
    step(&p, 0, 2);
    p.facingLeft = 0;
    step(&p, PAD_ATTACK, 14);
    CHECK(set.e[0].hp == 1, "one slash = one hit (hp %d)", set.e[0].hp);
    // The blow is felt: freeze frames requested, a spark on the enemy, the enemy flashing.
    CHECK(set.hitStop && set.hitX > 120 && set.hitX < 150 && set.hitY > 160 && set.hitY < 200,
          "hit feedback: stop %d spark at %d,%d", set.hitStop, set.hitX, set.hitY);
    step(&p, 0, 100);   // let invulnerability from any contact pass
    p.x = set.e[0].x - (16 << 8);   // keep attackId: a new slash, not a reset player
    p.y = 200 << 8;
    p.facingLeft = 0;
    step(&p, 0, 2);
    step(&p, PAD_ATTACK, 5);
    CHECK(set.e[0].hp <= 0 || set.e[0].type == ENEMY_NONE, "second slash kills (hp %d)", set.e[0].hp);
    step(&p, 0, 30);
    CHECK(enemies_alive(&set) == 0, "walker gone");

    // Contact hurts, with knockback and invulnerability.
    enemies_clear(&set);
    enemies_add(&set, ENEMY_WALKER, 150, 200, 1);
    player_init(&p, 150, 200);
    step(&p, 0, 1);
    CHECK(p.hp == 5 && p.invulnerable, "Void walker contact: hp %d", p.hp);   // as hard as normal (2026-11-19)
    step(&p, 0, 10);
    CHECK(p.hp == 5, "invulnerable after a hit: hp %d", p.hp);

    // Downward slash while falling onto a walker: the player bounces up. The blow must be started ahead
    // (3 frames of wind-up): too late, the walker is touched first.
    enemies_clear(&set);
    enemies_add(&set, ENEMY_WALKER, 250, 200, 0);   // normal form: slow enough to stay under
    player_init(&p, 250, 140);                      // clear of the platform
    int bounced = 0, hit = 0;
    for (int f = 0; f < 60 && !bounced; f++)
    {
        s16 vyBefore = p.vy;
        step(&p, PAD_DOWN | (f == 6 ? PAD_ATTACK : 0), 1);
        if (set.e[0].hp < 2) hit = 1;
        if (hit && vyBefore > 0 && p.vy < 0) bounced = 1;
    }
    CHECK(hit && bounced && p.hp == p.maxHp, "pogo: hit %d bounced %d hp %d", hit, bounced, p.hp);

    // The Void transforms enemies and keeps the damage taken (the guard: 3 hp, 4 in the Void, x1.2).
    enemies_clear(&set);
    enemies_add(&set, ENEMY_GUARD, 150, 200, 0);
    set.e[0].hp = 1;
    enemies_setVoid(&set, 1);
    CHECK(set.e[0].voidForm && set.e[0].hp == 2, "Void form hp %d", set.e[0].hp);

    // Boss entrance: out of sight and untouchable, then it drops in, the screen shakes, it roars with
    // its life bar up, then fights.
    {
        enemies_clear(&set);
        Enemy *b = enemies_add(&set, ENEMY_BOSS, 160, 200, 0);
        player_init(&p, 170, 200);   // right where it will land
        step(&p, 0, 10);
        CHECK(b->state == BOSS_INTRO && b->step == 0 && p.hp == p.maxHp, "boss waits out of sight (hp %d)", p.hp);
        int shook = 0, fell = 0, bar = 0;
        for (int f = 0; f < 200 && b->state == BOSS_INTRO; f++)
        {
            p.x = 60 << 8;
            step(&p, 0, 1);
            fell |= b->step == 1 && (b->y >> 8) < 150;
            shook |= set.shake > 0;
            bar |= set.barTimer && set.barType == ENEMY_BOSS;
        }
        CHECK(fell && shook && bar && b->state == BOSS_IDLE && (b->y >> 8) == 200,
              "boss drops in (fell %d shook %d bar %d state %d y %d)", fell, shook, bar, b->state, (int) (b->y >> 8));
    }

    // Boss: goes through dash, leap and shots, stays inside the room, dies after 20 hits.
    enemies_clear(&set);
    enemies_add(&set, ENEMY_BOSS, 160, 200, 0);
    player_init(&p, 60, 200);
    int seen[6] = { 0 }, shots = 0, outside = 0;
    for (int f = 0; f < 900; f++)
    {
        p.invulnerable = 255;
        p.hp = 6;
        step(&p, 0, 1);
        seen[set.e[0].state] = 1;
        for (int i = 0; i < MAX_SHOTS; i++) shots += set.shots[i].active;
        int bx = set.e[0].x >> 8, by = set.e[0].y >> 8;
        if (bx < 16 || bx > 304 || by > 200) outside = 1;
    }
    CHECK(seen[BOSS_DASH] && seen[BOSS_LEAP] && shots && !outside,
          "boss cycle: dash %d leap %d shots %d outside %d", seen[BOSS_DASH], seen[BOSS_LEAP], shots, outside);
    CHECK(set.e[0].hp == 20, "boss starts with 20 hp (%d)", set.e[0].hp);
    int hits = 0;
    while (set.e[0].type == ENEMY_BOSS && !set.e[0].deathTimer && hits < 40)
    {
        p.x = set.e[0].x - (20 << 8);
        p.y = set.e[0].y;
        p.facingLeft = 0;
        p.attackTimer = 0;
        p.invulnerable = 255;
        step(&p, 0, 1);
        step(&p, PAD_ATTACK, 4);
        hits++;
    }
    CHECK(hits >= 20 && hits < 40, "boss dies after %d slashes (some miss while it leaps)", hits);

    // Archer: lobs an arrow that rises, then comes down on the player (level, above or below it);
    // the Void bulb's spore homes in.
    for (int form = 0; form < 2; form++)
    {
        static const s16 PLAYER_DY[3] = { 0, -60, -30 };   // the test room is flat: above only
        for (int k = 0; k < (form ? 1 : 3); k++)
        {
            enemies_clear(&set);
            enemies_add(&set, ENEMY_ARCHER, 250, 200, form);
            player_init(&p, 120, form ? 140 : 200 + PLAYER_DY[k]);
            int fired = 0, rose = 0, reached = 0;
            for (int f = 0; f < 200 && !reached; f++)
            {
                p.invulnerable = 255;
                enemies_update(&set, &room, &p);
                for (int i = 0; i < MAX_SHOTS; i++)
                    if (set.shots[i].active)
                    {
                        const Shot *sh = &set.shots[i];
                        fired = 1;
                        CHECK(sh->kind == (form ? SHOT_SPORE : SHOT_ARROW), "archer shot kind");
                        if (sh->vy < 0) rose = 1;   // lobbed, or the spore climbing to the player above
                        s32 dx = (sh->x - p.x) >> 8, dy = (sh->y - p.y) >> 8;
                        if (dx > -10 && dx < 10 && dy > -40 && dy < 0) reached = 1;
                    }
            }
            CHECK(fired && rose && (form || reached), "archer form %d, player dy %d: fired %d rose %d reached %d",
                  form, PLAYER_DY[k], fired, rose, reached);
        }
    }

    // A sword blow breaks an arrow on its way: slash when it is close, the player is not hurt.
    {
        enemies_clear(&set);
        enemies_add(&set, ENEMY_ARCHER, 250, 200, 0);
        player_init(&p, 140, 200);
        int broken = 0, slashed = 0;
        for (int f = 0; f < 200 && !broken; f++)
        {
            u16 pad = 0;
            for (int i = 0; i < MAX_SHOTS; i++)
            {
                const Shot *sh = &set.shots[i];
                if (sh->active && !slashed && ((sh->x - p.x) >> 8) < 40)
                {
                    pad = PAD_ATTACK;
                    slashed = 1;
                }
            }
            player_update(&p, &room, pad);
            int before = 0, after = 0;
            for (int i = 0; i < MAX_SHOTS; i++) before += set.shots[i].active;
            enemies_update(&set, &room, &p);
            for (int i = 0; i < MAX_SHOTS; i++) after += set.shots[i].active;
            if (slashed && after < before && p.hp == p.maxHp) broken = 1;
        }
        CHECK(broken, "a slash breaks the arrow (slashed %d, hp %d)", slashed, p.hp);
    }

    // Guard: the shield blocks a side slash from the front, the back is open. The Void colossus
    // also blocks a downward slash from the front.
    for (int form = 0; form < 2; form++)
    {
        enemies_clear(&set);
        Enemy *g = enemies_add(&set, ENEMY_GUARD, 200, 200, form);
        player_init(&p, 180, 200);
        p.facingLeft = 0;
        g->facingLeft = 1;                          // facing the player
        s8 hp = g->hp;
        step(&p, 0, 1);
        p.x = g->x - (16 << 8); p.facingLeft = 0; p.invulnerable = 255;
        step(&p, PAD_ATTACK, 4);
        CHECK(g->hp == hp, "guard form %d: front slash blocked (hp %d)", form, g->hp);
        p.attackTimer = 0;
        p.x = g->x - (14 << 8);
        p.y = g->y - (40 << 8);                     // above, slightly in front
        p.vx = p.vy = 0; p.onGround = 0; p.invulnerable = 255;   // the shield pushed the player back
        g->facingLeft = 1;
        step(&p, PAD_DOWN, 1);                      // releases B: the next press is a new slash
        step(&p, PAD_DOWN | PAD_ATTACK, 6);
        CHECK(form ? g->hp == hp : g->hp < hp, "guard form %d: downward slash (hp %d)", form, g->hp);
        hp = g->hp;
        p.attackTimer = 0;
        p.x = g->x + (16 << 8); p.y = g->y; p.vy = 0; p.facingLeft = 1; p.invulnerable = 255;
        g->facingLeft = 1;                          // back turned to the player
        g->vx = 0;
        p.prevPad = 0;                              // B released
        step(&p, PAD_ATTACK, 4);
        CHECK(g->hp < hp, "guard form %d: hit in the back (hp %d -> %d)", form, hp, g->hp);
    }

    // Jelly: splits in two (three in the Void); each small one dies in one hit.
    for (int form = 0; form < 2; form++)
    {
        enemies_clear(&set);
        enemies_add(&set, ENEMY_JELLY, 200, 200, form);
        set.e[0].stateTimer = 999;
        player_init(&p, 184, 200);
        p.facingLeft = 0;
        p.invulnerable = 255;
        step(&p, PAD_ATTACK, 4);
        int smalls = 0;
        for (int i = 0; i < MAX_ENEMIES; i++) smalls += set.e[i].type == ENEMY_JELLY_SMALL;
        CHECK(smalls == (form ? 3 : 2), "jelly form %d split into %d", form, smalls);
        for (int i = 0; i < MAX_ENEMIES; i++)
            if (set.e[i].type == ENEMY_JELLY_SMALL) CHECK(set.e[i].hp == 1, "small jelly hp %d", set.e[i].hp);
    }

    // Ninja: leaps at the player; the normal one throws a shuriken, the Void hunter does not.
    for (int form = 0; form < 2; form++)
    {
        enemies_clear(&set);
        enemies_add(&set, ENEMY_NINJA, 250, 200, form);
        player_init(&p, 150, 200);
        int leapt = 0, shuriken = 0;
        for (int f = 0; f < 240; f++)
        {
            p.invulnerable = 255;
            p.x = 150 << 8;
            enemies_update(&set, &room, &p);
            if (set.e[0].vy < 0) leapt = 1;
            for (int i = 0; i < MAX_SHOTS; i++) shuriken |= set.shots[i].active && set.shots[i].kind == SHOT_SHURIKEN;
        }
        CHECK(leapt && shuriken == !form, "ninja form %d: leapt %d shuriken %d", form, leapt, shuriken);
    }

    // Spirits drift through inner walls but never leave the room.
    enemies_clear(&set);
    enemies_add(&set, ENEMY_FLYER, 40, 100, 1);
    player_init(&p, 30, 200);
    int escaped = 0;
    for (int f = 0; f < 600; f++)
    {
        p.invulnerable = 255;
        p.x = -40 * 256;                // lure it out of the room
        enemies_update(&set, &room, &p);
        if ((set.e[0].x >> 8) < 16 || (set.e[0].x >> 8) > 304) escaped = 1;
    }
    CHECK(!escaped, "flyer left the room");

    // Light protection: 20 s inside the Void, then the pact (double slash damage, +1 damage taken).
    player_init(&p, 250, 200);
    for (int f = 0; f < 20 * 60 - 1; f++) player_updateLight(&p, 1);
    CHECK(!p.corrupted && p.light == 1, "light lasts 20 s (light %u)", p.light);
    player_updateLight(&p, 1);
    CHECK(p.corrupted && player_slashDamage(&p) == 2, "corrupted when the light runs out");
    p.invulnerable = 0;
    player_hurt(&p, 1, 0);
    CHECK(p.hp == 4, "pact: hits cost one more (hp %d)", p.hp);
    for (int f = 0; f < 149; f++) player_updateLight(&p, 0);
    CHECK(p.corrupted, "still corrupted before a quarter of light is back");
    player_updateLight(&p, 0);
    CHECK(!p.corrupted, "released at a quarter of light (light %u)", p.light);

    // Soldier: raises its sword when the player is in front, then strikes further than its body.
    // Standing in reach = hurt; crouching under the swing = safe; a slash during the wind-up stops it.
    for (int crouch = 0; crouch < 2; crouch++)
    {
        enemies_clear(&set);
        enemies_add(&set, ENEMY_WALKER, 150, 200, 0);
        set.e[0].facingLeft = 0;
        player_init(&p, 172, 200);   // 22 px in front: out of contact, inside the sword's reach
        int windup = 0, strike = 0, hurtAt = -1;
        for (int f = 0; f < 60; f++)
        {
            if (strike && set.e[0].pose != ENEMY_POSE_STRIKE) break;   // blow over: it walks on, into the player
            step(&p, crouch ? PAD_DOWN : 0, 1);
            if (set.e[0].pose == ENEMY_POSE_WINDUP && !strike) windup++;
            if (set.e[0].pose == ENEMY_POSE_STRIKE) strike++;
            if (p.hp < p.maxHp && hurtAt < 0) hurtAt = f;
        }
        if (crouch) CHECK(strike && p.hp == p.maxHp, "crouching dodges the soldier's swing (hp %d)", p.hp);
        else CHECK(windup >= 20 && hurtAt >= 20, "soldier: wind-up %d frames, then hurts at frame %d", windup, hurtAt);
    }
    enemies_clear(&set);
    enemies_add(&set, ENEMY_WALKER, 150, 200, 1);   // Void form: 4 hp, survives the slash
    set.e[0].facingLeft = 0;
    player_init(&p, 172, 200);
    p.facingLeft = 1;
    step(&p, 0, 4);
    CHECK(set.e[0].pose == ENEMY_POSE_WINDUP, "soldier winds up (pose %d)", set.e[0].pose);
    step(&p, PAD_ATTACK, 8);
    CHECK(set.e[0].hp < 4 && set.e[0].pose != ENEMY_POSE_WINDUP && set.e[0].pose != ENEMY_POSE_STRIKE,
          "a slash breaks the wind-up (hp %d pose %d)", set.e[0].hp, set.e[0].pose);
    step(&p, 0, 12);
    CHECK(p.hp == p.maxHp, "no blow after a broken wind-up (hp %d)", p.hp);

    // Guard: same blow with its spear, and crouching does not save from it.
    enemies_clear(&set);
    enemies_add(&set, ENEMY_GUARD, 150, 200, 0);
    set.e[0].facingLeft = 0;
    player_init(&p, 176, 200);
    step(&p, PAD_DOWN, 60);
    CHECK(p.hp < p.maxHp, "the guard's spear reaches a crouching player (hp %d)", p.hp);

    // Guard: spear behind the back, then a crescent while lunging: it reaches a player 50 px away
    // (the lunge), and hits harder than a touch.
    {
        enemies_clear(&set);
        Enemy *g = enemies_add(&set, ENEMY_GUARD, 120, 200, 0);
        g->facingLeft = 0;
        player_init(&p, 170, 200);
        int windup = 0, slashSeen = 0;
        s32 x0 = g->x;
        for (int f = 0; f < 60 && p.hp == p.maxHp; f++)
        {
            p.x = 170 << 8;   // the player stands still
            enemies_update(&set, &room, &p);
            if (g->pose == ENEMY_POSE_WINDUP && !slashSeen) windup++;
            if (g->slash == GUARD_SLASH_SIDE) slashSeen = 1;
        }
        CHECK(windup > 10 && slashSeen && g->x - x0 > (20 << 8) && p.hp <= p.maxHp - 2,
              "guard lunge: windup %d slash %d moved %d hp %d", windup, slashSeen, (int) ((g->x - x0) >> 8), p.hp);
    }
    // Guard: the player above it gets the crescent upwards.
    {
        enemies_clear(&set);
        Enemy *g = enemies_add(&set, ENEMY_GUARD, 120, 200, 0);
        player_init(&p, 125, 144);   // on the platform over it
        int up = 0;
        for (int f = 0; f < 60 && p.hp == p.maxHp; f++)
        {
            enemies_update(&set, &room, &p);
            if (g->pose == ENEMY_POSE_UP && g->slash == GUARD_SLASH_UP) up = 1;
        }
        CHECK(up && p.hp < p.maxHp, "guard strikes upwards (up %d, hp %d)", up, p.hp);
    }
    // Bat: hangs from the ceiling over its spawn point, then swoops at the player nearby and hurts.
    {
        enemies_clear(&set);
        Enemy *bat = enemies_add(&set, ENEMY_BAT, 260, 120, 0);
        player_init(&p, 60, 200);   // far: it stays hanging
        enemies_update(&set, &room, &p);
        int hangY = bat->y >> 8;
        for (int f = 0; f < 60; f++) enemies_update(&set, &room, &p);
        CHECK(bat->pose == ENEMY_POSE_WINDUP && hangY < 50 && (bat->y >> 8) == hangY,
              "bat hangs from the ceiling (y %d, pose %d)", hangY, bat->pose);
        player_init(&p, 230, 200);
        int dived = 0;
        for (int f = 0; f < 120 && p.hp == p.maxHp; f++)
        {
            enemies_update(&set, &room, &p);
            if (bat->state == 1) dived = 1;
        }
        CHECK(dived && p.hp < p.maxHp, "bat swoops and hurts (dived %d, hp %d)", dived, p.hp);
    }

    // Clash: a downward slash on the guard's upward crescent: nobody hurt, the player bounces.
    {
        enemies_clear(&set);
        Enemy *g = enemies_add(&set, ENEMY_GUARD, 120, 200, 0);
        player_init(&p, 122, 120);
        p.onGround = 0;
        int hp = g->hp, clashed = 0;
        for (int f = 0; f < 80 && !clashed; f++)
        {
            u16 pad = PAD_DOWN;
            if (g->pose == ENEMY_POSE_WINDUP && g->attackTimer == 53) pad |= PAD_ATTACK;   // just before its crescent
            p.y = 120 << 8;    // hovering above it
            p.vy = 0;
            p.onGround = 0;
            player_update(&p, &room, pad);
            p.y = 120 << 8;
            p.onGround = 0;
            enemies_update(&set, &room, &p);
            if (set.blocked && p.vy < 0) clashed = 1;
        }
        CHECK(clashed && p.hp == p.maxHp && g->hp == hp, "clash: %d, player hp %d, guard hp %d", clashed, p.hp, g->hp);
    }

    // Poses for the display: walking legs alternate, the archer aims before shooting, the jelly
    // stretches in the air, the spectre lunges when it chases.
    enemies_clear(&set);
    enemies_add(&set, ENEMY_WALKER, 120, 144, 0);
    player_init(&p, 300, 200);
    int seenPose[ENEMY_POSES] = { 0 };
    for (int f = 0; f < 200; f++)
    {
        enemies_update(&set, &room, &p);
        seenPose[set.e[0].pose] = 1;
    }
    CHECK(seenPose[ENEMY_POSE_A] && seenPose[ENEMY_POSE_B] && !seenPose[ENEMY_POSE_WINDUP] && !seenPose[ENEMY_POSE_STRIKE],
          "patrolling soldier only walks");
    enemies_clear(&set);
    enemies_add(&set, ENEMY_ARCHER, 100, 200, 0);
    player_init(&p, 220, 200);
    int aimed = 0, order = 1, released = 0;
    for (int f = 0; f < 200; f++)
    {
        p.invulnerable = 255;
        enemies_update(&set, &room, &p);
        if (set.e[0].pose == ENEMY_POSE_WINDUP) aimed++;
        if (set.e[0].pose == ENEMY_POSE_STRIKE)
        {
            if (!released && aimed < 20) order = 0;
            released++;
        }
    }
    CHECK(aimed && released && order, "archer aims (%d frames) before releasing (%d)", aimed, released);
    enemies_clear(&set);
    enemies_add(&set, ENEMY_JELLY, 100, 200, 0);
    enemies_add(&set, ENEMY_FLYER, 200, 100, 0);
    player_init(&p, 220, 200);
    int flat = 0, tall = 0, lunge = 0;
    for (int f = 0; f < 200; f++)
    {
        p.invulnerable = 255;
        enemies_update(&set, &room, &p);
        if (set.e[0].pose == ENEMY_POSE_A) flat++;
        if (set.e[0].pose == ENEMY_POSE_B) tall++;
        if (set.e[1].pose == ENEMY_POSE_STRIKE) lunge++;
    }
    CHECK(flat && tall && lunge, "jelly flat %d stretched %d, spectre lunging %d", flat, tall, lunge);

    // Deeper floors (Thomas, 2026-11-18): more life, and more blows.
    for (int t = 0; t < 3; t++)
    {
        enemies_setTier(t);
        enemies_clear(&set);
        Enemy *w = enemies_add(&set, ENEMY_WALKER, 150, 200, 0);
        CHECK(w->hp == 2 * (2 + t) / 2, "tier %d: soldier hp %d", t, w->hp);

        // The archer's volley: 1, 2, 3 arrows.
        enemies_clear(&set);
        enemies_add(&set, ENEMY_ARCHER, 100, 200, 0);
        player_init(&p, 220, 200);
        int most = 0;
        for (int f = 0; f < 120; f++)
        {
            p.invulnerable = 255;
            enemies_update(&set, &room, &p);
            int n = 0;
            for (int i = 0; i < MAX_SHOTS; i++) n += set.shots[i].active && set.shots[i].kind == SHOT_ARROW;
            if (n > most) most = n;
        }
        CHECK(most == t + 1, "tier %d: volley of %d arrows", t, most);

        // The spectre's fireballs: 1, 3, then 5 all around it.
        enemies_clear(&set);
        Enemy *sp = enemies_add(&set, ENEMY_FLYER, 200, 120, 0);
        sp->stateTimer = 1;
        player_init(&p, 140, 200);
        int fire = 0, up = 0;
        for (int f = 0; f < 60 && !fire; f++)
        {
            p.invulnerable = 255;
            enemies_update(&set, &room, &p);
            for (int i = 0; i < MAX_SHOTS; i++)
                if (set.shots[i].active && set.shots[i].kind == SHOT_FIRE) { fire++; up |= set.shots[i].vy < 0; }
        }
        CHECK(fire == (t == 0 ? 1 : t == 1 ? 3 : 5) && (t < 2 || up), "tier %d: %d fireballs (one up %d)", t, fire, up);

        // The guard: one lunge, then two in a row.
        enemies_clear(&set);
        Enemy *g = enemies_add(&set, ENEMY_GUARD, 120, 200, 0);
        g->facingLeft = 0;
        player_init(&p, 170, 200);
        int lunges = 0, was = 0;
        for (int f = 0; f < 90; f++)
        {
            p.invulnerable = 255;
            p.x = 170 << 8;
            enemies_update(&set, &room, &p);
            int now = g->slash == GUARD_SLASH_SIDE;
            if (now && !was) lunges++;
            was = now;
        }
        CHECK(lunges == (t ? 2 : 1), "tier %d: guard lunges %d", t, lunges);
    }
    enemies_setTier(0);

    printf("%d failures\n", failures);
    return failures != 0;
}

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
    step(&p, 0, 100);   // let invulnerability from any contact pass
    p.x = set.e[0].x - (16 << 8);   // keep attackId: a new slash, not a reset player
    p.y = 200 << 8;
    p.facingLeft = 0;
    step(&p, 0, 2);
    step(&p, PAD_ATTACK, 3);
    CHECK(set.e[0].hp <= 0 || set.e[0].type == ENEMY_NONE, "second slash kills (hp %d)", set.e[0].hp);
    step(&p, 0, 30);
    CHECK(enemies_alive(&set) == 0, "walker gone");

    // Contact hurts, with knockback and invulnerability.
    enemies_clear(&set);
    enemies_add(&set, ENEMY_WALKER, 150, 200, 1);
    player_init(&p, 150, 200);
    step(&p, 0, 1);
    CHECK(p.hp == 4 && p.invulnerable, "Void walker contact: hp %d", p.hp);
    step(&p, 0, 10);
    CHECK(p.hp == 4, "invulnerable after a hit: hp %d", p.hp);

    // Downward slash while falling onto a walker: the player bounces up.
    enemies_clear(&set);
    enemies_add(&set, ENEMY_WALKER, 250, 200, 0);   // normal form: slow enough to stay under
    player_init(&p, 250, 140);                      // clear of the platform
    int bounced = 0, hit = 0;
    for (int f = 0; f < 60 && !bounced; f++)
    {
        s16 vyBefore = p.vy;
        step(&p, PAD_DOWN | (f == 8 ? PAD_ATTACK : 0), 1);
        if (set.e[0].hp < 2) hit = 1;
        if (hit && vyBefore > 0 && p.vy < 0) bounced = 1;
    }
    CHECK(hit && bounced && p.hp == p.maxHp, "pogo: hit %d bounced %d hp %d", hit, bounced, p.hp);

    // The Void transforms enemies and keeps the damage taken.
    enemies_clear(&set);
    enemies_add(&set, ENEMY_WALKER, 150, 200, 0);
    set.e[0].hp = 1;
    enemies_setVoid(&set, 1);
    CHECK(set.e[0].voidForm && set.e[0].hp == 3, "Void form hp %d", set.e[0].hp);

    // Boss: goes through dash, leap and shots, stays inside the room, dies after 16 hits.
    enemies_clear(&set);
    enemies_add(&set, ENEMY_BOSS, 160, 200, 0);
    player_init(&p, 60, 200);
    int seen[5] = { 0 }, shots = 0, outside = 0;
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
    CHECK(set.e[0].hp == 16, "boss starts with 16 hp (%d)", set.e[0].hp);
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
    CHECK(hits >= 16 && hits < 40, "boss dies after %d slashes (some miss while it leaps)", hits);

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

    printf("%d failures\n", failures);
    return failures != 0;
}

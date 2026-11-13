// PC test of the player physics in the test room (NTSC and PAL).
//   wsl gcc -std=c99 -Wall -Wextra -DPC_TEST -Iinc tests/player_test.c src/game_player.c src/game_room.c -o out/player_test && out/player_test
#include <stdio.h>
#include "game_player.h"

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static Room room;

// The original one-screen test room: the checks below rely on its exact coordinates.
static void buildRoom(void)
{
    room_clear(&room, 40, 28);
    room_fill(&room, 0, 0, 39, 1, TILE_SOLID);
    room_fill(&room, 0, 0, 1, 27, TILE_SOLID);
    room_fill(&room, 38, 0, 39, 27, TILE_SOLID);
    room_fill(&room, 0, 25, 39, 27, TILE_SOLID);
    room_fill(&room, 15, 18, 21, 19, TILE_SOLID);
    room_fill(&room, 27, 22, 32, 24, TILE_SOLID);
    room_fill(&room, 30, 14, 37, 15, TILE_SOLID);
    room_fill(&room, 4, 20, 11, 20, TILE_PLATFORM);
    room_fill(&room, 12, 14, 22, 14, TILE_PLATFORM);
    room_fill(&room, 3, 9, 10, 9, TILE_PLATFORM);
}

static int px(const Player *p) { return p->x >> 8; }
static int py(const Player *p) { return p->y >> 8; }

static void run(Player *p, u16 pad, int frames)
{
    while (frames--) player_update(p, &room, pad);
}

static void region(int pal)
{
    const char *name = pal ? "PAL" : "NTSC";
    int fps = pal ? 50 : 60;
    Player p;
    player_setRegion(pal);

    // Standing still: stays grounded on the floor (top at y = 200), no flicker. x = 110 is clear above.
    player_init(&p, 110, 200);
    int flicker = 0;
    for (int i = 0; i < fps; i++) { player_update(&p, &room, 0); if (!p.onGround) flicker++; }
    CHECK(flicker == 0 && py(&p) == 200, "%s standing: flicker %d, y %d", name, flicker, py(&p));

    // Full jump height and air time.
    int minY = 200, air = 0;
    player_update(&p, &room, PAD_JUMP);
    while (!p.onGround && air < 200) { player_update(&p, &room, PAD_JUMP); air++; if (py(&p) < minY) minY = py(&p); }
    printf("%s full jump: %d px, %.2f s\n", name, 200 - minY, (air + 1) / (double) fps);
    CHECK(200 - minY >= 76 && 200 - minY <= 84, "%s jump height %d", name, 200 - minY);

    // Short hop when C is tapped.
    run(&p, 0, 5);
    minY = 200;
    run(&p, PAD_JUMP, 2);
    for (int i = 0; i < fps; i++) { player_update(&p, &room, 0); if (py(&p) < minY) minY = py(&p); }
    printf("%s short hop: %d px\n", name, 200 - minY);
    CHECK(200 - minY < 30, "%s short hop %d", name, 200 - minY);

    // Run speed: pixels per second at full speed.
    player_init(&p, 48, 200);
    run(&p, PAD_RIGHT, 20);
    int x0 = px(&p);
    run(&p, PAD_RIGHT, fps / 2);
    printf("%s run: %d px/s\n", name, (px(&p) - x0) * 2);

    // Wall: running into the step (x 216) stops at its left face.
    run(&p, PAD_RIGHT, fps * 2);
    CHECK(px(&p) + PLAYER_W / 2 == 216, "%s step wall: right edge %d", name, px(&p) + PLAYER_W / 2);

    // Climb the step, then jump to the high ledge (top y = 112) from its left part.
    run(&p, PAD_RIGHT | PAD_JUMP, 20);
    run(&p, 0, fps);
    CHECK(py(&p) == 176 && p.onGround, "%s on step: y %d", name, py(&p));
    player_init(&p, 222, 176);
    run(&p, 0, 3);
    run(&p, PAD_JUMP, 12);
    run(&p, PAD_JUMP | PAD_RIGHT, fps);
    run(&p, PAD_RIGHT, fps);
    CHECK(py(&p) == 112 && p.onGround, "%s high ledge: x %d y %d", name, px(&p), py(&p));

    // Head bump under the low ceiling (bottom y = 160): the head never goes above it.
    player_init(&p, 144, 200);
    run(&p, 0, 2);
    minY = 200;
    for (int i = 0; i < fps; i++) { player_update(&p, &room, PAD_JUMP); if (py(&p) < minY) minY = py(&p); }
    CHECK(minY - PLAYER_H >= 160, "%s head bump: head at %d", name, minY - PLAYER_H);

    // One-way platform (row 20, top y = 160, x 32..95): jump through from below, land, then drop.
    player_init(&p, 64, 200);
    run(&p, 0, 2);
    run(&p, PAD_JUMP, fps * 2);
    CHECK(py(&p) == 160 && p.onGround, "%s land on platform: y %d", name, py(&p));
    run(&p, 0, 5);
    run(&p, PAD_DOWN | PAD_JUMP, 2);
    run(&p, 0, fps);
    CHECK(py(&p) == 200 && p.onGround, "%s drop through: y %d", name, py(&p));

    // Crouch under the low ceiling is allowed, standing up there is not.
    player_init(&p, 100, 200);
    run(&p, 0, 2);
    run(&p, PAD_DOWN, 5);
    CHECK(p.crouching, "%s crouch", name);

    // UP + B: upward slash, hitbox above the head.
    player_init(&p, 100, 200);
    run(&p, 0, 2);
    run(&p, PAD_UP | PAD_ATTACK, 3);
    s16 bx0, by0, bx1, by1;
    player_attackBox(&p, &bx0, &by0, &bx1, &by1);
    CHECK(p.attackDir == ATTACK_UP && player_attackActive(&p) && by1 < 200 - PLAYER_H + 8 && bx0 < 100 && bx1 > 100,
          "%s upward slash box %d,%d-%d,%d", name, bx0, by0, bx1, by1);
}

static void upgrades(void)
{
    Player p;
    player_setRegion(0);

    // Raven wings: a second jump in the air, once.
    player_init(&p, 110, 200);
    player_takeItem(&p, ITEM_RAVEN_WINGS);
    run(&p, 0, 2);
    run(&p, PAD_JUMP, 30);         // full first jump, now falling
    CHECK(p.vy > 0, "wings: falling before the second jump");
    run(&p, 0, 1);
    run(&p, PAD_JUMP, 1);
    CHECK(p.vy < 0, "wings: second jump (vy %d)", p.vy);
    int extra = 0;
    for (int f = 0; f < 120 && !p.onGround; f++)
    {
        s16 before = p.vy;
        run(&p, (f & 1) ? PAD_JUMP : 0, 1);
        if (!p.onGround && before >= 0 && p.vy < 0) extra++;
    }
    CHECK(extra == 0, "wings: no third jump (%d)", extra);

    // Wind boots run faster, lead boots slower.
    player_init(&p, 60, 200);
    player_takeItem(&p, ITEM_WIND_BOOTS);
    run(&p, PAD_RIGHT, 30);
    CHECK(p.vx > 704, "wind boots: vx %d", p.vx);
    player_init(&p, 60, 200);
    player_takeItem(&p, ITEM_LEAD_BOOTS);
    run(&p, PAD_RIGHT, 30);
    CHECK(p.vx < 704, "lead boots: vx %d", p.vx);

    // Hearts: stone adds one, glass removes one but hits harder; health never exceeds the max.
    player_init(&p, 60, 200);
    player_takeItem(&p, ITEM_STONE_HEART);
    CHECK(p.maxHp == 8 && p.hp == 8, "stone heart: %d/%d", p.hp, p.maxHp);
    player_takeItem(&p, ITEM_GLASS_HEART);
    CHECK(p.maxHp == 6 && p.hp == 6 && player_slashDamage(&p) == 2, "glass heart");
    player_takeItem(&p, ITEM_HEART);
    CHECK(p.hp == 6, "heart capped at max");

    // Lantern: 50 % more light; Void blood: drains twice as fast.
    player_init(&p, 60, 200);
    player_takeItem(&p, ITEM_LANTERN);
    CHECK(p.lightMax == 30 * 60, "lantern: %u", p.lightMax);
    player_takeItem(&p, ITEM_VOID_BLOOD);
    player_updateLight(&p, 1);
    CHECK(p.light == 30 * 60 - 2, "void blood drain");

    // Long blade reaches further; new floor keeps everything.
    player_init(&p, 100, 200);
    player_takeItem(&p, ITEM_LONG_BLADE);
    run(&p, 0, 2);
    run(&p, PAD_ATTACK, 3);
    s16 x0, y0, x1, y1;
    player_attackBox(&p, &x0, &y0, &x1, &y1);
    CHECK(x1 - x0 == 25, "long blade box %d", x1 - x0);
    player_place(&p, 50, 200);
    CHECK(p.reachBonus == 10 && p.hp == p.maxHp, "upgrades kept on a new floor");
}

int main(void)
{
    buildRoom();
    upgrades();
    region(0);
    region(1);
    printf("%d failures\n", failures);
    return failures != 0;
}

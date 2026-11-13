// PC test of the room templates and of moving between rooms.
//   wsl gcc -std=c99 -Wall -Wextra -DPC_TEST -Iinc tests/level_test.c src/game_level.c src/rooms.c src/game_floor.c src/game_player.c src/game_room.c -o out/level_test && out/level_test
#include <stdio.h>
#include <string.h>
#include "game_level.h"

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static int bodyFree(const Room *r, const Player *p)
{
    int px = p->x >> 8, py = p->y >> 8;
    for (int y = py - PLAYER_H; y < py; y += 4)
        for (int x = px - PLAYER_W / 2; x < px + PLAYER_W / 2; x += 4)
            if (room_tileAt(r, x, y) == TILE_SOLID) return 0;
    return 1;
}

int main(void)
{
    // Template shapes: every row the right length, every door marker present.
    for (int i = 0; i < ROOM_TEMPLATE_COUNT; i++)
    {
        const RoomTemplate *t = &ROOM_TEMPLATES[i];
        CHECK(t->wBlocks * 2 <= ROOM_MAX_W && t->hBlocks * 2 <= ROOM_MAX_H, "template %d too big", i);
        int doors[4] = { 0 };
        for (int y = 0; y < t->hBlocks; y++)
        {
            CHECK((int) strlen(t->rows[y]) == t->wBlocks, "template %d row %d: %d chars", i, y, (int) strlen(t->rows[y]));
            for (int x = 0; x < t->wBlocks; x++)
            {
                const char *m = strchr("LRUD", t->rows[y][x]);
                if (m && t->rows[y][x]) doors[m - "LRUD"]++;
            }
        }
        CHECK(doors[0] && doors[1] && doors[2] && doors[3], "template %d misses a door marker", i);
    }

    // Walk through every door of many floors: the player must arrive inside the new room, free of
    // walls, settle on the ground within two seconds and not bounce straight back.
    player_setRegion(0);
    enemies_setRegion(0);
    static Level lv;
    Player p;
    player_init(&p, 0, 0);
    int moves = 0;
    for (unsigned seed = 1; seed < 2000; seed += 13)
    {
        level_start(&lv, &p, (u16) seed, 3);
        CHECK(bodyFree(&lv.room, &p), "seed %u: start inside a wall", seed);
        for (int y = 0; y < FLOOR_H; y++)
            for (int x = 0; x < FLOOR_W; x++)
            {
                u8 doors = lv.floor.doors[y][x];
                if (!doors) continue;
                static const u8 BIT[4] = { DOOR_LEFT, DOOR_RIGHT, DOOR_UP, DOOR_DOWN };
                static const int DX[4] = { -1, 1, 0, 0 }, DY[4] = { 0, 0, -1, 1 };
                for (int d = 0; d < 4; d++)
                {
                    if (!(doors & BIT[d])) continue;
                    // Stand in room (x, y), then push the player just past the door.
                    lv.cellX = x;
                    lv.cellY = y;
                    level_buildRoom(&lv.room, level_templateFor(&lv, x, y), doors);
                    s16 w = lv.room.w * 8, h = lv.room.h * 8;
                    s16 px = d == 0 ? -1 : d == 1 ? w : w / 2;
                    s16 py = d == 2 ? 0 : d == 3 ? h + PLAYER_H : h - 32;
                    player_init(&p, px, py);
                    p.vy = d == 3 ? 256 : 0;
                    CHECK(level_update(&lv, &p), "seed %u (%d,%d) dir %d: no move", seed, x, y, d);
                    CHECK(lv.cellX == x + DX[d] && lv.cellY == y + DY[d], "seed %u: wrong room", seed);
                    CHECK(bodyFree(&lv.room, &p), "seed %u (%d,%d) dir %d: arrived in a wall", seed, x, y, d);
                    int settled = 0;
                    for (int f = 0; f < 120 && !settled; f++)
                    {
                        player_update(&p, &lv.room, 0);
                        CHECK(!(level_update(&lv, &p) & LEVEL_REDRAW), "seed %u (%d,%d) dir %d: bounced back", seed, x, y, d);
                        settled = p.onGround;
                    }
                    CHECK(settled, "seed %u (%d,%d) dir %d: never landed", seed, x, y, d);
                    moves++;
                }
            }
    }
    // The Void: every room is safe at the start, the circle closes on the boss in VOID_SECONDS.
    for (unsigned seed = 1; seed < 400; seed += 7)
    {
        level_start(&lv, &p, (u16) seed, 3);
        for (int y = 0; y < FLOOR_H; y++)
            for (int x = 0; x < FLOOR_W; x++)
            {
                u8 t = lv.floor.type[y][x];
                if (t != ROOM_NONE && t != ROOM_SECRET && t != ROOM_SUPER_SECRET)
                    CHECK(!level_cellInVoid(&lv, x, y), "seed %u: room (%d,%d) in the Void at start", seed, x, y);
            }
        u32 frames = 0;
        while (lv.voidRunning && frames < 400 * 60) { level_update(&lv, &p); frames++; }
        CHECK(frames >= VOID_SECONDS * 60 * 97 / 100 && frames <= VOID_SECONDS * 60,   // rate rounded up: up to 3 % early
              "seed %u: Void closed in %lu frames", seed, (unsigned long) frames);
    }

    // Boss room: entering stops the Void, the doors lock without trapping the player in a wall,
    // the boss drops the gem, picking it up wins the floor.
    for (unsigned seed = 5; seed < 300; seed += 11)
    {
        level_start(&lv, &p, (u16) seed, 3);
        u8 bx = lv.floor.voidX, by = lv.floor.voidY, nx = bx, ny = by, dir = 0;
        u8 bd = lv.floor.doors[by][bx];
        if (bd & DOOR_LEFT) { nx = bx - 1; dir = 1; }        // neighbour exits through its right door
        else if (bd & DOOR_RIGHT) { nx = bx + 1; dir = 0; }
        else if (bd & DOOR_UP) { ny = by - 1; dir = 3; }
        else { ny = by + 1; dir = 2; }
        lv.cellX = nx;
        lv.cellY = ny;
        level_buildRoom(&lv.room, level_templateFor(&lv, nx, ny), lv.floor.doors[ny][nx]);
        s16 w = lv.room.w * 8, h = lv.room.h * 8;
        player_init(&p, dir == 0 ? -1 : dir == 1 ? w : w / 2, dir == 2 ? 0 : dir == 3 ? h + PLAYER_H : h - 32);
        level_update(&lv, &p);
        CHECK(lv.cellX == bx && lv.cellY == by && lv.bossFight && !lv.voidRunning, "seed %u: boss room entry", seed);
        int redraws = 0;
        for (int f = 0; f < 90; f++)
        {
            player_update(&p, &lv.room, 0);
            p.invulnerable = 255;   // the boss must not knock the player around during the check
            if (level_update(&lv, &p) & LEVEL_REDRAW) redraws++;
        }
        CHECK(redraws == 1 && bodyFree(&lv.room, &p), "seed %u: doors lock (%d) without trapping", seed, redraws);
        s8 slot = enemies_bossSlot(&lv.enemies);
        CHECK(slot >= 0, "seed %u: boss present", seed);
        if (slot < 0) continue;
        lv.enemies.e[slot].deathTimer = 64;   // as if killed
        p.x = 24 << 8;                        // away from the boss, or the gem is picked up at once
        int won = level_update(&lv, &p) & LEVEL_GEM;
        CHECK(!won && lv.gemActive && !lv.bossFight, "seed %u: gem dropped", seed);
        for (int f = 0; f < 120; f++) level_update(&lv, &p);
        p.x = lv.gemX;
        p.y = lv.gemY;
        p.invulnerable = 255;
        CHECK(level_update(&lv, &p) & LEVEL_GEM, "seed %u: gem picked up", seed);
    }
    // Secret rooms: a slash breaks the cracked wall, the passage opens on both sides and leads to
    // the secret room and its item.
    int secretsOpened = 0;
    for (unsigned seed = 3; seed < 600; seed += 9)
    {
        level_start(&lv, &p, (u16) seed, 3);
        for (int y = 0; y < FLOOR_H; y++)
            for (int x = 0; x < FLOOR_W; x++)
            {
                u8 t = lv.floor.type[y][x];
                if (!lv.floor.hidden[y][x] || t == ROOM_SECRET || t == ROOM_SUPER_SECRET) continue;
                level_enter(&lv, &p, x, y);
                enemies_clear(&lv.enemies);   // nobody in the way
                CHECK(lv.crackedCount > 0, "seed %u (%d,%d): no cracked wall", seed, x, y);
                if (!lv.crackedCount) continue;
                Cracked c = lv.cracked[0];
                s16 cx = (c.x0 + c.x1) / 2;
                u16 pad = PAD_ATTACK;
                if (c.dir == DOOR_LEFT) player_place(&p, c.x1 + 12, c.y1 + 1);
                else if (c.dir == DOOR_RIGHT) player_place(&p, c.x0 - 12, c.y1 + 1);
                else if (c.dir == DOOR_UP) { player_place(&p, cx, c.y1 + 35); pad |= PAD_UP; }
                else { player_place(&p, cx, c.y0 - 10); pad |= PAD_DOWN; }
                p.facingLeft = c.dir == DOOR_LEFT;
                int redraw = 0;
                for (int f = 0; f < 6; f++)
                {
                    player_update(&p, &lv.room, f ? pad : (pad & ~PAD_ATTACK));
                    p.facingLeft = c.dir == DOOR_LEFT;
                    if (level_update(&lv, &p) & LEVEL_REDRAW) redraw = 1;
                    if (lv.cellX != x || lv.cellY != y) break;
                }
                CHECK(redraw && (lv.opened[y][x] & c.dir), "seed %u (%d,%d) dir %d: wall not broken", seed, x, y, c.dir);
                if (!(lv.opened[y][x] & c.dir)) continue;
                int sx = x + (c.dir == DOOR_RIGHT) - (c.dir == DOOR_LEFT), sy = y + (c.dir == DOOR_DOWN) - (c.dir == DOOR_UP);
                // Walk through the opening.
                level_enter(&lv, &p, x, y);
                s16 w = lv.room.w * 8, h = lv.room.h * 8;
                player_place(&p, c.dir == DOOR_LEFT ? -1 : c.dir == DOOR_RIGHT ? w : cx,
                             c.dir == DOOR_UP ? 0 : c.dir == DOOR_DOWN ? h + PLAYER_H : c.y1 + 1);
                level_update(&lv, &p);
                CHECK(lv.cellX == sx && lv.cellY == sy, "seed %u: did not reach the secret room", seed);
                u8 st = lv.floor.type[sy][sx];
                CHECK(st == ROOM_SECRET ? lv.itemId == ITEM_HEART : lv.itemId >= ITEM_FIRST_UPGRADE,
                      "seed %u: secret room item %d", seed, lv.itemId);
                secretsOpened++;
            }
    }
    CHECK(secretsOpened > 50, "only %d secret passages tested", secretsOpened);

    printf("%d door crossings, %d secret passages, %d failures\n", moves, secretsOpened, failures);
    return failures != 0;
}

// PC test of the floor generator: checks the invariants on every seed.
//   wsl gcc -std=c99 -Wall -Wextra -DPC_TEST -Iinc tests/floor_test.c src/game_floor.c -o out/floor_test && out/floor_test
#include <stdio.h>
#include "game_floor.h"

static int failures = 0;

static void fail(u16 seed, u8 depth, const char *why)
{
    if (failures++ < 20) printf("seed %u depth %u: %s\n", seed, depth, why);
}

static int touchesType(const Floor *f, int x, int y, u8 type, int *count)
{
    static const int DX[4] = { -1, 1, 0, 0 }, DY[4] = { 0, 0, -1, 1 };
    int found = 0, n = 0;
    for (int d = 0; d < 4; d++)
    {
        int nx = x + DX[d], ny = y + DY[d];
        if (nx < 0 || nx >= FLOOR_W || ny < 0 || ny >= FLOOR_H) continue;
        u8 t = f->type[ny][nx];
        if (t != ROOM_NONE) n++;
        if (t == type) found = 1;
    }
    if (count) *count = n;
    return found;
}

int main(void)
{
    long secretNeighbours[5] = { 0 }, noShop = 0, noSuper = 0, total = 0;

    for (u8 depth = 1; depth <= 6; depth++)
        for (unsigned s = 0; s < 65536; s += 7)
        {
            Floor f;
            u16 seed = (u16) s;
            floor_generate(&f, seed, depth);
            total++;

            int counts[8] = { 0 };
            for (int y = 0; y < FLOOR_H; y++)
                for (int x = 0; x < FLOOR_W; x++) counts[f.type[y][x]]++;

            if (counts[ROOM_START] != 1) fail(seed, depth, "start != 1");
            if (counts[ROOM_BOSS] != 1) fail(seed, depth, "boss != 1");
            if (counts[ROOM_UPGRADE] != 1) fail(seed, depth, "upgrade != 1");
            if (counts[ROOM_SECRET] != 1) fail(seed, depth, "secret != 1");
            if (!counts[ROOM_SHOP]) noShop++;
            if (!counts[ROOM_SUPER_SECRET]) noSuper++;
            if (f.type[f.voidY][f.voidX] != ROOM_BOSS) fail(seed, depth, "void centre is not the boss");

            int regular = counts[ROOM_NORMAL] + counts[ROOM_START] + counts[ROOM_BOSS]
                + counts[ROOM_UPGRADE] + counts[ROOM_SHOP];
            int target = 8 + depth * 2 > 20 ? 20 : 8 + depth * 2;
            if (regular != target || f.roomCount != target) fail(seed, depth, "wrong room count");

            for (int y = 0; y < FLOOR_H; y++)
                for (int x = 0; x < FLOOR_W; x++)
                {
                    u8 t = f.type[y][x];
                    int n;
                    if (t == ROOM_SECRET)
                    {
                        if (touchesType(&f, x, y, ROOM_BOSS, &n)) fail(seed, depth, "secret touches boss");
                        secretNeighbours[n]++;
                        if (f.doors[y][x]) fail(seed, depth, "secret has a visible door");
                    }
                    if (t == ROOM_SUPER_SECRET)
                    {
                        if (touchesType(&f, x, y, ROOM_BOSS, &n)) fail(seed, depth, "super touches boss");
                        if (n != 1) fail(seed, depth, "super has several neighbours");
                    }
                    // The start room stays neutral (Thomas, 2026-11-18): no shop, upgrade or secret next to it.
                    if ((t == ROOM_UPGRADE || t == ROOM_SHOP || t == ROOM_SECRET || t == ROOM_SUPER_SECRET)
                        && touchesType(&f, x, y, ROOM_START, &n))
                        fail(seed, depth, "special room next to the start");
                    if (t == ROOM_BOSS || t == ROOM_UPGRADE || t == ROOM_SHOP)
                    {
                        u8 d = f.doors[y][x];
                        if (!d || (d & (d - 1))) fail(seed, depth, "special room is not a dead end");
                    }
                }

            // Every regular room must be reachable from the start through visible doors.
            u8 seen[FLOOR_H][FLOOR_W] = { { 0 } };
            int qx[FLOOR_W * FLOOR_H], qy[FLOOR_W * FLOOR_H], head = 0, tail = 0, reached = 0;
            seen[f.startY][f.startX] = 1;
            qx[tail] = f.startX; qy[tail++] = f.startY;
            while (head < tail)
            {
                int x = qx[head], y = qy[head++];
                reached++;
                static const int DX[4] = { -1, 1, 0, 0 }, DY[4] = { 0, 0, -1, 1 }, BIT[4] = { 1, 2, 4, 8 };
                for (int d = 0; d < 4; d++)
                    if (f.doors[y][x] & BIT[d])
                    {
                        int nx = x + DX[d], ny = y + DY[d];
                        if (!seen[ny][nx]) { seen[ny][nx] = 1; qx[tail] = nx; qy[tail++] = ny; }
                    }
            }
            if (reached != target) fail(seed, depth, "unreachable room");
        }

    printf("%ld floors, %d failures\n", total, failures);
    printf("no shop: %ld, no super secret: %ld\n", noShop, noSuper);
    printf("secret room neighbours: 1=%ld 2=%ld 3=%ld 4=%ld\n",
           secretNeighbours[1], secretNeighbours[2], secretNeighbours[3], secretNeighbours[4]);
    return failures != 0;
}

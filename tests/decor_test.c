// PC test of the zone-1 decor and room art: every template with all its doors open gets its decor;
// no decor tile may cover a solid tile or a platform, and every picture stands on the ground.
// Writes out/room_<n>.txt (art tile and decor tile of every cell) for tools/art/render_room.py.
//   wsl sh tools/test.sh
#include <stdio.h>
#include "game_level.h"
#include "game_decor.h"
#include "decor_tiles.h"

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static Room room;

int main(void)
{
    int placed = 0;
    decor_setWorld(DECOR_GRAVEYARD);
    for (int n = 0; n < ROOM_TEMPLATE_COUNT; n++)
    {
        level_buildRoom(&room, &ROOM_TEMPLATES[n], 15);
        decor_place(&room, 0x1234 + n * 77);
        placed += decor_count();
        for (int y = 0; y < room.h; y++)
            for (int x = 0; x < room.w; x++)
                CHECK(!decor_tileAt(x, y) || room.tiles[y][x] == TILE_EMPTY
                      || (room.tiles[y][x] == TILE_SOLID && room.tiles[y - 1][x] == TILE_EMPTY),
                      "template %d: decor over a wall or platform at (%d, %d)", n, x, y);
        // Every picture stands on ground at least 3 tiles thick (not on a block hanging in the air).
        for (int y = 0; y < room.h; y++)
            for (int x = 0; x < room.w; x++)
                if (decor_tileAt(x, y) && room.tiles[y][x] == TILE_SOLID)
                    for (int k = 1; k <= 2 && y + k < room.h; k++)
                        CHECK(room.tiles[y + k][x] == TILE_SOLID, "template %d: decor on a thin block at (%d, %d)", n, x, y);
        char path[32];
        sprintf(path, "out/room_%d.txt", n);
        FILE *f = fopen(path, "w");
        fprintf(f, "%d %d\n", room.w, room.h);
        for (int y = 0; y < room.h; y++)
        {
            for (int x = 0; x < room.w; x++)
            {
                unsigned t = decor_planeTile(&room, x, y);
                fprintf(f, "%u:%u ", t & DECOR_TILE ? 0 : t, t & DECOR_TILE ? t & 0xFF : 0);
            }
            fprintf(f, "\n");
        }
        fclose(f);
    }
    // World 2, inside the castle: candelabras and armour on the floor, banners and torches on the wall,
    // always over empty tiles (never in a wall or on a platform).
    decor_setWorld(DECOR_CASTLE);
    int castle = 0;
    for (int n = 0; n < ROOM_TEMPLATE_COUNT; n++)
    {
        level_buildRoom(&room, &ROOM_TEMPLATES[n], 15);
        decor_place(&room, 0x4321 + n * 31);
        castle += decor_count();
        char path2[32];
        sprintf(path2, "out/room2_%d.txt", n);   // for tools/art/render_room.py (world 2)
        FILE *f2 = fopen(path2, "w");
        fprintf(f2, "%d %d\n", room.w, room.h);
        for (int y = 0; y < room.h; y++)
        {
            for (int x = 0; x < room.w; x++)
            {
                unsigned t = decor_planeTile(&room, x, y);
                fprintf(f2, "%u:%u ", t & DECOR_TILE ? 0 : t, t & DECOR_TILE ? t & 0xFF : 0);
            }
            fprintf(f2, "\n");
        }
        fclose(f2);
        for (int y = 0; y < room.h; y++)
            for (int x = 0; x < room.w; x++)
                CHECK(!decor_tileAt(x, y) || room.tiles[y][x] == TILE_EMPTY
                      || (room.tiles[y][x] == TILE_SOLID && room.tiles[y - 1][x] == TILE_EMPTY),
                      "castle template %d: decor over a wall or platform at (%d, %d)", n, x, y);
    }
    CHECK(castle > 20, "only %d castle decor pictures", castle);
    // The forest: big trees and crystals, standing on the ground, over empty tiles only.
    decor_setWorld(DECOR_FOREST);
    int trees = 0;
    for (int n = 0; n < ROOM_TEMPLATE_COUNT; n++)
    {
        level_buildRoom(&room, &ROOM_TEMPLATES[n], 15);
        decor_place(&room, 0x5555 + n * 13);
        trees += decor_count();
        for (int y = 0; y < room.h; y++)
            for (int x = 0; x < room.w; x++)
                CHECK(!(decor_planeTile(&room, x, y) & DECOR_TILE) || room.tiles[y][x] == TILE_EMPTY
                      || (room.tiles[y][x] == TILE_SOLID && room.tiles[y - 1][x] == TILE_EMPTY),
                      "forest template %d: decor shown over a wall or platform at (%d, %d)", n, x, y);
        char pathA[32];
        sprintf(pathA, "out/roomA_%d.txt", n);   // for tools/art/render_room.py (the forest)
        FILE *fa = fopen(pathA, "w");
        fprintf(fa, "%d %d", room.w, room.h);
        fputc(10, fa);
        for (int y = 0; y < room.h; y++)
        {
            for (int x = 0; x < room.w; x++)
            {
                unsigned t = decor_planeTile(&room, x, y);
                fprintf(fa, "%u:%u ", t & DECOR_TILE ? 0 : t, t & DECOR_TILE ? t & 0xFF : 0);
            }
            fputc(10, fa);
        }
        fclose(fa);
    }
    CHECK(trees > 10, "only %d forest decor pictures", trees);
    decor_setWorld(DECOR_GRAVEYARD);
    printf("%d decor pictures in %d templates, %d failures\n", placed, ROOM_TEMPLATE_COUNT, failures);
    return failures != 0;
}

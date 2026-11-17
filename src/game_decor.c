#include "game_decor.h"
#include "decor_tiles.h"

typedef struct
{
    u8 kind;
    u8 x, y;   // top-left tile
} Decor;

static Decor decor[MAX_DECOR];
static u8 count;

// The picture's last row of cells is the ground itself (row ty + 1): its foot goes into the ground
// behind the grass (tools/art/outdoor.py). That row was drawn for a flat ground top on even columns,
// so the ground must be flat one tile past each side, and the rows above it empty. And it must be
// ground, not a block hanging in the air (one block, 2 tiles thick): a fence or a grave on one floats
// (Thomas). GROUND_DEPTH tiles of solid under the foot, or down to the room's edge.
#define GROUND_DEPTH 3
static u8 fits(const Room *r, u8 kind, u8 tx, u8 ty)
{
    u8 w = DECOR_W[kind], h = DECOR_H[kind];
    if ((tx & 1) || tx == 0 || tx + w + 1 > r->w || ty + 1 >= r->h || ty + 1 < h) return 0;
    for (u8 x = tx - 1; x <= tx + w; x++)
    {
        if (r->tiles[ty][x] != TILE_EMPTY) return 0;
        for (u8 y = ty + 1; y <= ty + GROUND_DEPTH && y < r->h; y++)
            if (r->tiles[y][x] != TILE_SOLID) return 0;
    }
    for (u8 x = tx; x < tx + w; x++)
        for (u8 y = ty + 2 - h; y <= ty; y++)
            if (r->tiles[y][x] != TILE_EMPTY) return 0;
    return 1;
}

void decor_place(const Room *r, u16 seed)
{
    u16 rnd = seed | 1;
    u8 trees = 0, fences = 0;
    count = 0;
    for (u8 ty = 1; ty + 1 < r->h; ty++)
        for (u8 tx = 0; tx < r->w && count < MAX_DECOR;)
        {
            rnd = rnd * 25173 + 13849;
            u8 roll = rnd >> 8;
            if (roll >= 90 || r->tiles[ty][tx] != TILE_EMPTY || r->tiles[ty + 1][tx] != TILE_SOLID)
            {
                tx++;
                continue;
            }
            // A few trees and fences, graves in between (they come in small groups: a graveyard).
            u8 kind = roll < 20 ? (roll & 1 ? DECOR_TREE_A : DECOR_TREE_B)
                    : roll < 35 ? DECOR_FENCE
                    : DECOR_GRAVE_ROUND + roll % (DECOR_KINDS - DECOR_GRAVE_ROUND);
            if ((kind <= DECOR_TREE_B && trees >= 2) || (kind == DECOR_FENCE && fences >= 2))
                kind = DECOR_GRAVE_ROUND + roll % (DECOR_KINDS - DECOR_GRAVE_ROUND);
            if (tx < 4 || !fits(r, kind, tx, ty))   // not against the left wall
            {
                tx++;
                continue;
            }
            Decor *d = &decor[count++];
            d->kind = kind;
            d->x = tx;
            d->y = ty + 2 - DECOR_H[kind];
            trees += kind <= DECOR_TREE_B;
            fences += kind == DECOR_FENCE;
            tx += DECOR_W[kind] + 4 + (rnd & 15);   // breathing room before the next one
        }
}

u8 decor_tileAt(s16 tx, s16 ty)
{
    for (u8 i = 0; i < count; i++)
    {
        const Decor *d = &decor[i];
        s16 dx = tx - d->x, dy = ty - d->y;
        if (dx >= 0 && dy >= 0 && dx < DECOR_W[d->kind] && dy < DECOR_H[d->kind])
        {
            u8 c = DECOR_CELLS[DECOR_START[d->kind] + dy * DECOR_W[d->kind] + dx];
            if (c) return c;   // an empty cell lets a picture behind show (overlaps are rare)
        }
    }
    return 0;
}

u8 decor_count(void)
{
    return count;
}

u16 decor_planeTile(const Room *r, s16 tx, s16 ty)
{
    if (tx >= 0 && ty >= 0 && tx < r->w && ty < r->h && r->tiles[ty][tx] != TILE_PLATFORM)
    {
        u8 d = decor_tileAt(tx, ty);   // on an empty tile, or the ground under a picture's foot
        if (d) return DECOR_TILE | d;
    }
    return room_artTile(r, tx, ty);
}

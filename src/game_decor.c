#include "game_decor.h"
#include "forest_decor_tiles.h"   // the forest (tools/art/forest.py)
#include "decor_tiles.h"          // the graveyard outside (tools/art/outdoor.py)
#include "decor2_tiles.h"         // inside the castle (tools/art/interior.py)

// Each world's pictures (generated headers).
typedef struct
{
    const u8 *w, *h;
    const u16 *start;
    const u8 *cells;
} DecorSet;
static const DecorSet SETS[DECOR_WORLDS] =
{
    { DECORF_W, DECORF_H, DECORF_START, DECORF_CELLS },
    { DECOR_W, DECOR_H, DECOR_START, DECOR_CELLS },
    { DECOR2_W, DECOR2_H, DECOR2_START, DECOR2_CELLS },
};
static u8 world;

void decor_setWorld(u8 w)
{
    world = w < DECOR_WORLDS ? w : DECOR_WORLDS - 1;
}

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
    u8 w = SETS[world].w[kind], h = SETS[world].h[kind];
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

// World 2: banners and torches hang on the castle's back wall, `lift` tiles over the floor: a
// rectangle of empty tiles there, over a floor at least as wide.
static u8 fitsWall(const Room *r, u8 kind, u8 tx, u8 ty, u8 lift)
{
    u8 w = DECOR2_W[kind], h = DECOR2_H[kind];
    if (tx + w > r->w || ty < lift + h) return 0;
    for (u8 x = tx; x < tx + w; x++)
    {
        if (r->tiles[ty + 1][x] != TILE_SOLID) return 0;
        for (u8 y = ty + 1 - lift - h; y <= ty; y++)
            if (r->tiles[y][x] != TILE_EMPTY) return 0;
    }
    return 1;
}

// World 2 (Thomas, 2026-11-19): candelabras and suits of armour on the floor, banners and torches on
// the wall above it.
static void placeCastle(const Room *r, u16 rnd)
{
    u8 armours = 0;
    for (u8 ty = 1; ty + 1 < r->h; ty++)
        for (u8 tx = 4; tx < r->w && count < MAX_DECOR;)
        {
            rnd = rnd * 25173 + 13849;
            u8 roll = rnd >> 8;
            if (roll >= 120 || r->tiles[ty][tx] != TILE_EMPTY || r->tiles[ty + 1][tx] != TILE_SOLID)
            {
                tx++;
                continue;
            }
            u8 kind = roll < 30 ? DECOR2_CANDELABRA : roll < 50 ? DECOR2_ARMOR : roll < 85 ? DECOR2_BANNER : DECOR2_TORCH;
            if (kind == DECOR2_ARMOR && armours >= 2) kind = DECOR2_CANDELABRA;
            u8 lift = kind == DECOR2_BANNER ? 4 : 7;   // tiles between the floor and the picture's bottom
            u8 wall = kind == DECOR2_BANNER || kind == DECOR2_TORCH;
            if (wall ? !fitsWall(r, kind, tx, ty, lift) : !fits(r, kind, tx, ty))
            {
                tx++;
                continue;
            }
            Decor *d = &decor[count++];
            d->kind = kind;
            d->x = tx;
            d->y = wall ? ty + 1 - lift - DECOR2_H[kind] : ty + 2 - DECOR2_H[kind];
            armours += kind == DECOR2_ARMOR;
            tx += DECOR2_W[kind] + 3 + (rnd & 15);
        }
}

// A big tree only needs room for its trunk (its middle columns, up to TREE_TRUNK rows) and flat thick
// ground under its whole width: its crown may reach behind walls and platforms, which hide it
// (decor only shows on empty tiles).
#define TREE_TRUNK 8
static u8 fitsTree(const Room *r, u8 tx, u8 ty)
{
    u8 w = DECORF_W[DECORF_BIG_TREE], h = DECORF_H[DECORF_BIG_TREE];
    if ((tx & 1) || tx == 0 || tx + w + 1 > r->w || ty + 1 >= r->h || ty + 1 < h) return 0;
    for (u8 x = tx; x < tx + w; x++)
        for (u8 y = ty + 1; y <= ty + GROUND_DEPTH && y < r->h; y++)
            if (r->tiles[y][x] != TILE_SOLID) return 0;
    for (u8 x = tx + w / 2 - 2; x < tx + w / 2 + 2; x++)
        for (u8 y = ty + 1 - TREE_TRUNK; y <= ty; y++)
            if (r->tiles[y][x] != TILE_EMPTY) return 0;
    for (u8 x = tx; x < tx + w; x++)   // the roots, on the row over the ground
        if (r->tiles[ty][x] != TILE_EMPTY) return 0;
    return 1;
}

// The forest (Thomas, 2026-11-19, the mockup A): a few big twisted trees, well apart, and glowing
// crystals between them.
static void placeForest(const Room *r, u16 rnd)
{
    u8 trees = 0;
    for (u8 ty = 1; ty + 1 < r->h; ty++)
        for (u8 tx = 4; tx < r->w && count < MAX_DECOR;)
        {
            rnd = rnd * 25173 + 13849;
            u8 roll = rnd >> 8;
            if (roll >= 110 || r->tiles[ty][tx] != TILE_EMPTY || r->tiles[ty + 1][tx] != TILE_SOLID)
            {
                tx++;
                continue;
            }
            u8 kind = roll < 50 && trees < 2 ? DECORF_BIG_TREE : DECORF_CRYSTALS;
            if (kind == DECORF_BIG_TREE ? !fitsTree(r, tx, ty) : !fits(r, kind, tx, ty))
            {
                tx++;
                continue;
            }
            decor[count].kind = kind;
            decor[count].x = tx;
            decor[count].y = ty + 2 - DECORF_H[kind];
            count++;
            trees += kind == DECORF_BIG_TREE;
            tx += DECORF_W[kind] + 6 + (rnd & 15);
        }
}

void decor_place(const Room *r, u16 seed)
{
    u16 rnd = seed | 1;
    u8 trees = 0, fences = 0;
    count = 0;
    if (world == DECOR_CASTLE)
    {
        placeCastle(r, rnd);
        return;
    }
    if (world == DECOR_FOREST)
    {
        placeForest(r, rnd);
        return;
    }
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
        const DecorSet *set = &SETS[world];
        if (dx >= 0 && dy >= 0 && dx < set->w[d->kind] && dy < set->h[d->kind])
        {
            u8 c = set->cells[set->start[d->kind] + dy * set->w[d->kind] + dx];
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
    // On empty tiles, and on the ground's top row (a picture's foot); never over a platform nor
    // inside a wall (the forest's big trees reach behind them).
    if (tx >= 0 && ty >= 0 && tx < r->w && ty < r->h
        && (r->tiles[ty][tx] == TILE_EMPTY || (r->tiles[ty][tx] == TILE_SOLID && ty > 0 && r->tiles[ty - 1][tx] == TILE_EMPTY)))
    {
        u8 d = decor_tileAt(tx, ty);   // on an empty tile, or the ground under a picture's foot
        if (d) return DECOR_TILE | d;
    }
    return room_artTile(r, tx, ty);
}

#include "game_decor.h"
#include "decor_tiles.h"    // world 1, outside (tools/art/outdoor.py)
#include "decor2_tiles.h"   // world 2, inside the castle (tools/art/interior.py)

// Each world's pictures (generated headers).
typedef struct
{
    const u8 *w, *h;
    const u16 *start;
    const u8 *cells;
} DecorSet;
static const DecorSet SETS[DECOR_WORLDS] =
{
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

#define GRAVE_KINDS (DECOR_GRAVE_SKULL + 1 - DECOR_GRAVE_ROUND)

static void addDecor(u8 kind, u8 x, u8 y)
{
    if (count >= MAX_DECOR) return;
    decor[count].kind = kind;
    decor[count].x = x;
    decor[count].y = y;
    count++;
}

// World 1 (Thomas, 2026-11-19: the branches float in the air): a dead tree's trunk under the middle
// of each long branch, from just under it down to the ground, when nothing is in the way. Pieces 4
// tiles wide, the trunk in their two middle columns: the fork under the branch, straight pieces, the
// roots on the ground (their last row is the ground itself, like the other pictures).
static void placeTrunks(const Room *r)
{
    u8 top = DECOR_H[DECOR_TRUNK_TOP], mid = DECOR_H[DECOR_TRUNK_MID], root = DECOR_H[DECOR_TRUNK_ROOT];
    for (u8 ty = 1; ty + 2 < r->h; ty++)
        for (u8 tx = 0; tx < r->w;)
        {
            if (r->tiles[ty][tx] != TILE_PLATFORM)
            {
                tx++;
                continue;
            }
            u8 end = tx;
            while (end < r->w && r->tiles[ty][end] == TILE_PLATFORM) end++;
            u8 run = end - tx, from = tx;
            tx = end;
            if (run < 6) continue;
            // Under the middle of the branch, else under a quarter of it (another branch may be in
            // the way); the piece's left column, the trunk in its two middle columns.
            const u8 at[3] = { from + run / 2 - 2, from + run / 4 - 1, from + run * 3 / 4 - 3 };
            for (u8 c = 0; c < 3; c++)
            {
                u8 x = at[c];
                if (x < 1 || x + 5 > r->w) continue;
                // Down from under the branch: the trunk's columns empty, then ground under the roots.
                u8 y = ty + 1;
                while (y < r->h && r->tiles[y][x + 1] == TILE_EMPTY && r->tiles[y][x + 2] == TILE_EMPTY) y++;
                if (y >= r->h || y - (ty + 1) < top + root - 1) continue;   // no ground, or too short
                u8 ok = 1;
                for (u8 k = 0; k < 4 && ok; k++)   // the roots: flat ground, thick enough, air above
                {
                    if (r->tiles[y - 1][x + k] != TILE_EMPTY) ok = 0;
                    for (u8 d = 0; d < GROUND_DEPTH && y + d < r->h; d++)
                        if (r->tiles[y + d][x + k] != TILE_SOLID) ok = 0;
                }
                if (!ok) continue;
                // The roots' last row is the ground row y; the fork right under the branch; straight
                // pieces between (the last one may hide under the fork).
                u8 rootY = y + 1 - root;
                addDecor(DECOR_TRUNK_TOP, x, ty + 1);
                for (s16 my = rootY - mid; my > ty + 1; my -= mid) addDecor(DECOR_TRUNK_MID, x, my);
                addDecor(DECOR_TRUNK_ROOT, x, rootY);
                break;
            }
        }
}

void decor_place(const Room *r, u16 seed)
{
    u16 rnd = seed | 1;
    u8 trees = 0, fences = 0;
    count = 0;
    if (world == 1)
    {
        placeCastle(r, rnd);
        return;
    }
    placeTrunks(r);
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
                    : DECOR_GRAVE_ROUND + roll % GRAVE_KINDS;
            if ((kind <= DECOR_TREE_B && trees >= 2) || (kind == DECOR_FENCE && fences >= 2))
                kind = DECOR_GRAVE_ROUND + roll % GRAVE_KINDS;
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
    if (tx >= 0 && ty >= 0 && tx < r->w && ty < r->h && r->tiles[ty][tx] != TILE_PLATFORM)
    {
        u8 d = decor_tileAt(tx, ty);   // on an empty tile, or the ground under a picture's foot
        if (d) return DECOR_TILE | d;
    }
    return room_artTile(r, tx, ty);
}

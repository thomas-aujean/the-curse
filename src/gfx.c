#include "gfx.h"

// Digits are palette indices, '.' is transparent.
// PAL0 (room): 1-3 bricks, 4-5 platform, 15 text.  PAL1 (ninja): 1 outline, 2-3 suit, 4 skin,
// 5 red scarf, 6 blade.

static const char *const NINJA[32] =
{
    "......1111......",
    ".....133331.....",
    "....13333331....",
    "....13444441.55.",
    "....13444441155.",
    "....13333331..5.",
    ".....133331.....",
    "......1331......",
    "....11333311....",
    "...1333333331...",
    "..133133331331..",
    "..131133331131..",
    "..131133331131..",
    "..131155551131..",
    "..144133331441..",
    "...11.3333.11...",
    ".....133331.....",
    ".....133331.....",
    "....13311331....",
    "....1331.1331...",
    "....1331..1331..",
    "...1331...1331..",
    "...1331....1331.",
    "...1331....1331.",
    "...1331.....131.",
    "..13331.....1331",
    "..1331......1331",
    "..1331......1331",
    ".13331.....13331",
    ".11111.....11111",
    "................",
    "................",
};

static const char *const SLASH[16] =
{
    "..........666666........",
    "............66666666....",
    "...............6666666..",
    ".................666666.",
    "..................66666.",
    "...................6666.",
    "...................6666.",
    "..................66666.",
    ".................666666.",
    "...............6666666..",
    "............66666666....",
    "..........666666........",
    "........................",
    "........................",
    "........................",
    "........................",
};

static const char *const WALKER[24] =
{
    "......1111......",
    ".....122221.....",
    "....12222221....",
    "....12412421....",
    "....12222221....",
    ".....121121.....",
    "......1221......",
    "....11222211....",
    "...1232222321...",
    "..123.1221.321..",
    "..12..1331..21..",
    "..12..1221..21..",
    "..11..1331..11..",
    "......1221......",
    ".....122221.....",
    ".....12..21.....",
    ".....12..21.....",
    ".....12..21.....",
    ".....13..31.....",
    ".....12..21.....",
    ".....12..21.....",
    "....122..221....",
    "....111..111....",
    "................",
};

static const char *const FLYER[32] =   // two 16-row frames: wings up, wings down
{
    "1..............1",
    "11............11",
    "121..........121",
    "1221........1221",
    "12221.1111.12221",
    ".12221222212221.",
    "..122224422221..",
    "...1222222221...",
    "....12222221....",
    ".....122221.....",
    "......1221......",
    ".......11.......",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "......1111......",
    ".....122221.....",
    "....12244221....",
    "..112222222211..",
    ".12222222222221.",
    "1222111221112221",
    "1221...11...1221",
    "121..........121",
    "11............11",
    "1..............1",
    "................",
    "................",
};

static const u32 SOLID_TILE[8] =
{
    0x22222221, 0x23333331, 0x23333331, 0x11111111,
    0x33312333, 0x33312333, 0x33312333, 0x11111111,
};

static const u32 PLATFORM_TILE[8] =
{
    0x55555555, 0x44444444, 0x04000040, 0x04000040,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

static const u32 SHOT_TILE[8] =   // PAL2: red orb with a light core
{
    0x00444400, 0x04422440, 0x44222244, 0x42222224,
    0x42222224, 0x44222244, 0x04422440, 0x00444400,
};

static const u32 GEM_TILE[8] =    // PAL3: cyan gem
{
    0x00011000, 0x00144100, 0x01444410, 0x14424441,
    0x14444441, 0x01444410, 0x00144100, 0x00011000,
};

// Same bricks with a thin crack: "slightly different", the player learns to spot it.
static const u32 CRACKED_TILE[8] =
{
    0x22222221, 0x23313331, 0x23331331, 0x11111111,
    0x33312333, 0x33121333, 0x33312333, 0x11111111,
};

static const u32 HUD_TILES[7][8] =
{
    { 0x05500550, 0x55555555, 0x55555555, 0x55555555, 0x05555550, 0x00555500, 0x00055000, 0x00000000 },   // heart
    { 0x05500110, 0x55551001, 0x55551001, 0x55551001, 0x05551010, 0x00551100, 0x00051000, 0x00000000 },   // half
    { 0x01100110, 0x10011001, 0x10000001, 0x10000001, 0x01000010, 0x00100100, 0x00011000, 0x00000000 },   // empty
    { 0x00000000, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x22222222, 0x00000000, 0x00000000 },   // light
    { 0x00000000, 0x33333333, 0x30000003, 0x30000003, 0x30000003, 0x33333333, 0x00000000, 0x00000000 },   // light empty
    { 0x00000000, 0x0FFFFFF0, 0x0FFFFFF0, 0x0FFFFFF0, 0x0FFFFFF0, 0x0FFFFFF0, 0x0FFFFFF0, 0x00000000 },   // map filled
    { 0x00000000, 0x0FFFFFF0, 0x0F0000F0, 0x0F0000F0, 0x0F0000F0, 0x0F0000F0, 0x0FFFFFF0, 0x00000000 },   // map outline
};

static const char *const RELIC[16] =   // PAL2: a mysterious chalice (good or bad, nobody knows)
{
    "................",
    "...1111111111...",
    "..122222222221..",
    "..124444444421..",
    "..122444444221..",
    "...1222442221...",
    "....12222221....",
    ".....122221.....",
    "......1221......",
    "......1331......",
    "......1221......",
    ".....123321.....",
    "....12222221....",
    "...1333333331...",
    "...1111111111...",
    "................",
};

// Empty space outside the circle: near-black violet with sparse specks (PAL3 colours 10 and 11).
static const u32 VOID_TILE[8] =
{
    0xAAAAAAAA, 0xAAAAAABA, 0xAAAAAAAA, 0xAABAAAAA,
    0xAAAAAAAA, 0xAAAAAAAA, 0xBAAAAAAA, 0xAAAAABAA,
};

// Same drawing, colours moved to PAL3 indices 5.. (room colour i -> 4 + i).
static void remapTile(const u32 *src, u32 *dst)
{
    for (u8 y = 0; y < 8; y++)
    {
        u32 in = src[y], out = 0;
        for (u8 x = 0; x < 8; x++)
        {
            u32 nib = (in >> (28 - x * 4)) & 0xF;
            out |= (nib ? nib + 4 : 0) << (28 - x * 4);
        }
        dst[y] = out;
    }
}

static u8 artPixel(const char *row, u8 x)
{
    for (u8 i = 0; i < x; i++)
        if (!row[i]) return 0;   // short row: transparent
    char c = row[x];
    if (c >= '1' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

typedef u8 PixelFn(const char *const *rows, u8 x, u8 y);

static u8 plainPixel(const char *const *rows, u8 x, u8 y)
{
    return artPixel(rows[y], x);
}

// Horizontal art turned a quarter: its right side (the edge of the arc) ends up on top.
static u8 rotatedPixel(const char *const *rows, u8 x, u8 y)
{
    return artPixel(rows[x], 23 - y);
}

// Art drawn at double size (placeholder boss).
static u8 doubledPixel(const char *const *rows, u8 x, u8 y)
{
    return artPixel(rows[y >> 1], x >> 1);
}

// The other quarter turn: the edge of the arc ends up at the bottom.
static u8 rotatedDownPixel(const char *const *rows, u8 x, u8 y)
{
    return artPixel(rows[15 - x], y);
}

// Sprite tiles are stored column by column (all rows of column 0, then column 1...).
static void artToTiles(const char *const *rows, PixelFn *pixel, u8 wTiles, u8 hTiles, u32 *out)
{
    for (u8 tx = 0; tx < wTiles; tx++)
        for (u8 ty = 0; ty < hTiles; ty++)
            for (u8 y = 0; y < 8; y++)
            {
                u32 line = 0;
                for (u8 x = 0; x < 8; x++) line = (line << 4) | pixel(rows, tx * 8 + x, ty * 8 + y);
                *out++ = line;
            }
}

void gfx_load(void)
{
    static u32 buffer[16 * 8];
    const char *crouch[32];

    PAL_setColor(0, RGB24_TO_VDPCOLOR(0x101028));   // background
    PAL_setColor(1, RGB24_TO_VDPCOLOR(0x442222));
    PAL_setColor(2, RGB24_TO_VDPCOLOR(0x884444));
    PAL_setColor(3, RGB24_TO_VDPCOLOR(0xAA6644));
    PAL_setColor(4, RGB24_TO_VDPCOLOR(0x886622));
    PAL_setColor(5, RGB24_TO_VDPCOLOR(0xCCAA66));
    PAL_setColor(15, RGB24_TO_VDPCOLOR(0xEEEEEE));

    PAL_setColor(16 + 1, RGB24_TO_VDPCOLOR(0x000000));
    PAL_setColor(16 + 2, RGB24_TO_VDPCOLOR(0x222266));
    PAL_setColor(16 + 3, RGB24_TO_VDPCOLOR(0x4444AA));
    PAL_setColor(16 + 4, RGB24_TO_VDPCOLOR(0xEEAA88));
    PAL_setColor(16 + 5, RGB24_TO_VDPCOLOR(0xEE2222));
    PAL_setColor(16 + 6, RGB24_TO_VDPCOLOR(0xEEEEEE));
    PAL_setColor(16 + 15, RGB24_TO_VDPCOLOR(0xEE4444));   // map: boss
    PAL_setColor(32 + 15, RGB24_TO_VDPCOLOR(0xEEEE44));   // map: special rooms, secrets
    PAL_setColor(48 + 15, RGB24_TO_VDPCOLOR(0x664488));   // map: rooms swallowed by the Void

    VDP_loadTileData(SOLID_TILE, GFX_TILE_SOLID, 1, CPU);
    VDP_loadTileData(PLATFORM_TILE, GFX_TILE_PLATFORM, 1, CPU);
    {
        u32 tile[8];
        remapTile(SOLID_TILE, tile);
        VDP_loadTileData(tile, GFX_TILE_SOLID_VOID, 1, CPU);
        remapTile(PLATFORM_TILE, tile);
        VDP_loadTileData(tile, GFX_TILE_PLATFORM_VOID, 1, CPU);
        VDP_loadTileData(VOID_TILE, GFX_TILE_VOID, 1, CPU);
        VDP_loadTileData(CRACKED_TILE, GFX_TILE_CRACKED, 1, CPU);
        remapTile(CRACKED_TILE, tile);
        VDP_loadTileData(tile, GFX_TILE_CRACKED_VOID, 1, CPU);
        VDP_loadTileData(HUD_TILES[0], GFX_TILE_HEART, 7, CPU);
    }
    // Room colours outside the circle (PAL3 5..11): cold and dead.
    static const u32 VOID_ROOM[7] = { 0x220044, 0x442266, 0x664488, 0x444466, 0x8888AA, 0x0A0012, 0x3A2266 };
    for (u8 i = 0; i < 7; i++) PAL_setColor(48 + 5 + i, RGB24_TO_VDPCOLOR(VOID_ROOM[i]));

    artToTiles(NINJA, plainPixel, 2, 4, buffer);
    VDP_loadTileData(buffer, GFX_TILE_STAND, 8, CPU);

    // Crouch: upper body lowered by 10 rows over the bottom of the legs, feet stay on the same row.
    for (u8 y = 0; y < 32; y++) crouch[y] = y < 10 ? "" : y < 24 ? NINJA[y - 10] : NINJA[y];
    artToTiles(crouch, plainPixel, 2, 4, buffer);
    VDP_loadTileData(buffer, GFX_TILE_CROUCH, 8, CPU);

    artToTiles(SLASH, plainPixel, 3, 2, buffer);
    VDP_loadTileData(buffer, GFX_TILE_SLASH, 6, CPU);
    artToTiles(SLASH, rotatedPixel, 2, 3, buffer);
    VDP_loadTileData(buffer, GFX_TILE_SLASH_UP, 6, CPU);
    artToTiles(SLASH, rotatedDownPixel, 2, 3, buffer);
    VDP_loadTileData(buffer, GFX_TILE_SLASH_DOWN, 6, CPU);
    artToTiles(WALKER, plainPixel, 2, 3, buffer);
    VDP_loadTileData(buffer, GFX_TILE_WALKER, 6, CPU);
    artToTiles(FLYER, plainPixel, 2, 2, buffer);
    VDP_loadTileData(buffer, GFX_TILE_FLYER, 4, CPU);
    artToTiles(FLYER + 16, plainPixel, 2, 2, buffer);
    VDP_loadTileData(buffer, GFX_TILE_FLYER + 4, 4, CPU);

    artToTiles(WALKER, doubledPixel, 4, 4, buffer);
    VDP_loadTileData(buffer, GFX_TILE_BOSS_TOP, 16, CPU);
    artToTiles(WALKER + 16, doubledPixel, 4, 2, buffer);
    VDP_loadTileData(buffer, GFX_TILE_BOSS_BOTTOM, 8, CPU);
    VDP_loadTileData(SHOT_TILE, GFX_TILE_SHOT, 1, CPU);
    VDP_loadTileData(GEM_TILE, GFX_TILE_GEM, 1, CPU);
    artToTiles(RELIC, plainPixel, 2, 2, buffer);
    VDP_loadTileData(buffer, GFX_TILE_RELIC, 4, CPU);

    // Enemies: PAL2 normal form (bone and red eyes), PAL3 Void form (violet and cyan eyes).
    PAL_setColor(32 + 1, RGB24_TO_VDPCOLOR(0x000000));
    PAL_setColor(32 + 2, RGB24_TO_VDPCOLOR(0xCCCCAA));
    PAL_setColor(32 + 3, RGB24_TO_VDPCOLOR(0x888866));
    PAL_setColor(32 + 4, RGB24_TO_VDPCOLOR(0xEE2222));
    PAL_setColor(48 + 1, RGB24_TO_VDPCOLOR(0x000000));
    PAL_setColor(48 + 2, RGB24_TO_VDPCOLOR(0x8844CC));
    PAL_setColor(48 + 3, RGB24_TO_VDPCOLOR(0x442288));
    PAL_setColor(48 + 4, RGB24_TO_VDPCOLOR(0x44EEEE));
}

void gfx_setRoomPalette(u8 inVoid)
{
    static const u32 NORMAL[6] = { 0x101028, 0x442222, 0x884444, 0xAA6644, 0x886622, 0xCCAA66 };
    static const u32 VOIDED[6] = { 0x000000, 0x220044, 0x442266, 0x664488, 0x444466, 0x8888AA };
    const u32 *c = inVoid ? VOIDED : NORMAL;
    for (u8 i = 0; i < 6; i++) PAL_setColor(i, RGB24_TO_VDPCOLOR(c[i]));
}
void gfx_setPlayerCorrupted(u8 corrupted)
{
    PAL_setColor(16 + 2, RGB24_TO_VDPCOLOR(corrupted ? 0x440066 : 0x222266));
    PAL_setColor(16 + 3, RGB24_TO_VDPCOLOR(corrupted ? 0x8822CC : 0x4444AA));
    PAL_setColor(16 + 5, RGB24_TO_VDPCOLOR(corrupted ? 0x44EEEE : 0xEE2222));
}
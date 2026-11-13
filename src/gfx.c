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

static u8 artPixel(const char *row, u8 x)
{
    for (u8 i = 0; i < x; i++)
        if (!row[i]) return 0;   // short row: transparent
    char c = row[x];
    if (c >= '1' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

// Sprite tiles are stored column by column (all rows of column 0, then column 1...).
static void artToTiles(const char *const *rows, u8 wTiles, u8 hTiles, u32 *out)
{
    for (u8 tx = 0; tx < wTiles; tx++)
        for (u8 ty = 0; ty < hTiles; ty++)
            for (u8 y = 0; y < 8; y++)
            {
                u32 line = 0;
                for (u8 x = 0; x < 8; x++) line = (line << 4) | artPixel(rows[ty * 8 + y], tx * 8 + x);
                *out++ = line;
            }
}

void gfx_load(void)
{
    static u32 buffer[8 * 8];
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

    artToTiles(NINJA, 2, 4, buffer);
    VDP_loadTileData(buffer, GFX_TILE_STAND, 8, CPU);

    // Crouch: upper body lowered by 10 rows over the bottom of the legs, feet stay on the same row.
    for (u8 y = 0; y < 32; y++) crouch[y] = y < 10 ? "" : y < 24 ? NINJA[y - 10] : NINJA[y];
    artToTiles(crouch, 2, 4, buffer);
    VDP_loadTileData(buffer, GFX_TILE_CROUCH, 8, CPU);

    artToTiles(SLASH, 3, 2, buffer);
    VDP_loadTileData(buffer, GFX_TILE_SLASH, 6, CPU);
}

#ifndef GFX_H
#define GFX_H

#include <genesis.h>

// Placeholder graphics, built from ASCII art at boot until real art goes through rescomp.
#define GFX_TILE_SOLID    (TILE_USER_INDEX + 0)
#define GFX_TILE_PLATFORM (TILE_USER_INDEX + 1)
// PixelLab item icons (res/items.png, tools/art/items.py): 2 x 2 tiles (PAL2) each, in the order of
// the upgrades (ITEM_STONE_HEART ...). In the place of the old placeholder ninja, up to 34.
#define GFX_ITEMS         (TILE_USER_INDEX + 2)    // 8 x 4 tiles

#define GFX_TILE_SOLID_VOID    (TILE_USER_INDEX + 50)   // room tiles outside the circle (PAL3)
#define GFX_TILE_PLATFORM_VOID (TILE_USER_INDEX + 51)
#define GFX_TILE_VOID          (TILE_USER_INDEX + 52)   // empty space swallowed by the Void

#define GFX_TILE_BOSS_TOP      (TILE_USER_INDEX + 60)   // 4 x 4 tiles (walker art doubled)
#define GFX_TILE_BOSS_BOTTOM   (TILE_USER_INDEX + 76)   // 4 x 2 tiles
#define GFX_TILE_SHOT          (TILE_USER_INDEX + 84)
#define GFX_TILE_GEM           (TILE_USER_INDEX + 85)
#define GFX_TILE_CRACKED       (TILE_USER_INDEX + 86)   // wall hiding a secret room
#define GFX_TILE_CRACKED_VOID  (TILE_USER_INDEX + 87)
#define GFX_TILE_HEART         (TILE_USER_INDEX + 88)   // HUD (PAL1)
#define GFX_TILE_HEART_HALF    (TILE_USER_INDEX + 89)
#define GFX_TILE_HEART_EMPTY   (TILE_USER_INDEX + 90)
#define GFX_TILE_LIGHT         (TILE_USER_INDEX + 91)   // HUD (PAL2, PAL3 when corrupted)
#define GFX_TILE_LIGHT_EMPTY   (TILE_USER_INDEX + 92)
#define GFX_TILE_MAP_FILLED    (TILE_USER_INDEX + 93)   // minimap, colour 15 of the palette
#define GFX_TILE_MAP_OUTLINE   (TILE_USER_INDEX + 94)
#define GFX_TILE_RELIC         (TILE_USER_INDEX + 95)   // 2 x 2 tiles (PAL2)
#define GFX_TILE_JELLY_SMALL   (TILE_USER_INDEX + 123)  // 1 x 1, bottom on row 6
#define GFX_TILE_ARROW         (TILE_USER_INDEX + 124)
#define GFX_TILE_SHURIKEN      (TILE_USER_INDEX + 125)
#define GFX_TILE_COIN          (TILE_USER_INDEX + 126)  // PAL2 gold
#define GFX_TILE_JAR           (TILE_USER_INDEX + 127)  // 2 x 2 (PAL2), bottom on row 15
#define GFX_TILE_HIT           (TILE_USER_INDEX + 135)  // 2 x 2 (PAL2), spark of a blow that lands
#define GFX_TILE_TRAP_ARROW    (TILE_USER_INDEX + 131)  // 2 x 2 (PAL2), gold arrow shown over a trapdoor
// HUD sprites (main.c, drawHud): their tiles are composed in RAM and sent when what they show changes.
#define GFX_HUD_MAP            (TILE_USER_INDEX + 34)   // 15 tiles: 5 x 3 rooms, column by column
#define GFX_HUD_COINS          (TILE_USER_INDEX + 53)   // 4 tiles: coin and 3 digits
#define GFX_HUD_KEYS           (TILE_USER_INDEX + 57)   // 2 tiles: key and 1 digit
#define GFX_HUD_HEARTS         (TILE_USER_INDEX + 99)   // 12 tiles
#define GFX_HUD_LIGHT          (TILE_USER_INDEX + 111)  // 10 tiles, up to 121


#define GFX_FEET_ROW 29   // last pixel row of the feet in the 32-row ninja art

// Room tiles (res/castle.png, tools/art/castle.py and outdoor.py): every map tile is a quarter of a
// 16 x 16 block, drawn from the solid neighbours around the nearest block corner. Then cracked walls
// and the branches of zone 1 (outside: earth and grass, branches as one-way platforms).
#define GFX_CASTLE         (TILE_USER_INDEX + 140)   // 88 tiles (CASTLE_TILES), up to 228
// The room is on plane A, in front of the far background (plane B): the Void darkens the room with
// the shadow mode (low priority on both planes), which needs the room in front. The HUD is sprites,
// so the top of the room, its doors, shows under the hearts.
#define ROOM_PLANE         BG_A
#define FAR_PLANE          BG_B
// Far background (plane B, tools/art/far.py): its tiles fill free stretches of video memory listed in
// far.py (RANGES) and far_map.h: 228..479, 576..599, 656..699, 716..759 and 1960..2015.
// Tile indices: CASTLE_* in game_room.h.

// Zone-1 decor (res/decor.png, tools/art/outdoor.py, src/game_decor.c), in two stretches of video
// memory. The second one is the unused part of the window plane's map (0xD000, 64 x 32 cells): the HUD
// only uses its 3 top rows. A full-screen message clears the whole window (the decor blinks out
// behind it), and every room entry loads these tiles again (gfx_loadDecor).
// Plane sizes 64 x 32 (SGDK): plane B 0xC000, window 0xD000, plane A 0xE000, scroll table 0xF000,
// sprites 0xF400, free from 0xF700 (the far background's last stretch, tiles 1976..2047).
#define GFX_DECOR_A        (TILE_USER_INDEX + 1304)  // 120 tiles, after the enemies, up to the font
#define GFX_DECOR_B        (0xD180 / 32)             // 116 tiles, window rows 3..31
#define DECOR_A_LEN        120
#define DECOR_B_LEN        116

// Hero frames (res/ninja.png, tools/art/ninja.py): 32 x 48 (4 x 6 tiles), body centred on x 16, soles on
// row GFX_HERO_FEET. Too many frames for VRAM: only the current one is streamed (DMA) to GFX_NINJA, its
// 32 x 32 extension cells (slash arcs, res/ninja_fx.png) to GFX_NINJA_EXT.
#define GFX_NINJA          (TILE_USER_INDEX + 600)   // 24 + 2 x 16 tiles, up to 656
#define GFX_NINJA_EXT      (GFX_NINJA + 24)
#define GFX_NINJA_EXT3     (TILE_USER_INDEX + 2016)  // third cell (big slash crescents), last 16 tiles of VRAM
#define GFX_HERO_FEET 46
#include "ninja_frames.h"

// PixelLab enemies (res/enemies.png, tools/art/enemies.py), facing right: every pose of every enemy stays
// in video memory (ENEMY_TILE_COUNT tiles, after the props and up to the font), cut into 1 to 4 sprites
// per pose (enemy_frames.h). The old place of the enemies (TILE_USER_INDEX + 480..607) holds the chests.
#define GFX_ENEMIES        (TILE_USER_INDEX + 760)
#include "enemy_frames.h"
// The guard's crescents (res/slash.png, tools/art/slash.py, PAL2): side 4 x 4 + 4 x 2, then up
// 4 x 4 + 2 x 4, 48 tiles after the enemies, up to the arrow.
#define GFX_ENEMY_SLASH    (TILE_USER_INDEX + 1211)
#if 760 + ENEMY_TILE_COUNT > 1174
#error "enemy tiles run into the crate (inc/gfx.h)"
#endif
// The archer's arrow (res/arrow.png, tools/art/arrow.py): 5 angles of 3 x 3 tiles (PAL2), after the
// enemies (room freed by the ninja, out for now), up to the decor.
#define GFX_ARROWS         (TILE_USER_INDEX + 1259)

// Breakable props (res/props.png, tools/art/props.py): 2 x 2 tiles each, order of PROP_* (game_level.h).
#define GFX_PROPS          (TILE_USER_INDEX + 700)
// PixelLab chests (res/chests.png, tools/art/chests.py): 4 x 4 tiles each, bottom-centred, in the order
// bronze, bronze open, silver, silver open, gold, gold open. In the old place of the enemies.
#define GFX_CHESTS         (TILE_USER_INDEX + 480)
#define GFX_CHEST_TILES    16
// In the caravan's room and in the shop (no chest there), the same tiles hold the caravan (64 x 64:
// left half 4 x 4 + 4 x 4, then right half) and the merchant (32 x 48: 4 x 4 + 4 x 2), tools/art/shop.py.
#define GFX_CARAVAN        GFX_CHESTS
#define GFX_MERCHANT       (GFX_CHESTS + 64)
// The big wooden crate (res/crate.png, tools/art/crate.py): 4 x 4 tiles (PAL2), after the enemies.
#define GFX_CRATE          (TILE_USER_INDEX + 1174)
// The key and the padlock (res/keys.png, tools/art/keys.py): 2 x 2 tiles each (PAL2).
#define GFX_KEY            (TILE_USER_INDEX + 1190)
#define GFX_PADLOCK        (TILE_USER_INDEX + 1194)

void gfx_load(void);
void gfx_loadBoss(void);   // the boss's tiles (again after the exit door took their place)
void gfx_loadDoor(void);   // the exit door, on the boss's tiles (the boss is gone)
void gfx_loadShop(u8 shop);   // 1: the caravan and the merchant on the chests' tiles, 0: the chests again
// HUD tile pictures (4-bit pixels, 8 rows): 0 heart, 1 half, 2 empty, 3 light, 4 light empty,
// 5 map room here, 6 map room known (both in colour 15); the coin; the digits (colour 15).
const u32 *gfx_hudPicture(u8 i);
const u32 *gfx_coinPicture(void);
const u32 *gfx_keyPicture(void);
const u32 *gfx_digitPicture(u8 d);
u16 gfx_decorTile(u8 i);   // video memory index of decor tile i
void gfx_loadDecor(void);   // decor tiles into video memory (again after a full-screen message)
void gfx_drawFar(void);   // the parallax sky on FAR_PLANE
void gfx_setRoomPalette(u8 inVoid);
void gfx_setPlayerCorrupted(u8 corrupted);   // violet ninja while touched by the Void   // cold, dead colours once the Void has swallowed the room

#endif

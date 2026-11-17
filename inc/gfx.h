#ifndef GFX_H
#define GFX_H

#include <genesis.h>

// Placeholder graphics, built from ASCII art at boot until real art goes through rescomp.
#define GFX_TILE_SOLID    (TILE_USER_INDEX + 0)
#define GFX_TILE_PLATFORM (TILE_USER_INDEX + 1)
#define GFX_TILE_STAND    (TILE_USER_INDEX + 2)    // 2 x 4 tiles
#define GFX_TILE_CROUCH   (TILE_USER_INDEX + 10)   // 2 x 4 tiles
#define GFX_TILE_SLASH    (TILE_USER_INDEX + 18)   // 3 x 2 tiles
#define GFX_TILE_SLASH_UP (TILE_USER_INDEX + 24)   // 2 x 3 tiles
#define GFX_TILE_SLASH_DOWN (TILE_USER_INDEX + 30) // 2 x 3 tiles

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


#define GFX_FEET_ROW 29   // last pixel row of the feet in the 32-row ninja art

// Zone-1 castle tiles (res/castle.png, tools/art/castle.py): every map tile is a quarter of a 16 x 16
// block, drawn from the solid neighbours around the nearest block corner. Then cracked walls and planks.
#define GFX_CASTLE         (TILE_USER_INDEX + 140)   // 73 tiles
#define GFX_FAR            (TILE_USER_INDEX + 230)   // dusk sky on plane B (parallax, low priority)
#define CASTLE_STONE       1     // + quarter * 16 + bits (1 horizontal neighbour solid, 2 vertical, 4 diagonal, 8 self)
#define CASTLE_CRACKED     65    // + quarter
#define CASTLE_PLANK       69    // alone; 70 left end, 71 middle, 72 right end

// Hero frames (res/ninja.png, tools/art/ninja.py): 32 x 48 (4 x 6 tiles), body centred on x 16, soles on
// row GFX_HERO_FEET. Too many frames for VRAM: only the current one is streamed (DMA) to GFX_NINJA, its
// 32 x 32 extension cells (slash arcs, res/ninja_fx.png) to GFX_NINJA_EXT.
#define GFX_NINJA          (TILE_USER_INDEX + 400)
#define GFX_NINJA_EXT      (GFX_NINJA + 24)
#define GFX_HERO_FEET 46
#include "ninja_frames.h"

// PixelLab enemies (res/enemies.png, tools/art/enemies.py), facing right: every pose of every enemy stays
// in video memory (ENEMY_TILE_COUNT tiles, after the props and up to the font), cut into 1 to 4 sprites
// per pose (enemy_frames.h). The old place of the enemies (TILE_USER_INDEX + 480..607) holds the chests.
#define GFX_ENEMIES        (TILE_USER_INDEX + 760)
#include "enemy_frames.h"

// Breakable props (res/props.png, tools/art/props.py): 2 x 2 tiles each, order of PROP_* (game_level.h).
#define GFX_PROPS          (TILE_USER_INDEX + 700)
// PixelLab chests (res/chests.png, tools/art/chests.py): 4 x 4 tiles each, bottom-centred, in the order
// bronze, bronze open, silver, silver open, gold, gold open. In the old place of the enemies.
#define GFX_CHESTS         (TILE_USER_INDEX + 480)
#define GFX_CHEST_TILES    16

void gfx_load(void);
void gfx_drawFar(void);   // the parallax sky on plane B
void gfx_setRoomPalette(u8 inVoid);
void gfx_setPlayerCorrupted(u8 corrupted);   // violet ninja while touched by the Void   // cold, dead colours once the Void has swallowed the room

#endif

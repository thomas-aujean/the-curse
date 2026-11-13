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
#define GFX_TILE_WALKER   (TILE_USER_INDEX + 36)   // 2 x 3 tiles
#define GFX_TILE_FLYER    (TILE_USER_INDEX + 42)   // 2 frames of 2 x 2 tiles

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
#define GFX_TILE_ARCHER        (TILE_USER_INDEX + 99)   // 2 x 3, feet on row GFX_WALKER_FEET
#define GFX_TILE_GUARD         (TILE_USER_INDEX + 105)  // 2 x 3, idem
#define GFX_TILE_ENINJA        (TILE_USER_INDEX + 111)  // 2 x 4, ninja art in enemy colours
#define GFX_TILE_JELLY         (TILE_USER_INDEX + 119)  // 2 x 2, bottom on row 15
#define GFX_TILE_JELLY_SMALL   (TILE_USER_INDEX + 123)  // 1 x 1, bottom on row 6
#define GFX_TILE_ARROW         (TILE_USER_INDEX + 124)
#define GFX_TILE_SHURIKEN      (TILE_USER_INDEX + 125)
#define GFX_TILE_COIN          (TILE_USER_INDEX + 126)  // PAL2 gold
#define GFX_TILE_JAR           (TILE_USER_INDEX + 127)  // 2 x 2 (PAL2), bottom on row 15

#define GFX_WALKER_FEET 22   // last pixel row of the feet in the 24-row walker art
#define GFX_FLYER_BOTTOM 13

#define GFX_FEET_ROW 29   // last pixel row of the feet in the 32-row ninja art

// Zone-1 castle tiles (res/castle.png): stone autotiles [parity * 16 + open-edges mask], cracked
// walls, plank pieces. The Void copy uses the same layout in PAL3.
#define GFX_CASTLE         (TILE_USER_INDEX + 140)
#define GFX_CASTLE_VOID    (TILE_USER_INDEX + 180)
#define GFX_FAR            (TILE_USER_INDEX + 230)   // dusk sky on plane B (parallax, low priority)
#define CASTLE_STONE       1     // + parity * 16 + mask (1 top, 2 bottom, 4 left, 8 right open)
#define CASTLE_CRACKED     33    // + parity
#define CASTLE_PLANK       35    // alone; 36 left end, 37 middle, 38 right end

void gfx_load(void);
void gfx_drawFar(void);   // the parallax sky on plane B
void gfx_setRoomPalette(u8 inVoid);
void gfx_setPlayerCorrupted(u8 corrupted);   // violet ninja while touched by the Void   // cold, dead colours once the Void has swallowed the room

#endif

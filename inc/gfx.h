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

#define GFX_WALKER_FEET 22   // last pixel row of the feet in the 24-row walker art
#define GFX_FLYER_BOTTOM 13

#define GFX_FEET_ROW 29   // last pixel row of the feet in the 32-row ninja art

void gfx_load(void);
void gfx_setRoomPalette(u8 inVoid);   // cold, dead colours once the Void has swallowed the room

#endif

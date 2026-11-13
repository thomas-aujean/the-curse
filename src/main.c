#include <genesis.h>
#include "game_floor.h"

// Floor generator demo: START = new floor, B = show secrets, A = run/stop the Void, C = depth.

#define MAP_X 2
#define MAP_Y 4

static Floor floor;
static u8 depth = 1;
static bool showSecrets = FALSE;
static bool voidRunning = FALSE;
static u16 radius;          // Void radius, same units as floor_voidDist2()
static u16 frameCount;

static const char LETTER[] = { ' ', '.', 'S', 'B', 'U', '$', '?', '!' };

static u16 startRadius(void)
{
    u16 maxD2 = 0;
    for (u8 y = 0; y < FLOOR_H; y++)
        for (u8 x = 0; x < FLOOR_W; x++)
            if (floor.type[y][x] != ROOM_NONE)
            {
                u16 d2 = floor_voidDist2(&floor, x, y);
                if (d2 > maxD2) maxD2 = d2;
            }
    u16 r = 0;
    while (r * r <= maxD2) r++;
    return r;
}

static void drawCell(u8 x, u8 y)
{
    u8 t = floor.type[y][x];
    u8 secret = t == ROOM_SECRET || t == ROOM_SUPER_SECRET;
    if (t == ROOM_NONE || (secret && !showSecrets)) return;

    u16 px = MAP_X + x * 4, py = MAP_Y + y * 3;
    u16 pal = PAL0;
    if (t == ROOM_BOSS) pal = PAL1;
    else if (t >= ROOM_UPGRADE) pal = PAL2;
    if (!secret && floor_voidDist2(&floor, x, y) > radius * radius) pal = PAL3;

    char cell[4] = { '[', LETTER[t], ']', 0 };
    VDP_setTextPalette(pal);
    VDP_drawText(cell, px, py);

    // Doors are drawn by the room on their left/top side only.
    u8 doors = floor.doors[y][x], hidden = showSecrets ? floor.hidden[y][x] : 0;
    VDP_setTextPalette(PAL0);
    if (doors & DOOR_RIGHT) VDP_drawText("-", px + 3, py);
    if (doors & DOOR_DOWN) VDP_drawText("|", px + 1, py + 1);
    VDP_setTextPalette(PAL2);
    if (hidden & DOOR_RIGHT) VDP_drawText("=", px + 3, py);
    if (hidden & DOOR_DOWN) VDP_drawText(":", px + 1, py + 1);
}

static void drawMap(void)
{
    for (u8 y = 0; y < FLOOR_H; y++)
        for (u8 x = 0; x < FLOOR_W; x++) drawCell(x, y);
}

static void drawStatus(void)
{
    char line[41];
    VDP_setTextPalette(PAL0);
    sprintf(line, "GRAINE %04X  ETAGE %u  SALLES %u   ", floor.seed, depth, floor.roomCount);
    VDP_drawText(line, 2, 2);
    sprintf(line, "NEANT R=%3u %s   ", radius, voidRunning ? "AVANCE" : "ARRET ");
    VDP_drawText(line, 2, 27);
}

static void redraw(void)
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_setTextPalette(PAL0);
    VDP_drawText("THE CURSE - GENERATEUR", 2, 0);
    VDP_drawText("START nouv. B secrets A neant C etage", 2, 1);
    drawMap();
    drawStatus();
}

static void newFloor(u16 seed)
{
    floor_generate(&floor, seed, depth);
    radius = startRadius();
    voidRunning = FALSE;
    redraw();
}

static void onJoy(u16 joy, u16 changed, u16 state)
{
    if (joy != JOY_1) return;
    u16 pressed = changed & state;
    if (pressed & BUTTON_START) newFloor(random() ^ vtimer);
    if (pressed & BUTTON_C)
    {
        depth = depth % 6 + 1;
        newFloor(floor.seed);
    }
    if (pressed & BUTTON_B)
    {
        showSecrets = !showSecrets;
        redraw();
    }
    if (pressed & BUTTON_A)
    {
        if (radius == 0) radius = startRadius();
        voidRunning = !voidRunning;
        redraw();
    }
}

int main(bool hardReset)
{
    (void) hardReset;
    PAL_setColor(15, RGB24_TO_VDPCOLOR(0xEEEEEE));
    PAL_setColor(16 + 15, RGB24_TO_VDPCOLOR(0xEE4444));
    PAL_setColor(32 + 15, RGB24_TO_VDPCOLOR(0xEEEE44));
    PAL_setColor(48 + 15, RGB24_TO_VDPCOLOR(0x664488));

    JOY_setEventHandler(onJoy);
    newFloor(0x1234);

    // The Void shrinks by one unit per second, whatever the region.
    const u16 framesPerStep = IS_PAL_SYSTEM ? 50 : 60;
    while (TRUE)
    {
        if (voidRunning && ++frameCount >= framesPerStep)
        {
            frameCount = 0;
            if (radius) radius--;
            if (!radius) voidRunning = FALSE;
            drawMap();
            drawStatus();
        }
        SYS_doVBlankProcess();
    }
    return 0;
}

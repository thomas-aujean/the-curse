#ifndef GAME_CODEX_H
#define GAME_CODEX_H

#include "game_room.h"   // SGDK types (or their PC stand-ins)

// The bestiary (Thomas, 2026-11-18: from the title, every enemy met and every item seen, empty boxes
// for the others, with what each item does). Kept from run to run on the cartridge (main.c writes it
// to the SRAM); the logic only marks what the player meets.

enum { PAD_MODE_AUTO, PAD_MODE_3, PAD_MODE_6, PAD_MODES };   // options: the pad, detected or forced

typedef struct
{
    u16 enemies;   // bit per ENEMY_* met
    u16 items;     // bit per ITEM_* seen (on an altar, in the shop, out of a chest)
    u8 padMode;    // PAD_MODE_*
} Codex;

extern Codex codex;

void codex_clear(void);
void codex_seeEnemy(u8 type);
void codex_seeItem(u8 item);
u8 codex_enemySeen(u8 type);
u8 codex_itemSeen(u8 item);

// The bestiary's lists: the enemies shown (in order) and their names, the items' descriptions.
#define CODEX_ENEMIES 7
extern const u8 CODEX_ENEMY[CODEX_ENEMIES];
const char *codex_enemyName(u8 type);
const char *codex_enemyLine(u8 type);    // what it does, 36 characters at most
const char *codex_itemLine(u8 item);     // what it does, 36 characters at most

// Saved form: CODEX_BYTES bytes (a mark, the version, then the fields), read back by codex_unpack
// (which returns 0 and clears the codex when the bytes are not a save).
#define CODEX_BYTES 8
void codex_pack(u8 *out);
u8 codex_unpack(const u8 *in);

#endif

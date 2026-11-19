#include "game_codex.h"
#include "game_enemy.h"
#include "game_player.h"

Codex codex;

void codex_clear(void)
{
    codex.enemies = codex.items = 0;
    codex.padMode = PAD_MODE_AUTO;
}

void codex_seeEnemy(u8 type)
{
    if (type == ENEMY_JELLY_SMALL) type = ENEMY_JELLY;
    if (type < 16) codex.enemies |= 1 << type;
}

void codex_seeItem(u8 item)
{
    if (item != ITEM_NONE && item < 16) codex.items |= 1 << item;
}

u8 codex_enemySeen(u8 type)
{
    return type < 16 && (codex.enemies & (1 << type)) != 0;
}

u8 codex_itemSeen(u8 item)
{
    return item < 16 && (codex.items & (1 << item)) != 0;
}

const u8 CODEX_ENEMY[CODEX_ENEMIES] =
{
    ENEMY_WALKER, ENEMY_ARCHER, ENEMY_GUARD, ENEMY_FLYER, ENEMY_JELLY, ENEMY_BAT, ENEMY_BOSS,
};

const char *codex_enemyName(u8 type)
{
    switch (type)
    {
        case ENEMY_WALKER: return "SOLDAT";
        case ENEMY_ARCHER: return "ARCHER";
        case ENEMY_GUARD: return "GARDE";
        case ENEMY_FLYER: return "SPECTRE";
        case ENEMY_JELLY: return "GELEE";
        case ENEMY_BAT: return "CHAUVE-SOURIS";
        case ENEMY_BOSS: return "BOURREAU";
        default: return "";
    }
}

const char *codex_enemyLine(u8 type)
{
    switch (type)
    {
        case ENEMY_WALKER: return "PATROUILLE, PUIS FRAPPE DE L'EPEE";
        case ENEMY_ARCHER: return "TIRE DES FLECHES, L'EPEE LES BRISE";
        case ENEMY_GUARD: return "BOUCLIER DEVANT, CHARGE A LA LANCE";
        case ENEMY_FLYER: return "FOND SUR TOI ET CRACHE DU FEU";
        case ENEMY_JELLY: return "SE COUPE EN DEUX SOUS L'EPEE";
        case ENEMY_BAT: return "PENDUE AU PLAFOND, PIQUE SUR TOI";
        case ENEMY_BOSS: return "GARDE LA GEMME : CHARGE ET ECRASE";
        default: return "";
    }
}

// What an item does, nearly to the number (Thomas, 2026-11-18: "explain precisely, not in every
// detail but nearly").
const char *codex_itemLine(u8 item)
{
    switch (item)
    {
        case ITEM_HEART: return "REND UN COEUR DE VIE";
        case ITEM_STONE_HEART: return "UN COEUR GRIS, QUI PREND LES COUPS";
        case ITEM_WIND_BOOTS: return "COURSE NETTEMENT PLUS RAPIDE";
        case ITEM_RAVEN_WINGS: return "UN SECOND SAUT EN PLEIN AIR";
        case ITEM_LONG_BLADE: return "COUP D'EPEE PLUS LONG";
        case ITEM_LANTERN: return "LA LUMIERE DURE 50 % DE PLUS";
        case ITEM_GLASS_HEART: return "-1 COEUR MAXIMUM, EPEE +1 DEGAT";
        case ITEM_LEAD_BOOTS: return "COURSE NETTEMENT PLUS LENTE";
        case ITEM_VOID_BLOOD: return "LUMIERE USEE DEUX FOIS PLUS VITE";
        default: return "";
    }
}

#define CODEX_MARK 0xC5
#define CODEX_VERSION 1

void codex_pack(u8 *out)
{
    out[0] = CODEX_MARK;
    out[1] = CODEX_VERSION;
    out[2] = codex.enemies >> 8;
    out[3] = codex.enemies & 0xFF;
    out[4] = codex.items >> 8;
    out[5] = codex.items & 0xFF;
    out[6] = codex.padMode;
    out[7] = out[0] ^ out[1] ^ out[2] ^ out[3] ^ out[4] ^ out[5] ^ out[6];   // check byte
}

u8 codex_unpack(const u8 *in)
{
    codex_clear();
    if (in[0] != CODEX_MARK || in[1] != CODEX_VERSION || in[6] >= PAD_MODES
        || in[7] != (in[0] ^ in[1] ^ in[2] ^ in[3] ^ in[4] ^ in[5] ^ in[6]))
        return 0;
    codex.enemies = (in[2] << 8) | in[3];
    codex.items = (in[4] << 8) | in[5];
    codex.padMode = in[6];
    return 1;
}

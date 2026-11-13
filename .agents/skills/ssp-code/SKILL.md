---
name: ssp-code
description: "Écrire ou corriger le code C (SGDK 2.11) d'un jeu Mega Drive destiné à une vraie cartouche."
---

# Code Mega Drive — SGDK 2.11

## Cible
- Vraie Mega Drive (PAL 50 Hz **et** NTSC 60 Hz), cartouche physique. L'émulateur (BlastEm) ne suffit pas à valider.
- CPU 68000 à 7,6 MHz, 64 Ko de RAM, 64 Ko de VRAM, 4 palettes × 16 couleurs, 80 sprites (20 par ligne en H40).

## Règles
- C99 SGDK : types `s16/u16/fix16/fix32`, **jamais de `float`/`double`** (pas de FPU) ni de division dans une boucle de frame.
- Pas d'allocation dynamique en jeu (`MEM_alloc` seulement au chargement d'un niveau).
- Logique de jeu séparée de l'affichage : les modules `game_*.c` ne touchent pas au VDP → testables sur PC.
- Tout ce qui dépend de la fréquence (vitesses, timers) passe par une constante par frame dérivée de `IS_PAL_SYSTEM`.
- Ressources uniquement via `res/*.res` (rescomp) ; palettes Mega Drive (9 bits : composantes 0, 2, 4…E).
- Budget par frame : ne jamais attendre le VDP en boucle active ; transferts DMA regroupés en VBlank.
- Commentaires : le pourquoi (contrainte matérielle, astuce), pas le quoi.

## Definition of Done
- `build.bat` compile sans warning.
- Testé dans BlastEm en PAL et en NTSC (menu System ▸ Region).
- Pas de régression de performance visible (pas de ralentissement ni de clignotement nouveau).
- Ne jamais committer sans demande explicite de Thomas.

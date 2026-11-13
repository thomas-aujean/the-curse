# The Curse — guide de style

Objectif (Thomas) : un très beau jeu, la Mega Drive poussée au maximum. Inspirations : Shinobi III, Castlevania
Bloodlines, Rogue Legacy, Metroid pour le Néant. On s'inspire, on ne copie jamais un sprite ou un décor existant.

## Zone 1 — entrée du château, au crépuscule (choix A, 2026-11-13)
- Ciel en dégradé violet profond → magenta → orange, soleil bas et chaud à l'horizon.
- Pierre chaude (brun-rose), arêtes supérieures éclairées par le couchant, dessous dans l'ombre.
- Poutres de bois sombres pour les plateformes, mousse et lierre sur la pierre.
- Le Néant contraste : violet-noir, pierre froide et morte, fond noir moucheté.

## Plans et couches
| Couche | Contenu | Défilement |
|---|---|---|
| Plan B | ciel + soleil + remparts lointains | parallaxe 1/4 horizontal, 1/8 vertical |
| Plan A | la salle (tuiles), raccords automatiques | suit la caméra |
| Fenêtre | HUD (3 lignes en haut) | fixe |
| Sprites | ninja, ennemis, objets | — |

## Palettes (4 × 15 couleurs + transparent)
- **PAL0** décor : 1 contour, 2-6 pierre (ombre → reflet du couchant), 7-8 bois, 9 mousse, 10-14 ciel, 15 texte.
- **PAL1** ninja + cœurs du HUD.
- **PAL2** ennemis forme normale, pièces, objets.
- **PAL3** Néant : 1-4 ennemis forme Néant, 5-13 décor avalé (couleurs 1-9 de PAL0 refroidies), 14 fond du Néant, 15 texte.

## Proportions et animations (cibles)
- Ninja 16×32 px dans une cellule de sprite, ennemis 16×24 à 32×48, boss jusqu'à 64×64 (plusieurs sprites).
- Course 8 images, saut 3-4, sabre 3 (anticipation, frappe, retour), dégâts 1, accroupi 1.
- Contour sombre d'1 px, éclairage venant du soleil couchant (gauche-bas), 3 à 4 tons par matière.

## Chaîne de fabrication
- `tools/art/make_art.py` (Python dans WSL, sans dépendance) écrit les PNG indexés dans `res/`.
- `res/gfx.res` les déclare pour rescomp (SGDK). Un PNG venant d'un artiste ou de PixelLab peut remplacer
  un PNG généré tant qu'il respecte : PNG indexé, palette ci-dessus, tuiles 8×8.
- Planches de sprites : images côte à côte, découpées par colonnes (ordre des tuiles des sprites Mega Drive).

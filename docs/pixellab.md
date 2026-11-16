# Graphismes avec PixelLab — mode d'emploi

Les personnages, les ennemis et le tileset viennent de PixelLab (abonnement Tier 1 pris le 2026-11-16),
puis passent par `tools/art/` qui les adapte à la Mega Drive. Voir aussi `docs/style.md`.

## Chaîne
1. Générer dans PixelLab (serveur MCP `pixellab`, ou le site).
2. Télécharger l'export dans `art_in/` :
   - personnage : `https://api.pixellab.ai/mcp/characters/<id>/download` (zip, images PNG par animation) ;
   - tileset : `https://api.pixellab.ai/mcp/sidescroller-tilesets/<id>/image` et `/metadata`.
3. `wsl python3 tools/art/make_art.py` régénère `res/*.png` et `inc/ninja_frames.h`.
4. `build.bat`.

Aperçu sans lancer le jeu : `wsl python3 tools/art/preview.py knight OUT.png 2 10`.

## Identifiants PixelLab
| Quoi | Identifiant |
|---|---|
| Chevalier, épée en main (attaques) | `229a6c03-6cf6-4292-95ac-021f07cddc4c` |
| Chevalier, épée au fourreau (déplacements) | `5febe386-898f-488b-9caf-9c095ae0b930` |
| Ninja rival (vue de côté) | `7b728d8a-026b-494d-9139-57131522b179` |
| Spectre | `0e6df884-243e-4274-bfdd-95d19d787e37` |
| Gelée | `5555eafc-8c9a-4578-ac88-4cd744924642` |
| Garde (vue plongeante) | `7dfee3ab-6d2a-40cc-8a43-50c3d55da636` |
| Tileset château | `dfb2409a-ca35-4c54-87e1-cf55ece5258c` |

## Refonte en vue de côté (2026-11-16, en cours — à reprendre)
Les personnages en vue plongeante ont les pieds à des hauteurs différentes : tout est recréé en `view: side`.
Rien n'est encore importé dans le jeu (`tools/art/knight.py` et `enemies.py` lisent toujours les anciens).

| Quoi | Identifiant | État |
|---|---|---|
| Chevalier de côté, épée au fourreau (48×48) | `8e68e397-22bb-4c4f-8c52-3eba742dd967` | référence validée (cape gardée) ; animations `east` lancées : idle (`breathing-idle`), run, jump, flip (`front-flip`, squelette v3), crouch, hurt |
| Chevalier de côté, épée en main (64×64) | `e417bd96-234e-442f-ab30-4d1a297a06ab` | animations `east` lancées : slash, up, down (en l'air, lame vers le bas), air, crouch_slash |
| Soldat de côté | `1de605bc-2a9d-4b48-adbe-1445cbf23edd` | créé, pas vérifié, pas animé |
| Archer de côté | `3cf21de7-f386-4ef4-a6f3-e41309f7344f` | créé, pas vérifié, pas animé |
| Garde de côté | `a56d4251-bcf6-42c7-b67a-5c1421fbdbda` | créé, pas vérifié, pas animé |
| Chauve-souris (objet 1 direction, 32×32) | `f40d4984-5658-4beb-b58a-1f64504aa476` | 64 candidats à trier (`get_object` puis `select_object_frames`) |

Reste à faire :
- regarder chaque animation du chevalier, relancer les mauvaises, télécharger, mettre à jour `FRAMES` ;
- ennemis : marche (ou vol), attaque, touché, saut pour chacun (soldat, archer, garde, ninja rival, spectre,
  gelée, chauve-souris), puis les brancher dans le moteur. Attention à la VRAM : aujourd'hui une seule image
  par ennemi est chargée (24 tuiles par humain) ; des animations demandent soit un chargement par salle,
  soit un flux d'images comme pour le héros ;
- supprimer les anciens en vue plongeante une fois les nouveaux en jeu (chevalier `229a6c03…`/`5febe386…`,
  garde `7dfee3ab…`, soldat et archers `a7cb6b5f…`, `863d0ab9…`, `659e2501…`) ;
- objets (candélabres, coffres, table, chaise, caisses) : pas commencé.

Limite du Tier 1 : 8 générations en même temps.

## Héros (`tools/art/knight.py`)
- Source : `art_in/knight/<état>/<animation>/frame_NNN.png`, personnage tourné vers la droite.
  Deux états du même chevalier : `sheathed` (épée au fourreau) et `sword` (épée en main).
- La table `FRAMES` relie chaque image du moteur (`inc/ninja_frames.h`, 38 images) à une image exportée.
- Case de 32 × 48, corps centré sur la colonne 16, semelles sur la ligne 46. Ce qui dépasse part dans
  deux sprites d'extension de 32 × 32 au maximum, placés automatiquement.
- 15 couleurs pour tout le héros (PAL1), réduites automatiquement. Emplacements réservés : 1 contour,
  2-3 bleus de la tunique (teintés par le Néant), 5 rouge (cœurs du HUD).
- Les arcs de sabre viennent encore de `tools/art/ninja.py` (dessin par code).

À refaire dans PixelLab :
- **salto** (`front-flip`, squelette v3, état `sheathed`) : la génération a échoué, la pose groupée du saut
  le remplace ;
- **coup vers le bas** : l'épée pointe vers l'avant au lieu du bas ;
- **coup en l'air et coup accroupi** : pas d'animation propre, images empruntées au saut.

## Ennemis (`tools/art/enemies.py`)
- Source : `art_in/enemies/*.png`, une image fixe par ennemi, tournée vers la droite.
- Humains en 32 × 48, spectre et gelée en 32 × 32. Pas encore d'animation.
- PAL2 est partagée avec les objets : 6 couleurs fixes, 7 tirées des ennemis. La forme Néant montre les
  mêmes tuiles avec PAL3 (PAL2 teintée en violet), donc un ennemi n'utilise jamais les couleurs 14 et 15.

## Tileset (`tools/art/castle.py`)
- Source : `art_in/tiles/castle_tileset.png` + `.json` (16 tuiles Wang de 16 × 16).
- Chaque tuile est coupée en quarts de 8 × 8 ; les salles sont dessinées par blocs de 16 × 16 (`src/rooms.c`).
- Ses 9 couleurs occupent les emplacements 1 à 9 de PAL0.

## Consignes de génération
- Vue de côté (`view: side`), « dark realistic 16-bit Sega Genesis pixel art », contour sombre d'1 px,
  pas d'anticrénelage, lumière du couchant venant de gauche-bas.
- Animations en direction `east` seulement.
- Ne jamais demander « comme Shinobi » ou « comme Muramasa » : on décrit, on ne copie pas.

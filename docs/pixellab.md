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

Aperçu sans lancer le jeu : `wsl python3 tools/art/preview.py knight OUT.png 2 10` (ou `enemies`).
Voir les ennemis en jeu, un de chaque dans la première salle :
`.	oolseplay.ps1 -Name poses -Steps "-:50" -Flags "-DDEBUG_ENEMIES"`.

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
Le chevalier et les ennemis de côté sont en jeu (poses clés).

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
- ennemis : poses clés branchées (voir « Ennemis ») ; restent touché, saut, chauve-souris ;
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
- **coup en l'air** : pas d'animation propre, images empruntées au saut ;
- **accroupi** : PixelLab remplit sa toile quelle que soit la pose, donc sa pose à genoux était aussi
  haute que le chevalier debout. On prend l'image 4 de l'animation `sheathed/crouch` (bonne taille), et le
  coup accroupi est réduit à 84 % (`LOW_SCALE`). Une vraie pose basse dessinée à la bonne taille serait mieux.

## Ennemis (`tools/art/enemies.py`)
- Source : `art_in/enemies/poses/<ennemi>/<pose>.png`, tournés vers la droite (humains sur une toile de
  48 × 48, spectre et gelée sur 32 × 32). Les images fixes `art_in/enemies/*.png` ne servent plus.
- Chaque pose est découpée en 1 à 4 sprites (bloc de 32 × 32, jambes de 32 × 16, et une bande à droite
  de 8 ou 16 px quand une arme dépasse). Seules les parties qui contiennent des pixels prennent des tuiles.
  `inc/enemy_frames.h` (généré) donne pour chaque pose sa première tuile, sa position autour des pieds et
  ses parties.
- Mémoire vidéo : toutes les poses restent chargées (544 tuiles aujourd'hui), après les objets, à partir
  de `GFX_ENEMIES`. Il reste 120 tuiles avant la police ; `enemies.py` refuse de générer au-delà.
  L'ancienne place des ennemis (128 tuiles à partir de `TILE_USER_INDEX + 480`) est libre.
- Sprites : les ennemis se partagent 36 sprites (`ENEMY_SPRITES` dans `main.c`), le jeu en utilise 68
  sur les 80 de la console.
- PAL2 est partagée avec les objets : 6 couleurs fixes, 7 tirées des ennemis (toutes poses confondues).
  La forme Néant montre les mêmes tuiles avec PAL3 (PAL2 teintée en violet), donc un ennemi n'utilise
  jamais les couleurs 14 et 15.
- Le moteur choisit parmi 4 poses par ennemi (`ENEMY_POSE_*`, `game_enemy.c`) : deux de déplacement,
  une d'annonce, une d'attaque. Le tableau `POSE_FRAME` de `main.c` les relie aux dessins.

| Ennemi | Déplacement | Annonce | Attaque |
|---|---|---|---|
| Soldat | marche A/B, change tous les 8 px | épée levée | coup d'épée (portée 24 px, esquivé accroupi) |
| Garde | marche A/B | lance baissée | coup de lance (portée 28 px, touche même accroupi) |
| Archer | immobile | arc bandé | flèche partie |
| Ninja | course A/B | bras levé avant le bond | lancer, en l'air |
| Spectre | flotte A/B | — | fonce quand il poursuit |
| Gelée | écrasée au sol | — | étirée en l'air |

Manque : une pose d'attente pour le ninja (à l'arrêt il garde une pose de course), une marche pour
l'archer si on veut qu'il bouge (les images existent : `archer/walk_a` et `walk_b`).

## Tileset (`tools/art/castle.py`)
- Source : `art_in/tiles/castle_tileset.png` + `.json` (16 tuiles Wang de 16 × 16).
- Chaque tuile est coupée en quarts de 8 × 8 ; les salles sont dessinées par blocs de 16 × 16 (`src/rooms.c`).
- Ses 9 couleurs occupent les emplacements 1 à 9 de PAL0.

## Consignes de génération
- Vue de côté (`view: side`), « dark realistic 16-bit Sega Genesis pixel art », contour sombre d'1 px,
  pas d'anticrénelage, lumière du couchant venant de gauche-bas.
- Animations en direction `east` seulement.
- Ne jamais demander « comme Shinobi » ou « comme Muramasa » : on décrit, on ne copie pas.

## Poses clés (2026-11-16 soir)
Les animations d'action PixelLab (squelette) sont moches et changent la taille : on fait des poses
clés, une par state, sur la toile de 48 x 48 du personnage. Lames coupées par la toile : prolongées
dans `tools/art/knight.py` (`BLADES`).

Chevalier (`art_in/knight/poses/`) : windup 3e842b31, strike e6b7264c (remplacés en jeu par `windup_wide`
39809e8c et `strike_wide` 8b57c446 : mêmes poses refaites sur une toile de 80 x 80 avec `override_width` /
`override_height`, le personnage garde sa taille et la cape et l épée ne sont plus coupées par le bord), up cb4a2521, down 0e74bd52,
down2 50c15218 (coup vers le bas refait : genoux pliés, deux mains sur la garde, lame prolongée sous les pieds),
rise 223250ef, tuck e11895e6, fall 1015321f, land 3ee48d23, crouch faab12ec, crouch_strike 6d08e946.

Ennemis (états de profil, même méthode) :
- soldat 1de605bc : walk A 8150c291, walk B 140eef1f, windup 6634db2d, strike c8604fef
- archer 3cf21de7 : walk A ec82f5f3, walk B 186673a5, aim d87c7854, release e988ebc2
- garde a56d4251 : walk A bbd1f395, walk B 1828e1b8, brace 6c81a9b8, thrust bb2552c9
- ninja rival 7b728d8a : run A 6ca70115, run B 57d6b441, throw windup f38a1de3, throw 3cdcdf17
- spectre 0e6df884 : float A d043e820, float B f2c379a5, lunge ed11326a
- gelée 5555eafc : squash e1b1df96, stretch a814944c

Téléchargement d'une pose : `https://backblaze.pixellab.ai/file/pixellab-characters/1f4ad84c-b4e4-40e6-9923-41b6cf028790/<id complet>/rotations/east.png`
(l'id complet est donné par `get_character`).

# Héros avec PixelLab — mode d'emploi

But : un héros dessiné en vrai pixel art (outil IA PixelLab, puis retouches), branché dans le jeu sans
toucher au moteur. Voir aussi `docs/style.md` (section « Le héros »).

## Contraintes techniques (à respecter, sinon l'import échoue)
- Une image = **32 × 48 px**, personnage tourné **vers la droite**, fond transparent.
- Corps centré sur la colonne 16, **semelles sur la ligne 46** (la ligne 47 = contour sous les pieds).
- **15 couleurs maximum** pour tout le héros (+ transparent), les mêmes dans toutes les images.
  Garder un rouge vif (cœurs du HUD) et un contour sombre.
- Les arcs de sabre peuvent déborder : ils seront découpés en cases 32 × 32 à part (comme aujourd'hui).
- Export : une planche PNG par animation (images côte à côte) ou une image par fichier.

## Description à donner à PixelLab (en anglais, c'est mieux compris)
Personnage :
> side view, 32x48 pixel art, Sega Genesis / Mega Drive style, dark realistic 16-bit, a lean ninja in a
> dark navy blue suit, red headband with long tails flowing behind, cloth mask over the lower face,
> katana in a scabbard on his back with the grip above his shoulder, light bronze armour on forearms
> and shins, red belt, dusk lighting from lower left, 1-pixel dark outline, limited palette of 15 colours,
> facing right, transparent background

Style : « dark realistic 16-bit », réglages sans anticrénelage (no anti-aliasing), contour sélectif sombre.
Ne jamais demander « comme Shinobi » ou « comme Muramasa » : on décrit, on ne copie pas.

## Animations à produire (dans cet ordre)
| Animation | Images | Notes |
|---|---|---|
| idle | 4 | garde de combat, respire, main sur la poignée |
| run | 8 | penché en avant, grandes foulées, bandeau tendu derrière |
| rise | 1 | montée du saut |
| flip | 4 | salto au sommet |
| fall | 1 | chute |
| land | 1 | réception genou plié |
| slash | 5 | anticipation, 2 frappe (arc large), 2 retour |
| up | 3 | coup vers le haut |
| air | 4 | coup de côté en l'air |
| down | 3 | coup vers le bas en l'air |
| crouch | 1 | accroupi |
| crouch_slash | 2 | coup accroupi |
| hurt | 1 | touché, recul |

Soit 38 images (même liste que `inc/ninja_frames.h`, le moteur n'a rien à changer).

## Méthode conseillée
1. Générer d'abord **une seule image** du héros de profil (idle). Itérer jusqu'à ce qu'elle plaise :
   c'est elle qui sert de référence de style pour tout le reste.
2. Générer les animations à partir de cette référence (outil d'animation / squelette de PixelLab).
3. Retoucher : pixels parasites, contour, couleurs ramenées à la même palette.
4. Déposer les PNG dans `art_in/hero/` (un fichier par animation, nommé comme dans le tableau :
   `idle.png`, `run.png`…) et demander à Claude de lancer l'import : un script réduira à la palette,
   placera les pieds sur la ligne 46, découpera les arcs et régénérera `res/ninja.png`.

# The Curse — guide de style

Objectif (Thomas) : un très beau jeu, la Mega Drive poussée au maximum. Inspirations : Shinobi III, Castlevania
Bloodlines, Rogue Legacy, Metroid pour le Néant. On s'inspire, on ne copie jamais un sprite ou un décor existant.

## Identité visuelle (décidée avec Thomas, 2026-11-16)
1. **Rendu réaliste sombre**, façon Shinobi III / Castlevania Bloodlines : proportions humaines, ombrages
   en 3-4 tons par matière, ambiance gothique. Pas de grosses têtes, pas d'humour visuel.
2. **Héros grand, 32×48 px** (4×6 tuiles par image). Ennemis humains à la même échelle (32×48), petites
   créatures 16×16 à 24×24, boss 64×64 à 96×80 (plusieurs sprites). Le moteur est à adapter : hitbox et
   décalage du sprite du joueur, salles plus petites à l'écran.
3. **Néant organique façon Metroid** : le château est digéré. Chair violette, veines, pulsations
   (cycle de palette), excroissances. Forme Néant d'un ennemi = même silhouette envahie : yeux en plus,
   tentacules, carapace chitineuse, couleurs PAL3.
4. **Bestiaire = la garde maudite** : les anciens habitants du château corrompus par le méchant. Soldats,
   archers, ninjas rivaux, gardes à bouclier, serviteurs ; la gelée et le spectre sont des restes de la
   malédiction. Chaque ennemi garde des traces de son ancien rôle (blason, armure abîmée, uniforme
   déchiré).

### Le héros (Thomas, 2026-11-16) : entre Shinobi et Kisuke
**Visuel inspiré de Shinobi** : ninja sobre et lisible. Tenue près du corps, masque sur le bas du visage,
bandeau à longs pans, sabre dans le dos avec une main toujours posée sur la poignée. Garde l'identité
actuelle (tenue bleu nuit, bandeau et ceinture rouges) mais avec plus de détails : plis, sangles, armure
légère aux avant-bras et aux tibias.

**Mouvement inspiré de Kisuke (Muramasa)** : fluidité avant tout.
- Course penchée vers l'avant, foulées longues, bandeau et pans de tenue qui traînent loin derrière.
- Images de « traînée » (smear) sur les coups : arcs de sabre larges et courbes, en 2 tons, sur un sprite
  à part.
- Saut avec vrille au sommet (salto), retombée amortie (1 image de réception genou plié).
- Enchaînements qui coulent : chaque fin d'animation mène naturellement à la suivante, pas de pose figée.
- Tissus et cheveux en physique secondaire : ils réagissent avec 1-2 images de retard sur le corps.
- Toujours son propre dessin : ni sprite de Shinobi ni de Muramasa copié ou décalqué.

### Consignes de dessin (pour tout sprite)
- Lumière du couchant venant de gauche-bas en zone 1, contour sombre d'1 px (pas noir pur à l'intérieur).
- Silhouette lisible en noir sur fond : on doit reconnaître un ennemi et son attaque à sa seule forme.
- Signal d'attaque (anticipation) visible au moins 1 image avant chaque coup.
- Palette imposée par couche (voir plus bas), 15 couleurs + transparent, aucune couleur hors palette.
- S'inspirer de Shinobi / Bloodlines / Rogue Legacy / Metroid pour la qualité, ne jamais copier un sprite.

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
- **PAL0** décor : 1-9 tileset du château (pierre, mousse ; bois des planches pris dans ces couleurs), 10-14 ciel, 15 texte.
- **PAL1** héros + cœurs du HUD.
- **PAL2** ennemis forme normale, pièces, objets : 1-6 fixes (contour, os, rouge, or), 7-13 tirées des ennemis, 14 objets seulement.
- **PAL3** ennemis forme Néant : PAL2 teintée en violet (mêmes tuiles). Le décor avalé passe par le mode ombre.

## Proportions et animations (cibles)
- Ninja 32×48 px (4×6 tuiles), ennemis humains 32×48, créatures 16×16 à 24×24, boss 64×64 à 96×80.
- Course 8 images, saut 3-4, sabre 3 (anticipation, frappe, retour), dégâts 1, accroupi 1.
- Contour sombre d'1 px, éclairage venant du soleil couchant (gauche-bas), 3 à 4 tons par matière.

## Chaîne de fabrication
- `tools/art/make_art.py` (Python dans WSL, sans dépendance) écrit les PNG indexés dans `res/`, à partir des exports PixelLab rangés dans `art_in/` (voir `docs/pixellab.md`).
- `res/art.res` les déclare pour rescomp (SGDK). Un PNG venant d'un artiste ou de PixelLab peut remplacer
  un PNG généré tant qu'il respecte : PNG indexé, palette ci-dessus, tuiles 8×8.
- Planches de sprites : images côte à côte, découpées par colonnes (ordre des tuiles des sprites Mega Drive).

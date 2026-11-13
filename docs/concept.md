# The Curse

## Notes d'origine de Thomas (telles quelles)
> name : the curse
> -> cartes: voleur, mage shinobi, guerrier
> le néant approche et detruit tout. Il faut trouver la gemme qui annulera le sort
>
> Nivaeux générés aléatoirement
> Boss appelé quand gemme trouvé (corrélation gemme boss ?)
> -> nouveau pouvoir suivant boss affronté
>
> Pour trouver la gemme, il faut parcourir le niveau (clés, cartes, upgrades, downgrades, coeurs, enemis ...)
>
> Visuels
>  shinobi, look megadrive rapide puissant peu de défense saute haute
>  guerrier, look basique, moins rapide meilleure defense
>  mage, sorts bouclier magique (ressemble a merlin) besoin de mana (plus que les autres)
>  voleur rapide moins fort, bonne defense, n'a pas besoin de clés sauf pour super coffres
>
> Les persos ont plusieurs looks possibles
>
> J'avais pensé à une idée de sphère qui entoure l'ennemi (le néant)
> elle se rapproche de plus en plus de l'endroit de la gemme et il faut recup la gemme avant que la sphere ne soit
> arrivée à son but (compte à rebours), dans la sphere, les ennemis sont normaux, hors de la sphere, ambiance sombre
> et beacoup plus dure
> On peut trouver des fige temps ou des repousse neant mais aussi des accélérateurs
> Et d'autres améliorations du type de ce qu'on trouve dans rogue legacy
> mais je souhaite un style visuel ptet prus proche de metroid, castlevania shinobi
>
> On peut jouer en coop jusqu'à 4
> (j'ai noté faire regle de trois mais je sais plus le contexte de cette remarque)
>
> Questions : que faire si jeu fini rapidement / boss de fin ? / map générale ou pas, possibilité de retourner dans
> anciens niveaux ?

## Idée : contenu en couches et secrets (Thomas, 2026-10-07)
Piste de réponse à « que faire si jeu fini rapidement ». Principe : on peut finir vite, mais pas tout voir.

- **Trois couches de durée** :
  - run normale ~30–45 min (on avance, on bat le boss, on voit la fin) ;
  - exploration 1–2 h : passages secrets, salles cachées, objets facultatifs, personnages cachés, niveaux bonus,
    fins alternatives ;
  - 100 % : plusieurs heures, avec certains secrets volontairement absurdes à trouver.
- **Cheat codes à découvrir dans le jeu** (pas juste le Konami code). Ex. inscription dans une salle secrète
  « MARS — 23 — 07 — 91 » → combinaison sur l'écran titre → mode développeur.
  Effets possibles : 99 vies, arme spéciale, mode difficile, mode miroir, ralentissement, level select, boss rush,
  sound test chiptune, sprite viewer, New Game+ caché.
- **Secrets physiques via la notice** : indices cryptiques (« Certains disent que la lune n'apparaît jamais deux fois
  au même endroit. » → attendre sous la lune à un endroit précis ouvre un passage), codes imprimés sous forme
  d'énigme.
- **Périmètre** : ~6 niveaux courts mais bourrés d'embranchements et de secrets plutôt qu'un gros jeu ; compatible
  Mega Drive. Effet recherché : « T'as trouvé la salle derrière le mur du niveau 3 ? »

## Décisions
- Plateforme : Mega Drive (SGDK), cartouche physique + boîte + manuel.
- Sphère du Néant : pas de transparence ; limite nette (tuiles + couronne de sprites animés), monde « autre » dehors
  (palettes froides, mode shadow, tuiles corrompues).
- Séquence d'intro illustrée présentant les personnages ; une fin illustrée propre à chaque personnage.
- Se fait après Twin Thrust.
- **Genre (Thomas, 2026-11-13)** : action-plateforme à la Shinobi / Metroid / Rogue Legacy, avec une logique de
  salles à la Binding of Isaac. Il y a des salles secrètes, et à force de jouer on sait où les chercher.

## Structure d'étage (proposition du 2026-11-13, à valider)
- Étage = grille de salles générée à chaque partie. Les salles elles-mêmes sont dessinées à la main (gabarits),
  le générateur ne fait que les assembler. Ce sont des salles de plateforme vues de côté (pas en vue de dessus).
- Taille des salles : 1 écran (40×28 tuiles), plus des grandes salles 2×1 (couloir) et 1×2 (puits).
- Portes sur 4 côtés : gauche/droite au sol, haut/bas via puits et plateformes. Chaque gabarit déclare
  ses portes possibles.
- Salles spéciales : départ, trésor, boutique, clé/super coffre (voleur), gemme, boss.
- **Néant (Thomas, 2026-11-13)** : un cercle centré sur un point de la carte, qui se referme petit à petit.
  Au centre se trouve la salle du boss. Les salles hors du cercle passent en mode « autre » (palette froide,
  ennemis durs). Il faut trouver la gemme avant que la zone sûre soit trop petite. Minicarte façon Isaac dans le HUD.
- **Gemme (Thomas, 2026-11-13)** : le boss est le gardien de la gemme ; pas de salle de gemme séparée.
- **Salle d'upgrade (Thomas, 2026-11-13)** : une par étage, garantie. Comme dans Isaac, l'objet peut être bon
  ou mauvais (rejoint les « upgrades, downgrades » des notes d'origine).
- **Pouvoir de boss (Thomas, 2026-11-13)** : battre le boss donne un nouveau pouvoir (notes d'origine : « nouveau
  pouvoir suivant boss affronté »). Validé : c'est le boss qui lâche un pouvoir lié à lui (à la Mega Man).
  Les gemmes brisent la malédiction de chaque étage et se collectionnent jusqu'au boss final.
- **Fin d'étage (Thomas, 2026-11-13)** :
  - entrer dans la salle du boss lance le combat et arrête la progression du Néant ;
  - salle déjà avalée (cercle refermé jusqu'au boss) → boss plus fort ; sinon boss normal ;
  - arriver chaque fois au boss avant le Néant débloque un boss rush ;
  - boss battu → courte cinématique : une main tient la gemme, la zone sûre explose vers l'extérieur,
    le monde redevient normal → étage suivant.
- **Coop (Thomas, 2026-11-13)** : 2 joueurs maximum sur Mega Drive, à condition que ça apporte vraiment
  quelque chose. Un portage Steam pourra aller jusqu'à 4.
- **Règles fixes des secrets (apprenables par le joueur)** :
  - salle secrète : case vide touchant le plus de salles (3 ou 4 voisines) ;
  - super secrète : case vide qui ne touche qu'une seule salle, jamais le boss ;
  - mur cassable discret (brique un peu différente, son creux quand on frappe) ;
  - le « trou » dans la minicarte est un indice en soi.
- Les secrets fixes de l'idée du 07/10 (la lune, les inscriptions de codes) deviennent des salles spéciales
  rares, avec leurs propres règles d'apparition.
- Graine (seed) affichable et saisissable, ce qui permet aussi de partager une partie ou de cacher des codes.

## À décider
- Histoire, sauvegarde (SRAM/FRAM ou mots de passe), périmètre v1 (classes, coop 2 ou 4), graphismes et musique.
- Nombre d'étages, taille de la grille, ce qui ouvre un mur secret (arme normale, bombe, pouvoir).

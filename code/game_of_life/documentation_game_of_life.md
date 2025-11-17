# Projet : Implémentation du Jeu de la Vie en Rust et WebAssembly

## 1. Objectif du projet

L'objectif du projet était d'implémenter **le Jeu de la Vie de Conway** en utilisant le langage **Rust**, puis de compiler la logique du programme en **WebAssembly (WASM)** afin de l'exécuter directement dans un navigateur Web.\
Le projet devait aussi inclure une interface interactive permettant à l'utilisateur de contrôler la simulation.

---

## 2. Technologies utilisées

### Rust

Rust a été utilisé pour coder toute la logique du Jeu de la Vie.\
Les raisons du choix : 
- performance élevée
- sécurité mémoire
- excellente compatibilité WebAssembly

### WebAssembly

WASM sert à exécuter du code Rust dans le navigateur avec des
performances proches du natif.\
Le projet utilise : 
- le target Rust `wasm32-unknown-unknown`
- l'outil `wasm-pack` pour générer les fichiers WebAssembly et le code
JavaScript associé

### JavaScript

JavaScript est utilisé pour : 
- charger le module WebAssembly
- lire les données envoyées par Rust
- dessiner la grille dans un `<canvas>` HTML
- gérer les interactions utilisateur (boutons, souris, vitesse, etc.)

### HTML Canvas

Le `<canvas>` permet un affichage graphique simple mais efficace du
contenu du jeu.

---

## 3. Structure générale du projet

Le projet est divisé en deux parties principales :

### 3.1 Partie Rust

Le code Rust contient : 
- la structure `Universe`
- un vecteur de cellules (mortes/vivantes)
- les règles du Jeu de la Vie appliquées à chaque "tick"
- des fonctions exposées à JavaScript via `wasm_bindgen`

### 3.2 Partie Web (JavaScript + HTML)

Le code côté navigateur : 
- charge le module WASM
- initialise une grille graphique
- redessine la simulation à chaque mise à jour
- détecte les clics pour dessiner des cellules
- implémente les contrôles :
    - **Lecture/Pause**
    - **Vitesse réglable**
    - **Randomisation de la grille**
    - **Effacement complet**

---

## 4. Détails d'implémentation

### 4.1 Représentation de l'univers

L'univers est représenté sous forme de tableau 1D (`Vec<u8>`) où : 
- `0` = cellule morte
- `1` = cellule vivante

L'indexation est faite par :

    index = ligne * largeur + colonne

### 4.2 Application des règles

Chaque "tick" applique les quatre règles classiques du Jeu de la Vie :
1. Une cellule vivante avec moins de 2 voisins meurt
2. Une cellule vivante avec 2 ou 3 voisins reste vivante
3. Une cellule vivante avec plus de 3 voisins meurt
4. Une cellule morte avec exactement 3 voisins devient vivante

L'univers est **toroïdal** (les bordures se rejoignent).

---

## 5. Intégration WebAssembly

### 5.1 Compilation vers WASM

Le module est généré avec :

    wasm-pack build --target web

Cela crée un dossier `pkg/` contenant : 
- le fichier `.wasm`
- le fichier JavaScript qui expose les fonctions Rust au navigateur

### 5.2 Interaction avec JavaScript

JavaScript : 
1. charge le module WebAssembly
2. instancie un `Universe`
3. récupère la grille (avec `get_cells()`)
4. dessine les cellules dans le `<canvas>`
5. appelle Rust (`tick()`, `randomize()`, `clear()`, `toggle_cell()`)
selon les actions de l'utilisateur

---

## 6. Fonctionnalités interactives

### 6.1 Lecture / Pause

Le programme utilise une boucle basée sur `setTimeout` permettant de contrôler l'exécution de la simulation.

### 6.2 Contrôle de la vitesse

Une barre de défilement ajuste la vitesse entre 1 et 60 ticks par seconde.

### 6.3 Randomisation

Chaque cellule reçoit aléatoirement la valeur 0 ou 1 via la fonction Rust `randomize()`.

### 6.4 Effacement

Toutes les cellules sont remises à 0 via `clear()`.

### 6.5 Dessin avec la souris

En cliquant sur une cellule, JavaScript calcule sa position et appelle Rust pour changer son état, ce qui permet de dessiner des motifs.

---

## 7. Problèmes rencontrés et solutions

### 7.1 Fonctions Rust non reconnues dans JavaScript

Certaines méthodes ne s'exportaient pas parce qu'elles n'étaient pas annotées avec `#[wasm_bindgen]`.\
Solution : ajouter l'annotation sur toutes les fonctions publiques.

### 7.2 Mauvaise structure des fichiers

Le dossier `pkg/` n'était pas à l'endroit où le serveur local cherchait les fichiers.\
Solution : s'assurer que le serveur Python est lancé depuis le bon dossier.

### 7.3 Problèmes d'import dans HTML

Les chemins comme `../pkg/...` devaient être corrigés selon l'arborescence exacte.\
Une simple erreur de chemin provoquait un échec de chargement du module WASM.

### 7.4 Mise en cache du navigateur

Le navigateur stockait d'anciennes versions du module `.wasm`.\
Solution : recompiler proprement et recharger la page avec `Ctrl + F5`.

---

## 8. Résultat final

La version finale du projet offre : 
- une simulation fluide du Jeu de la Vie
- un rendu graphique clair et centré
- des boutons interactifs fonctionnels
- la possibilité de dessiner des cellules directement à la souris
- un contrôle complet de la vitesse
- une intégration propre entre Rust, WebAssembly et JavaScript

Le programme démontre l'intérêt de Rust et WebAssembly pour créer des applications Web performantes.

---

## 9. Conclusion

Ce projet a permis de comprendre : 
- comment compiler du Rust en WebAssembly
- comment exposer des fonctions Rust au JavaScript
- comment gérer la communication entre WASM et le navigateur
- comment dessiner et animer une grille dans un canvas
- comment faire une interface web interactive

Le Jeu de la Vie est un excellent exemple pour explorer ces technologies, car il combine une logique interne simple mais computationnellement intensive avec un affichage graphique continu.

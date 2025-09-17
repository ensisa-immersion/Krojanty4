# Krojanty4

Code source de Krojanty réalisé par le groupe 4.

## Introduction du projet

Ce projet a été réalisé lors des deux premières semaines à l'ENSISA. Le but est de créer, en groupe, un projet couvrant divers points de l'informatique et des réseaux.

Composition du groupe :

- Corentin BOEGLIN
- Mamadou DIALLO
- Georges EL CHATER
- Kioko FIAULT
- Loïc MAURER
- Anna MAUROY
- Alexis MIRAS
- Ranim ...

## Installation des dépendances

Les dépendances suivantes sont à installer sur votre machine :

- `gcc` - Compilation des fichiers C
- `gtk4` - Interface graphique du jeu
- `make` - Utilisation des commandes de compilation projet
- `doxygen` - Compilation de la documentation

## Compilation

### Compiler le jeu

Pour compiler le jeu, vous devez effectuer la commande suivante :

```cmd
make
```

Le fichier binaire sera créé dans le dossier `build/` et s'appellera `game`.

### Documentation

Pour mettre en place la documentation, vous devez effectuer la commande suivante :

```cmd
make docs
```

Cela aura pour effet de créer un dossier `output` dans `docs/`. Il contient tous les fichiers statiques pour héberger la documentation.

Vous devez avoir un serveur web fonctionnel pour visualiser la documentation sous format HTML.

__La documentation contient :__

- La description du projet.
- Comment compiler les divers fichiers (Binaires, docs, etc.).
- Comment contribuer.
- La documentation technique du code.

### Nettoyage des résidus

Pour nettoyer les résidus de compilation, vous devez effectuer la commande suivante :

```cmd
make clean
```

## Lancement du jeu

> [!note]
> Vous devez d'abord compiler le jeu avant de le lancer.

Vous pouvez lancer le jeu dans trois modes différents : en local, en serveur ou en client.

En mode serveur, le jeu héberge une partie sur un port donné.
En mode client, le jeu se connecte à un serveur à l'aide d'une IP:port donné.

### Local

Pour lancer le jeu en mode local, vous devez effectuer la commande suivante :

```cmd
./build/game -l
```

### Serveur

Pour lancer le jeu en mode serveur, vous devez effectuer la commande suivante :

```cmd
./build/game -s <port>
```

`<port>` est le port sur lequel le serveur va écouter les connexions entrantes.

### Client

Pour lancer le jeu en mode client, vous devez effectuer la commande suivante :

```cmd
./build/game -c <ip>:<port>
```

### Lancer le jeu avec une IA

Pour lancer le jeu avec une IA, vous devez ajouter l'option `-ia` à la commande du client.

Par exemple :

```cmd
./build/game -ia -c <ip>:<port>
```

Une IA peut aussi être lancée que ce soit en mode serveur ou en mode client.

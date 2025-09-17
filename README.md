# Krojanty4

Code source de Krojanty réalisé par le groupe 4.

## Introduction du projet

Ce projet a été réalisé lors des deux premières semaines à l'ENSISA. Le but est de créer, en groupe, un projet couvrant divers points de l'informatique et des réseaux.

C'est un jeu de stratégie (Avec IA intégrée) en 2D. Inspiré du Tablut/Hnefatafl, vous pouvez lire les règles du jeu dans le fichier `intro_jeu.pdf`.

Le but principal est de prendre le plus possible de cases en déplaçant ses pions sur le plateau. L'adversaire pouvant faire de même, de la stratégie est nécessaire pour gagner.

Composition du groupe :

- Boeglin Corentin
- Diallo Mamadou
- El Chater Georges
- Fiault Kioko
- Maurer Loïc
- Mauroy Anna
- Miras Alexis
- ... Ranim

## Installation des dépendances

Les dépendances suivantes sont à installées sur votre machine :

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

Le fichier binaire sera créé dans le dossier `bin/` et s'appelle `game`.

### Documentation

Pour mettre en place la documentation, vous devez effectuer la commande suivante :

```cmd
make docs
```

Cela aura pour effet de créer un dossier dans `docs/output`. Il contient tout les fichiers statiques pour héberger la documentation.

Vous devez avoir un serveur web fonctionnel pour visualiser la documentation sous format HTML.

La documentation contient :

- La description du projet.
- Comment compiler les divers fichiers (Binaires, docs, etc.).
- Comment contribuer.
- La documentation technique du code.

### Nettoyage des résidus

Pour nettoyer les résidus de compilation, vous devez effectuer la commande suivante :

```cmd
make clean-all
```

## Lancement du jeu

> [!note]
> Vous devez d'abord compiler le jeu avant de le lancer.

Vous pouvez lancer le jeu dans deux modes différents : serveur ou client.

En mode serveur, le jeu héberge une partie sur un port donné.
En mode client, le jeu se connecte à un serveur à l'aide d'une IP:port donné.

### Serveur

Pour lancer le jeu en mode serveur, vous devez effectuer la commande suivante :

```cmd
./bin/game -s <port>
```

`<port>` est le port sur lequel le serveur va écouter les connexions entrantes.

### Client

Pour lancer le jeu en mode client, vous devez effectuer la commande suivante :

```cmd
./bin/game -c <ip>:<port>
```

### Lancer le jeu avec une IA

Pour lancer le jeu avec une IA, vous devez ajouter l'option `-ia` à la commande du client.

Par exemple :

```cmd
./bin/game -ia -c <ip>:<port>
```

Une IA peut aussi être lancée que ce soit en mode serveur ou en mode client.

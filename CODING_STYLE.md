# Guide de Style de Code C

## 1. Indentation et Espaces

- Utilisez 4 espaces pour l'indentation (pas de tabulations).
- Limitez la longueur des lignes à 100 caractères.

## 2. Accolades

- Placez l'accolade ouvrante sur la même ligne que la déclaration (style K&R) :

    ```c
    if (condition) {
            // ...
    }
    ```

## 3. Nommage

- Fonctions et variables : snake_case (`ma_fonction`, `ma_variable`).
- Constantes et macros : MAJUSCULES_AVEC_UNDERSCORE.
- Types (struct, enum, typedef) : snake_case ou suffixe `_t`.

## 4. Commentaires

- Utilisez le format Doxygen pour documenter les fonctions et structures :

    ```c
    /**
     * @brief Additionne deux entiers.
     * @param a Premier entier
     * @param b Deuxième entier
     * @return Somme des deux entiers
     */
    int addition(int a, int b);
    ```

- Commentez les parties complexes du code.

## 5. Organisation des fichiers

- Un module = un fichier .c et un .h associés.
- Les headers sont dans `include/`, les sources dans `src/`.

## 6. Divers

- Toujours inclure les headers nécessaires.
- Protégez les headers avec des include guards (`#ifndef ... #define ... #endif`).
- Initialisez les variables.
- Privilégiez les fonctions courtes et lisibles.

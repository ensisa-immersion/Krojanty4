/**
 * @file move_util_test.h
 * @brief Fichier d'en-tête pour les fonctions de test des utilitaires de mouvement
 * 
 * Cet en-tête contient les déclarations pour les fonctions utilitaires utilisées dans les tests
 * des opérations de mouvement, incluant :
 * - Utilitaires de mappage et conversion de colonnes
 * - Fonctions d'application de mouvement pour environnement de test
 * - Stubs d'intégration GTK pour tester sans interface graphique
 * 
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 */

#ifndef MOVE_UTIL_TEST_H
#define MOVE_UTIL_TEST_H

#include "game.h"

/* Mappage des colonnes A-I */
extern const char COLS_MAP[10];

/* Convertit une lettre de colonne en indice (0..8) */
int col_depuis_lettre(char L);

#ifdef TEST_BUILD
/* Versions simplifiées pour les tests (sans GTK) */
int appliquer_mouvement_inactif(void *donnees);
void poster_mouvement_vers_gtk(Game *jeu, const char m[4]);
#endif

#endif /* MOVE_UTIL_TEST_H */

/**
 * @file test_move_util.c
 * @brief Tests unitaires pour les utilitaires de mouvement
 */

#include <stdio.h>
#include <string.h>

#include "game.h"
#include "move_util_test.h"
#include "logging.h"

// Compteurs de tests
static int tests_passed = 0;
static int tests_failed = 0;

// Macro pour les assertions avec messages
#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("[TEST][MOVE][OK] %s\n", message); \
            LOG_SUCCESS_MSG("[TEST][MOVE][OK] %s", message); \
            tests_passed++; \
        } else { \
            printf("[TEST][MOVE][KO] %s\n", message); \
            LOG_ERROR_MSG("[TEST][MOVE][KO] %s", message); \
            tests_failed++; \
        } \
    } while(0)

/**
 * Test de conversion des colonnes
 */
void test_col_from_letter() {
    TEST_ASSERT(col_from_letter('A') == 0, "Colonne A -> index 0");
    TEST_ASSERT(col_from_letter('B') == 1, "Colonne B -> index 1");
    TEST_ASSERT(col_from_letter('C') == 2, "Colonne C -> index 2");
    TEST_ASSERT(col_from_letter('D') == 3, "Colonne D -> index 3");
    TEST_ASSERT(col_from_letter('E') == 4, "Colonne E -> index 4");
    TEST_ASSERT(col_from_letter('F') == 5, "Colonne F -> index 5");
    TEST_ASSERT(col_from_letter('G') == 6, "Colonne G -> index 6");
    TEST_ASSERT(col_from_letter('H') == 7, "Colonne H -> index 7");
    TEST_ASSERT(col_from_letter('I') == 8, "Colonne I -> index 8");

    // Test des caractères invalides
    TEST_ASSERT(col_from_letter('J') == -1, "Colonne invalide J -> -1");
    TEST_ASSERT(col_from_letter('Z') == -1, "Colonne invalide Z -> -1");
    TEST_ASSERT(col_from_letter('a') == -1, "Colonne minuscule a -> -1");
}

/**
 * Test du mappage des colonnes
 */
void test_cols_map() {
    TEST_ASSERT(COLS_MAP[0] == 'A', "COLS_MAP[0] = A");
    TEST_ASSERT(COLS_MAP[8] == 'I', "COLS_MAP[8] = I");
    TEST_ASSERT(strlen(COLS_MAP) == 9, "COLS_MAP contient 9 caractères");

    // Vérifier que toutes les lettres sont présentes
    const char *expected = "ABCDEFGHI";
    for (int i = 0; i < 9; i++) {
        char msg[50];
        snprintf(msg, sizeof(msg), "COLS_MAP[%d] = %c", i, expected[i]);
        TEST_ASSERT(COLS_MAP[i] == expected[i], msg);
    }
}

/**
 * Test de conversion des mouvements (simulation)
 */
void test_move_conversion() {
    // Test de conversion manuelle (on simule post_move_to_gtk sans GTK)
    char move1[] = "A9B9";  // De A9 à B9
    int sc1 = col_from_letter(move1[0]);
    int sr1 = 9 - (move1[1] - '0');
    int dc1 = col_from_letter(move1[2]);
    int dr1 = 9 - (move1[3] - '0');

    TEST_ASSERT(sc1 == 0, "A9B9: source colonne = 0 (A)");
    TEST_ASSERT(sr1 == 0, "A9B9: source ligne = 0 (ligne 9)");
    TEST_ASSERT(dc1 == 1, "A9B9: destination colonne = 1 (B)");
    TEST_ASSERT(dr1 == 0, "A9B9: destination ligne = 0 (ligne 9)");

    char move2[] = "C8D7";  // De C8 à D7
    int sc2 = col_from_letter(move2[0]);
    int sr2 = 9 - (move2[1] - '0');
    int dc2 = col_from_letter(move2[2]);
    int dr2 = 9 - (move2[3] - '0');

    TEST_ASSERT(sc2 == 2, "C8D7: source colonne = 2 (C)");
    TEST_ASSERT(sr2 == 1, "C8D7: source ligne = 1 (ligne 8)");
    TEST_ASSERT(dc2 == 3, "C8D7: destination colonne = 3 (D)");
    TEST_ASSERT(dr2 == 2, "C8D7: destination ligne = 2 (ligne 7)");

    char move3[] = "I1A1";  // De I1 à A1
    int sc3 = col_from_letter(move3[0]);
    int sr3 = 9 - (move3[1] - '0');
    int dc3 = col_from_letter(move3[2]);
    int dr3 = 9 - (move3[3] - '0');

    TEST_ASSERT(sc3 == 8, "I1A1: source colonne = 8 (I)");
    TEST_ASSERT(sr3 == 8, "I1A1: source ligne = 8 (ligne 1)");
    TEST_ASSERT(dc3 == 0, "I1A1: destination colonne = 0 (A)");
    TEST_ASSERT(dr3 == 8, "I1A1: destination ligne = 8 (ligne 1)");
}

/**
 * Test des mouvements invalides
 */
void test_invalid_moves() {
    // Test avec caractères invalides
    char invalid1[] = "Z9A9";
    int sc_invalid = col_from_letter(invalid1[0]);
    TEST_ASSERT(sc_invalid == -1, "Mouvement avec colonne invalide Z");

    char invalid2[] = "A0B1";  // Ligne 0 n'existe pas
    int sr_invalid = 9 - (invalid2[1] - '0');
    TEST_ASSERT(sr_invalid == 9, "Ligne 0 convertie en index 9 (hors limites)");
}

/**
 * Test de cohérence avec le système de coordonnées
 */
void test_coordinate_consistency() {
    // Vérifier que A9 correspond bien au coin supérieur gauche (0,0)
    int col_A = col_from_letter('A');
    int row_9 = 9 - 9;  // 9 - 9 = 0
    TEST_ASSERT(col_A == 0 && row_9 == 0, "A9 -> (0,0)");

    // Vérifier que I1 correspond bien au coin inférieur droit (8,8)
    int col_I = col_from_letter('I');
    int row_1 = 9 - 1;  // 9 - 1 = 8
    TEST_ASSERT(col_I == 8 && row_1 == 8, "I1 -> (8,8)");

    // Test de symétrie
    for (char col = 'A'; col <= 'I'; col++) {
        for (char row = '1'; row <= '9'; row++) {
            int c_idx = col_from_letter(col);
            int r_idx = 9 - (row - '0');

            char msg[100];
            snprintf(msg, sizeof(msg), "%c%c -> (%d,%d) dans les limites", col, row, r_idx, c_idx);
            TEST_ASSERT(c_idx >= 0 && c_idx < 9 && r_idx >= 0 && r_idx < 9, msg);
        }
    }
}

/**
 * Fonction principale des tests
 */
int main() {
    if (logger_init("./logs/test.log", LOG_DEBUG) != 0) {
        fprintf(stderr, "Impossible d'initialiser le logger\n");
        return 1;
    }

    test_col_from_letter();
    test_cols_map();
    test_move_conversion();
    test_invalid_moves();
    test_coordinate_consistency();

    printf("[TEST][MOVE][RESULT] %d/%d\n", tests_passed, tests_passed + tests_failed);
    LOG_INFO_MSG("[TEST][MOVE][RESULT] %d/%d", tests_passed, tests_passed + tests_failed);

    // Clean up
    logger_cleanup();
}

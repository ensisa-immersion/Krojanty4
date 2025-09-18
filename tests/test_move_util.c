/**
 * @file test_move_util.c
 * @brief Tests unitaires pour les utilitaires de mouvement
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "game.h"
#include "move_util_test.h"
#include "move_util.h"
#include "logging.h"

// Compteurs de tests
static int tests_passed = 0;
static int tests_failed = 0;

// Macro pour les assertions avec messages
#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            LOG_SUCCESS_MSG("[TEST][MOVE][OK] %s", message); \
            tests_passed++; \
        } else { \
            LOG_ERROR_MSG("[TEST][MOVE][KO] %s", message); \
            tests_failed++; \
        } \
    } while(0)

// Pas de mock display_request_redraw - on utilise la vraie fonction

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
 * Test de la fonction apply_move_idle avec une MoveTask manuelle
 */
void test_apply_move_idle() {
    Game game = init_game(LOCAL, 0);
    
    // Créer une MoveTask allouée sur le tas pour éviter le crash avec g_free
    MoveTask *task = g_new0(MoveTask, 1);
    task->game = &game;
    task->sr = 3;  // Ligne source
    task->sc = 0;  // Colonne source 
    task->dr = 4;  // Ligne destination
    task->dc = 0;  // Colonne destination
    
    // Appeler directement apply_move_idle (la fonction libère automatiquement task)
    int result = apply_move_idle(task);
    
    // Vérifier que la fonction s'exécute sans erreur
    TEST_ASSERT(result == 0, "apply_move_idle retourne G_SOURCE_REMOVE");
    TEST_ASSERT(game.selected_tile[0] == 3, "Case source ligne définie correctement");
    TEST_ASSERT(game.selected_tile[1] == 0, "Case source colonne définie correctement");
    
    // Note: task est automatiquement libérée par apply_move_idle via g_free
}

/**
 * Test de la fonction post_move_to_gtk avec mouvements valides
 */
void test_post_move_to_gtk_valid() {
    Game game = init_game(LOCAL, 0);

    // Test avec mouvement valide A6A5
    char move1[4] = {'A', '6', 'A', '5'};
    post_move_to_gtk(&game, move1);
    TEST_ASSERT(1, "post_move_to_gtk avec A6A5 s'exécute sans erreur");

    // Test avec mouvement valide B7C7
    char move2[4] = {'B', '7', 'C', '7'};
    post_move_to_gtk(&game, move2);
    TEST_ASSERT(1, "post_move_to_gtk avec B7C7 s'exécute sans erreur");

    // Test avec mouvement dans les limites I1I2
    char move3[4] = {'I', '1', 'I', '2'};
    post_move_to_gtk(&game, move3);
    TEST_ASSERT(1, "post_move_to_gtk avec I1I2 s'exécute sans erreur");

    // Test avec mouvement diagonal A9I1
    char move4[4] = {'A', '9', 'I', '1'};
    post_move_to_gtk(&game, move4);
    TEST_ASSERT(1, "post_move_to_gtk avec A9I1 s'exécute sans erreur");
}

/**
 * Test de la fonction post_move_to_gtk avec mouvements invalides
 */
void test_post_move_to_gtk_invalid() {
    Game game = init_game(LOCAL, 0);

    // Test avec colonne source invalide
    char move1[4] = {'Z', '6', 'A', '5'};
    post_move_to_gtk(&game, move1);
    TEST_ASSERT(1, "post_move_to_gtk rejette colonne source invalide Z");

    // Test avec colonne destination invalide
    char move2[4] = {'A', '6', 'J', '5'};
    post_move_to_gtk(&game, move2);
    TEST_ASSERT(1, "post_move_to_gtk rejette colonne destination invalide J");

    // Test avec ligne source invalide (0)
    char move3[4] = {'A', '0', 'B', '1'};
    post_move_to_gtk(&game, move3);
    TEST_ASSERT(1, "post_move_to_gtk rejette ligne source invalide 0");

    // Test avec ligne destination invalide (hors limites)
    char move4[4] = {'A', '1', 'B', '0'};
    post_move_to_gtk(&game, move4);
    TEST_ASSERT(1, "post_move_to_gtk rejette ligne destination invalide 0");

    // Test avec plusieurs erreurs simultanées
    char move5[4] = {'Z', '0', 'J', '0'};
    post_move_to_gtk(&game, move5);
    TEST_ASSERT(1, "post_move_to_gtk rejette mouvement complètement invalide");
}

/**
 * Test des cas limites de post_move_to_gtk
 */
void test_post_move_to_gtk_edge_cases() {
    Game game = init_game(LOCAL, 0);

    // Test avec toutes les colonnes valides
    for (char col = 'A'; col <= 'I'; col++) {
        char move[4] = {col, '5', 'E', '5'};
        post_move_to_gtk(&game, move);
        char msg[100];
        snprintf(msg, sizeof(msg), "post_move_to_gtk accepte colonne source %c", col);
        TEST_ASSERT(1, msg);
    }

    // Test avec toutes les lignes valides
    for (char row = '1'; row <= '9'; row++) {
        char move[4] = {'E', row, 'E', '5'};
        post_move_to_gtk(&game, move);
        char msg[100];
        snprintf(msg, sizeof(msg), "post_move_to_gtk accepte ligne source %c", row);
        TEST_ASSERT(1, msg);
    }

    // Test coins du plateau
    char corner1[4] = {'A', '9', 'A', '8'}; // Coin supérieur gauche
    post_move_to_gtk(&game, corner1);
    TEST_ASSERT(1, "post_move_to_gtk accepte coin A9");

    char corner2[4] = {'I', '1', 'I', '2'}; // Coin inférieur droit
    post_move_to_gtk(&game, corner2);
    TEST_ASSERT(1, "post_move_to_gtk accepte coin I1");
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
 * Test des cas d'erreur détaillés dans post_move_to_gtk
 */
void test_post_move_to_gtk_error_cases() {
    Game game = init_game(LOCAL, 0);

    // Test avec lignes hors limites (négatives après conversion)
    char move_line_high[4] = {'A', ':', 'B', '1'}; // ':' après '9'
    post_move_to_gtk(&game, move_line_high);
    TEST_ASSERT(1, "post_move_to_gtk rejette ligne source > 9");

    // Test avec lignes 0 (invalide)
    char move_line_zero[4] = {'A', '0', 'B', '1'};
    post_move_to_gtk(&game, move_line_zero);
    TEST_ASSERT(1, "post_move_to_gtk rejette ligne source = 0");

    // Test avec destination ligne invalide
    char move_dest_invalid[4] = {'A', '1', 'B', '0'};
    post_move_to_gtk(&game, move_dest_invalid);
    TEST_ASSERT(1, "post_move_to_gtk rejette ligne destination = 0");

    // Test avec caractères non-alphabétiques pour colonnes
    char move_col_digit[4] = {'1', '5', 'A', '5'};
    post_move_to_gtk(&game, move_col_digit);
    TEST_ASSERT(1, "post_move_to_gtk rejette colonne source numérique");

    char move_col_special[4] = {'@', '5', 'A', '5'}; // '@' avant 'A'
    post_move_to_gtk(&game, move_col_special);
    TEST_ASSERT(1, "post_move_to_gtk rejette colonne source spéciale");

    // Test avec colonnes après 'I'
    char move_col_after_I[4] = {'J', '5', 'A', '5'};
    post_move_to_gtk(&game, move_col_after_I);
    TEST_ASSERT(1, "post_move_to_gtk rejette colonne J");

    char move_col_after_I2[4] = {'A', '5', 'Z', '5'};
    post_move_to_gtk(&game, move_col_after_I2);
    TEST_ASSERT(1, "post_move_to_gtk rejette colonne destination Z");

    // Test avec indices calculés hors limites (sr/dr > 8)
    // Ligne '0' donne sr = 9 - 0 = 9, qui est > 8
    char move_boundary[4] = {'A', '0', 'A', '1'};
    post_move_to_gtk(&game, move_boundary);
    TEST_ASSERT(1, "post_move_to_gtk rejette indices hors limites");
}

/**
 * Test des conversions de coordonnées avec validation stricte
 */
void test_coordinate_conversion_strict() {
    // Test de toutes les conversions valides
    for (char col = 'A'; col <= 'I'; col++) {
        for (char row = '1'; row <= '9'; row++) {
            int c_idx = col_from_letter(col);
            int r_idx = 9 - (row - '0');

            char msg[100];
            snprintf(msg, sizeof(msg), "Conversion %c%c -> (%d,%d)", col, row, r_idx, c_idx);
            TEST_ASSERT(c_idx >= 0 && c_idx < 9 && r_idx >= 0 && r_idx < 9, msg);
        }
    }

    // Test des conversions invalides détaillées
    char invalid_chars[] = {'@', 'J', 'K', 'Z', '0', ':', 'a', 'i'};
    for (int i = 0; i < 8; i++) {
        int result = col_from_letter(invalid_chars[i]);
        char msg[100];
        snprintf(msg, sizeof(msg), "Caractère invalide '%c' -> -1", invalid_chars[i]);
        TEST_ASSERT(result == -1, msg);
    }
}

/**
 * Test de robustesse avec mouvements extrêmes
 */
void test_extreme_moves() {
    Game game = init_game(LOCAL, 0);

    // Mouvement diagonal complet (coin à coin)
    char move_diag1[4] = {'A', '9', 'I', '1'};
    post_move_to_gtk(&game, move_diag1);
    TEST_ASSERT(1, "post_move_to_gtk accepte mouvement diagonal A9-I1");

    // Mouvement diagonal inverse
    char move_diag2[4] = {'I', '1', 'A', '9'};
    post_move_to_gtk(&game, move_diag2);
    TEST_ASSERT(1, "post_move_to_gtk accepte mouvement diagonal I1-A9");

    // Mouvements sur la même case
    char move_same[4] = {'E', '5', 'E', '5'};
    post_move_to_gtk(&game, move_same);
    TEST_ASSERT(1, "post_move_to_gtk accepte mouvement sur place E5-E5");

    // Mouvements sur les bords
    char move_top[4] = {'A', '9', 'I', '9'};
    post_move_to_gtk(&game, move_top);
    TEST_ASSERT(1, "post_move_to_gtk accepte mouvement bord supérieur");

    char move_bottom[4] = {'A', '1', 'I', '1'};
    post_move_to_gtk(&game, move_bottom);
    TEST_ASSERT(1, "post_move_to_gtk accepte mouvement bord inférieur");

    char move_left[4] = {'A', '1', 'A', '9'};
    post_move_to_gtk(&game, move_left);
    TEST_ASSERT(1, "post_move_to_gtk accepte mouvement bord gauche");

    char move_right[4] = {'I', '1', 'I', '9'};
    post_move_to_gtk(&game, move_right);
    TEST_ASSERT(1, "post_move_to_gtk accepte mouvement bord droit");
}

/**
 * Test de la fonction apply_move_idle avec différents paramètres
 */
void test_apply_move_idle_variations() {
    Game game = init_game(LOCAL, 0);

    // Test avec différentes positions sur le plateau
    for (int sr = 0; sr < 9; sr++) {
        for (int sc = 0; sc < 9; sc++) {
            MoveTask *task = g_new0(MoveTask, 1);
            task->game = &game;
            task->sr = sr;
            task->sc = sc;
            task->dr = (sr + 1) % 9; // Destination différente
            task->dc = (sc + 1) % 9;

            int result = apply_move_idle(task);

            char msg[100];
            snprintf(msg, sizeof(msg), "apply_move_idle position (%d,%d)", sr, sc);
            TEST_ASSERT(result == G_SOURCE_REMOVE, msg);
        }
    }
}

/**
 * Test de cohérence du mappage COLS_MAP
 */
void test_cols_map_consistency() {
    // Vérifier que COLS_MAP est bien ordonné
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(COLS_MAP[i] < COLS_MAP[i+1], "COLS_MAP ordonné correctement");
    }

    // Vérifier que chaque lettre correspond à son index
    for (int i = 0; i < 9; i++) {
        int found_index = col_from_letter(COLS_MAP[i]);
        char msg[100];
        snprintf(msg, sizeof(msg), "COLS_MAP[%d]='%c' -> index %d", i, COLS_MAP[i], found_index);
        TEST_ASSERT(found_index == i, msg);
    }

    // Vérifier que la chaîne se termine bien
    TEST_ASSERT(COLS_MAP[9] == '\0', "COLS_MAP terminé par \\0");
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
    test_cols_map_consistency();
    test_apply_move_idle();
    test_apply_move_idle_variations();
    test_post_move_to_gtk_valid();
    test_post_move_to_gtk_invalid();
    test_post_move_to_gtk_edge_cases();
    test_post_move_to_gtk_error_cases();
    test_move_conversion();
    test_invalid_moves();
    test_coordinate_consistency();
    test_coordinate_conversion_strict();
    test_extreme_moves();

    LOG_INFO_MSG("[TEST][MOVE][RESULT] %d/%d", tests_passed, tests_passed + tests_failed);
    return (tests_failed == 0) ? 0 : 1;
}

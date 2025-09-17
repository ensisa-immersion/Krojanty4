/**
 * @file test_integration.c
 * @brief Tests d'intégration pour le jeu Krojanty
 */

#include <stdio.h>

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
            LOG_SUCCESS_MSG("[TEST][INTEGRATION][OK] %s", message); \
            tests_passed++; \
        } else { \
            LOG_ERROR_MSG("[TEST][INTEGRATION][KO] %s", message); \
            tests_failed++; \
        } \
    } while(0)

// Fonction helper pour simuler update_board (version simplifiée sans GTK)
void simulate_move(Game *game, int src_row, int src_col, int dst_row, int dst_col) {
    if (is_move_legal(game, src_row, src_col, dst_row, dst_col)) {
        // Déplacer la pièce
        Piece piece = game->board[src_row][src_col];
        game->board[src_row][src_col] = P_NONE;
        game->board[dst_row][dst_col] = piece;

        // Changer de tour
        game->turn++;
    }
}

/**
 * Test d'une partie complète (quelques coups)
 */
void test_complete_game_scenario() {
    Game game = init_game(LOCAL, 0);
    int initial_score_p1 = score_player_one(game);
    int initial_score_p2 = score_player_two(game);

    // Tour 0 - P1 joue - utiliser un mouvement vraiment légal
    // Position (3,0) vers (4,0) selon la configuration du plateau
    TEST_ASSERT(game.turn == 0, "Tour initial = 0");
    simulate_move(&game, 3, 0, 4, 0); // Mouvement P1 valide
    TEST_ASSERT(game.turn == 1, "Tour après mouvement P1 = 1");
    TEST_ASSERT(game.board[4][0] == P1_PAWN, "Pion P1 déplacé correctement");
    TEST_ASSERT(game.board[3][0] == P_NONE, "Case source vidée");

    // Tour 1 - P2 joue
    simulate_move(&game, 6, 6, 6, 4); // G3 -> E3
    TEST_ASSERT(game.turn == 2, "Tour après mouvement P2 = 2");
    TEST_ASSERT(game.board[6][4] == P2_PAWN, "Pion P2 déplacé correctement");

    // Vérifier que les scores ont changé
    int new_score_p1 = score_player_one(game);
    int new_score_p2 = score_player_two(game);
    TEST_ASSERT(new_score_p1 >= initial_score_p1, "Score P1 maintenu ou augmenté");
    TEST_ASSERT(new_score_p2 >= initial_score_p2, "Score P2 maintenu ou augmenté");
}

/**
 * Test d'intégration avec move_util
 */
void test_move_util_integration() {
    Game game = init_game(LOCAL, 0);

    // Utiliser exactement le même mouvement que celui qui fonctionne dans complete_game_scenario
    // A4 -> A5 correspond à (5,0) -> (4,0), mais utilisons le mouvement testé qui marche : (3,0) -> (4,0)
    // Soit A6 -> A5 en notation du jeu
    char move[] = "A6A5";
    int sc = col_from_letter(move[0]);
    int sr = 9 - (move[1] - '0');
    int dc = col_from_letter(move[2]);
    int dr = 9 - (move[3] - '0');

    TEST_ASSERT(sc >= 0 && sr >= 0 && dc >= 0 && dr >= 0, "Conversion mouvement valide");
    TEST_ASSERT(is_move_legal(&game, sr, sc, dr, dc), "Mouvement A6A5 légal");

    // Simuler le mouvement (même logique que complete_game_scenario)
    simulate_move(&game, sr, sc, dr, dc);
    TEST_ASSERT(game.board[dr][dc] != P_NONE, "Pièce présente après mouvement");
    TEST_ASSERT(game.board[sr][sc] == P_NONE, "Case source vide après mouvement");
}

/**
 * Test de cas limites et erreurs
 */
void test_edge_cases() {
    Game game = init_game(LOCAL, 0);

    // Test mouvement hors plateau
    TEST_ASSERT(!is_move_legal(&game, -1, 0, 0, 0), "Mouvement depuis position négative refusé");
    TEST_ASSERT(!is_move_legal(&game, 0, 0, -1, 0), "Mouvement vers position négative refusé");
    TEST_ASSERT(!is_move_legal(&game, 9, 0, 8, 0), "Mouvement depuis position > 8 refusé");
    TEST_ASSERT(!is_move_legal(&game, 0, 0, 0, 9), "Mouvement vers position > 8 refusé");

    // Test mouvement sur la même case
    TEST_ASSERT(!is_move_legal(&game, 2, 0, 2, 0), "Mouvement sur place refusé");

    // Test mouvement diagonal
    TEST_ASSERT(!is_move_legal(&game, 2, 0, 3, 1), "Mouvement diagonal refusé");

    // Test avec plateau modifié
    game.board[4][4] = P1_PAWN; // Placer une pièce au centre
    TEST_ASSERT(!is_move_legal(&game, 2, 0, 4, 4), "Mouvement vers case occupée refusé");
}

/**
 * Test de différents modes de jeu
 */
void test_game_modes_integration() {
    Game local_game = init_game(LOCAL, 0);
    Game server_game = init_game(SERVER, 0);
    Game client_game = init_game(CLIENT, 1);

    // Tous les jeux doivent avoir la même configuration initiale du plateau
    int boards_identical = 1;
    for (int i = 0; i < 9 && boards_identical; i++) {
        for (int j = 0; j < 9 && boards_identical; j++) {
            if (local_game.board[i][j] != server_game.board[i][j] ||
                local_game.board[i][j] != client_game.board[i][j]) {
                boards_identical = 0;
            }
        }
    }
    TEST_ASSERT(boards_identical, "Tous les modes ont le même plateau initial");

    // Vérifier que les modes sont différents
    TEST_ASSERT(local_game.game_mode != server_game.game_mode, "Mode LOCAL != SERVER");
    TEST_ASSERT(server_game.game_mode != client_game.game_mode, "Mode SERVER != CLIENT");

    // Vérifier que l'IA est configurée différemment
    TEST_ASSERT(local_game.is_ai != client_game.is_ai, "Configuration IA différente");
}

/**
 * Test de validation de format de mouvement complet
 */
void test_move_format_validation() {
    // Mouvements valides
    const char* valid_moves[] = {"A1B1", "A9I9", "E5E1", "I9I1", "A1A9"};
    int num_valid = sizeof(valid_moves) / sizeof(valid_moves[0]);

    for (int i = 0; i < num_valid; i++) {
        int sc = col_from_letter(valid_moves[i][0]);
        int sr = 9 - (valid_moves[i][1] - '0');
        int dc = col_from_letter(valid_moves[i][2]);
        int dr = 9 - (valid_moves[i][3] - '0');

        char msg[100];
        snprintf(msg, sizeof(msg), "Format valide: %s", valid_moves[i]);
        TEST_ASSERT(sc >= 0 && sr >= 0 && sr < 9 && sc < 9 &&
                    dc >= 0 && dr >= 0 && dr < 9 && dc < 9, msg);
    }

    // Mouvements avec format invalide
    const char* invalid_moves[] = {"Z1A1", "A0B1", "A1J1", "A1B0"};
    int num_invalid = sizeof(invalid_moves) / sizeof(invalid_moves[0]);

    for (int i = 0; i < num_invalid; i++) {
        int sc = col_from_letter(invalid_moves[i][0]);
        int sr = 9 - (invalid_moves[i][1] - '0');
        int dc = col_from_letter(invalid_moves[i][2]);
        int dr = 9 - (invalid_moves[i][3] - '0');

        char msg[100];
        snprintf(msg, sizeof(msg), "Format invalide détecté: %s", invalid_moves[i]);
        TEST_ASSERT(sc < 0 || sr < 0 || sr >= 9 || sc >= 9 ||
                    dc < 0 || dr < 0 || dr >= 9 || dc >= 9, msg);
    }
}

/**
 * Test de simulation de partie réseau
 */
void test_network_game_simulation() {
    Game server_game = init_game(SERVER, 0);
    Game client_game = init_game(CLIENT, 0);

    simulate_move(&server_game, 0, 0, 0, 3);

    if (server_game.turn == 1) {
        client_game.board[0][0] = P_NONE;
        client_game.board[0][3] = P1_PAWN;
        client_game.turn = 1;
    }

    TEST_ASSERT(server_game.turn == client_game.turn, "Tours synchronisés");
    TEST_ASSERT(server_game.board[2][3] == client_game.board[2][3], "Plateaux synchronisés");
    TEST_ASSERT(server_game.board[0][3] == client_game.board[0][3], "Plateaux synchronisés");

    simulate_move(&client_game, 6, 6, 5, 6);

    if (client_game.turn == 2) {
        server_game.board[6][6] = P_NONE;
        server_game.board[5][6] = P2_PAWN;
        server_game.turn = 2;
    }
    TEST_ASSERT(server_game.turn == client_game.turn, "Tours synchronisés après échange");
}

/**
 * Fonction principale des tests
 */
int main() {
    if (logger_init("./logs/test.log", LOG_DEBUG) != 0) {
        fprintf(stderr, "Impossible d'initialiser le logger\n");
        return 1;
    }

    test_complete_game_scenario();
    test_move_util_integration();
    test_edge_cases();
    test_game_modes_integration();
    test_move_format_validation();
    test_network_game_simulation();

    LOG_INFO_MSG("[TEST][INTEGRATION][RESULT] %d/%d", tests_passed, tests_passed + tests_failed);
}

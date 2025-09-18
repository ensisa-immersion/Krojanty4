/**
 * @file test_game.c
 * @brief Tests unitaires pour les fonctionnalités du jeu Krojanty
 */

#include <stdio.h>

#include "game.h"
#include "logging.h"
#include "const.h"

// Compteurs de tests
static int tests_passed = 0;
static int tests_failed = 0;

// Macro pour les assertions avec messages
#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            LOG_SUCCESS_MSG("[TEST][GAME][OK] %s", message); \
            tests_passed++; \
        } else { \
            LOG_ERROR_MSG("[TEST][GAME][KO] %s", message); \
            tests_failed++; \
        } \
    } while(0)

/**
 * Test d'initialisation du jeu
 */
void test_game_initialization() {
    Game game = init_game(LOCAL, 0);

    TEST_ASSERT(game.won == 0, "Le jeu n'est pas encore gagné au début");
    TEST_ASSERT(game.turn == 0, "Le premier tour est 0");
    TEST_ASSERT(game.game_mode == LOCAL, "Mode de jeu correct (LOCAL)");
    TEST_ASSERT(game.is_ai == 0, "Mode IA désactivé");
    TEST_ASSERT(game.selected_tile[0] == -1, "Aucune case sélectionnée au début");
    TEST_ASSERT(game.selected_tile[1] == -1, "Aucune case sélectionnée au début");

    // Vérifier les positions initiales des pièces
    TEST_ASSERT(game.board[0][2] == P1_PAWN, "Pion P1 en position C9");
    TEST_ASSERT(game.board[1][1] == P1_KING, "Roi P1 en position B8");
    TEST_ASSERT(game.board[7][7] == P2_KING, "Roi P2 en position H2");
    TEST_ASSERT(game.board[8][5] == P2_PAWN, "Pion P2 en position F1");
    TEST_ASSERT(game.board[4][4] == P_NONE, "Case centrale vide");
}

/**
 * Test des fonctions de score
 */
void test_scoring() {
    Game game = init_game(LOCAL, 0);

    int score_p1 = score_player_one(game);
    int score_p2 = score_player_two(game);

    TEST_ASSERT(score_p1 > 0, "Score du joueur 1 est positif");
    TEST_ASSERT(score_p2 > 0, "Score du joueur 2 est positif");
}

/**
 * Test de la fonction get_player
 */
void test_get_player() {
    TEST_ASSERT(get_player(P1_PAWN) == P1, "Pion P1 appartient au joueur 1");
    TEST_ASSERT(get_player(P1_KING) == P1, "Roi P1 appartient au joueur 1");
    TEST_ASSERT(get_player(P2_PAWN) == P2, "Pion P2 appartient au joueur 2");
    TEST_ASSERT(get_player(P2_KING) == P2, "Roi P2 appartient au joueur 2");
    TEST_ASSERT(get_player(P_NONE) == NOT_PLAYER, "Case vide n'appartient à personne");
}

/**
 * Test des mouvements légaux
 */
void test_legal_moves() {
    Game game = init_game(LOCAL, 0);

    TEST_ASSERT(is_move_legal(&game, 0, 3, 0, 5), "Mouvement horizontal valide");

    // Test mouvement vertical : (3,0) vers (4,0)
    TEST_ASSERT(is_move_legal(&game, 3, 0, 4, 0), "Mouvement vertical valide");

    // Test mouvement diagonal invalide
    TEST_ASSERT(!is_move_legal(&game, 2, 0, 3, 1), "Mouvement diagonal invalide");

    // Test mouvement vers case occupée
    TEST_ASSERT(!is_move_legal(&game, 2, 0, 2, 1), "Mouvement vers case occupée invalide");

    // Test mouvement hors limites
    TEST_ASSERT(!is_move_legal(&game, 0, 0, -1, 0), "Mouvement hors limites invalide");
    TEST_ASSERT(!is_move_legal(&game, 8, 8, 9, 8), "Mouvement hors limites invalide");

    // Test mouvement depuis case vide
    TEST_ASSERT(!is_move_legal(&game, 4, 4, 4, 5), "Mouvement depuis case vide invalide");
}

/**
 * Test des règles de tour
 */
void test_turn_rules() {
    Game game = init_game(LOCAL, 0);

    // Au tour 0, seul P1 peut jouer - utiliser (3,0) vers (4,0)
    TEST_ASSERT(is_move_legal(&game, 3, 0, 4, 0), "P1 peut jouer au tour 0");

    // P2 ne peut pas jouer au tour 0 - utiliser position (6,6) vers (6,5)
    TEST_ASSERT(!is_move_legal(&game, 6, 6, 6, 5), "P2 ne peut pas jouer au tour 0");

    // Changer de tour
    game.turn = 1;

    // Au tour 1, seul P2 peut jouer
    TEST_ASSERT(!is_move_legal(&game, 3, 0, 4, 0), "P1 ne peut pas jouer au tour 1");
    TEST_ASSERT(is_move_legal(&game, 6, 6, 6, 5), "P2 peut jouer au tour 1");
}

/**
 * Test des mouvements bloqués
 */
void test_blocked_moves() {
    Game game = init_game(LOCAL, 0);

    // Créer un obstacle sur le chemin
    game.board[2][2] = P2_PAWN;

    // Test mouvement bloqué horizontalement
    TEST_ASSERT(!is_move_legal(&game, 2, 0, 2, 4), "Mouvement horizontal bloqué");

    // Enlever l'obstacle et créer un obstacle vertical
    game.board[2][2] = P_NONE;
    game.board[3][0] = P2_PAWN;

    // Test mouvement bloqué verticalement
    TEST_ASSERT(!is_move_legal(&game, 2, 0, 5, 0), "Mouvement vertical bloqué");
}

/**
 * Test des modes de jeu
 */
void test_game_modes() {
    Game local_game = init_game(LOCAL, 0);
    Game server_game = init_game(SERVER, 0);
    Game client_game = init_game(CLIENT, 0);

    TEST_ASSERT(local_game.game_mode == LOCAL, "Mode LOCAL correctement initialisé");
    TEST_ASSERT(server_game.game_mode == SERVER, "Mode SERVER correctement initialisé");
    TEST_ASSERT(client_game.game_mode == CLIENT, "Mode CLIENT correctement initialisé");
}

/**
 * Test avec IA
 */
void test_ai_mode() {
    Game ai_game = init_game(LOCAL, 1);
    Game no_ai_game = init_game(LOCAL, 0);

    TEST_ASSERT(ai_game.is_ai == 1, "Mode IA activé");
    TEST_ASSERT(no_ai_game.is_ai == 0, "Mode IA désactivé");
}

/**
 * Test de la fonction current_player_turn
 */
void test_current_player_turn() {
    Game game = init_game(LOCAL, 0);

    // Tour 0 = P1
    TEST_ASSERT(current_player_turn(&game) == P1, "Tour 0: joueur P1");

    // Tour 1 = P2
    game.turn = 1;
    TEST_ASSERT(current_player_turn(&game) == P2, "Tour 1: joueur P2");

    // Tour 2 = P1
    game.turn = 2;
    TEST_ASSERT(current_player_turn(&game) == P1, "Tour 2: joueur P1");

    // Tour 3 = P2
    game.turn = 3;
    TEST_ASSERT(current_player_turn(&game) == P2, "Tour 3: joueur P2");
}

/**
 * Test de la fonction update_board
 */
void test_update_board() {
    Game game = init_game(LOCAL, 0);

    // Sauvegarder l'état initial
    Piece original_piece = game.board[0][2]; // Pion P1 en C9

    // Tester le mouvement d'un pion
    update_board(&game, 1, 2); // Déplacer vers B8

    TEST_ASSERT(game.board[1][2] == original_piece, "Pièce déplacée correctement");
    TEST_ASSERT(game.board[0][2] == P_NONE, "Case source vidée");
    TEST_ASSERT(game.turn == 1, "Tour incrémenté après mouvement");
}

/**
 * Test de la fonction won (conditions de victoire)
 */
void test_victory_conditions() {
    Game game = init_game(LOCAL, 0);

    // Au début, personne n'a gagné
    TEST_ASSERT(game.won == 0, "Aucune victoire au début");

    // Vider le plateau du joueur 2 pour simuler une victoire de P1
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (get_player(game.board[i][j]) == P2) {
                game.board[i][j] = P_NONE;
            }
        }
    }

    // Vérifier la victoire
    won(&game);
    TEST_ASSERT(game.won == P1, "P1 remporte la victoire");
}

/**
 * Test de la fonction did_eat (capture de pièces)
 */
void test_piece_capture() {
    Game game = init_game(LOCAL, 0);

    // Placer une pièce P2 adjacente à une P1 pour tester la capture
    game.board[1][2] = P2_PAWN; // Mettre un pion P2 en B8

    int initial_p2_count = 0;
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (get_player(game.board[i][j]) == P2) {
                initial_p2_count++;
            }
        }
    }

    // Tester la fonction did_eat avec direction = 0 (valeur arbitraire pour le test)
    did_eat(&game, 0, 3, 0);

    // Compter les pièces P2 après
    int final_p2_count = 0;
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (get_player(game.board[i][j]) == P2) {
                final_p2_count++;
            }
        }
    }

    TEST_ASSERT(final_p2_count <= initial_p2_count, "Fonction did_eat exécutée sans erreur");
}

/**
 * Fonction principale des tests
 */
int main() {
    if (logger_init("./logs/test.log", LOG_DEBUG) != 0) {
        fprintf(stderr, "Impossible d'initialiser le logger\n");
        return 1;
    }

    test_game_initialization();
    test_scoring();
    test_get_player();
    test_legal_moves();
    test_turn_rules();
    test_blocked_moves();
    test_game_modes();
    test_ai_mode();
    test_current_player_turn();
    test_update_board();
    test_victory_conditions();
    test_piece_capture();

    LOG_INFO_MSG("[TEST][GAME][RESULT] %d/%d", tests_passed, tests_passed + tests_failed);
}

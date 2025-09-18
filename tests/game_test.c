/**
 * @file game_test.c
 * @brief Tests unitaires pour le module de jeu
 * 
 * Ce fichier contient tous les tests unitaires pour les fonctions du jeu, incluant :
 * - L'initialisation du jeu
 * - Le calcul des scores des joueurs
 * - La validation des mouvements légaux
 * - La gestion des pièces et des joueurs
 * - Les fonctions utilitaires du jeu
 * 
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 */


#include "game.h"
#include "const.h"

// Initialise le jeu pour commencer une partie
Game init_game(GameMode mode, int artificial_intelligence) {
    (void)mode;
    (void)artificial_intelligence;

    Game game;
    
    game.won = 0;
    game.turn = 0;

    // Initialiser avec le plateau de départ centralisé
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            game.board[i][j] = STARTING_BOARD[i][j];
        }
    }

    game.selected_tile[0] = -1;
    game.selected_tile[1] = -1;
    game.game_mode = mode;
    game.is_ai = artificial_intelligence ? 1 : 0;

    return game;
}

// Retourne le score du joueur 1
int score_player_one(Game game) {
    int player_one_score = 0;
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (game.board[i][j] == P1_VISITED) player_one_score++;
            if (get_player(game.board[i][j]) == P1) player_one_score += 2;
        }
    }
    return player_one_score;
}

// Retourne le score du joueur 2
int score_player_two(Game game) {
    int player_two_score = 0;
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (game.board[i][j] == P2_VISITED) player_two_score++;
            if (get_player(game.board[i][j]) == P2) player_two_score += 2;
        }
    }
    return player_two_score;
}

/**
 * Vérifie si un déplacement est légal selon les règles du jeu.
 * - La destination est dans les limites du plateau
 * - La destination n'est pas occupée par une pièce
 * - Le déplacement est en ligne droite (horizontal ou vertical)
 * - Le chemin entre la source et la destination n'est pas bloqué
 */
int is_move_legal(Game *game, int src_row, int src_col, int dst_row, int dst_col) {
    if (src_row < 0 || src_row >= GRID_SIZE || src_col < 0 || src_col >= GRID_SIZE) return 0;
    if (dst_row < 0 || dst_row >= GRID_SIZE || dst_col < 0 || dst_col >= GRID_SIZE) return 0;

    // Doit y avoir une pièce à la source
    if (game->board[src_row][src_col] == P_NONE) return 0;

    // Déplacement en ligne droite uniquement
    if (src_row != dst_row && src_col != dst_col) return 0;

    // Destination doit être vide
    if (game->board[dst_row][dst_col] != P_NONE) return 0;

    // Vérifier que c'est le bon joueur qui joue
    if ((game->turn % 2 == 0) &&
        (game->board[src_row][src_col] == P2_PAWN || game->board[src_row][src_col] == P2_KING)) return 0;
    if ((game->turn % 2 == 1) &&
        (game->board[src_row][src_col] == P1_PAWN || game->board[src_row][src_col] == P1_KING)) return 0;

    // Vérification du chemin horizontal
    if (src_row == dst_row) {
        int step = (dst_col > src_col) ? 1 : -1;
        for (int c = src_col + step; c != dst_col; c += step) {
            if (game->board[src_row][c] != P_NONE) return 0;
        }
    }

    // Vérification du chemin vertical
    if (src_col == dst_col) {
        int step = (dst_row > src_row) ? 1 : -1;
        for (int r = src_row + step; r != dst_row; r += step) {
            if (game->board[r][src_col] != P_NONE) return 0;
        }
    }

    return 1;
}

// Retourne le joueur associé à une pièce
Player get_player(Piece piece) {
    if (piece == P1_PAWN || piece == P1_KING) return P1;
    if (piece == P2_PAWN || piece == P2_KING) return P2;
    return NOT_PLAYER;
}

// Version simplifiée pour les tests
#ifdef TEST_BUILD
void update_board(Game* game, int dst_col, int dst_row) {
    (void)game;
    (void)dst_col;
    (void)dst_row;
}
#endif

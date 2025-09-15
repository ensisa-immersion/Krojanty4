#include "game.h"
#include "const.h"

// Initialize the game to play
Game init_game(GameMode mode, int artificial_intelligence) {
    (void)mode;                    // évite warnings si pas utilisé partout
    (void)artificial_intelligence; // idem

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

    // Valeurs cohérentes pour éviter des états non-initialisés
    game.game_mode = mode;
    game.is_ai = artificial_intelligence ? 1 : 0;

    return game;
}


// Return player 1's score
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

// Returns player 2's score
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
 * C'est à dire :
 * - La destination est dans les limites du plateau.
 * - La destination n'est pas occupée par une pièce du même joueur.
 * - Le déplacement est en ligne droite (horizontal ou vertical).
 * - Le chemin entre la source et la destination n'est pas bloqué par d'autres pièces
 * @param game Pointeur vers la structure de jeu contenant l'état actuel du plateau et le tour.
 * @param src_row Ligne source (0-8).
 * @param src_col Colonne source (0-8).
 * @param dst_row Ligne destination (0-8).
 * @param dst_col Colonne destination (0-8).
 * @return 1 si le déplacement est légal, sinon 0.
 */
int is_move_legal(Game *game, int src_row, int src_col, int dst_row, int dst_col) {
    if (src_row < 0 || src_row >= GRID_SIZE || src_col < 0 || src_col >= GRID_SIZE) return 0;
    if (dst_row < 0 || dst_row >= GRID_SIZE || dst_col < 0 || dst_col >= GRID_SIZE) return 0;

    // Must have a piece at source
    if (game->board[src_row][src_col] == P_NONE) return 0;

    // Must move straight (horizontal or vertical)
    if (src_row != dst_row && src_col != dst_col) return 0;

    // Destination must be empty
    if (game->board[dst_row][dst_col] != P_NONE) return 0;

    // Check turn ownership
    if ((game->turn % 2 == 0) &&
        (game->board[src_row][src_col] == P2_PAWN || game->board[src_row][src_col] == P2_KING)) return 0;
    if ((game->turn % 2 == 1) &&
        (game->board[src_row][src_col] == P1_PAWN || game->board[src_row][src_col] == P1_KING)) return 0;

    // Horizontal path check
    if (src_row == dst_row) {
        int step = (dst_col > src_col) ? 1 : -1;
        for (int c = src_col + step; c != dst_col; c += step) {
            if (game->board[src_row][c] != P_NONE) return 0; // blocked
        }
    }

    // Vertical path check
    if (src_col == dst_col) {
        int step = (dst_row > src_row) ? 1 : -1;
        for (int r = src_row + step; r != dst_row; r += step) {
            if (game->board[r][src_col] != P_NONE) return 0; // blocked
        }
    }

    return 1; // Move is legal
}


// Returns player based on tile
Player get_player(Piece piece) {
    if (piece == P1_PAWN || piece == P1_KING) return P1;
    if (piece == P2_PAWN || piece == P2_KING) return P2;
    return NOT_PLAYER;
}

// Fonctions vides pour les tests (remplacent les dépendances display.h)
#ifdef TEST_BUILD
void update_board(Game* game, int dst_col, int dst_row) {
    // Version simplifiée pour les tests
    (void)game;
    (void)dst_col;
    (void)dst_row;
}
#endif

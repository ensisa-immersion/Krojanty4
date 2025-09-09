#include <math.h>
#include "../include/game.h"
#include "../include/display.h"

// Initialize the game to play
Game init_game(void) {
    Game game;
    // Read enum in game.h to understand what number is which piece
    Piece starting_board[9][9] = { {0, 0, 1, 1, 0, 0, 0, 0, 0},
                                   {0, 3, 1, 1, 0, 0, 0, 0, 0},
                                   {1, 1, 1, 0, 0, 0, 0, 0, 0},
                                   {1, 1, 0, 0, 0, 0, 0, 0, 0},
                                   {0, 0, 0, 0, 0, 0, 0, 0, 0},
                                   {0, 0, 0, 0, 0, 0, 0, 2, 2},
                                   {0, 0, 0, 0, 0, 0, 2, 2, 2},
                                   {0, 0, 0, 0, 0, 2, 2, 4, 0},
                                   {0, 0, 0, 0, 0, 2, 2, 0, 0},
                                };
    game.won = 0;
    game.turn = 0;

    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            game.last_visited[i][j] = starting_board[i][j];
            game.board[i][j] = starting_board[i][j];
        }
    }

    game.selected_tile[0] = -1;
    game.selected_tile[1] = -1;

    return game;
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

typedef enum {
    DIR_NONE = 0,
    DIR_TOP,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_DOWN
} Direction;


// Helper function to assure pawns are eating eachother
void did_eat(Game* game, int row, int col, Direction sprint_direction) {
    Piece opponent = (game->turn % 2 == 0) ? P2_PAWN : P1_PAWN;
    Piece player = (game->turn % 2 == 1) ? P2_PAWN : P1_PAWN;

    Piece top = (row - 1 >= 0)? game->board[row - 1][col] : P_NONE;
    Piece left = (col - 1 >= 0)? game->board[row][col - 1] : P_NONE;
    Piece right = (col + 1 <= 8)? game->board[row][col + 1] : P_NONE;
    Piece down = (row + 1 <= 8)? game->board[row + 1][col] : P_NONE;


    // Eats if sprint towards opponent without one behind him defending or when sandwiched
    if (top == opponent) {
        if ( ((row - 2 < 0 || game->board[row - 2][col] != opponent) && sprint_direction == DIR_TOP ) ||
              (game->board[row - 2][col] == player) )  {
            game->board[row - 1][col] = P_NONE;
            game->last_visited[row - 1][col] = P_NONE;
        }
    } else if (left == opponent) {
        if ( ((col - 2 < 0 || game->board[row][col - 2] != opponent) && sprint_direction == DIR_LEFT ) ||
              (game->board[row][col - 2] == player) ) {
            game->board[row][col - 1] = P_NONE;
            game->last_visited[row][col - 1] = P_NONE;
        }
    } else if (right == opponent) {
        if ( ((col + 2 > 8 || game->board[row][col + 2] != opponent) && sprint_direction == DIR_RIGHT ) ||
              (game->board[row][col + 2] == player) ) {
            game->board[row][col + 1] = P_NONE;
            game->last_visited[row][col + 1] = P_NONE;
        }
    } else if (down == opponent) {
        if ( ((row + 2 > 8 || game->board[row + 2][col] != opponent) && sprint_direction == DIR_DOWN ) ||
              (game->board[row + 2][col] == player) ) {
            game->board[row + 1][col] = P_NONE;
            game->last_visited[row + 1][col] = P_NONE;
        }
    }
}


// Changes pieces placing around the board
void update_board(Game *game, int dst_row, int dst_col) {
    int src_row = game->selected_tile[0];
    int src_col = game->selected_tile[1];

    // Nothing selected so do nothing
    if (src_row < 0 || src_col < 0) return;

    if (is_move_legal(game, src_row, src_col, dst_row, dst_col)) {
        // Move piece
        game->board[dst_row][dst_col] = game->board[src_row][src_col];
        game->board[src_row][src_col] = P_NONE;

        Piece last_piece = game->board[dst_row][dst_col];
        game->last_visited[dst_row][dst_col] = last_piece;

        // Check if someone was eaten
        Direction direction;
        if (dst_row != src_row) {
            if (dst_row > src_row) {
                direction = DIR_DOWN;
            } else {
                direction = DIR_TOP;
            }
        } else if (dst_col != dst_row) {
            if (dst_col > src_col) {
                direction = DIR_RIGHT;
            } else {
                direction = DIR_LEFT;
            }
        }
        did_eat(game, dst_row, dst_col, direction);

        // Handle wins
        // int has_won = won(game);
        // if ()

        // Advance turn
        game->turn++;

        // Reset selection
        game->selected_tile[0] = -1;
        game->selected_tile[1] = -1;
    }
}


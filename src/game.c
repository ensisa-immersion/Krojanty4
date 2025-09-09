#include "../include/game.h"
#include <math.h>

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


// Helper function that determines if a move is legal
int is_move_legal(Game *game, int src_row, int src_col, int dst_row, int dst_col) {
    if (game->board[src_row][src_col] == P_NONE) return 0;
    if (src_row - dst_row != 0 && src_col - dst_col !=0) return 0;
    if (game->board[dst_row][dst_col] != P_NONE) return 0;

    int delta_x = (src_col - dst_col >= 0) ? src_col - dst_col : dst_col - src_col ;
    int delta_y = (src_row - dst_row >= 0) ? src_row - dst_row : dst_row - src_row ;
    /*if (delta_x != 0) {
        for (int )
    } else {

    }
    */

    return 1;
}


// Changes pieces placing around the board
void update_board(Game *game, int dst_row, int dst_col) {
    Piece temp = game->board[dst_row][dst_col];
    int selected_row = game->selected_tile[0];
    int selected_col = game->selected_tile[1];

    game->board[dst_row][dst_col] = game->board[selected_row][selected_col];
    game->board[selected_row][selected_col] = temp;
}
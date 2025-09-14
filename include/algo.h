#ifndef ALGO_H_INCLUDED
#define ALGO_H_INCLUDED

#include "game.h"

typedef struct {
    int src_row;
    int src_col;
    int dst_row;
    int dst_col;
    int score;
} Move;

typedef struct {
    Move s_move;
    int score;
} ScoredMove;

// Essential functions that describe game state
int utility(Game * game, Player player);
int all_possible_moves(Game * game, Move * move_list, Player player);
int all_possible_moves_ordered(Game *game, Move * move_list, Player player);

// AI computing functions
int minimax_alpha_beta(Game * game, int depth, int maximizing, int alpha, int beta, Player initial_player);
Move minimax_best_move(Game * game, int depth);

// API for game.c and main.c
void client_first_move(Game * game);
void ai_next_move(Game* game);

#endif // ALGO_H_INCLUDED

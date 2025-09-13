#include <stdlib.h>
#include <stdio.h>
#include "game.h"
#include "algo.h"

#define DEPTH 3

//Helper function that updates board with move
void update_with_move(Game * game, Move move) {
    game->selected_tile[0] = move.src_row;
    game->selected_tile[1] = move.src_col;
    update_board(game, move.dst_row, move.dst_col);
}

// Computes how well a situation is
int utility(Game * game) {
    int score_one = 8 * score_player_one(*game);
    int score_two = 8 * score_player_two(*game);

    if (game->won && game->won != 2) {
        if ( (game->turn & 1) == 1 || (game->turn > 63 && score_two > score_one) ) {
            return 10000; // Win
        }
        return -10000; // Loss
    } else if (game->won == 2) {
        return 24; // Gives ties a good score to encourage them (unless we have a 3 score point advantage)
    }

    Move moves[10*16];
    score_two += all_possible_moves(game, moves, P2);  // AI mobility
    score_two -= all_possible_moves(game, moves, P1);

    return score_two - score_one;
}


// Helper function that returns the number of possible moves and modifies a list to contain them
int all_possible_moves(Game * game, Move * list, Player player) {
    int size = 0;
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (get_player(game->board[i][j]) == player) {
                int k = 1;
                while (is_move_legal(game, i, j, i + k, j)) {
                    Move current_move = {i, j, i + k, j, -1};
                    list[size++] = current_move;
                    k++;
                }

                k = 1;
                while (is_move_legal(game, i, j, i - k, j)) {
                    Move current_move = {i, j, i - k, j, -1};
                    list[size++] = current_move;
                    k++;
                }

                k = 1;
                while (is_move_legal(game, i, j, i, j + k)) {
                    Move current_move = {i, j, i, j + k, -1};
                    list[size++] = current_move;
                    k++;
                }

                k = 1;
                while (is_move_legal(game, i, j, i, j - k)) {
                    Move current_move = {i, j, i, j - k, -1};
                    list[size++] = current_move;
                    k++;
                }

            }
        }
    }

    return size;
}

// Qsort function (merci les heures sur leetcode)
int compare_moves_desc(const void *a, const void *b) {
    ScoredMove *m1 = (ScoredMove*)a;
    ScoredMove *m2 = (ScoredMove*)b;
    // Higher scores first for AI (P2), you can reverse for P1
    return m2->score - m1->score;
}

// Ordered move to help make alpha-beta pruning more efficient
int all_possible_moves_ordered(Game *game, Move *move_list, Player player) {
    ScoredMove scored_moves[10*16];
    int size = 0;

    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (get_player(game->board[i][j]) == player) {
                // 4 directions: up, down, right, left
                int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
                for (int d = 0; d < 4; d++) {
                    int k = 1;
                    while (is_move_legal(game, i, j, i + k*dirs[d][0], j + k*dirs[d][1])) {
                        Move move = {i, j, i + k*dirs[d][0], j + k*dirs[d][1], -1};
                        Game temp = *game;
                        update_with_move(&temp, move);
                        int score = utility(&temp); // shallow evaluation (depth 1)

                        scored_moves[size].s_move = move;
                        scored_moves[size].score = score;
                        size++;
                        k++;
                    }
                }
            }
        }
    }

    // Sort moves by score descending
    qsort(scored_moves, size, sizeof(ScoredMove), compare_moves_desc);

    // Copy back to list
    for (int i = 0; i < size; i++) {
        move_list[i] = scored_moves[i].s_move;
    }

    return size;
}


// Minimax algorithm to predict the score for a position
int minimax_alpha_beta(Game * game, int depth, int maximizing, int alpha, int beta) {
    if (depth == 0 || game->won) return utility(game);
    Player current_player = (maximizing) ? P2 : P1;

    Move possible_moves[10 * 16]; // 10 pawns with 16 moves each at best (oui le dernier ne peux que faire 15 mouvements au max mais hassoul)
    int size = all_possible_moves(game, possible_moves, current_player);

    if (maximizing) {
        int best_score = -10001; //-INF
        for (int i = 0; i < size; i++) {
            Game temp = *game;
            Move current_move = possible_moves[i];
            update_with_move(&temp, current_move);

            int current_score = minimax_alpha_beta(&temp, depth - 1, !maximizing, alpha, beta);
            best_score = (current_score >= best_score) ? current_score : best_score;
            alpha = (alpha > best_score) ? alpha : best_score;
            if (beta <= alpha) {
                break;
            }
        }
        return best_score;

    } else {
        int best_score = 10001; //INF
        for (int i = 0; i < size; i++) {
            Game temp = *game;
            Move current_move = possible_moves[i];
            update_with_move(&temp, current_move);

            int current_score = minimax_alpha_beta(&temp, depth - 1, !maximizing, alpha, beta);
            best_score = (current_score <= best_score) ? current_score : best_score;
            beta = (beta < best_score) ? beta : best_score;
            if (beta <= alpha) {
                break;
            }
        }
        return best_score;
    }
}


// Checks score for each move possible and returns best move
Move minimax_best_move(Game * game, int depth) {
    Player current_player = ( (game->turn & 1) == 0) ? P1 : P2;

    Move possible_moves[10 * 16]; // 10 pawns with 16 moves each at best (oui le dernier ne peux que faire 15 mouvements au max mais hassoul)
    int size = all_possible_moves_ordered(game, possible_moves, current_player);
    int best_score = -10001;
    Move best_move = {-1, -1, -1, -1, -10001};

    for (int i = 0; i < size; i++) {
        Game temp = *game;
        int maximizing = (game->game_mode == CLIENT) ? 1 : 0;

        update_with_move(&temp, possible_moves[i]);
        int current_score = minimax_alpha_beta(&temp, depth - 1, maximizing, -10000, 10000);

        if (current_score > best_score) {
            best_move = possible_moves[i];
            best_score = current_score;
        }
    }

    return best_move;
}


// Returns computed move
void ai_next_move(Game* game) {
    Game copy = *game;
    copy.is_ai = 0; // Makes it look like a local game for the AI not to call itself
    Move best_move = minimax_best_move(&copy, DEPTH);
    update_with_move(game, best_move);
}

#include <stdlib.h>
#include <stdio.h>
#include "game.h"
#include "algo.h"

#define DEPTH 3

// Lighter version of update board
void update_board_ai(Game * game, int dst_row, int dst_col) {
    int src_row = game->selected_tile[0];
    int src_col = game->selected_tile[1];

    game->board[dst_row][dst_col] = game->board[src_row][src_col];
    game->board[src_row][src_col] = (get_player(game->board[src_row][src_col]) == P1) ? P1_VISITED : P2_VISITED;

    // Check if someone was eaten
    Direction direction;
    if (dst_row != src_row) {
        direction = (dst_row < src_row) ? DIR_TOP : DIR_DOWN;
    } else if (dst_col != src_col) {
        direction = (src_col > dst_col) ? DIR_LEFT : DIR_RIGHT;
    }
    did_eat(game, dst_row, dst_col, direction);

    // Handle wins
    int has_won = 1;
    if (has_won) {
        game->won = has_won;
        return;
    } else {
        // Advance turn
        game->turn++;
    }
}



//Helper function that updates board with move
void update_with_move(Game * game, Move move) {
    game->selected_tile[0] = move.src_row;
    game->selected_tile[1] = move.src_col;
    update_board_ai(game, move.dst_row, move.dst_col);
}


// Computes how well a situation is
int utility(Game * game, Player player) {
    int score_one = 10 * score_player_one(*game);
    int score_two = 10 * score_player_two(*game);

    int result = 1;
    if (result == 1) return (player == P1) ? 50000 : -50000;
    if (result == 2) return (player == P2) ? 50000 : -50000;
    if (result == 8) return 0; // draw

    Move moves[10*16];
    score_two += all_possible_moves(game, moves, P2);  // AI mobility
    score_two -= all_possible_moves(game, moves, P1);

    int final_score = (player == P2) ? score_two - score_one : score_one - score_two;
    return final_score;
}


// Helper function that returns the number of possible moves and modifies a list to contain them
int all_possible_moves(Game * game, Move * list, Player player) {
    int size = 0;
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (get_player(game->board[i][j]) == player) {
                int k = 1;
                while ( (i + k) < 9 && (get_player(game->board[i + k][j]) == NOT_PLAYER) ) {
                    Move current_move = {i, j, i + k, j, -1};
                    list[size++] = current_move;
                    k++;
                }

                k = 1;
                while ( (i - k) >= 0 && (get_player(game->board[i - k][j]) == NOT_PLAYER) )  {
                    Move current_move = {i, j, i - k, j, -1};
                    list[size++] = current_move;
                    k++;
                }

                k = 1;
                while ( (j + k) < 9 && (get_player(game->board[i][j + k]) == NOT_PLAYER) )  {
                    Move current_move = {i, j, i, j + k, -1};
                    list[size++] = current_move;
                    k++;
                }

                k = 1;
                while ( (j - k) >= 0 && (get_player(game->board[i][j - k]) == NOT_PLAYER) )  {
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

    int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}}; // Up, down, right, left

    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (get_player(game->board[i][j]) != player) continue;

            for (int d = 0; d < 4; d++) {
                int k = 1;
                while (1) {
                    int ni = i + k * dirs[d][0];
                    int nj = j + k * dirs[d][1];

                    // Stop if out of bounds
                    if (ni < 0 || ni >= 9 || nj < 0 || nj >= 9) break;

                    // Stop if cell is not empty
                    if (get_player(game->board[ni][nj]) != NOT_PLAYER) break;

                    // Create move and score it
                    Move move = {i, j, ni, nj, -1};
                    Game temp = *game;
                    update_with_move(&temp, move);

                    int score = score_player_two(temp) - score_player_one(temp);

                    if (size < 10*16) { // Prevent overflow
                        scored_moves[size].s_move = move;
                        scored_moves[size].score = score;
                        size++;
                    }

                    k++;
                }
            }
        }
    }

    // Sort moves descending by score
    qsort(scored_moves, size, sizeof(ScoredMove), compare_moves_desc);

    // Copy back to move_list
    for (int i = 0; i < size; i++) {
        move_list[i] = scored_moves[i].s_move;
    }

    return size;
}


// Minimax algorithm to predict the score for a position
int minimax_alpha_beta(Game * game, int depth, int maximizing, int alpha, int beta, Player initial_player) {
    if (depth == 0 || game->won) return utility(game, initial_player);
    Player current_player = ( (game->turn & 1) == 1) ? P2 : P1;

    Move possible_moves[10 * 16]; // 10 pawns with 16 moves each at best (oui le dernier ne peux que faire 15 mouvements au max mais hassoul)
    int size = all_possible_moves(game, possible_moves, current_player);

    if (maximizing) {
        int best_score = -100001; //-INF
        for (int i = 0; i < size; i++) {
            Game temp = *game;
            Move current_move = possible_moves[i];
            update_with_move(&temp, current_move);

            int current_score = minimax_alpha_beta(&temp, depth - 1, !maximizing, alpha, beta, initial_player);
            best_score = (current_score >= best_score) ? current_score : best_score;
            alpha = (alpha > best_score) ? alpha : best_score;
            if (beta <= alpha) {
                break;
            }
        }
        return best_score;

    } else {
        int best_score = 100001; //INF
        for (int i = 0; i < size; i++) {
            Game temp = *game;
            Move current_move = possible_moves[i];
            update_with_move(&temp, current_move);

            int current_score = minimax_alpha_beta(&temp, depth - 1, !maximizing, alpha, beta, initial_player);
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
    int best_score = -100001;
    Move best_move = {-1, -1, -1, -1, -10001};

    for (int i = 0; i < size; i++) {
        Game temp = *game;

        update_with_move(&temp, possible_moves[i]);
        int current_score = minimax_alpha_beta(&temp, depth - 1, 0, -100000, 100000, current_player);

        if (current_score > best_score) {
            best_move = possible_moves[i];
            best_score = current_score;
        }
    }

    printf(" Best score: %d, Player 2: %d\n", best_score, (game->turn & 1) == 1);
    return best_move;
}

void client_first_move(Game * game) {
    printf("DEBUG first move\n");
    fflush(stdout);
    Move first_move = {0, 3, 0, 4, -1};

    game->selected_tile[0] = first_move.src_row;
    game->selected_tile[1] = first_move.src_col;
    update_board(game, first_move.dst_row, first_move.dst_col);
}


// Returns computed move
void ai_next_move(Game* game) {
    Game copy = *game;
    copy.is_ai = 0; // Makes it look like a local game for the AI not to call itself
    Move best_move = minimax_best_move(&copy, DEPTH);

    game->selected_tile[0] = best_move.src_row;
    game->selected_tile[1] = best_move.src_col;
    update_board(game, best_move.dst_row, best_move.dst_col);
}

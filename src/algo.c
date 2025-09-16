#include <stdlib.h>
#include <stdio.h>

#include "game.h"
#include "algo.h"
#include "const.h"

/**
 * Fonction de mise à jour du plateau pour l'IA
 * Elle est similaire à update_board mais sans SDL_Delay.
 *
 * @param game Pointeur vers la structure de jeu
 * @param dst_row Ligne de destination
 * @param dst_col Colonne de destination
 * @return void
 */
// Required info to undo a move
typedef struct {
    int src_row, src_col;
    int dst_row, dst_col;

    int src_piece;
    int dst_piece;

    int turn_before;
    int won_before;

    // eaten pawns
    int eaten_count;
    EatenPiece eaten[4];
} UndoInfo;

// Checks if AI ate a pawn and updates UndoInfo
void did_eat_ai(Game *game, int row, int col, Direction sprint_direction, UndoInfo *undo) {
    undo->eaten_count = 0; // reset

    Player player = ((game->turn & 1) == 0) ? P1 : P2;
    Player opponent = (player == P1) ? P2 : P1;

    Player top = (row - 1 >= 0)? get_player(game->board[row - 1][col]) : NOT_PLAYER;
    Player left = (col - 1 >= 0)? get_player(game->board[row][col - 1]) : NOT_PLAYER;
    Player right = (col + 1 <= 8)? get_player(game->board[row][col + 1]) : NOT_PLAYER;
    Player down = (row + 1 <= 8)? get_player(game->board[row + 1][col]) : NOT_PLAYER;

    // --- same logic, but record before removal ---
    if (top == opponent) {
        if (((row - 2 < 0 || get_player(game->board[row - 2][col]) != opponent) && sprint_direction == DIR_TOP) ||
            (get_player(game->board[row - 2][col]) == player && row - 2 >= 0)) {

            undo->eaten[undo->eaten_count].row = row - 1;
            undo->eaten[undo->eaten_count].col = col;
            undo->eaten[undo->eaten_count].piece = game->board[row - 1][col];
            undo->eaten_count++;
            game->board[row - 1][col] = P_NONE;
        }
    }

    if (left == opponent) {
        if (((col - 2 < 0 || get_player(game->board[row][col - 2]) != opponent) && sprint_direction == DIR_LEFT) ||
            (get_player(game->board[row][col - 2]) == player && col - 2 >= 0)) {

            undo->eaten[undo->eaten_count].row = row;
            undo->eaten[undo->eaten_count].col = col - 1;
            undo->eaten[undo->eaten_count].piece = game->board[row][col - 1];
            undo->eaten_count++;
            game->board[row][col - 1] = P_NONE;
        }
    }

    if (right == opponent) {
        if (((col + 2 > 8 || get_player(game->board[row][col + 2]) != opponent) && sprint_direction == DIR_RIGHT) ||
            (get_player(game->board[row][col + 2]) == player && col + 2 <= 8)) {

            undo->eaten[undo->eaten_count].row = row;
            undo->eaten[undo->eaten_count].col = col + 1;
            undo->eaten[undo->eaten_count].piece = game->board[row][col + 1];
            undo->eaten_count++;
            game->board[row][col + 1] = P_NONE;
        }
    }

    if (down == opponent) {
        if (((row + 2 > 8 || get_player(game->board[row + 2][col]) != opponent) && sprint_direction == DIR_DOWN) ||
            (get_player(game->board[row + 2][col]) == player && row + 2 <= 8)) {

            undo->eaten[undo->eaten_count].row = row + 1;
            undo->eaten[undo->eaten_count].col = col;
            undo->eaten[undo->eaten_count].piece = game->board[row + 1][col];
            undo->eaten_count++;
            game->board[row + 1][col] = P_NONE;
        }
    }
}

// Update sboard and stores necessary info to undo it
UndoInfo update_board_ai(Game *game, int dst_row, int dst_col) {
    UndoInfo undo;
    int src_row = game->selected_tile[0];
    int src_col = game->selected_tile[1];

    undo.src_row = src_row;
    undo.src_col = src_col;
    undo.dst_row = dst_row;
    undo.dst_col = dst_col;
    undo.src_piece = game->board[src_row][src_col];
    undo.dst_piece = game->board[dst_row][dst_col];
    undo.turn_before = game->turn;
    undo.won_before = game->won;
    undo.eaten_count = 0;

    // apply move
    game->board[dst_row][dst_col] = undo.src_piece;
    game->board[src_row][src_col] = (get_player(undo.src_piece) == P1) ? P1_VISITED : P2_VISITED;

    // check captures
    Direction direction;
    if (dst_row != src_row) {
        direction = (dst_row < src_row) ? DIR_TOP : DIR_DOWN;
    } else {
        direction = (src_col > dst_col) ? DIR_LEFT : DIR_RIGHT;
    }
    did_eat_ai(game, dst_row, dst_col, direction, &undo);

    // win / turn
    won(game);
    if (game->won != NOT_PLAYER) {
        // Game ended
    } else {
        game->turn++;
    }

    game->turn++;

    return undo;
}

void undo_board_ai(Game *game, UndoInfo undo) {
    // restore board
    game->board[undo.src_row][undo.src_col] = undo.src_piece;
    game->board[undo.dst_row][undo.dst_col] = undo.dst_piece;

    // restore eaten pawns
    for (int i = 0; i < undo.eaten_count; i++) {
        game->board[undo.eaten[i].row][undo.eaten[i].col] = undo.eaten[i].piece;
    }

    // restore state
    game->turn = undo.turn_before;
    game->won = undo.won_before;
}



/**
 * Fonction de mise à jour du plateau avec un mouvement donné.
 * Elle met à jour la position sélectionnée et applique le mouvement pour les simulations.
 *
 * @param game Pointeur vers la structure de jeu
 * @param move Mouvement à appliquer
 * @return void
 */
void update_with_move(Game * game, Move move) {
    game->selected_tile[0] = move.src_row;
    game->selected_tile[1] = move.src_col;

    // Direct board update without using update_board_ai for simulations
    Piece moving_piece = game->board[move.src_row][move.src_col];
    game->board[move.dst_row][move.dst_col] = moving_piece;
    game->board[move.src_row][move.src_col] = (get_player(moving_piece) == P1) ? P1_VISITED : P2_VISITED;

    // Check captures
    Direction direction = NONE;
    if (move.dst_row != move.src_row) {
        direction = (move.dst_row < move.src_row) ? DIR_TOP : DIR_DOWN;
    } else if (move.dst_col != move.src_col) {
        direction = (move.dst_col > move.src_col) ? DIR_RIGHT : DIR_LEFT;
    }

    if (direction != NONE) {
        did_eat(game, move.dst_row, move.dst_col, direction);
    }

    // Check win condition and advance turn
    won(game);
    if (game->won == NOT_PLAYER) {
        game->turn++;
    }
}


/**
 * Fonction d'évaluation de l'état du jeu pour un joueur donné.
 * Elle combine le score des joueurs et la mobilité.
 *
 * @param game Pointeur vers la structure de jeu
 * @param player Joueur pour lequel évaluer (P1 ou P2)
 * @return Score évalué
 */
int utility(Game * game, Player player) {
    int score_one = 10 * score_player_one(*game);
    int score_two = 10 * score_player_two(*game);

    // Check win conditions
    if (game->won == P1) return (player == P1) ? 50000 : -50000;
    if (game->won == P2) return (player == P2) ? 50000 : -50000;
    if (game->won == DRAW) return 0; // draw

    Move moves[10*16];
    score_two += all_possible_moves(game, moves, P2);  // AI mobility
    score_one += all_possible_moves(game, moves, P1);

    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (get_player(game->board[i][j]) == P2) score_two += 10;
            if (get_player(game->board[i][j]) == P1) score_one += 10;
        }
    }

    int final_score = (player == P2) ? score_two - score_one : score_one - score_two;
    return final_score;
}


/**
 * Génère tous les mouvements possibles pour un joueur donné.
 *
 * @param game Pointeur vers la structure de jeu
 * @param list Tableau pour stocker les mouvements possibles
 * @param player Joueur pour lequel générer les mouvements (P1 ou P2)
 * @return Nombre de mouvements générés
 */
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


/**
 * Fonction de comparaison pour qsort
 * Trie les mouvements par score décroissant (merci les heures sur leetcode)
 *
 * @param a Pointeur vers le premier mouvement
 * @param b Pointeur vers le second mouvement
 * @return Entier indiquant l'ordre des mouvements
 */
int compare_moves_desc(const void *a, const void *b) {
    ScoredMove *m1 = (ScoredMove*)a;
    ScoredMove *m2 = (ScoredMove*)b;
    // Higher scores first for AI (P2), you can reverse for P1
    return m2->score - m1->score;
}

/**
 * Génère tous les mouvements possibles pour un joueur donné, les évalue et les trie par score décroissant.
 *
 * @param game Pointeur vers la structure de jeu
 * @param move_list Tableau pour stock
 */
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


/**
 * Minimax avec élagage alpha-bêta
 *
 * @param game Pointeur vers la structure de jeu
 * @param depth Profondeur actuelle de l'arbre
 * @param maximizing Booléen indiquant si on maximise ou minimise
 * @param alpha Valeur alpha pour l'élagage
 * @param beta Valeur beta pour l'élagage
 * @param initial_player Joueur initial pour l'évaluation
 *
 * @return Score évalué
 */
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

/**
 * Fonction principale pour obtenir le meilleur mouvement en utilisant minimax avec élagage alpha-bêta
 *
 * @param game Pointeur vers la structure de jeu
 * @param depth Profondeur maximale pour la recherche
 * @return Meilleur mouvement trouvé
 */
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

/**
 * Premier mouvement fixe pour l'IA
 *
 * @param game Pointeur vers la structure de jeu
 * @return void
 */
void client_first_move(Game * game) {
    printf("DEBUG first move\n");
    fflush(stdout);
    Move first_move = {0, 3, 0, 4, -1};

    game->selected_tile[0] = first_move.src_row;
    game->selected_tile[1] = first_move.src_col;
    update_board(game, first_move.dst_row, first_move.dst_col);
}


/**
 * Fonction principale pour que l'IA joue son prochain mouvement.
 *
 * @param game Pointeur vers la structure de jeu
 * @return void
 */
void ai_next_move(Game* game) {
    Game copy = *game;
    copy.is_ai = 0; // Makes it look like a local game for the AI not to call itself
    Move best_move = minimax_best_move(&copy, DEPTH);

    game->selected_tile[0] = best_move.src_row;
    game->selected_tile[1] = best_move.src_col;
    update_board(game, best_move.dst_row, best_move.dst_col);

}

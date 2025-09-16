#include <math.h>

#include "game.h"
#include "display.h"
#include "algo.h"
#include "input.h"
#include "const.h"
#include "algo.h"


/**
 * Initialise une nouvelle partie avec le mode de jeu et l'IA spécifiés.
 * @param mode Le mode de jeu (LOCAL, SERVER, CLIENT)
 * @param artificial_intelligence 1 si l'IA est activée, 0 sinon
 * @return Une structure Game initialisée
 */
Game init_game(GameMode mode, int artificial_intelligence) {
    (void)mode;                    // évite warnings si pas utilisé partout
    (void)artificial_intelligence; // idem

    Game game;

    game.won = 0;
    game.turn = 0;

    // initialise les états avec le plateau de départ défini dans const.h
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


/**
 * Retourne le score du joueur 1.
 * 
 * @param game La structure de jeu contenant l'état actuel du plateau.
 * @return Le score du joueur 1.
 */
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

/**
 * Retourne le score du joueur 2.
 * 
 * @param game La structure de jeu contenant l'état actuel du plateau.
 * @return Le score du joueur 2.
 */
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
    if (get_player(game->board[src_row][src_col]) == NOT_PLAYER) return 0;

    // Must move straight (horizontal or vertical)
    if (src_row != dst_row && src_col != dst_col) return 0;

    // Destination must be empty
    if (get_player(game->board[dst_row][dst_col]) != NOT_PLAYER) return 0;

    // Check turn ownership
    if ((current_player_turn(game) == P1) &&
        (game->board[src_row][src_col] == P2_PAWN || game->board[src_row][src_col] == P2_KING)) return 0;
    if ((current_player_turn(game) == P2) &&
        (game->board[src_row][src_col] == P1_PAWN || game->board[src_row][src_col] == P1_KING)) return 0;

    // Horizontal path check
    if (src_row == dst_row) {
        int step = (dst_col > src_col) ? 1 : -1;
        for (int c = src_col + step; c != dst_col; c += step) {
            if (get_player(game->board[src_row][c]) != NOT_PLAYER) return 0; // blocked
        }
    }

    // Vertical path check
    if (src_col == dst_col) {
        int step = (dst_row > src_row) ? 1 : -1;
        for (int r = src_row + step; r != dst_row; r += step) {
            if (get_player(game->board[r][src_col]) != NOT_PLAYER) return 0; // blocked
        }
    }

    return 1; // Move is legal
}


/**
 * Retourne le joueur associé à une pièce donnée.
 * @param piece La pièce dont on veut connaître le joueur.
 * @return Le joueur (P1, P2) ou NOT_PLAYER si la pièce n
 */
Player get_player(Piece piece) {
    if (piece == P1_PAWN || piece == P1_KING) return P1;
    if (piece == P2_PAWN || piece == P2_KING) return P2;
    return NOT_PLAYER;
}

/**
 * Vérifie et effectue les captures après un déplacement.
 * 
 * @param game Pointeur vers la structure de jeu contenant l'état actuel du plateau et le tour.
 * @param row La ligne où la pièce a été déplacée.
 * @param col La colonne où la pièce a été déplacée.
 * @param sprint_direction La direction du déplacement (DIR_TOP, DIR_DOWN, DIR_LEFT,
 * DIR_RIGHT).
 * @return void
 */
void did_eat(Game* game, int row, int col, Direction sprint_direction) {
    Player player = current_player_turn(game);
    Player opponent = (player == P1) ? P2 : P1;
    

    Player top = (row - 1 >= 0)? get_player(game->board[row - 1][col]) : NOT_PLAYER;
    Player left = (col - 1 >= 0)? get_player(game->board[row][col - 1]) : NOT_PLAYER;
    Player right = (col + 1 < GRID_SIZE)? get_player(game->board[row][col + 1]) : NOT_PLAYER;
    Player down = (row + 1 < GRID_SIZE)? get_player(game->board[row + 1][col]) : NOT_PLAYER;


    // Eats if sprint towards opponent without one behind him defending or when sandwiched
    if (top == opponent) {
        if ( ((row - 2 < 0 || get_player(game->board[row - 2][col]) != opponent) && sprint_direction == DIR_TOP ) ||
              (get_player(game->board[row - 2][col]) == player && row - 2 >= 0) )  {
            game->board[row - 1][col] = P_NONE;
        }
    }

    if (left == opponent) {
        if ( ((col - 2 < 0 || get_player(game->board[row][col - 2]) != opponent) && sprint_direction == DIR_LEFT ) ||
              (get_player(game->board[row][col - 2]) == player && col - 2 >= 0) ) {
            game->board[row][col - 1] = P_NONE;
        }
    }

    if (right == opponent) {
        if ( ((col + 2 >= GRID_SIZE || get_player(game->board[row][col + 2]) != opponent) && sprint_direction == DIR_RIGHT ) ||
              (get_player(game->board[row][col + 2]) == player && col + 2 < GRID_SIZE) ) {
            game->board[row][col + 1] = P_NONE;
        }
    }

    if (down == opponent) {
        if ( ((row + 2 >= GRID_SIZE || get_player(game->board[row + 2][col]) != opponent) && sprint_direction == DIR_DOWN ) ||
              (get_player(game->board[row + 2][col]) == player && row + 2 < GRID_SIZE) ) {
            game->board[row + 1][col] = P_NONE;
        }
    }
}


/**
 * Vérifie les conditions de victoire et met à jour l'état du jeu.
 * @param game Pointeur vers la structure de jeu contenant l'état actuel du plateau et le tour.
 * @return void
 */
void won(Game* game) {

    if (game->board[GRID_SIZE-1][GRID_SIZE-1] == P1_KING && game->won == NOT_PLAYER) {
        game->won = P1;
    } else if (game->board[0][0] == P2_KING && game->won == NOT_PLAYER) {
        game->won = P2;
    }

    // Vérifie si les rois sont encore vivants
    int is_blue_king_alive = 0;
    int is_red_king_alive = 0;
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (game->board[i][j] == P1_KING) is_blue_king_alive++;
            if (game->board[i][j] == P2_KING) is_red_king_alive++;
        }
    }

    if ((!is_blue_king_alive || !is_red_king_alive) && game->won == NOT_PLAYER) {
        if (!is_blue_king_alive) {
            game->won=P2;
        } else if (!is_red_king_alive) {
            game->won=P1;
        } else {
            game->won=NOT_PLAYER;
        }
    }

    if (game->won == NOT_PLAYER) {
        // Vérifie le nombre de pièces restantes pour chaque camp
        int p1_piece = 0;
        int p2_piece = 0;

        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {

                if (game->board[i][j] == P1_PAWN || game->board[i][j] == P1_KING) {
                    p1_piece++;
                }
                if (game->board[i][j] == P2_PAWN || game->board[i][j] == P2_KING) {
                    p2_piece++;
                }
            }
        }

        // Si le joueur 1 n'a plus que 2 pièces (roi + 1 soldat), joueur 2 gagne
        // Sinon le joueur 2 n'a plus que 2 pièces (roi + 1 soldat), joueur 1 gagne
        if (p1_piece < 3) {
            game->won = P2;
        } else if (p2_piece < 3) {
            game->won = P1;
        }
    }

    if (game->turn >= 63 && game->won == NOT_PLAYER) {
        int counter = score_player_one(*game) - score_player_two(*game);
        if (counter != 0) {
            game->won = (counter > 0) ? P1 : P2;
        } else {
            game->won = DRAW;
        }
    }
}


/**
 * Met à jour le plateau de jeu pour le mode LAN.
 * Dans ce mode, il n'y a pas de coups automatiques.
 * 
 * @param game Pointeur vers la structure de jeu contenant l'état actuel du plateau et le tour.
 * @return 1 si la mise à jour est réussie, sinon 0.
 */
int update_board_lan(Game* game) {
    // Plus de coups automatiques - la synchronisation se fait via le réseau
    (void)game; // Éviter le warning de paramètre non utilisé
    return 1;
}


/**
 * Met à jour le plateau de jeu en fonction du déplacement effectué.
 * Vérifie si le déplacement est légal, effectue le déplacement, gère les captures,
 */
void update_board(Game *game, int dst_row, int dst_col) {
    int src_row = game->selected_tile[0];
    int src_col = game->selected_tile[1];

    // Nothing selected so do nothing
    if (src_row < 0 || src_col < 0) return;

    if (is_move_legal(game, src_row, src_col, dst_row, dst_col)) {
        // Move piece
        game->board[dst_row][dst_col] = game->board[src_row][src_col];
        game->board[src_row][src_col] = (get_player(game->board[src_row][src_col]) == P1) ? P1_VISITED : P2_VISITED;

        // Check if someone was eaten
        Direction direction = NONE;
        if (dst_row != src_row) {
            if (dst_row > src_row) {
                direction = DIR_DOWN;
            } else {
                direction = DIR_TOP;
            }
        } else if (dst_col != src_col) {
            if (dst_col > src_col) {
                direction = DIR_RIGHT;
            } else {
                direction = DIR_LEFT;
            }
        }
        did_eat(game, dst_row, dst_col, direction);

        won(game);

        // Advance turn
        game->turn++;

        // Reset selection
        game->selected_tile[0] = -1;
        game->selected_tile[1] = -1;

        int next_move_status __attribute__((unused)) = update_board_lan(game);
        
        // Vérifier si l'IA doit jouer après ce changement d'état
        check_ai_turn(game);
    }
}

// Retourne le joueur dont c'est le tour actuel
Player current_player_turn(Game *game) {
    return (game->turn % 2 == 0) ? P1 : P2;
}

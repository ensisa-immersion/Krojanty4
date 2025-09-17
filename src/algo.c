/**
 * @file algo.c
 * @brief Implémentation de l'intelligence artificielle pour le jeu
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 * 
 * Ce fichier contient toutes les fonctions liées à l'IA du jeu, incluant :
 * - L'évaluation des positions (fonction utility)
 * - L'algorithme minimax avec élagage alpha-bêta
 * - La génération et le tri des mouvements possibles
 * - Les fonctions de simulation de mouvements pour l'IA
 */

#include <stdlib.h>

#include "game.h"
#include "algo.h"
#include "const.h"
#include "logging.h"

/**
 * @struct UndoInfo
 * @brief Structure contenant les informations nécessaires pour annuler un mouvement
 * 
 * Cette structure est utilisée par l'IA pour sauvegarder l'état du jeu
 * avant de simuler un mouvement, permettant de revenir à l'état précédent.
 */
typedef struct {
    int src_row;        /**< Ligne source du mouvement */
    int src_col;        /**< Colonne source du mouvement */
    int dst_row;        /**< Ligne destination du mouvement */
    int dst_col;        /**< Colonne destination du mouvement */
    
    int src_piece;      /**< Pièce à la position source */
    int dst_piece;      /**< Pièce à la position destination */
    
    int turn_before;    /**< Numéro du tour avant le mouvement */
    int won_before;     /**< État de victoire avant le mouvement */
    
    int eaten_count;    /**< Nombre de pièces capturées */
    EatenPiece eaten[4]; /**< Tableau des pièces capturées (max 4) */
} UndoInfo;

/**
 * @brief Vérifie et applique les captures lors d'un mouvement de l'IA
 * 
 * Cette fonction examine les cases adjacentes à la position donnée pour détecter
 * des pièces adverses qui peuvent être capturées selon les règles du jeu.
 * Elle met à jour la structure UndoInfo avec les informations des captures.
 * 
 * @param game Pointeur vers la structure de jeu
 * @param row Ligne de la position à examiner
 * @param col Colonne de la position à examiner  
 * @param sprint_direction Direction du mouvement effectué
 * @param undo Pointeur vers la structure UndoInfo à mettre à jour
 */
void did_eat_ai(Game *game, int row, int col, Direction sprint_direction, UndoInfo *undo) {
    undo->eaten_count = 0; // Réinitialisation du compteur de captures

    // Détermination des joueurs actuel et adverse
    Player player = ((game->turn & 1) == 0) ? P1 : P2;
    Player opponent = (player == P1) ? P2 : P1;

    // Vérification des cases adjacentes pour détecter les adversaires
    Player top = (row - 1 >= 0) ? get_player(game->board[row - 1][col]) : NOT_PLAYER;
    Player left = (col - 1 >= 0) ? get_player(game->board[row][col - 1]) : NOT_PLAYER;
    Player right = (col + 1 <= 8) ? get_player(game->board[row][col + 1]) : NOT_PLAYER;
    Player down = (row + 1 <= 8) ? get_player(game->board[row + 1][col]) : NOT_PLAYER;

    // Si haut est un adversaire et qu'il peut être capturé
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
    // Si gauche est un adversaire et qu'il peut être capturé
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

    // Si droite est un adversaire et qu'il peut être capturé
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

    // Si bas est un adversaire et qu'il peut être capturé
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

/**
 * @brief Applique un mouvement sur le plateau pour les simulations de l'IA
 * 
 * Cette fonction met à jour le plateau de jeu en appliquant un mouvement donné.
 * Elle gère également les captures résultantes et sauvegarde les informations
 * nécessaires pour pouvoir annuler le mouvement.
 * 
 * @param game Pointeur vers la structure de jeu à modifier
 * @param dst_row Ligne de destination du mouvement
 * @param dst_col Colonne de destination du mouvement
 * @return UndoInfo Structure contenant les informations pour annuler le mouvement
 */
UndoInfo update_board_ai(Game *game, int dst_row, int dst_col) {
    UndoInfo undo;
    // Récupération de la position source sélectionnée
    int src_row = game->selected_tile[0];
    int src_col = game->selected_tile[1];

    // Initialisation de la structure d'annulation
    undo.src_row = src_row;
    undo.src_col = src_col;
    undo.dst_row = dst_row;
    undo.dst_col = dst_col;
    undo.src_piece = game->board[src_row][src_col];
    undo.dst_piece = game->board[dst_row][dst_col];
    undo.turn_before = game->turn;
    undo.won_before = game->won;
    undo.eaten_count = 0;

    // Application du mouvement sur le plateau
    game->board[dst_row][dst_col] = undo.src_piece;
    game->board[src_row][src_col] = (get_player(undo.src_piece) == P1) ? P1_VISITED : P2_VISITED;

    // Détermination de la direction du mouvement pour les captures
    Direction direction;
    if (dst_row != src_row) {
        direction = (dst_row < src_row) ? DIR_TOP : DIR_DOWN;
    } else {
        direction = (src_col > dst_col) ? DIR_LEFT : DIR_RIGHT;
    }
    
    // Vérification et application des captures
    did_eat_ai(game, dst_row, dst_col, direction, &undo);

    // Avancement du tour (won() est commenté pour éviter les effets de bord)
    game->turn++;

    return undo;
}

/**
 * @brief Annule un mouvement précédemment appliqué par l'IA
 * 
 * Cette fonction restaure l'état du plateau de jeu à partir des informations
 * sauvegardées dans la structure UndoInfo. Elle remet en place toutes les
 * pièces à leur position d'origine et restaure l'état du jeu.
 * 
 * @param game Pointeur vers la structure de jeu à restaurer
 * @param undo Structure contenant les informations de restauration
 */
void undo_board_ai(Game *game, UndoInfo undo) {
    // Restauration des positions des pièces
    game->board[undo.src_row][undo.src_col] = undo.src_piece;
    game->board[undo.dst_row][undo.dst_col] = undo.dst_piece;

    // Restauration des pièces capturées
    for (int i = 0; i < undo.eaten_count; i++) {
        game->board[undo.eaten[i].row][undo.eaten[i].col] = undo.eaten[i].piece;
    }

    // Restauration de l'état du jeu
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

    // Mise à jour directe du plateau sans utiliser update_board_ai (réservé aux simulations IA)
    Piece moving_piece = game->board[move.src_row][move.src_col];
    game->board[move.dst_row][move.dst_col] = moving_piece;
    game->board[move.src_row][move.src_col] = (get_player(moving_piece) == P1) ? P1_VISITED : P2_VISITED;

    // Vérification et application des captures éventuelles après le mouvement
    Direction direction = NONE;
    if (move.dst_row != move.src_row) {
        direction = (move.dst_row < move.src_row) ? DIR_TOP : DIR_DOWN;
    } else if (move.dst_col != move.src_col) {
        direction = (move.dst_col > move.src_col) ? DIR_RIGHT : DIR_LEFT;
    }

    if (direction != NONE) {
        did_eat(game, move.dst_row, move.dst_col, direction);
    }

    // Vérification de la condition de victoire
    if (game->won == NOT_PLAYER) {
        game->turn++;
    }
}

// ÉVALUATION DES PIÈCES : Compter les pièces (facteur principal)
int util_pieces(Game* game, Player player) {
    int pieces_p1 = score_player_one(*game);
    int pieces_p2 = score_player_two(*game);
    
    int piece_value = (pieces_p1 <= ENDGAME_PIECE_THRESHOLD || pieces_p2 <= ENDGAME_PIECE_THRESHOLD) ? 30 : 100;

    int score_p1 = pieces_p1 * piece_value;
    int score_p2 = pieces_p2 * piece_value;

    return (player == P1) ? (score_p1 - score_p2) : (score_p2 - score_p1);
}

// ÉVALUATION DE LA MOBILITÉ : Plus de mouvements = meilleure position
int util_mobility(Game* game, Player player) {
    Move moves[10*16];
    int mobility_p1 = all_possible_moves(game, moves, P1);
    int mobility_p2 = all_possible_moves(game, moves, P2);

    return (player == P1) ? (mobility_p1 - mobility_p2) * 50 : (mobility_p2 - mobility_p1) * 50;
}

// CONTRÔLE DU CENTRE : Occuper le centre est avantageux
int util_center(Game* game, Player player) {
    int score_p1 = 0;
    int score_p2 = 0;
    for (int i = 3; i <= 5; i++) {
        for (int j = 3; j <= 5; j++) {
                Player piece_owner = get_player(game->board[i][j]);
                if (piece_owner == P1) score_p1 += 125;
                if (piece_owner == P2) score_p2 += 125;
        }
    }
    return (player == P1) ? (score_p1 - score_p2) : (score_p2 - score_p1);
}

// POSITION AVANCÉE : Avancer vers l'adversaire
int util_forward(Game* game, Player player) {
    int score_p1 = 0;
    int score_p2 = 0;
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
                Player piece_owner = get_player(game->board[i][j]);
                if (piece_owner == P1) {
                    // P1 veut avancer vers le bas (grandes valeurs de i)
                    score_p1 += i * 3;
                }
                if (piece_owner == P2) {
                    // P2 veut avancer vers le haut (petites valeurs de i)
                    score_p2 += (8 - i) * 3;
                }
        }
    }
    return (player == P1) ? (score_p1 - score_p2) : (score_p2 - score_p1);
}

// Détection si un roi est menacé de capture immédiate
int king_threats(Game* game, Player king_owner) {
    int dirs[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if ((king_owner == P1 && game->board[i][j] == P1_KING) ||
                (king_owner == P2 && game->board[i][j] == P2_KING)) {
                int threats = 0;
                for (int d = 0; d < 4; d++) {
                    int ni = i + dirs[d][0];
                    int nj = j + dirs[d][1];
                    if (ni >= 0 && ni < GRID_SIZE && nj >= 0 && nj < GRID_SIZE) {
                        if (get_player(game->board[ni][nj]) != king_owner &&
                            get_player(game->board[ni][nj]) != NOT_PLAYER) {
                            threats++;
                        }
                    }
                }
                if (threats >= 2) return 1;
            }
        }
    }
    return 0;
}

// ÉVALUATION DES ROIS : Protection et positionnement stratégique
int util_kings(Game* game, Player player) {
    int score_p1 = 0;
    int score_p2 = 0;
    int piece_p1 = score_player_one(*game);
    int piece_p2 = score_player_two(*game);

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (game->board[i][j] == P1_KING) {
                score_p1 += 500; // Valeur intrinsèque élevée du roi
                
                // Bonus spécial en fin de partie pour atteindre les coins
                if (player == P1 && piece_p1 <= ENDGAME_PIECE_THRESHOLD) {
                    if ((i == 8) || (j == 8)) {
                        score_p1 += 1000; // Bonus élevé pour atteindre un coin
                    } else {
                        score_p1 += 300; // Bonus modéré pour autres positions
                    }
                }
                if (king_threats(game, P1)) {
                    score_p1 -= 9000;
                }
            }
            if (game->board[i][j] == P2_KING) {
                score_p2 += 500; // Valeur intrinsèque élevée du roi
                
                // Bonus spécial en fin de partie pour atteindre les coins
                if (player == P2 && piece_p2 <= ENDGAME_PIECE_THRESHOLD) {
                    if ((i == 0) || (j == 0)) {
                        score_p2 += 800; // Bonus élevé pour atteindre un coin
                    } else {
                        score_p2 += 300; // Bonus modéré pour autres positions
                    }
                }
                if (king_threats(game, P2)) {
                    score_p2 -= 9000;
                }
            }
        }
    }
    return (player == P1) ? (score_p1 - score_p2) : (score_p2 - score_p1);
}

// FORMATION TACTIQUE : Bonus pour les pièces qui se protègent mutuellement
int util_tactics(Game* game, Player player) {
    int score_p1 = 0;
    int score_p2 = 0;

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            Player piece = get_player(game->board[i][j]);
            if (piece != NOT_PLAYER) {
                int allies_nearby = 0;
                
                // Vérification des cases adjacentes pour détecter les alliés
                for (int di = -1; di <= 1; di++) {
                    for (int dj = -1; dj <= 1; dj++) {
                        if (di == 0 && dj == 0) continue; // Ignorer la case actuelle
                        int ni = i + di, nj = j + dj;
                        if (ni >= 0 && ni < 9 && nj >= 0 && nj < 9) {
                            if (get_player(game->board[ni][nj]) == piece) {
                                allies_nearby++;
                            }
                        }
                    }
                }
                // Bonus proportionnel au nombre d'alliés adjacents
                if (piece == P1) score_p1 += allies_nearby * 50;
                if (piece == P2) score_p2 += allies_nearby * 50;
            }
        }
    }
    return (player == P1) ? (score_p1 - score_p2) : (score_p2 - score_p1);
}

// ANALYSE DES MENACES : Détection des pièces en danger de capture
int util_threats(Game* game, Player player) {
    int score_p1 = 0;
    int score_p2 = 0;
    int dirs[4][2] = {{-1,0}, {1,0}, {0,-1}, {0,1}};

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            Player piece = get_player(game->board[i][j]);
            if (piece != NOT_PLAYER) {
                Player opponent = (piece == P1) ? P2 : P1;
                int threats = 0;

                // Vérification des menaces directes dans les 4 directions cardinales
                for (int d = 0; d < 4; d++) {
                    int ni = i + dirs[d][0];
                    int nj = j + dirs[d][1];
                    if (ni >= 0 && ni < 9 && nj >= 0 && nj < 9) {
                        if (get_player(game->board[ni][nj]) == opponent) {
                            threats++; // Comptage des adversaires adjacents
                        }
                    }
                }
                // Malus plus sévère si le roi est menacé
                if (piece == P1 && game->board[i][j] == P1_KING) score_p1 -= threats * 500; 
                else if (piece == P1) score_p1 -= threats * 50;

                if (piece == P2 && game->board[i][j] == P2_KING) score_p2 -= threats * 500;
                else if (piece == P2) score_p2 -= threats * 50;
            }
        }
    }
    return (player == P1) ? (score_p1 - score_p2) : (score_p2 - score_p1);
}

/**
 * @brief Fonction d'évaluation heuristique de l'état du jeu
 * 
 * Cette fonction évalue la qualité d'une position pour un joueur donné.
 * Elle prend en compte plusieurs facteurs stratégiques :
 * - Les conditions de victoire/défaite
 * - Le nombre de pièces de chaque joueur
 * - La mobilité (nombre de mouvements possibles)
 * - Le contrôle du centre du plateau
 * - La position et la sécurité des rois
 * - Les menaces sur les pièces adverses
 * 
 * @param game Pointeur vers la structure de jeu à évaluer
 * @param player Joueur pour lequel effectuer l'évaluation (P1 ou P2)
 * @return int Score d'évaluation (positif = avantageux, négatif = désavantageux)
 */
int utility(Game * game, Player player) {
    Game temp = *game;
    won(&temp);
    Player winner = temp.won;

    int piece_p1 = score_player_one(*game);
    int piece_p2 = score_player_two(*game);

    // Vérification des conditions de victoire (priorité absolue)
    if (winner == P1) return (player == P1) ? 5000 : -5000;
    if (winner == P2) return (player == P2) ? 5000 : -5000;
    if (winner == DRAW) return 0;

    // Vérification si un roi est en danger immédiat
    if (king_threats(game, P1)) return (player == P1) ? -10000 : 10000;  
    if (king_threats(game, P2)) return (player == P2) ? -10000 : 10000;

    // Vérification des bases capturées
    if (game->board[8][0] == P1_KING || game->board[8][0] == P1) {
        return (player == P2) ? -5000 : 5000;
    }

    // Vérification des conditions de fin de partie
    if ((piece_p2 <= 2 && king_alive(game, P2)) || game->turn >= 64) {
        int score_points = piece_p2 - piece_p1; 
        return (player == P2) ? score_points : -score_points;
    }

    // Initialisation des scores pour chaque joueur
    // int score_p1 = 0, score_p2 = 0;
    int score = 0;

    // score += util_forward(game, player);

    if (piece_p1 <= ENDGAME_PIECE_THRESHOLD || piece_p2 <= ENDGAME_PIECE_THRESHOLD) {
        score += util_kings(game, player) * 10;
        score += util_threats(game, player) * 6;
        score += util_mobility(game, player) * 2; 
        score += util_pieces(game, player) * 2; 
        score += util_center(game, player) * 1;
        score += util_tactics(game, player) * 1;
    } else {
        score += util_center(game, player) * 1;
        score += util_kings(game, player) * 5;
        score += util_pieces(game, player) * 2;
        score += util_mobility(game, player) * 3;
        score += util_tactics(game, player) * 2;
        score += util_threats(game, player) * 4;
    }



    // Calcul du score final relatif au joueur évalué
    // int final_score = (player == P2) ? score_p2 - score_p1 : score_p1 - score_p2;
    return score;
    // return final_score;
}


/**
 * @brief Génère tous les mouvements possibles pour un joueur donné
 * 
 * Cette fonction parcourt le plateau de jeu et génère tous les mouvements
 * légaux pour les pièces du joueur spécifié. Elle explore les 4 directions
 * cardinales (haut, bas, gauche, droite) pour chaque pièce du joueur.
 * 
 * @param game Pointeur vers la structure de jeu
 * @param list Tableau pour stocker les mouvements possibles générés
 * @param player Joueur pour lequel générer les mouvements (P1 ou P2)
 * @return int Nombre total de mouvements générés
 */
int all_possible_moves(Game * game, Move * list, Player player) {
    int size = 0; // Compteur de mouvements générés
    
    // Parcours de toutes les cases du plateau
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            // Vérification si la case contient une pièce du joueur
            if (get_player(game->board[i][j]) == player) {
                
                // Exploration vers le bas (direction positive i)
                int k = 1;
                while ((i + k) < 9 && (get_player(game->board[i + k][j]) == NOT_PLAYER)) {
                    Move current_move = {i, j, i + k, j, -1};
                    list[size++] = current_move;
                    k++;
                }

                // Exploration vers le haut (direction négative i)
                k = 1;
                while ((i - k) >= 0 && (get_player(game->board[i - k][j]) == NOT_PLAYER)) {
                    Move current_move = {i, j, i - k, j, -1};
                    list[size++] = current_move;
                    k++;
                }

                // Exploration vers la droite (direction positive j)
                k = 1;
                while ((j + k) < 9 && (get_player(game->board[i][j + k]) == NOT_PLAYER)) {
                    Move current_move = {i, j, i, j + k, -1};
                    list[size++] = current_move;
                    k++;
                }

                // Exploration vers la gauche (direction négative j)
                k = 1;
                while ((j - k) >= 0 && (get_player(game->board[i][j - k]) == NOT_PLAYER)) {
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
 * @brief Fonction de comparaison pour le tri des mouvements
 * 
 * Cette fonction compare deux mouvements scorés pour le tri par ordre décroissant.
 * Utilisée par qsort() pour ordonner les mouvements du plus prometteur au moins prometteur.
 * 
 * @param a Pointeur vers le premier mouvement scoré
 * @param b Pointeur vers le second mouvement scoré
 * @return int Valeur de comparaison pour le tri décroissant
 */
int compare_moves_desc(const void *a, const void *b) {
    ScoredMove *m1 = (ScoredMove*)a;
    ScoredMove *m2 = (ScoredMove*)b;
    // Tri par score décroissant : les meilleurs scores en premier
    return m2->score - m1->score;
}

/**
 * @brief Génère tous les mouvements possibles pour un joueur et les trie par score
 * 
 * Cette fonction génère tous les mouvements légaux pour un joueur donné,
 * évalue chaque mouvement avec une heuristique simple, puis trie les
 * mouvements par ordre de préférence décroissant.
 * 
 * @param game Pointeur vers la structure de jeu
 * @param move_list Tableau pour stocker les mouvements triés
 * @param player Joueur pour lequel générer les mouvements (P1 ou P2)
 * @return int Nombre de mouvements générés et triés
 */
int all_possible_moves_ordered(Game *game, Move *move_list, Player player) {
    ScoredMove scored_moves[10*16]; // Tableau des mouvements avec scores
    int size = 0; // Compteur de mouvements générés

    // Directions de mouvement : bas, haut, droite, gauche
    int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};

    // Parcours de toutes les cases du plateau
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            // Vérification si la case contient une pièce du joueur
            if (get_player(game->board[i][j]) != player) continue;

            // Exploration des 4 directions possibles
            for (int d = 0; d < 4; d++) {
                int k = 1; // Distance de déplacement
                while (1) {
                    // Calcul de la nouvelle position
                    int ni = i + k * dirs[d][0];
                    int nj = j + k * dirs[d][1];

                    // Arrêt si hors du plateau
                    if (ni < 0 || ni >= 9 || nj < 0 || nj >= 9) break;

                    // Arrêt si la case n'est pas vide
                    if (get_player(game->board[ni][nj]) != NOT_PLAYER) break;

                    // Création et évaluation du mouvement
                    Move move = {i, j, ni, nj, -1};
                    Game temp = *game;
                    
                    // Simulation rapide du mouvement
                    temp.board[ni][nj] = temp.board[i][j];
                    temp.board[i][j] = (get_player(temp.board[i][j]) == P1) ? P1_VISITED : P2_VISITED;

                    // Évaluation simple basée sur le score des joueurs
                    // int score = score_player_two(temp) - score_player_one(temp);
                    int score = utility(&temp, player); // TEST

                    // Ajout du mouvement au tableau si il y a de la place
                    if (size < 10*16) {
                        scored_moves[size].s_move = move;
                        scored_moves[size].score = score;
                        size++;
                    }

                    k++; // Passage à la distance suivante
                }
            }
        }
    }

    // Tri des mouvements par score décroissant
    qsort(scored_moves, size, sizeof(ScoredMove), compare_moves_desc);

    // Copie des mouvements triés dans le tableau de sortie
    for (int i = 0; i < size; i++) {
        move_list[i] = scored_moves[i].s_move;
    }

    return size;
}


/**
 * @brief Algorithme minimax avec élagage alpha-bêta
 * 
 * Implémentation de l'algorithme minimax avec optimisation alpha-bêta pour
 * l'évaluation des mouvements. Cet algorithme explore l'arbre de jeu en
 * alternant entre maximisation et minimisation du score selon le joueur.
 * 
 * @param game Pointeur vers la structure de jeu à évaluer
 * @param depth Profondeur restante de recherche dans l'arbre
 * @param maximizing 1 si le joueur actuel maximise, 0 s'il minimise
 * @param alpha Valeur alpha pour l'élagage (meilleur score pour maximizing)
 * @param beta Valeur beta pour l'élagage (meilleur score pour minimizing)
 * @param initial_player Joueur pour lequel on évalue la position
 * @return int Score de la position évaluée
 */
int minimax_alpha_beta(Game * game, int depth, int maximizing, int alpha, int beta, Player initial_player) {
    // Condition d'arrêt : profondeur atteinte ou jeu terminé
    if (depth == 0 || game->won != NOT_PLAYER) {
        return utility(game, initial_player);
    }
    
    // Détermination du joueur actuel
    Player current_player = ( (game->turn & 1) == 1) ? P2 : P1;

    // Génération de tous les mouvements possibles pour le joueur actuel
    Move possible_moves[10 * 16]; // Maximum théorique : 10 pièces × 16 mouvements chacune
    int size = all_possible_moves(game, possible_moves, current_player);

    if (maximizing) {
        int best_score = -100001; // Initialisation à -∞
        
        // Exploration de tous les mouvements possibles
        for (int i = 0; i < size; i++) {
            Move current_move = possible_moves[i];
            
            // Application du mouvement et sauvegarde pour l'annulation
            game->selected_tile[0] = current_move.src_row;
            game->selected_tile[1] = current_move.src_col;
            UndoInfo undo_info = update_board_ai(game, current_move.dst_row, current_move.dst_col);

            // Évaluation récursive du mouvement
            int current_score = minimax_alpha_beta(game, depth - 1, !maximizing, alpha, beta, initial_player);
            undo_board_ai(game, undo_info);

            // Mise à jour du meilleur score et élagage alpha-bêta
            best_score = (current_score >= best_score) ? current_score : best_score;
            alpha = (alpha > best_score) ? alpha : best_score;
            if (beta <= alpha) {
                break;
            }
        }
        return best_score;

    } else {
        int best_score = 100001; // Initialisation à +∞ 
        
        // Exploration de tous les mouvements possibles
        for (int i = 0; i < size; i++) {
            Move current_move = possible_moves[i];
            
            // Application du mouvement et sauvegarde pour l'annulation
            game->selected_tile[0] = current_move.src_row;
            game->selected_tile[1] = current_move.src_col;
            UndoInfo undo_info = update_board_ai(game, current_move.dst_row, current_move.dst_col);

            // Évaluation récursive du mouvement
            int current_score = minimax_alpha_beta(game, depth - 1, !maximizing, alpha, beta, initial_player);
            undo_board_ai(game, undo_info);

            // Mise à jour du meilleur score et élagage alpha-bêta
            best_score = (current_score <= best_score) ? current_score : best_score;
            beta = (beta < best_score) ? beta : best_score;
            if (beta <= alpha) {
                break; // Élagage : pas besoin d'explorer plus
            }
        }
        return best_score;
    }
}

/**
 * @brief Trouve le meilleur mouvement en utilisant l'algorithme minimax avec élagage alpha-bêta
 * 
 * Cette fonction est le point d'entrée principal de l'IA. Elle génère tous les
 * mouvements possibles, les évalue en utilisant l'algorithme minimax, et retourne
 * le mouvement avec le meilleur score.
 * 
 * @param game Pointeur vers la structure de jeu
 * @param depth Profondeur maximale de recherche dans l'arbre de jeu
 * @return Move Le meilleur mouvement trouvé par l'algorithme
 */
Move minimax_best_move(Game * game, int depth) {
    // Détermination du joueur actuel
    Player current_player = ( (game->turn & 1) == 0) ? P1 : P2;

    // Génération et tri des mouvements possibles
    Move possible_moves[10 * 16]; // Capacité maximale théorique
    int size = all_possible_moves_ordered(game, possible_moves, current_player);
    
    // Initialisation des variables de recherche
    int best_score = -100001; // Score initial très bas
    Move best_move = {-1, -1, -1, -1, -10001}; // Mouvement par défaut invalide

    // Évaluation de chaque mouvement possible
    for (int i = 0; i < size; i++) {
        Move current_move = possible_moves[i];

        // Application du mouvement et sauvegarde de l'état
        game->selected_tile[0] = current_move.src_row;
        game->selected_tile[1] = current_move.src_col;
        UndoInfo undo_info = update_board_ai(game, current_move.dst_row, current_move.dst_col);

        // Évaluation du mouvement avec minimax
        int current_score = minimax_alpha_beta(game, depth, 1, -100000, 100000, current_player);
        undo_board_ai(game, undo_info);

        // Mise à jour du meilleur mouvement si nécessaire
        if (current_score > best_score) {
            best_move = possible_moves[i];
            best_score = current_score;
        }
    }

    // Affichage du résultat pour le débogage
    LOG_INFO_MSG("[IA] Best score: %d, Player 2: %d", best_score, (game->turn & 1) == 1);
    return best_move;
}

/**
 * @brief Exécute un premier mouvement prédéfini pour l'IA
 * 
 * Cette fonction fait jouer un mouvement d'ouverture fixe à l'IA.
 * Utilisée dans certains modes de jeu où l'IA doit commencer avec
 * un mouvement prédéterminé.
 * 
 * @param game Pointeur vers la structure de jeu à modifier
 */
void client_first_move(Game * game) {
    // Mouvement d'ouverture prédéfini : déplacement de (2,2) vers (4,2)
    Move first_move = {2, 2, 4, 2, -1};

    // Application du mouvement
    game->selected_tile[0] = first_move.src_row;
    game->selected_tile[1] = first_move.src_col;
    update_board(game, first_move.dst_row, first_move.dst_col);
}


/**
 * @brief Fonction principale pour que l'IA joue son prochain mouvement
 * 
 * Cette fonction est appelée quand c'est au tour de l'IA de jouer.
 * Elle utilise l'algorithme minimax pour déterminer le meilleur mouvement
 * possible et l'applique au jeu.
 * 
 * @param game Pointeur vers la structure de jeu à modifier
 */
void ai_next_move(Game* game) {
    // Création d'une copie pour éviter les effets de bord
    Game copy = *game;
    copy.is_ai = 0; // Configuration pour éviter la récursion infinie
    
    // Calcul du meilleur mouvement avec la profondeur configurée
    Move best_move = minimax_best_move(&copy, DEPTH);

    // Application du mouvement choisi au jeu réel
    game->selected_tile[0] = best_move.src_row;
    game->selected_tile[1] = best_move.src_col;
    update_board(game, best_move.dst_row, best_move.dst_col);
}

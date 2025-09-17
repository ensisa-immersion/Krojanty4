/**
 * @file game.c
 * @brief Implémentation de la logique de jeu principale
 * 
 * Ce fichier contient toutes les fonctions liées à la logique de jeu, incluant :
 * - L'initialisation d'une nouvelle partie
 * - La gestion des déplacements et validation des coups
 * - Le calcul des scores des joueurs
 * - La détection et application des captures
 * - La vérification des conditions de victoire
 * - La gestion des tours et de l'IA
 * 
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 */

#include <math.h>

#include "game.h"
#include "display.h"
#include "algo.h"
#include "input.h"
#include "const.h"
#include "algo.h"

/**
 * @brief Initialise une nouvelle partie avec le mode et l'IA spécifiés
 * 
 * Cette fonction configure l'état initial du jeu en copiant le plateau
 * de départ depuis les constantes, en initialisant les scores, et en
 * configurant les paramètres de mode de jeu et d'intelligence artificielle.
 * 
 * @param mode Le mode de jeu (LOCAL, SERVER, CLIENT)
 * @param artificial_intelligence 1 si l'IA est activée, 0 sinon
 * @return Game Structure de jeu entièrement initialisée
 */
Game init_game(GameMode mode, int artificial_intelligence) {
    (void)mode;                    // évite warnings si pas utilisé partout
    (void)artificial_intelligence; // idem

    Game game;

    // Initialisation de l'état de victoire et du compteur de tours
    game.won = 0;
    game.turn = 0;

    // Copie du plateau de départ défini dans const.h
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            game.board[i][j] = STARTING_BOARD[i][j];
        }
    }

    // Aucune case sélectionnée au démarrage
    game.selected_tile[0] = -1;
    game.selected_tile[1] = -1;

    // Configuration du mode de jeu et de l'IA
    game.game_mode = mode;
    game.is_ai = artificial_intelligence ? 1 : 0;

    return game;
}

/**
 * @brief Calcule le score du joueur 1 (Bleu)
 * 
 * Le score est calculé selon les règles suivantes :
 * - +1 point par case visitée (P1_VISITED)
 * - +2 points par pièce encore en vie (pions et roi)
 * 
 * @param game Structure de jeu contenant l'état actuel du plateau
 * @return int Score total du joueur 1
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
 * @brief Calcule le score du joueur 2 (Rouge)
 * 
 * Le score est calculé selon les règles suivantes :
 * - +1 point par case visitée (P2_VISITED)
 * - +2 points par pièce encore en vie (pions et roi)
 * 
 * @param game Structure de jeu contenant l'état actuel du plateau
 * @return int Score total du joueur 2
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
 * @brief Vérifie si un déplacement est légal selon les règles du jeu
 * 
 * Les conditions vérifiées sont :
 * - Les coordonnées source et destination sont dans les limites du plateau
 * - Une pièce du joueur actuel existe à la position source
 * - La destination est vide (pas de pièce)
 * - Le déplacement est en ligne droite (horizontal ou vertical uniquement)
 * - Le chemin entre source et destination n'est pas bloqué par d'autres pièces
 * - Le joueur déplace bien sa propre pièce
 * 
 * @param game Pointeur vers la structure de jeu
 * @param src_row Ligne source (0 à GRID_SIZE-1)
 * @param src_col Colonne source (0 à GRID_SIZE-1)
 * @param dst_row Ligne destination (0 à GRID_SIZE-1)
 * @param dst_col Colonne destination (0 à GRID_SIZE-1)
 * @return int 1 si le déplacement est légal, 0 sinon
 */
int is_move_legal(Game *game, int src_row, int src_col, int dst_row, int dst_col) {
    // Vérification des limites du plateau pour source et destination
    if (src_row < 0 || src_row >= GRID_SIZE || src_col < 0 || src_col >= GRID_SIZE) return 0;
    if (dst_row < 0 || dst_row >= GRID_SIZE || dst_col < 0 || dst_col >= GRID_SIZE) return 0;

    // Vérification qu'une pièce existe à la position source
    if (get_player(game->board[src_row][src_col]) == NOT_PLAYER) return 0;

    // Vérification que le déplacement est en ligne droite uniquement
    if (src_row != dst_row && src_col != dst_col) return 0;

    // Vérification que la destination est libre
    if (get_player(game->board[dst_row][dst_col]) != NOT_PLAYER) return 0;

    // Vérification que le joueur déplace bien sa propre pièce
    if ((current_player_turn(game) == P1) &&
        (game->board[src_row][src_col] == P2_PAWN || game->board[src_row][src_col] == P2_KING)) return 0;
    if ((current_player_turn(game) == P2) &&
        (game->board[src_row][src_col] == P1_PAWN || game->board[src_row][src_col] == P1_KING)) return 0;

    // Vérification du chemin libre pour déplacement horizontal
    if (src_row == dst_row) {
        int step = (dst_col > src_col) ? 1 : -1;
        for (int c = src_col + step; c != dst_col; c += step) {
            if (get_player(game->board[src_row][c]) != NOT_PLAYER) return 0; // chemin bloqué
        }
    }

    // Vérification du chemin libre pour déplacement vertical
    if (src_col == dst_col) {
        int step = (dst_row > src_row) ? 1 : -1;
        for (int r = src_row + step; r != dst_row; r += step) {
            if (get_player(game->board[r][src_col]) != NOT_PLAYER) return 0; // chemin bloqué
        }
    }

    return 1; // Déplacement légal
}

/**
 * @brief Détermine le joueur propriétaire d'une pièce donnée
 * 
 * Cette fonction analyse le type de pièce et retourne le joueur
 * correspondant ou NOT_PLAYER si la case est vide ou contient
 * une case visitée.
 * 
 * @param piece Type de pièce à analyser
 * @return Player P1, P2 ou NOT_PLAYER selon le type de pièce
 */
Player get_player(Piece piece) {
    if (piece == P1_PAWN || piece == P1_KING) return P1;
    if (piece == P2_PAWN || piece == P2_KING) return P2;
    return NOT_PLAYER;
}

/**
 * @brief Vérifie et effectue les captures après un déplacement
 * 
 * Cette fonction implémente la logique de capture du jeu :
 * - Capture par "sprint" : quand on se déplace vers un adversaire
 *   sans défenseur derrière lui dans la direction du mouvement
 * - Capture par "sandwich" : quand un adversaire est pris entre
 *   deux pièces alliées après le déplacement
 * 
 * @param game Pointeur vers la structure de jeu
 * @param row Ligne où la pièce a été déplacée
 * @param col Colonne où la pièce a été déplacée
 * @param sprint_direction Direction du déplacement (DIR_TOP, DIR_DOWN, DIR_LEFT, DIR_RIGHT)
 * @return void
 */
void did_eat(Game* game, int row, int col, Direction sprint_direction) {
    Player player = current_player_turn(game);
    Player opponent = (player == P1) ? P2 : P1;
    
    // Analyse des joueurs dans les 4 directions adjacentes
    Player top = (row - 1 >= 0)? get_player(game->board[row - 1][col]) : NOT_PLAYER;
    Player left = (col - 1 >= 0)? get_player(game->board[row][col - 1]) : NOT_PLAYER;
    Player right = (col + 1 < GRID_SIZE)? get_player(game->board[row][col + 1]) : NOT_PLAYER;
    Player down = (row + 1 < GRID_SIZE)? get_player(game->board[row + 1][col]) : NOT_PLAYER;

    // Vérification capture vers le haut
    if (top == opponent) {
        if ( ((row - 2 < 0 || get_player(game->board[row - 2][col]) != opponent) && sprint_direction == DIR_TOP ) ||
              (get_player(game->board[row - 2][col]) == player && row - 2 >= 0) )  {
            game->board[row - 1][col] = P_NONE;
        }
    }

    // Vérification capture vers la gauche
    if (left == opponent) {
        if ( ((col - 2 < 0 || get_player(game->board[row][col - 2]) != opponent) && sprint_direction == DIR_LEFT ) ||
              (get_player(game->board[row][col - 2]) == player && col - 2 >= 0) ) {
            game->board[row][col - 1] = P_NONE;
        }
    }

    // Vérification capture vers la droite
    if (right == opponent) {
        if ( ((col + 2 >= GRID_SIZE || get_player(game->board[row][col + 2]) != opponent) && sprint_direction == DIR_RIGHT ) ||
              (get_player(game->board[row][col + 2]) == player && col + 2 < GRID_SIZE) ) {
            game->board[row][col + 1] = P_NONE;
        }
    }

    // Vérification capture vers le bas
    if (down == opponent) {
        if ( ((row + 2 >= GRID_SIZE || get_player(game->board[row + 2][col]) != opponent) && sprint_direction == DIR_DOWN ) ||
              (get_player(game->board[row + 2][col]) == player && row + 2 < GRID_SIZE) ) {
            game->board[row + 1][col] = P_NONE;
        }
    }
}

/**
 * @brief Vérifie les conditions de victoire et met à jour l'état du jeu
 * 
 * Cette fonction vérifie plusieurs conditions de victoire dans l'ordre :
 * 1. Victoire par objectif : roi P1 atteint coin bas-droit, roi P2 atteint coin haut-gauche
 * 2. Victoire par élimination : un des rois est capturé
 * 3. Victoire par domination : un joueur n'a plus que 2 pièces (roi + 1 soldat)
 * 4. Victoire par score : après 63 tours, le joueur avec le meilleur score gagne
 * 
 * @param game Pointeur vers la structure de jeu
 * @return void
 */
void won(Game* game) {
    // Vérification victoire par objectif (atteindre le coin opposé)
    if (game->board[GRID_SIZE-1][GRID_SIZE-1] == P1_KING && game->won == NOT_PLAYER) {
        game->won = P1;
    } else if (game->board[0][0] == P2_KING && game->won == NOT_PLAYER) {
        game->won = P2;
    }

    // Vérification de la survie des rois
    int is_blue_king_alive = 0;
    int is_red_king_alive = 0;
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (game->board[i][j] == P1_KING) is_blue_king_alive++;
            if (game->board[i][j] == P2_KING) is_red_king_alive++;
        }
    }

    // Victoire par élimination du roi adverse
    if ((!is_blue_king_alive || !is_red_king_alive) && game->won == NOT_PLAYER) {
        if (!is_blue_king_alive) {
            game->won=P2;
        } else if (!is_red_king_alive) {
            game->won=P1;
        } else {
            game->won=NOT_PLAYER;
        }
    }

    // Victoire par domination (adversaire réduit à 2 pièces)
    if (game->won == NOT_PLAYER) {
        int p1_piece = 0;
        int p2_piece = 0;

        // Comptage des pièces restantes pour chaque joueur
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

        // Victoire si l'adversaire n'a plus que roi + 1 soldat
        if (p1_piece <= 2) {
            game->won = P2;
        } else if (p2_piece <= 2) {
            game->won = P1;
        }
    }

    // Victoire par score après 63 tours
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
 * @brief Met à jour le plateau de jeu pour le mode LAN
 * 
 * Dans le mode réseau, cette fonction ne fait aucune action automatique
 * car la synchronisation des coups se fait via les communications réseau
 * entre les clients et le serveur.
 * 
 * @param game Pointeur vers la structure de jeu (non utilisé)
 * @return int 1 pour indiquer le succès (toujours)
 */
int update_board_lan(Game* game) {
    // Pas de coups automatiques - synchronisation via réseau
    (void)game; // Éviter le warning de paramètre non utilisé
    return 1;
}

/**
 * @brief Met à jour le plateau après validation et exécution d'un déplacement
 * 
 * Cette fonction coordonne toute la logique d'un tour de jeu :
 * 1. Vérifie qu'une pièce est sélectionnée
 * 2. Valide la légalité du déplacement
 * 3. Effectue le déplacement et marque la case source comme visitée
 * 4. Détermine la direction du mouvement pour les captures
 * 5. Applique les règles de capture
 * 6. Vérifie les conditions de victoire
 * 7. Avance le compteur de tours
 * 8. Déclenche le tour de l'IA si nécessaire
 * 
 * @param game Pointeur vers la structure de jeu
 * @param dst_row Ligne de destination du déplacement
 * @param dst_col Colonne de destination du déplacement
 * @return void
 */
void update_board(Game *game, int dst_row, int dst_col) {
    int src_row = game->selected_tile[0];
    int src_col = game->selected_tile[1];

    // Aucune pièce sélectionnée, rien à faire
    if (src_row < 0 || src_col < 0) return;

    // Validation et exécution du déplacement
    if (is_move_legal(game, src_row, src_col, dst_row, dst_col)) {
        // Déplacement de la pièce et marquage de la case source
        game->board[dst_row][dst_col] = game->board[src_row][src_col];
        game->board[src_row][src_col] = (get_player(game->board[src_row][src_col]) == P1) ? P1_VISITED : P2_VISITED;

        // Détermination de la direction du mouvement pour les captures
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
        
        // Application des règles de capture
        did_eat(game, dst_row, dst_col, direction);

        // Vérification des conditions de victoire
        won(game);

        // Avancement du tour et reset de la sélection
        game->turn++;
        game->selected_tile[0] = -1;
        game->selected_tile[1] = -1;

        // Mise à jour pour le mode réseau
        int next_move_status __attribute__((unused)) = update_board_lan(game);
        
        // Déclenchement du tour de l'IA si nécessaire
        check_ai_turn(game);
    }
}

/**
 * @brief Détermine quel joueur doit jouer au tour actuel
 * 
 * Le joueur 1 (Bleu) joue aux tours pairs (0, 2, 4, ...)
 * Le joueur 2 (Rouge) joue aux tours impairs (1, 3, 5, ...)
 * 
 * @param game Pointeur vers la structure de jeu
 * @return Player P1 pour les tours pairs, P2 pour les tours impairs
 */
Player current_player_turn(Game *game) {
    return (game->turn % 2 == 0) ? P1 : P2;
}

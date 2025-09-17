/**
 * @file algo.h
 * @brief Algorithmes et structures pour l'IA du jeu
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 * 
 * Ce fichier contient les algorithmes et structures pour l'IA, incluant :
 * - Les structures de données pour les coups
 * - Les fonctions d'évaluation
 * - Les algorithmes minimax
 * - L'API pour l'IA
 * - Les fonctions de génération de coups
 */

#ifndef ALGO_H_INCLUDED
#define ALGO_H_INCLUDED

#include "game.h"

/**
 * @brief Structure représentant un coup de jeu
 * 
 * Contient les coordonnées source et destination pour un mouvement de pièce,
 * ainsi qu'un score associé pour l'évaluation du coup.
 */
typedef struct {
    int src_row;    ///< Coordonnée de ligne source
    int src_col;    ///< Coordonnée de colonne source
    int dst_row;    ///< Coordonnée de ligne destination
    int dst_col;    ///< Coordonnée de colonne destination
    int score;      ///< Score associé à ce coup
} Move;

/**
 * @brief Structure représentant une pièce capturée
 * 
 * Stocke la position et le type d'une pièce qui a été mangée/capturée
 * pendant le jeu.
 */
typedef struct {
    int row, col;   ///< Position où la pièce a été capturée
    int piece;      ///< Type/valeur de la pièce capturée
} EatenPiece;

/**
 * @brief Structure combinant un coup avec son score d'évaluation
 * 
 * Utilisée pour l'ordonnancement et l'évaluation des coups dans les algorithmes d'IA.
 */
typedef struct {
    Move s_move;    ///< Le coup de jeu
    int score;      ///< Score d'évaluation pour ce coup
} ScoredMove;

// Fonctions essentielles qui décrivent l'état du jeu
int utility(Game * game, Player player);
int all_possible_moves(Game * game, Move * move_list, Player player);
int all_possible_moves_ordered(Game *game, Move * move_list, Player player);

// Fonctions de calcul IA
int minimax_alpha_beta(Game * game, int depth, int maximizing, int alpha, int beta, Player initial_player);
Move minimax_best_move(Game * game, int depth);

// API pour game.c et main.c
void client_first_move(Game * game);
void ai_next_move(Game* game);

#endif // ALGO_H_INCLUDED

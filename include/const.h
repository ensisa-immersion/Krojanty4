/**
 * @file const.h
 * @brief Constantes globales pour le jeu
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 * 
 * Ce fichier contient les constantes globales du jeu, incluant :
 * - Les dimensions du plateau de jeu
 * - Les constantes d'affichage
 * - Les paramètres de l'IA
 * - Les constantes de logging et réseau
 * - Le plateau de départ standard
 */


#ifndef IMMERSION_CONST_H
#define IMMERSION_CONST_H

#include "game.h"

#define GRID_SIZE 9

// Constantes d'affichages
#define CELL_SIZE 40
#define MAX_POSSIBLE_MOVES 64

// Constantes de l'IA
#define DEPTH 4
#define DEPTH_ENDGAME 3  // Profondeur plus élevée en fin de partie
#define ENDGAME_PIECE_THRESHOLD 3  // Seuil pour considérer comme fin de partie

// Constantes de logging
#define MAX_FILENAME_LEN 256
#define MAX_LOG_MESSAGE_LEN 1024
#define MAX_LOG_FILES 1000

// Constantes réseau
#define BUFFER_SIZE 1024
#define DEFAULT_PORT 5555

// Plateau de départ standard pour le jeu
static const Piece STARTING_BOARD[GRID_SIZE][GRID_SIZE] = {
    {0, 0, 1, 1, 0, 0, 0, 0, 0},
    {0, 3, 1, 1, 0, 0, 0, 0, 0},
    {1, 1, 1, 0, 0, 0, 0, 0, 0},
    {1, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 2, 2},
    {0, 0, 0, 0, 0, 0, 2, 2, 2},
    {0, 0, 0, 0, 0, 2, 2, 4, 0},
    {0, 0, 0, 0, 0, 2, 2, 0, 0},
};

#endif //IMMERSION_CONST_H

#ifndef IMMERSION_CONST_H
#define IMMERSION_CONST_H

#include "game.h"

#define GRID_SIZE 9

// Constantes d'affichages
#define CELL_SIZE 40
#define MAX_POSSIBLE_MOVES 64

// Constantes de l'IA
#define DEPTH 3

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

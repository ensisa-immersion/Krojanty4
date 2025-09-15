#ifndef IMMERSION_CONST_H
#define IMMERSION_CONST_H

#include "game.h"

#define GRID_SIZE 9

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

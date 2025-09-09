#ifndef GAME_H
#define GAME_H

struct user_move_s
{
    int delta_x; // changement en x (ligne)
    int delta_y; // changement en y (colonne)
};
typedef struct user_move_s user_move;

int coord_valide(char col, char row);
int entree_valide(const char *buffer);
user_move parse_move(const char *buffer);
int deplacement_diagonale_interdit(user_move move);

#endif // CLIENT_H

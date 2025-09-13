#ifndef MOVE_UTIL_H
#define MOVE_UTIL_H

#include <gtk/gtk.h>
#include "game.h"

typedef struct {
    Game *game;
    int sr, sc, dr, dc;
} MoveTask;

extern const char COLS_MAP[10];

int col_from_letter(char L);

/* Exécuté dans le thread GTK (via g_idle_add) */
gboolean apply_move_idle(gpointer data);

void post_move_to_gtk(Game *game, const char m[4]);

#endif 

#ifndef MOVE_UTIL_H
#define MOVE_UTIL_H

#include <gtk/gtk.h>
#include "game.h"

typedef struct {
    Game *game;
    int sr, sc, dr, dc;
} MoveTask;

/* Mappage des colonnes A-I */
extern const char COLS_MAP[10];

/* Convertit une lettre de colonne en indice (0..8) */
int col_from_letter(char L);

/* Exécuté dans le thread GTK (via g_idle_add) */
gboolean apply_move_idle(gpointer data);

/* Convertit "A2B2" → indices et poste vers le thread GTK */
void post_move_to_gtk(Game *game, const char m[4]);

#endif /* MOVE_UTIL_H */

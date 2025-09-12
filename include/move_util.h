#ifndef MOVE_UTIL_H
#define MOVE_UTIL_H

#include <gtk/gtk.h>
#include "game.h"
#include "display.h"

/* Mappage des colonnes sans 'I' (Shōgi-like) */
static const char COLS_MAP[10] = "ABCDEFGHJ";

static inline int col_from_letter(char L) {
    for (int i = 0; i < 9; i++) if (COLS_MAP[i] == L) return i;
    return -1;
}

typedef struct {
    Game *game;
    int sr, sc, dr, dc;
} MoveTask;

/* Exécuté dans le thread GTK (via g_idle_add) */
static inline gboolean apply_move_idle(gpointer data) {
    MoveTask *t = (MoveTask*)data;
    /* 1) positionne la case source (si ton update_board lit selected_tile) */
    t->game->selected_tile[0] = t->sr;
    t->game->selected_tile[1] = t->sc;
    /* 2) applique la destination */
    update_board(t->game, t->dr, t->dc);
    /* 3) redessine */
    display_request_redraw();
    g_free(t);
    return G_SOURCE_REMOVE;
}

/* Convertit "A2B2" → indices (0..8) et poste vers le thread GTK */
void post_move_to_gtk(Game *game, const char *move4);
//     int sc = col_from_letter(m[0]);
//     int sr = (m[1] - '1');
//     int dc = col_from_letter(m[2]);
//     int dr = (m[3] - '1');

//     if (sc < 0 || dc < 0 || sr < 0 || sr > 8 || dr < 0 || dr > 8) {
//         g_warning("[RX] Mouvement invalide: %c%c%c%c", m[0],m[1],m[2],m[3]);
//         return;
//     }

//     MoveTask *t = g_new0(MoveTask, 1);
//     t->game = game; t->sr = sr; t->sc = sc; t->dr = dr; t->dc = dc;
//     g_idle_add(apply_move_idle, t);
// }

#endif /* MOVE_UTIL_H */

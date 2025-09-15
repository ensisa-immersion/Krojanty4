#include "move_util.h"
#include "display.h"
#include "game.h"
#include <gtk/gtk.h>

/* Mappage des colonnes A-I */
const char COLS_MAP[10] = "ABCDEFGHI";

/**
 * Convertit une lettre de colonne (A-I) en indice (0-8).
 * Retourne -1 si la lettre est invalide.
 */
int col_from_letter(char L) {
    for (int i = 0; i < 9; i++) if (COLS_MAP[i] == L) return i;
    return -1;
}

/**
 * Applique un mouvement dans le thread GTK.
 * 
 * @param data Pointeur vers une structure MoveTask.
 * @return G_SOURCE_REMOVE pour indiquer que la tâche est terminée.
 */
gboolean apply_move_idle(gpointer data) {
    MoveTask *t = (MoveTask*)data;
    /* 1) positionne la case source*/
    t->game->selected_tile[0] = t->sr;
    t->game->selected_tile[1] = t->sc;
    /* 2) applique la destination */
    update_board(t->game, t->dr, t->dc);
    /* 3) redessine */
    display_request_redraw();
    g_free(t);
    return G_SOURCE_REMOVE;
}

/**
 * Poste un mouvement reçu (format "A1B2") pour qu'il soit appliqué dans le thread GTK.
 * @param game Pointeur vers la structure de jeu.
 * @param m Chaîne de caractères représentant le mouvement (4 caractères).
 * @return void
 */
void post_move_to_gtk(Game *game, const char m[4]) {
    int sc = col_from_letter(m[0]);
    int sr = 9 - (m[1] - '0');
    int dc = col_from_letter(m[2]);
    int dr = 9 - (m[3] - '0');

    if (sc < 0 || dc < 0 || sr < 0 || sr > 8 || dr < 0 || dr > 8) {
        g_warning("[RX] Mouvement invalide: %c%c%c%c", m[0],m[1],m[2],m[3]);
        return;
    }

    MoveTask *t = g_new0(MoveTask, 1);
    t->game = game; t->sr = sr; t->sc = sc; t->dr = dr; t->dc = dc;
    g_idle_add(apply_move_idle, t);
}

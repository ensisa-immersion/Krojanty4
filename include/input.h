#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED

#include "game.h"

static void on_click(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data);

/* fonction pour traiter un mouvement décidé par l'utilisateur */
void on_user_move_decided(Game *game, int src_r, int src_c, int dst_r, int dst_c);

/* branchement du contrôleur */
void detect_click(GtkWidget *drawing_area, Game *game);

#endif // INPUT_H_INCLUDED

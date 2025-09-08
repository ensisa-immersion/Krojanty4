#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED

#include "game.h"

static void on_click(GtkGestureClick *gesture,
                     int n_press,
                     double x, double y,
                     gpointer user_data);

void detect_click(GtkWidget *drawing_area, Game *game);

#endif // INPUT_H_INCLUDED

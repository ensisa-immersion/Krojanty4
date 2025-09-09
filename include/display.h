#ifndef DISPLAY_H_INCLUDED
#define DISPLAY_H_INCLUDED

#include <gtk/gtk.h>
#include "game.h"

#define GRID_SIZE 9
#define CELL_SIZE 40

void draw_callback(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data);
void activate(GtkApplication *app, gpointer user_data);
int initialize_display(int argc, char **argv, Game *game);

#endif // DISPLAY_H_INCLUDED

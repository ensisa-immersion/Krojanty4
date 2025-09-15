#ifndef DISPLAY_H_INCLUDED
#define DISPLAY_H_INCLUDED

#include <gtk/gtk.h>

#include "game.h"
#include "const.h"

void draw_callback(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data);
void activate (GtkApplication *app, gpointer user_data);
int initialize_display(int argc, char** argv, Game* game);

/* redraw depuis thread réseau */
void display_request_redraw(void);

#endif // DISPLAY_H_INCLUDED

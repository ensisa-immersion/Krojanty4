#include <gtk/gtk.h>

#include "../include/display.h"
#include "../include/game.h"
#include "../include/input.h"

void draw_callback(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data) {
    Game* game = (Game*) user_data;

    const int grid_width = GRID_SIZE * CELL_SIZE;
    const int grid_height = GRID_SIZE * CELL_SIZE;

    // Center the grid
    int start_x = (width - grid_width) / 2;
    int start_y = (height - grid_height) / 2;

    // Background
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    // Grid drawing
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            int tile = game->board[i][j];

            // Chooses color according to player on tile
            switch (tile) {
            case 1:
                cairo_set_source_rgb(cr, 0.1, 0.7, 0.8); // Blue for player 1 pawns
                break;
            case 2:
                cairo_set_source_rgb(cr, 0.9, 0.2, 0); // Red for player 2 pawns
                break;
            case 4:
                cairo_set_source_rgb(cr, 0.1, 0.4, 0.5); // Dark blue for player 1's king
                break;
            case 5:
                cairo_set_source_rgb(cr, 0.5, 0.2, 0.1); // Dark red for player 2's king
                break;
            default:
                cairo_set_source_rgb(cr, 0.9, 0.9, 0.9); // Grey by default
                break;
            }

            // Chooses color for house tiles
            if (i + j == 0) {
                cairo_set_source_rgb(cr, 0, 0, 1);
            } else if (i + j == 16) {
                cairo_set_source_rgb(cr, 1, 0, 0);
            }

            cairo_rectangle(cr, start_x + i * CELL_SIZE, start_y + j * CELL_SIZE, CELL_SIZE, CELL_SIZE); // Creer le rectangle
            cairo_fill_preserve(cr); // Colorier rectangle

            // Dessiner contour du rectangle
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_set_line_width(cr, 1.5f);
            cairo_stroke(cr);
        }
    }
}


void activate (GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *frame;

    // Gets the game struct from the data slot that gtk offers (on le passe au black quoi)
    Game* game = (Game*) user_data;

    // Makes a window
    window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), "Krojanty");
    gtk_window_set_default_size (GTK_WINDOW (window), 800, 500);

    // Creates drawing space
    frame = gtk_drawing_area_new();
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(frame), draw_callback, game, NULL);

    // Starts listening to user input
    detect_click(frame, game); // Should be in main.c in the futur

    gtk_window_set_child(GTK_WINDOW(window), frame);
    gtk_window_present (GTK_WINDOW (window));
}


// From what I understood this function initializes the display by calling activate
int initialize_display(int argc, char** argv, Game* game) {
    GtkApplication *app;
    int status;

    // Remet les valeurs des paramètres arguments à 0
    argc = 0;
    argv = NULL;

    app = gtk_application_new ("krojanty.grp4", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect (app, "activate", G_CALLBACK (activate), game);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;
}

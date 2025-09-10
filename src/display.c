#include <gtk/gtk.h>
#include "../include/display.h"
#include "../include/game.h"
#include "../include/input.h"

// Helper function to draw scores and messages
void draw_ui(cairo_t *cr, Game *game, int start_x, int start_y, int grid_width, int grid_height) {
    // Compute player scores
    int player_one_score = score_player_one(*game);
    int player_two_score = score_player_two(*game);

    char score_text[50];
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 20.0);
    cairo_set_source_rgb(cr, 0, 0, 0);

    // Player 1 score
    cairo_move_to(cr, start_x - 200, start_y + grid_height / 2 - 10);
    cairo_show_text(cr, "Score Joueur 1:");
    snprintf(score_text, sizeof(score_text), "%d", player_one_score);
    cairo_move_to(cr, start_x - 120, start_y + grid_height / 2 + 10);
    cairo_show_text(cr, score_text);

    // Player 2 score
    cairo_move_to(cr, start_x + grid_width + 30, start_y + grid_height / 2 - 10);
    cairo_show_text(cr, "Score Joueur 2:");
    snprintf(score_text, sizeof(score_text), "%d", player_two_score);
    cairo_move_to(cr, start_x + grid_width + 110, start_y + grid_height / 2 + 10);
    cairo_show_text(cr, score_text);

    // Win message or turn message
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 32.0);
    cairo_set_source_rgb(cr, 0, 0, 0);

    char msg[100];
    if (game->won != 0) {
        if (game->won == 2) {
            snprintf(msg, sizeof(msg), "Egalite!");
        } else if (game->won == 1) {
            if (game->turn % 2 == 0) {
                snprintf(msg, sizeof(msg), "Joueur 1 a gagner!");
            } else {
                snprintf(msg, sizeof(msg), "Joueur 2 a gagner!");
            }
        }
    } else {
        snprintf(msg, sizeof(msg), "Tour: %d", game->turn + 1);
    }

    cairo_text_extents_t extents;
    cairo_text_extents(cr, msg, &extents);

    double text_x = start_x + (grid_width - extents.width) / 2 - extents.x_bearing;
    double text_y = start_y - 20;

    cairo_move_to(cr, text_x, text_y);
    cairo_show_text(cr, msg);
    cairo_stroke(cr);
}


// Actual drawing part
void draw_callback(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data) {
    Game* game = (Game*) user_data;

    // Background
    if (game->turn % 2 == 0) {
        cairo_set_source_rgb(cr, 0.8, 0.9, 1);
    } else {
        cairo_set_source_rgb(cr, 1, 0.8, 0.8);
    }
    cairo_paint(cr);

    const int grid_width = GRID_SIZE * CELL_SIZE;
    const int grid_height = GRID_SIZE * CELL_SIZE;
    int start_x = (width - grid_width) / 2;
    int start_y = (height - grid_height) / 2;

    // Draw UI (scores + message)
    draw_ui(cr, game, start_x, start_y, grid_width, grid_height);

    // Draw grid
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            int tile = game->board[j][i];
            int visited = game->last_visited[j][i];

            if (i + j == 0) {
                cairo_set_source_rgb(cr, 0.85, 0.85, 0.90);
            } else if (i + j == 16) {
                cairo_set_source_rgb(cr, 0.90, 0.85, 0.85);
            } else {
                cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
            }

            switch (visited) {
                case P1_PAWN: cairo_set_source_rgb(cr, 0.85, 0.95, 1); break;
                case P2_PAWN: cairo_set_source_rgb(cr, 1, 0.85, 0.85); break;
                case P1_KING: cairo_set_source_rgb(cr, 0.85, 0.95, 1); break;
                case P2_KING: cairo_set_source_rgb(cr, 1, 0.85, 0.85); break;
            }

            switch (tile) {
                case P1_PAWN: cairo_set_source_rgb(cr, 0.1, 0.7, 0.8); break;
                case P2_PAWN: cairo_set_source_rgb(cr, 0.95, 0.3, 0.1); break;
                case P1_KING: cairo_set_source_rgb(cr, 0.1, 0.4, 0.5); break;
                case P2_KING: cairo_set_source_rgb(cr, 0.7, 0.2, 0.1); break;
            }

            cairo_rectangle(cr, start_x + i * CELL_SIZE, start_y + j * CELL_SIZE, CELL_SIZE, CELL_SIZE);
            cairo_fill_preserve(cr);
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
    detect_click(frame, game); // Should be in main.c in the future but flemme

    gtk_window_set_child(GTK_WINDOW(window), frame);
    gtk_window_present (GTK_WINDOW (window));
}


// From what I understood this function initializes the display by calling activate
int initialize_display(int argc, char** argv, Game* game) {
    GtkApplication *app;
    int status;

    app = gtk_application_new ("krojanty.grp4", 0);
    g_signal_connect (app, "activate", G_CALLBACK (activate), game);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;
}

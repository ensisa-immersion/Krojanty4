#include <gtk/gtk.h>

#include "display.h"
#include "game.h"
#include "input.h"

/* === Référence globale du DrawingArea (sert pour redraw thread-safe) === */
static GtkWidget *g_main_drawing_area = NULL;

/* === Variables pour gérer les clics source/destination === */
static gboolean have_source = FALSE;
static int src_r = -1, src_c = -1;

/* === Fonction pour forcer le redraw depuis n'importe quel thread === */
static gboolean force_redraw_callback(gpointer data) {
    (void)data;
    if (g_main_drawing_area) {
        gtk_widget_queue_draw(g_main_drawing_area);
        printf("[DISPLAY] Redraw forcé depuis le thread principal\n");
    }
    return G_SOURCE_REMOVE;
}

void display_request_redraw(void) {
    g_idle_add(force_redraw_callback, NULL);
}

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
    cairo_set_font_size(cr, 24.0); // Réduit de 32 à 24 pour les messages plus longs
    cairo_set_source_rgb(cr, 0, 0, 0);

    char msg[100];
    if (game->won != 0) {
        if (game->won == DRAW) {
            snprintf(msg, sizeof(msg), "Egalite!");
        } /* else if (game->won == 1) {
            if (game->turn % 2 == 0) {
                snprintf(msg, sizeof(msg), "Joueur 1 a gagner!");
            } else {
                snprintf(msg, sizeof(msg), "Joueur 2 a gagner!");
            }
        } */ 
            else if (game->won==P1) {
                snprintf(msg, sizeof(msg), "Joueur 1 (Bleu) a gagner!");
            } else if (game->won==P2) {
                snprintf(msg, sizeof(msg), "Joueur 2 (Rouge) a gagner!");
            }
    } else {
        /* Afficher le tour et qui doit jouer */
        if (game->game_mode == LOCAL) {
            snprintf(msg, sizeof(msg), "Tour: %d", game->turn + 1);
        } else {
            /* Mode réseau : indiquer qui doit jouer */
            int is_server_turn = (game->turn % 2 == 1);
            const char* current_player = is_server_turn ? "Serveur (Rouge)" : "Client (Bleu)";
            const char* your_turn = "";
            
            if ((game->game_mode == SERVER && is_server_turn) ||
                (game->game_mode == CLIENT && !is_server_turn)) {
                your_turn = " - VOTRE TOUR";
            } else {
                your_turn = " - Tour adversaire";
            }
            
            snprintf(msg, sizeof(msg), "Tour %d: %s%s", game->turn + 1, current_player, your_turn);
        }
    }

    cairo_text_extents_t extents;
    cairo_text_extents(cr, msg, &extents);

    double text_x = start_x + (grid_width - extents.width) / 2 - extents.x_bearing;
    double text_y = start_y - 20;

    cairo_move_to(cr, text_x, text_y);
    cairo_show_text(cr, msg);

    // Afficher les coordonnées de la grille
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 16.0);
    cairo_set_source_rgb(cr, 0, 0, 0);

    // Labels des colonnes (A-I) en haut
    for (int i = 0; i < GRID_SIZE; i++) {
        char col_label[2] = {'A' + i, '\0'};
        cairo_text_extents_t extents;
        cairo_text_extents(cr, col_label, &extents);
        double text_x = start_x + i * CELL_SIZE + (CELL_SIZE - extents.width) / 2 - extents.x_bearing;
        double text_y = start_y - 5;
        cairo_move_to(cr, text_x, text_y);
        cairo_show_text(cr, col_label);
    }

    // Labels des lignes (9-1) à gauche (ordre inversé)
    for (int j = 0; j < GRID_SIZE; j++) {
        char row_label[2] = {'9' - j, '\0'};
        cairo_text_extents_t extents;
        cairo_text_extents(cr, row_label, &extents);
        double text_x = start_x - 20;
        double text_y = start_y + j * CELL_SIZE + (CELL_SIZE + extents.height) / 2;
        cairo_move_to(cr, text_x, text_y);
        cairo_show_text(cr, row_label);
    }

    cairo_stroke(cr);
}


// Actual drawing part
void draw_callback(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data) {
    Game* game = (Game*) user_data;

    // Background
    if (game->turn % 2 == 0) {
        cairo_set_source_rgb(cr, 0.8, 0.9, 1); // Bleu pour client
    } else {
        cairo_set_source_rgb(cr, 1, 0.8, 0.8); // Rouge pour serveur
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

/* ------- Gestion du clic souris -------- */
static void on_mouse_click(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data) {
    (void)gesture;
    (void)n_press;
    Game *game = (Game*) user_data;

    GtkWidget *widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    int grid_width = GRID_SIZE * CELL_SIZE;
    int grid_height = GRID_SIZE * CELL_SIZE;
    int start_x = (width - grid_width) / 2;
    int start_y = (height - grid_height) / 2;

    int col = (x - start_x) / CELL_SIZE;
    int row = (y - start_y) / CELL_SIZE;

    if (col >= 0 && col < GRID_SIZE && row >= 0 && row < GRID_SIZE) {
        if (!have_source) {
            src_r = row;
            src_c = col;
            have_source = TRUE;
            printf("[CLICK] Source sélectionnée: %d,%d\n", src_r, src_c);
        } else {
            printf("[CLICK] Destination: %d,%d\n", row, col);
            on_user_move_decided(game, src_r, src_c, row, col);
            have_source = FALSE; // reset
            src_r = -1;  // reset to use the variables
            src_c = -1;
        }
    }
}

/* ------- Création de la fenêtre GTK -------- */
static void on_app_activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *frame;
    GtkGesture *click_gesture;
    Game* game = (Game*) user_data;

    window = gtk_application_window_new(app);

    /* Titre différent selon le mode */
    const char *title;
    switch (game->game_mode) {
        case SERVER:
            title = "Krojanty - Serveur (Host)";
            break;
        case CLIENT:
            title = "Krojanty - Client";
            break;
        default:
            title = "Krojanty - Local";
            break;
    }

    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 500);

    frame = gtk_drawing_area_new();
    g_main_drawing_area = frame;   // on garde une référence globale
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(frame), draw_callback, game, NULL);

    /* Utilise GTK4 gesture controller pour les clics */
    click_gesture = gtk_gesture_click_new();
    gtk_widget_add_controller(frame, GTK_EVENT_CONTROLLER(click_gesture));
    g_signal_connect(click_gesture, "pressed", G_CALLBACK(on_mouse_click), game);

    gtk_window_set_child(GTK_WINDOW(window), frame);
    gtk_window_present(GTK_WINDOW(window));
}

int initialize_display(int argc, char** argv, Game* game) {
    const char *app_id;

    /* Différents IDs d'application selon le mode */
    switch (game->game_mode) {
        case SERVER:
            app_id = "krojanty.grp4.server";
            break;
        case CLIENT:
            app_id = "krojanty.grp4.client";
            break;
        default:
            app_id = "krojanty.grp4.local";
            break;
    }

    GtkApplication *app = gtk_application_new(app_id, 0);
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), game);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

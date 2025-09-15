#include <gtk/gtk.h>
#include "display.h"
#include "game.h"
#include "input.h"

/* Référence globale du DrawingArea (sert pour redraw thread-safe) */
static GtkWidget *g_main_drawing_area = NULL;

/* Variables pour gérer les clics source/destination */
static gboolean have_source = FALSE;
static int src_r = -1, src_c = -1;

/* Variables pour stocker les mouvements possibles */
#define MAX_POSSIBLE_MOVES 64
static int possible_moves[MAX_POSSIBLE_MOVES][2];
static int num_possible_moves = 0;

/**
 * Constantes pour la taille de la grille et des cellules
 * Ces valeurs peuvent être ajustées pour changer la taille de la grille
 * dans la fenêtre.
 */
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

/**
 * Calcule les mouvements possibles pour une pièce donnée.
 * Remplit le tableau possible_moves et met à jour num_possible_moves.
 */
static void calculate_possible_moves(Game *game, int row, int col) {
    num_possible_moves = 0;

    if (!game || row < 0 || row >= GRID_SIZE || col < 0 || col >= GRID_SIZE) {
        return;
    }

    // Vérifier toutes les positions possibles dans les 4 directions
    int directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}}; // haut, bas, gauche, droite

    for (int dir = 0; dir < 4; dir++) {
        int dr = directions[dir][0];
        int dc = directions[dir][1];

        // Explorer dans cette direction jusqu'à ce qu'on trouve un obstacle ou le bord
        for (int dist = 1; dist < GRID_SIZE; dist++) {
            int new_row = row + dr * dist;
            int new_col = col + dc * dist;

            // Vérifier les limites
            if (new_row < 0 || new_row >= GRID_SIZE || new_col < 0 || new_col >= GRID_SIZE) {
                break;
            }

            // Utiliser is_move_legal pour vérifier si ce mouvement est valide
            if (is_move_legal(game, row, col, new_row, new_col)) {
                if (num_possible_moves < MAX_POSSIBLE_MOVES) {
                    possible_moves[num_possible_moves][0] = new_row;
                    possible_moves[num_possible_moves][1] = new_col;
                    num_possible_moves++;
                }
            } else {
                // Si ce mouvement n'est pas légal, arrêter dans cette direction
                break;
            }
        }
    }
}

/**
 * Vérifie si une position donnée est dans les mouvements possibles.
 * 
 * @param row La ligne à vérifier
 * @param col La colonne à vérifier
 * @return TRUE si la position est un mouvement possible, FALSE sinon
 */
static gboolean is_possible_move(int row, int col) {
    for (int i = 0; i < num_possible_moves; i++) {
        if (possible_moves[i][0] == row && possible_moves[i][1] == col) {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * Dessine l'interface utilisateur (scores, messages, coordonnées).
 * @param cr Le contexte Cairo pour le dessin
 * @param game Le pointeur vers l'état du jeu
 * @param start_x La position x de départ de la grille
 * @param start_y La position y de départ de la grille
 * @param grid_width La largeur totale de la grille
 * @param grid_height La hauteur totale de la grille
 * @return void
 */
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
        } else if (game->won==P1) {
            snprintf(msg, sizeof(msg), "Joueur 1 (Bleu) a gagne!");
        } else if (game->won==P2) {
            snprintf(msg, sizeof(msg), "Joueur 2 (Rouge) a gagne!");
        }

        // Désactiver les interactions une fois la partie terminée
        /*
        - Have_source -> Pour désactiver la sélection de source
        - Num_possible_moves -> Pour désactiver la sélection de destination
        - Src_r, Src_c -> Pour réinitialiser la position de la source
        */
        have_source = FALSE;
        src_r = -1;
        src_c = -1;
        num_possible_moves = 0;

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


/**
 * Fonction de callback pour dessiner la grille et les pièces.
 * @param area Le DrawingArea GTK & if area useless G_GNUC_UNUSED or (void)area
 * @param cr Le contexte Cairo pour le dessin
 * @param width La largeur de la zone de dessin
 * @param height La hauteur de la zone de dessin
 * @param user_data Pointeur vers l'état du jeu (Game*)
 * @return void
 */
void draw_callback(GtkDrawingArea *area G_GNUC_UNUSED, cairo_t *cr, int width, int height, gpointer user_data) {
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

            // Couleur de base de la cellule
            if (i + j == 0) {
                cairo_set_source_rgb(cr, 0.85, 0.85, 0.90);
            } else if (i + j == 16) {
                cairo_set_source_rgb(cr, 0.90, 0.85, 0.85);
            } else {
                cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
            }

            // Couleur de la cellule visitée
            switch (tile) {
                case P1_VISITED: cairo_set_source_rgb(cr, 0.85, 0.95, 1); break;
                case P2_VISITED: cairo_set_source_rgb(cr, 1, 0.85, 0.85); break;
            }

            // Highlighting de la cellule sélectionnée
            if (have_source && src_r == j && src_c == i) {
                cairo_set_source_rgb(cr, 1.0, 1.0, 0.7); // Jaune clair
            }

            // Dessiner le rectangle de la cellule
            cairo_rectangle(cr, start_x + i * CELL_SIZE, start_y + j * CELL_SIZE, CELL_SIZE, CELL_SIZE);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_set_line_width(cr, 1.5f);
            cairo_stroke(cr);

            // Dessiner les pièces par-dessus le fond
            if (tile != P_NONE) {
                const char *symbol = NULL;

                // Détection des bases (coins uniquement)
                if ((i == 0 && j == 0) || (i == 8 && j == 8)) {
                    symbol = "🏰"; // Château seulement dans les vrais coins (A9 et I1)
                } else if (tile == P1_KING || tile == P2_KING) {
                    symbol = "♔";
                } else if (tile == P1_PAWN || tile == P2_PAWN) {
                    symbol = "♜";
                } else {
                    symbol = "⚑";
                }

                // Couleur de la pièce selon l'équipe
                switch (tile) {
                    case P1_PAWN:
                    case P1_KING:
                        cairo_set_source_rgb(cr, 0.1, 0.4, 0.8); // Bleu pour P1
                        break;
                    case P2_PAWN:
                    case P2_KING:
                        cairo_set_source_rgb(cr, 0.8, 0.1, 0.1); // Rouge pour P2
                        break;
                }

                // Afficher le symbole
                cairo_select_font_face(cr, "DejaVu Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
                cairo_set_font_size(cr, CELL_SIZE * 0.7);

                cairo_text_extents_t extents;
                cairo_text_extents(cr, symbol, &extents);
                double text_x = start_x + i * CELL_SIZE + (CELL_SIZE - extents.width) / 2 - extents.x_bearing;
                double text_y = start_y + j * CELL_SIZE + (CELL_SIZE + extents.height) / 2;

                cairo_move_to(cr, text_x, text_y);
                cairo_show_text(cr, symbol);
            }

            // Dessiner les points gris pour les mouvements possibles PAR-DESSUS les pièces
            if (have_source && is_possible_move(j, i)) {
                cairo_set_source_rgb(cr, 0.3, 0.3, 0.3); // Gris foncé pour mieux voir
                cairo_arc(cr,
                         start_x + i * CELL_SIZE + CELL_SIZE / 2,
                         start_y + j * CELL_SIZE + CELL_SIZE / 2,
                         12, 0, 2 * G_PI);
                cairo_fill(cr);

                // Contour blanc pour le contraste
                cairo_set_source_rgb(cr, 1, 1, 1);
                cairo_set_line_width(cr, 2);
                cairo_arc(cr,
                         start_x + i * CELL_SIZE + CELL_SIZE / 2,
                         start_y + j * CELL_SIZE + CELL_SIZE / 2,
                         12, 0, 2 * G_PI);
                cairo_stroke(cr);
            }
        }
    }
}

/**
 * Callback pour gérer les clics de souris.
 * Gère la sélection source/destination et appelle on_user_move_decided.
 * 
 * @param gesture Le geste de clic
 * @param n_press Le nombre de pressions (1 pour simple clic)
 * @param x La position x du clic
 * @param y La position y du clic
 * @param user_data Pointeur vers l'état du jeu (Game*)
 * @return void
 */
static void on_mouse_click(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data) {
    (void)gesture;
    (void)n_press;
    Game *game = (Game*) user_data;

    GtkWidget *widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
    // int width = gtk_widget_get_allocated_width(widget); 
    // int height = gtk_widget_get_allocated_height(widget); 
    // --- deprecated --- 
    int width = gtk_widget_get_width(widget);
    int height = gtk_widget_get_height(widget);

    int grid_width = GRID_SIZE * CELL_SIZE;
    int grid_height = GRID_SIZE * CELL_SIZE;
    int start_x = (width - grid_width) / 2;
    int start_y = (height - grid_height) / 2;

    int col = (x - start_x) / CELL_SIZE;
    int row = (y - start_y) / CELL_SIZE;

    if (col >= 0 && col < GRID_SIZE && row >= 0 && row < GRID_SIZE) {
        if (!have_source) {
            // Vérifier qu'il y a une pièce à cette position
            if (game->board[row][col] != P_NONE) {
                src_r = row;
                src_c = col;
                have_source = TRUE;

                // Calculer les mouvements possibles pour cette pièce
                calculate_possible_moves(game, row, col);

                printf("[CLICK] Source sélectionnée: %d,%d (%d mouvements possibles)\n",
                       src_r, src_c, num_possible_moves);

                // Redessiner pour afficher la sélection et les points
                gtk_widget_queue_draw(g_main_drawing_area);
            }
        } else {
            printf("[CLICK] Destination: %d,%d\n", row, col);
            on_user_move_decided(game, src_r, src_c, row, col);

            // Réinitialiser la sélection
            have_source = FALSE;
            src_r = -1;
            src_c = -1;
            num_possible_moves = 0;

            // Redessiner pour effacer la sélection et les points
            gtk_widget_queue_draw(g_main_drawing_area);
        }
    }
}

/**
 * Callback pour l'activation de l'application GTK.
 * Crée la fenêtre principale, le DrawingArea, et configure les événements.
 * 
 * @param app Le GtkApplication
 * @param user_data Pointeur vers l'état du jeu (Game*)
 * @return void
 */
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

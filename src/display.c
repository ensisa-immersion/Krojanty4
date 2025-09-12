#include <gtk/gtk.h>
#include <librsvg/rsvg.h>

#include "display.h"
#include "game.h"
#include "input.h"

/* === Référence globale du DrawingArea (sert pour redraw thread-safe) === */
static GtkWidget *g_main_drawing_area = NULL;

/* === Variables pour gérer les clics source/destination === */
static gboolean have_source = FALSE;
static int src_r = -1, src_c = -1;

/* === Variables pour stocker les mouvements possibles === */
#define MAX_POSSIBLE_MOVES 64
static int possible_moves[MAX_POSSIBLE_MOVES][2];
static int num_possible_moves = 0;

/* === SVG Handles globaux === */
static RsvgHandle *svg_cite = NULL;
static RsvgHandle *svg_couronne = NULL;
static RsvgHandle *svg_soldat = NULL;

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

/* === Initialisation des SVG depuis les ressources === */
static void init_svg_resources(void) {
    GError *error = NULL;
    GBytes *bytes;
    GResource *resource = g_resource_load("assets.gresource", &error);

    if (svg_cite) return; // Déjà initialisé

    // Charger cite.svg
    bytes = g_resource_lookup_data(resource, 
                                   "/krojanty/assets/cite.svg", 
                                   G_RESOURCE_LOOKUP_FLAGS_NONE, &error);
    if (bytes) {
        svg_cite = rsvg_handle_new_from_data(g_bytes_get_data(bytes, NULL),
                                           g_bytes_get_size(bytes), &error);
        g_bytes_unref(bytes);
    }
    if (error) {
        g_warning("Failed to load cite.svg: %s", error->message);
        g_error_free(error);
        error = NULL;
    }
    
    // Charger couronne.svg
    bytes = g_resource_lookup_data(resource, 
                                   "/krojanty/assets/couronne.svg", 
                                   G_RESOURCE_LOOKUP_FLAGS_NONE, &error);
    if (bytes) {
        svg_couronne = rsvg_handle_new_from_data(g_bytes_get_data(bytes, NULL),
                                               g_bytes_get_size(bytes), &error);
        g_bytes_unref(bytes);
    }
    if (error) {
        g_warning("Failed to load couronne.svg: %s", error->message);
        g_error_free(error);
        error = NULL;
    }
    
    // Charger soldat.svg
    bytes = g_resource_lookup_data(resource, 
                                   "/krojanty/assets/soldat.svg", 
                                   G_RESOURCE_LOOKUP_FLAGS_NONE, &error);
    if (bytes) {
        svg_soldat = rsvg_handle_new_from_data(g_bytes_get_data(bytes, NULL),
                                             g_bytes_get_size(bytes), &error);
        g_bytes_unref(bytes);
    }
    if (error) {
        g_warning("Failed to load soldat.svg: %s", error->message);
        g_error_free(error);
    }
}

/* === Fonction pour dessiner un SVG redimensionné === */
static void draw_svg_in_cell(cairo_t *cr, RsvgHandle *handle, int x, int y, int cell_size) {
    if (!handle) return;
    
    cairo_save(cr);
    
    // Déplacer au coin de la cellule
    cairo_translate(cr, x, y);
    
    // Obtenir les dimensions naturelles du SVG
    RsvgRectangle viewport = {0, 0, cell_size, cell_size};
    RsvgRectangle natural_size;
    
    if (rsvg_handle_get_intrinsic_size_in_pixels(handle, &natural_size.width, &natural_size.height)) {
        // Calculer l'échelle pour s'adapter à la cellule
        double scale_x = cell_size / natural_size.width;
        double scale_y = cell_size / natural_size.height;
        double scale = MIN(scale_x, scale_y) * 0.8; // 80% de la taille de la cellule
        
        // Centrer le SVG dans la cellule
        double offset_x = (cell_size - natural_size.width * scale) / 2;
        double offset_y = (cell_size - natural_size.height * scale) / 2;
        
        cairo_translate(cr, offset_x, offset_y);
        cairo_scale(cr, scale, scale);
    } else {
        // Fallback: utiliser directement la taille de la cellule
        cairo_scale(cr, 0.8, 0.8);
        cairo_translate(cr, cell_size * 0.1, cell_size * 0.1);
    }
    
    // Configurer pour contour noir sans remplissage
    cairo_set_source_rgba(cr, 0, 0, 0, 0); // Transparent fill
    rsvg_handle_render_document(handle, cr, &viewport, NULL);
    
    // Dessiner le contour
    cairo_set_source_rgb(cr, 0, 0, 0); // Noir
    cairo_set_line_width(cr, 1.5);
    cairo_stroke_preserve(cr);
    
    cairo_restore(cr);
}

/* === Calculer les mouvements possibles pour une pièce === */
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

/* === Vérifier si une position est dans la liste des mouvements possibles === */
static gboolean is_possible_move(int row, int col) {
    for (int i = 0; i < num_possible_moves; i++) {
        if (possible_moves[i][0] == row && possible_moves[i][1] == col) {
            return TRUE;
        }
    }
    return FALSE;
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

    // Initialiser les ressources SVG si nécessaire
    init_svg_resources();

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

            // Couleur de base de la cellule
            if (i + j == 0) {
                cairo_set_source_rgb(cr, 0.85, 0.85, 0.90);
            } else if (i + j == 16) {
                cairo_set_source_rgb(cr, 0.90, 0.85, 0.85);
            } else {
                cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
            }

            // Couleur de la cellule visitée
            switch (visited) {
                case P1_PAWN: cairo_set_source_rgb(cr, 0.85, 0.95, 1); break;
                case P2_PAWN: cairo_set_source_rgb(cr, 1, 0.85, 0.85); break;
                case P1_KING: cairo_set_source_rgb(cr, 0.85, 0.95, 1); break;
                case P2_KING: cairo_set_source_rgb(cr, 1, 0.85, 0.85); break;
            }

            // Highlighting de la cellule sélectionnée
            if (have_source && src_r == j && src_c == i) {
                cairo_set_source_rgb(cr, 1.0, 1.0, 0.7); // Jaune clair
            }

            // Couleur de la pièce
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

            // Dessiner les points gris pour les mouvements possibles
            if (have_source && is_possible_move(j, i)) {
                cairo_set_source_rgb(cr, 0.5, 0.5, 0.5); // Gris
                cairo_arc(cr, 
                         start_x + i * CELL_SIZE + CELL_SIZE / 2,
                         start_y + j * CELL_SIZE + CELL_SIZE / 2,
                         8, 0, 2 * M_PI);
                cairo_fill(cr);
            }

            // Dessiner les SVG pour les pièces
            int cell_x = start_x + i * CELL_SIZE;
            int cell_y = start_y + j * CELL_SIZE;
            
            if (tile != P_NONE) {
                RsvgHandle *svg_to_use = NULL;
                
                // Choisir le bon SVG selon le type de pièce et la position
                if ((i + j == 0) || (i + j == 16)) {
                    // Coins: utiliser cite.svg
                    svg_to_use = svg_cite;
                } else if (tile == P1_KING || tile == P2_KING) {
                    // Rois: utiliser couronne.svg
                    svg_to_use = svg_couronne;
                } else {
                    // Pions normaux: utiliser soldat.svg
                    svg_to_use = svg_soldat;
                }
                
                draw_svg_in_cell(cr, svg_to_use, cell_x, cell_y, CELL_SIZE);
            }
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

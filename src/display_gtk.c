/**
 * @file display.c
 * @brief Interface graphique GTK4 pour le jeu Krojanty
 * 
 * Ce fichier contient toutes les fonctions liées à l'interface graphique du jeu, incluant :
 * - Le rendu graphique de la grille de jeu et des pièces avec Cairo
 * - La gestion des événements de souris pour les interactions utilisateur
 * - L'affichage des scores, messages de tour et coordonnées de grille
 * - La mise en évidence des mouvements possibles et de la sélection
 * - L'intégration avec les timers d'IA et les callbacks périodiques
 * - Le support multi-mode (LOCAL, CLIENT, SERVER) avec indicateurs visuels
 * 
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 */

#include <gtk/gtk.h>

#include "display_gtk.h"
#include "game.h"
#include "input.h"
#include "const.h"
#include "logging.h"

/** @brief Référence globale du DrawingArea principal (pour redraw thread-safe) */
static GtkWidget *g_main_drawing_area = NULL;

/** @brief Indicateur de sélection d'une pièce source active */
static gboolean have_source = FALSE;

/** @brief Coordonnées de la pièce source sélectionnée */
static int src_r = -1, src_c = -1;

/** @brief Tableau des mouvements possibles pour la pièce sélectionnée */
static int possible_moves[MAX_POSSIBLE_MOVES][2];

/** @brief Nombre de mouvements possibles calculés pour la pièce actuelle */
static int num_possible_moves = 0;

/**
 * @brief Callback de déclenchement initial de l'IA après initialisation
 * 
 * Cette fonction est appelée une seule fois via g_idle_add() pour déclencher
 * le premier mouvement de l'IA après que l'interface graphique soit complètement
 * initialisée. Garantit que l'IA commence à jouer au bon moment.
 * 
 * @param user_data Pointeur vers la structure Game
 * @return gboolean G_SOURCE_REMOVE pour supprimer ce callback après exécution
 */
static gboolean trigger_ai_initial_move(gpointer user_data) {
    Game *game = (Game*)user_data;
    check_ai_initial_move(game);
    return G_SOURCE_REMOVE; // Remove this idle callback
}

/**
 * @brief Timer périodique de vérification des tours d'IA
 * 
 * Cette fonction s'exécute toutes les 500ms pour vérifier si l'IA doit jouer.
 * Elle détermine le joueur actuel selon le mode de jeu (LOCAL, CLIENT, SERVER)
 * et évite les appels multiples grâce à la variable statique last_ai_turn.
 * 
 * @param user_data Pointeur vers la structure Game
 * @return gboolean G_SOURCE_CONTINUE pour maintenir le timer actif
 */
static gboolean check_ai_periodic(gpointer user_data) {
    Game *game = (Game*)user_data;
    
    // Vérification préalable : IA activée et partie en cours
    if (game->is_ai && game->won == NOT_PLAYER) {
        int should_ai_play = 0;
        
        // Logique de détermination du tour d'IA selon le mode
        if ((game->game_mode == LOCAL || game->game_mode == SERVER) && (current_player_turn(game) == P2)) {
            should_ai_play = 1;
        } else if (game->game_mode == CLIENT && (current_player_turn(game) == P1)) {
            should_ai_play = 1;
        }
        
        // Déclenchement de l'IA avec protection contre les appels multiples
        if (should_ai_play) {
            static int last_ai_turn = -1;
            if (game->turn != last_ai_turn) {
                last_ai_turn = game->turn;
                LOG_INFO_MSG("[AI] Timer: C'est le tour de l'IA (tour %d)", game->turn);
                check_ai_turn(game);
            }
        }
    }
    
    return G_SOURCE_CONTINUE; // Continue the timer
}

/**
 * @brief Callback thread-safe pour forcer un redraw de l'interface
 * 
 * Cette fonction est exécutée dans le thread principal GTK via g_idle_add()
 * pour garantir un redraw thread-safe depuis d'autres threads (comme l'IA
 * ou les threads réseau).
 * 
 * @param data Données non utilisées (peut être NULL)
 * @return gboolean G_SOURCE_REMOVE pour supprimer le callback après exécution
 */
static gboolean force_redraw_callback(gpointer data) {
    (void)data;
    if (g_main_drawing_area) {
        gtk_widget_queue_draw(g_main_drawing_area);
        LOG_INFO_MSG("[DISPLAY] Redraw forcé depuis le thread principal");
    }
    return G_SOURCE_REMOVE;
}

/**
 * @brief Interface publique pour demander un redraw thread-safe
 * 
 * Cette fonction peut être appelée depuis n'importe quel thread pour
 * demander un redraw de l'interface. Elle utilise g_idle_add() pour
 * s'assurer que le redraw soit exécuté dans le thread principal GTK.
 * 
 * @return void
 */
void display_request_redraw(void) {
    g_idle_add(force_redraw_callback, NULL);
}

/**
 * @brief Calcule et stocke tous les mouvements possibles pour une pièce
 * 
 * Cette fonction explore les 4 directions cardinales depuis la position donnée
 * et utilise is_move_legal() pour valider chaque mouvement potentiel. Les
 * mouvements valides sont stockés dans le tableau possible_moves global.
 * 
 * @param game Pointeur vers l'état du jeu
 * @param row Ligne de la pièce source (0-8)
 * @param col Colonne de la pièce source (0-8)
 * @return void
 */
static void calculate_possible_moves(Game *game, int row, int col) {
    num_possible_moves = 0;

    // Validation des paramètres d'entrée
    if (!game || row < 0 || row >= GRID_SIZE || col < 0 || col >= GRID_SIZE) {
        return;
    }

    // Directions de mouvement : haut, bas, gauche, droite
    int directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    // Exploration de chaque direction jusqu'aux limites ou obstacles
    for (int dir = 0; dir < 4; dir++) {
        int dr = directions[dir][0];
        int dc = directions[dir][1];

        // Test de chaque distance dans la direction courante
        for (int dist = 1; dist < GRID_SIZE; dist++) {
            int new_row = row + dr * dist;
            int new_col = col + dc * dist;

            // Vérification des limites de la grille
            if (new_row < 0 || new_row >= GRID_SIZE || new_col < 0 || new_col >= GRID_SIZE) {
                break;
            }

            // Validation du mouvement via la logique métier
            if (is_move_legal(game, row, col, new_row, new_col)) {
                if (num_possible_moves < MAX_POSSIBLE_MOVES) {
                    possible_moves[num_possible_moves][0] = new_row;
                    possible_moves[num_possible_moves][1] = new_col;
                    num_possible_moves++;
                }
            } else {
                // Arrêt dans cette direction si mouvement illégal
                break;
            }
        }
    }
}

/**
 * @brief Vérifie si une position fait partie des mouvements possibles
 * 
 * Cette fonction parcourt le tableau des mouvements possibles calculés
 * pour déterminer si la position donnée est une destination valide.
 * 
 * @param row Ligne à vérifier (0-8)
 * @param col Colonne à vérifier (0-8)
 * @return gboolean TRUE si la position est un mouvement possible, FALSE sinon
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
 * @brief Dessine l'interface utilisateur (scores, messages, coordonnées)
 * 
 * Cette fonction affiche tous les éléments UI autour de la grille de jeu :
 * - Scores des deux joueurs avec mise à jour en temps réel
 * - Messages de victoire, égalité ou indicateur de tour
 * - Coordonnées de grille (A-I horizontalement, 9-1 verticalement)
 * - Gestion spéciale pour les modes réseau avec indicateurs de tour
 * 
 * @param cr Contexte Cairo pour le rendu graphique
 * @param game Pointeur vers l'état du jeu
 * @param start_x Position x de départ de la grille
 * @param start_y Position y de départ de la grille
 * @param grid_width Largeur totale de la grille en pixels
 * @param grid_height Hauteur totale de la grille en pixels
 * @return void
 */
void draw_ui(cairo_t *cr, Game *game, int start_x, int start_y, int grid_width, int grid_height) {
    // Calcul des scores en temps réel
    int player_one_score = score_player_one(*game);
    int player_two_score = score_player_two(*game);

    // Configuration du rendu de texte
    char score_text[50];
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 20.0);
    cairo_set_source_rgb(cr, 0, 0, 0);

    // Affichage du score Joueur 1 (côté gauche)
    cairo_move_to(cr, start_x - 200, start_y + grid_height / 2 - 10);
    cairo_show_text(cr, "Score Joueur 1:");
    snprintf(score_text, sizeof(score_text), "%d", player_one_score);
    cairo_move_to(cr, start_x - 120, start_y + grid_height / 2 + 10);
    cairo_show_text(cr, score_text);

    // Affichage du score Joueur 2 (côté droit)
    cairo_move_to(cr, start_x + grid_width + 30, start_y + grid_height / 2 - 10);
    cairo_show_text(cr, "Score Joueur 2:");
    snprintf(score_text, sizeof(score_text), "%d", player_two_score);
    cairo_move_to(cr, start_x + grid_width + 110, start_y + grid_height / 2 + 10);
    cairo_show_text(cr, score_text);

    // Configuration pour les messages principaux
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 24.0);
    cairo_set_source_rgb(cr, 0, 0, 0);

    char msg[100];
    if (game->won != 0) {
        // Messages de fin de partie
        if (game->won == DRAW) {
            snprintf(msg, sizeof(msg), "Egalité !");
        } else if (game->won==P1) {
            snprintf(msg, sizeof(msg), "Joueur 1 (Bleu) a gagné !");
        } else if (game->won==P2) {
            snprintf(msg, sizeof(msg), "Joueur 2 (Rouge) a gagné !");
        }

        // Désactivation des interactions en fin de partie
        have_source = FALSE;
        src_r = -1;
        src_c = -1;
        num_possible_moves = 0;

    } else {
        // Messages de tour en cours selon le mode de jeu
        if (game->game_mode == LOCAL) {
            snprintf(msg, sizeof(msg), "Tour : %d", game->turn + 1);
        } else {
            // Mode réseau : indicateurs détaillés de tour
            int is_server_turn = (current_player_turn(game) == P2);
            const char* current_player = is_server_turn ? "Serveur (Rouge)" : "Client (Bleu)";
            const char* your_turn = "";

            // Détermination du tour local vs distant
            if ((game->game_mode == CLIENT && !is_server_turn) ||
                (game->game_mode == SERVER && is_server_turn)) {
                your_turn = " - VOTRE TOUR";
            } else {
                your_turn = " - Tour adversaire";
            }

            snprintf(msg, sizeof(msg), "Tour %d: %s%s", game->turn + 1, current_player, your_turn);
        }
    }

    // Centrage et affichage du message principal
    cairo_text_extents_t extents;
    cairo_text_extents(cr, msg, &extents);
    double text_x = start_x + (grid_width - extents.width) / 2 - extents.x_bearing;
    double text_y = start_y - 20;
    cairo_move_to(cr, text_x, text_y);
    cairo_show_text(cr, msg);

    // Configuration pour les coordonnées de grille
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 16.0);
    cairo_set_source_rgb(cr, 0, 0, 0);

    // Labels des colonnes (A-I) en haut de la grille
    for (int i = 0; i < GRID_SIZE; i++) {
        char col_label[2] = {'A' + i, '\0'};
        cairo_text_extents_t extents;
        cairo_text_extents(cr, col_label, &extents);
        double text_x = start_x + i * CELL_SIZE + (CELL_SIZE - extents.width) / 2 - extents.x_bearing;
        double text_y = start_y - 5;
        cairo_move_to(cr, text_x, text_y);
        cairo_show_text(cr, col_label);
    }

    // Labels des lignes (9-1) à gauche de la grille (ordre inversé)
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
 * @brief Callback principal de rendu graphique de la grille et des pièces
 * 
 * Cette fonction gère le rendu complet de l'interface de jeu :
 * - Arrière-plan coloré selon le joueur actuel
 * - Grille de jeu avec cellules colorées selon leur état
 * - Pièces avec symboles Unicode et couleurs d'équipe
 * - Mise en évidence de la sélection et des mouvements possibles
 * - Intégration de tous les éléments UI via draw_ui()
 * 
 * @param area Widget DrawingArea GTK (non utilisé)
 * @param cr Contexte Cairo pour le rendu graphique
 * @param width Largeur de la zone de dessin
 * @param height Hauteur de la zone de dessin
 * @param user_data Pointeur vers la structure Game
 * @return void
 */
void draw_callback(GtkDrawingArea *area G_GNUC_UNUSED, cairo_t *cr, int width, int height, gpointer user_data) {
    Game* game = (Game*) user_data;

    // Arrière-plan coloré selon le joueur actuel
    if (current_player_turn(game) == P1) {
        cairo_set_source_rgb(cr, 0.8, 0.9, 1); // Bleu pour P1
    } else {
        cairo_set_source_rgb(cr, 1, 0.8, 0.8); // Rouge pour P2
    }
    cairo_paint(cr);

    // Calcul du centrage de la grille
    const int grid_width = GRID_SIZE * CELL_SIZE;
    const int grid_height = GRID_SIZE * CELL_SIZE;
    int start_x = (width - grid_width) / 2;
    int start_y = (height - grid_height) / 2;

    // Rendu de l'interface utilisateur
    draw_ui(cr, game, start_x, start_y, grid_width, grid_height);

    // Rendu de chaque cellule de la grille
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            int tile = game->board[j][i];

            // Couleur de base selon la position spéciale ou normale
            if (i + j == 0) {
                cairo_set_source_rgb(cr, 0.85, 0.85, 0.90); // Coin P1
            } else if (i + j == 16) {
                cairo_set_source_rgb(cr, 0.90, 0.85, 0.85); // Coin P2
            } else {
                cairo_set_source_rgb(cr, 0.9, 0.9, 0.9); // Cellule normale
            }

            // Couleur des cellules visitées
            switch (tile) {
                case P1_VISITED: cairo_set_source_rgb(cr, 0.85, 0.95, 1); break;
                case P2_VISITED: cairo_set_source_rgb(cr, 1, 0.85, 0.85); break;
            }

            // Mise en évidence de la cellule sélectionnée
            if (have_source && src_r == j && src_c == i) {
                cairo_set_source_rgb(cr, 1.0, 1.0, 0.7); // Jaune clair
            }

            // Rendu du rectangle de cellule avec bordure
            cairo_rectangle(cr, start_x + i * CELL_SIZE, start_y + j * CELL_SIZE, CELL_SIZE, CELL_SIZE);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_set_line_width(cr, 1.5f);
            cairo_stroke(cr);

            // Rendu des pièces par-dessus le fond
            if (tile != P_NONE) {
                const char *symbol = NULL;

                // Sélection du symbole selon le type de pièce
                if (tile == P1_KING || tile == P2_KING) {
                    symbol = "♔";
                } else if (tile == P1_PAWN || tile == P2_PAWN) {
                    symbol = "♜";
                }

                // Couleur selon l'équipe
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

                // Rendu centré du symbole Unicode
                cairo_select_font_face(cr, "DejaVu Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
                cairo_set_font_size(cr, CELL_SIZE * 0.7);

                cairo_text_extents_t extents;
                cairo_text_extents(cr, symbol, &extents);
                double text_x = start_x + i * CELL_SIZE + (CELL_SIZE - extents.width) / 2 - extents.x_bearing;
                double text_y = start_y + j * CELL_SIZE + (CELL_SIZE + extents.height) / 2;

                cairo_move_to(cr, text_x, text_y);
                cairo_show_text(cr, symbol);
            }

            // Indicateurs visuels des mouvements possibles (par-dessus tout)
            if (have_source && is_possible_move(j, i)) {
                // Cercle gris foncé pour la visibilité
                cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
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
 * @brief Gestionnaire d'événements de clic souris pour l'interaction utilisateur
 * 
 * Cette fonction implémente la logique complète de sélection et de mouvement :
 * - Conversion des coordonnées pixels en coordonnées grille
 * - Sélection de pièce source avec validation d'appartenance au joueur
 * - Calcul et affichage des mouvements possibles
 * - Validation et exécution des mouvements de destination
 * - Désélection par clic sur la même pièce
 * - Gestion des cas d'erreur avec messages informatifs
 * 
 * @param gesture Contrôleur de geste GTK4
 * @param n_press Nombre de pressions (1 pour simple clic)
 * @param x Position x du clic en pixels
 * @param y Position y du clic en pixels
 * @param user_data Pointeur vers la structure Game
 * @return void
 */
static void on_mouse_click(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data) {
    (void)gesture;
    (void)n_press;
    Game *game = (Game*) user_data;

    // Récupération des dimensions du widget pour le calcul de centrage
    GtkWidget *widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
    int width = gtk_widget_get_width(widget);
    int height = gtk_widget_get_height(widget);

    // Calcul de la position de la grille centrée
    int grid_width = GRID_SIZE * CELL_SIZE;
    int grid_height = GRID_SIZE * CELL_SIZE;
    int start_x = (width - grid_width) / 2;
    int start_y = (height - grid_height) / 2;

    // Conversion coordonnées pixels -> coordonnées grille
    int col = (x - start_x) / CELL_SIZE;
    int row = (y - start_y) / CELL_SIZE;

    // Validation des limites de grille
    if (col >= 0 && col < GRID_SIZE && row >= 0 && row < GRID_SIZE) {
        if (!have_source) {
            // Mode sélection de source : validation de présence de pièce
            if (game->board[row][col] != P_NONE) {

                // Validation d'appartenance au joueur courant
                Player current_player = current_player_turn(game);
                Player piece_owner = get_player(game->board[row][col]);
                if (piece_owner != current_player) {
                    LOG_INFO_MSG("[CLICK] Impossible de sélectionner une pièce adverse !");
                    return;
                }

                // Activation de la sélection et calcul des mouvements
                src_r = row;
                src_c = col;
                have_source = TRUE;
                calculate_possible_moves(game, row, col);

                LOG_INFO_MSG("[CLICK] Source sélectionnée: %d,%d (%d mouvements possibles)", src_r, src_c, num_possible_moves);

                // Redraw pour afficher la sélection et les indicateurs
                gtk_widget_queue_draw(g_main_drawing_area);
            }
        } else {
            // Mode sélection de destination
            
            // Gestion de la désélection par clic sur la même pièce
            if (row == src_r && col == src_c) {
                LOG_INFO_MSG("[CLICK] Désélection de la pièce %d,%d", src_r, src_c);
                have_source = FALSE;
                src_r = -1;
                src_c = -1;
                num_possible_moves = 0;
                gtk_widget_queue_draw(g_main_drawing_area);
                return;
            }
            
            // Validation du mouvement contre la liste des mouvements possibles
            int move_valid = 0;
            for (int i = 0; i < num_possible_moves; i++) {
                if (possible_moves[i][0] == row && possible_moves[i][1] == col) {
                    move_valid = 1;
                    break;
                }
            }
            
            // Exécution ou rejet du mouvement selon sa validité
            if (move_valid) {
                LOG_INFO_MSG("[CLICK] Destination valide: %d,%d", row, col);
                on_user_move_decided(game, src_r, src_c, row, col);
                
                // Réinitialisation de la sélection après mouvement valide
                have_source = FALSE;
                src_r = -1;
                src_c = -1;
                num_possible_moves = 0;
                gtk_widget_queue_draw(g_main_drawing_area);
            } else {
                LOG_INFO_MSG("[CLICK] Destination invalide: %d,%d (coup ignoré)", row, col);
                // Conservation de la sélection actuelle pour permettre un nouveau choix
                return;
            }
        }
    }
}

/**
 * @brief Callback d'activation de l'application GTK
 * 
 * Cette fonction initialise complètement l'interface graphique :
 * - Création de la fenêtre principale avec titre adapté au mode
 * - Configuration du DrawingArea avec callback de rendu
 * - Mise en place des contrôleurs d'événements souris GTK4
 * - Démarrage des timers d'IA si nécessaire
 * - Affichage final de l'interface utilisateur
 * 
 * @param app Instance GtkApplication
 * @param user_data Pointeur vers la structure Game
 * @return void
 */
static void on_app_activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *frame;
    GtkGesture *click_gesture;
    Game* game = (Game*) user_data;

    // Création de la fenêtre principale
    window = gtk_application_window_new(app);

    // Configuration du titre selon le mode de jeu
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

    // Configuration du DrawingArea avec callback de rendu
    frame = gtk_drawing_area_new();
    g_main_drawing_area = frame; // Référence globale pour redraw thread-safe
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(frame), draw_callback, game, NULL);

    // Configuration du contrôleur d'événements clic GTK4
    click_gesture = gtk_gesture_click_new();
    gtk_widget_add_controller(frame, GTK_EVENT_CONTROLLER(click_gesture));
    g_signal_connect(click_gesture, "pressed", G_CALLBACK(on_mouse_click), game);

    // Assemblage et affichage de l'interface
    gtk_window_set_child(GTK_WINDOW(window), frame);
    gtk_window_present(GTK_WINDOW(window));
    
    // Activation des timers d'IA si le mode IA est activé
    if (game->is_ai) {
        g_idle_add(trigger_ai_initial_move, game);
        g_timeout_add(500, check_ai_periodic, game); // Timer 500ms pour vérifications périodiques
    }
}

/**
 * @brief Point d'entrée principal de l'interface graphique GTK
 * 
 * Cette fonction initialise et lance l'application GTK4 avec un ID unique
 * selon le mode de jeu pour permettre l'exécution simultanée de plusieurs
 * instances. Configure les callbacks principaux et gère le cycle de vie
 * complet de l'application graphique.
 * 
 * @param argc Nombre d'arguments de ligne de commande
 * @param argv Tableau des arguments de ligne de commande
 * @param game Pointeur vers la structure Game initialisée
 * @return int Code de retour de l'application (0 = succès)
 */
int initialize_display(int argc, char** argv, Game* game) {
    const char *app_id;

    // Configuration d'ID d'application unique selon le mode
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

    // Création et lancement de l'application GTK
    GtkApplication *app = gtk_application_new(app_id, 0);
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), game);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

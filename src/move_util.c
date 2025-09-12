#include <stdio.h>
#include <gtk/gtk.h>
#include "move_util.h"
#include "game.h"
#include "display.h"

/* Structure pour passer les données de mouvement au thread principal */
typedef struct {
    Game *game;
    int src_row, src_col, dst_row, dst_col;
} MoveData;

/* Callback exécuté dans le thread principal GTK */
static gboolean apply_network_move(gpointer user_data) {
    MoveData *data = (MoveData*)user_data;
    
    /* Appliquer le mouvement */
    data->game->selected_tile[0] = data->src_row;
    data->game->selected_tile[1] = data->src_col;
    update_board(data->game, data->dst_row, data->dst_col);
    
    /* Forcer le redraw */
    display_request_redraw();
    
    /* Libérer la mémoire */
    g_free(data);
    
    return G_SOURCE_REMOVE; /* Exécuter une seule fois */
}

/* Fonction pour convertir un char de colonne en index */
static int col_char_to_index(char c) {
    if (c >= 'A' && c <= 'I') {
        return c - 'A';
    } else if (c >= 'a' && c <= 'i') {
        return c - 'a';
    }
    return -1;
}

/* Fonction pour poster un mouvement depuis un thread réseau */
void post_move_to_gtk(Game *game, const char *move4) {
    if (!move4 || !game) return;
    
    /* Parser le mouvement: "A1C3" */
    int src_col = col_char_to_index(move4[0]);
    int src_row = move4[1] - '1';
    int dst_col = col_char_to_index(move4[2]);
    int dst_row = move4[3] - '1';
    
    /* Vérifier la validité des coordonnées */
    if (src_col < 0 || src_col >= 9 || src_row < 0 || src_row >= 9 ||
        dst_col < 0 || dst_col >= 9 || dst_row < 0 || dst_row >= 9) {
        printf("[MOVE_UTIL] Coordonnées invalides: %s\n", move4);
        return;
    }
    
    /* Créer les données du mouvement */
    MoveData *data = g_malloc(sizeof(MoveData));
    data->game = game;
    data->src_row = src_row;
    data->src_col = src_col;
    data->dst_row = dst_row;
    data->dst_col = dst_col;
    
    /* Programmer l'exécution dans le thread principal */
    g_idle_add(apply_network_move, data);
}
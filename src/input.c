#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#include "input.h"
#include "game.h"
#include "display.h"
#include "move_util.h"

/* Déclaré dans client.c */
extern int g_client_socket;
void send_message(int client_socket, const char *move4);

/* Socket du client connecté au serveur (pour serveur → client) */
extern int g_server_client_socket;
void send_message_to_client(int server_socket, const char *move4);

/* Ex: appelé après que l'utilisateur ait choisi une destination valide */
void on_user_move_decided(Game *game, int src_r, int src_c, int dst_r, int dst_c) {
    /* Prépare le message de mouvement */
    char move[5];
    move[0] = COLS_MAP[src_c];
    move[1] = (char)('1' + src_r);
    move[2] = COLS_MAP[dst_c];
    move[3] = (char)('1' + dst_r);
    move[4] = '\0'; /* pour logs */

    if (game->game_mode == LOCAL) {
        /* Mode local : applique directement */
        game->selected_tile[0] = src_r;
        game->selected_tile[1] = src_c;
        update_board(game, dst_r, dst_c);
        display_request_redraw();

    } else {
        /* Mode réseau : appliquer le coup localement ET l'envoyer à l'adversaire */
        printf("[MOVE] Envoi coup: %s (Tour %d)\n", move, game->turn);

        if (game->game_mode == CLIENT && g_client_socket >= 0) {
            /* CLIENT : applique son coup localement et l'envoie au serveur */
            game->selected_tile[0] = src_r;
            game->selected_tile[1] = src_c;
            update_board(game, dst_r, dst_c);
            display_request_redraw();
            
            /* Envoie au serveur */
            send_message(g_client_socket, move);
            
        } else if (game->game_mode == SERVER && g_server_client_socket >= 0) {
            /* SERVEUR : applique son coup localement et l'envoie au client */
            game->selected_tile[0] = src_r;
            game->selected_tile[1] = src_c;
            update_board(game, dst_r, dst_c);
            display_request_redraw();
            
            /* Envoie au client */
            send_message_to_client(g_server_client_socket, move);
        }
    }
}

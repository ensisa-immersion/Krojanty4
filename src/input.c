#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>

#include "input.h"
#include "game.h"
#include "display.h"
#include "move_util.h"
#include "algo.h"

/* Déclaré dans client.c */
extern int g_client_socket;
void send_message(int client_socket, const char *move4);

/* Socket du client connecté au serveur (pour serveur → client) */
extern int g_server_client_socket;
void send_message_to_client(int server_socket, const char *move4);

/**
 * Fonction pour que l'IA joue son coup en mode réseau
 * @param game Pointeur vers la structure de jeu
 * @return void
 */
void ai_network_move(Game *game) {
    if (game->won != NOT_PLAYER) return;  // Jeu terminé
    
    printf("[AI] IA calcule son prochain coup...\n");
    
    // Calculer le meilleur coup pour l'IA
    Game copy = *game;
    copy.is_ai = 0; // Éviter la récursion
    Move best_move = minimax_best_move(&copy, 3);  // Profondeur 3
    
    if (best_move.src_row < 0 || best_move.src_col < 0) {
        printf("[AI] Aucun coup valide trouvé\n");
        return;
    }
    
    // Convertir le mouvement en format réseau (ex: "A1B2")
    char move[5];
    move[0] = COLS_MAP[best_move.src_col];
    move[1] = (char)('9' - best_move.src_row);
    move[2] = COLS_MAP[best_move.dst_col];
    move[3] = (char)('9' - best_move.dst_row);
    move[4] = '\0';
    
    printf("[AI] IA joue: %s\n", move);
    
    // Appliquer le mouvement localement
    game->selected_tile[0] = best_move.src_row;
    game->selected_tile[1] = best_move.src_col;
    update_board(game, best_move.dst_row, best_move.dst_col);
    display_request_redraw();
    
    // Envoyer le mouvement selon le mode
    if (game->game_mode == SERVER && g_server_client_socket >= 0) {
        send_message_to_client(g_server_client_socket, move);
    } else if (game->game_mode == CLIENT && g_client_socket >= 0) {
        send_message(g_client_socket, move);
    }
}

/**
 * Vérifier si l'IA doit commencer à jouer au début de la partie
 * @param game Pointeur vers la structure de jeu
 * @return void
 */
void check_ai_initial_move(Game *game) {
    if (!game->is_ai || game->won != NOT_PLAYER) return;
    
    // L'IA commence si:
    // - En mode serveur et c'est le tour 0 (serveur commence toujours) 
    // - En mode client et c'est le tour 0 (client commence aussi)
    if ((game->game_mode == SERVER || game->game_mode == CLIENT) && game->turn == 0) {
        const char* mode_str = (game->game_mode == SERVER) ? "serveur" : "client";
        printf("[AI] IA commence la partie (%s)\n", mode_str);
        sleep(1000000); // 1 seconde de délai pour laisser l'interface se charger
        
        if (game->game_mode == SERVER || game->game_mode == CLIENT) {
            ai_network_move(game);
        }
    }
}

/**
 * Vérifier si l'IA doit jouer après un changement d'état
 * @param game Pointeur vers la structure de jeu
 * @return void
 */
void check_ai_turn(Game *game) {
    if (!game->is_ai || game->won != NOT_PLAYER) return;
    
    int is_ai_turn = 0;
    
    // Déterminer si c'est le tour de l'IA
    if (game->game_mode == LOCAL && (game->turn % 2 == 1)) {
        // En mode local, l'IA joue le joueur 2 (tours impairs)
        is_ai_turn = 1;
    } else if (game->game_mode == SERVER && (game->turn % 2 == 1)) {
        // En mode serveur, l'IA joue le serveur (tours impairs)
        is_ai_turn = 1;
    } else if (game->game_mode == CLIENT && (game->turn % 2 == 0)) {
        // En mode client, l'IA joue le client (tours pairs)
        is_ai_turn = 1;
    }
    
    if (is_ai_turn) {
        printf("[AI] C'est le tour de l'IA (tour %d)\n", game->turn);
        if (game->game_mode == LOCAL) {
            ai_next_move(game);
            display_request_redraw();
        } else {
            sleep(500000); // 500ms de délai
            ai_network_move(game);
        }
    }
}

/**
 * Gestion du clic souris pour sélectionner source/destination
 * et appeler on_user_move_decided.
 * 
 * @param gesture Le geste de clic
 * @param n_press Le nombre de pressions (1 pour simple clic)
 * @param x La position x du clic
 * @param y La position y du clic
 * @param user_data Pointeur vers l'état du jeu (Game*)
 * @return void
 */
void on_user_move_decided(Game *game, int src_r, int src_c, int dst_r, int dst_c) {
    /* Prépare le message de mouvement */
    char move[5];
    move[0] = COLS_MAP[src_c];
    move[1] = (char)('9' - src_r); // Inversion: index 0 → ligne 9, index 8 → ligne 1
    move[2] = COLS_MAP[dst_c];
    move[3] = (char)('9' - dst_r); // Inversion: index 0 → ligne 9, index 8 → ligne 1
    move[4] = '\0'; /* pour logs */

    if (game->game_mode == LOCAL) {
        /* Mode local : si IA activée, seul le joueur 1 peut jouer */
        if (game->is_ai && (game->turn % 2 == 1)) {
            printf("[INPUT] IA contrôle le joueur 2, input humain bloqué\n");
            return;
        }
        /* applique directement */
        game->selected_tile[0] = src_r;
        game->selected_tile[1] = src_c;
        update_board(game, dst_r, dst_c);
        display_request_redraw();
    } else {
        /* Mode réseau : vérifier que c'est le bon tour avant d'autoriser le coup */
        printf("[MOVE] Tentative coup: %s (Tour %d)\n", move, game->turn);

        /* VALIDATION DU TOUR */
        int is_server_turn = (game->turn % 2 == 1);  // Tours impaires = serveur (rouge)
        int is_client_turn = (game->turn % 2 == 0);  // Tours pairs = client (bleu)
        
        /* Bloquer l'input humain si l'IA contrôle ce joueur */
        if (game->is_ai) {
            if (game->game_mode == SERVER && is_server_turn) {
                printf("[INPUT] IA contrôle le serveur, input humain bloqué\n");
                return;
            }
            if (game->game_mode == CLIENT && is_client_turn) {
                printf("[INPUT] IA contrôle le client, input humain bloqué\n");
                return;
            }
        }
        
        if (game->game_mode == SERVER && !is_server_turn) {
            printf("[MOVE] REFUSÉ - Pas le tour du serveur (tour %d)\n", game->turn);
            return;
        }
        
        if (game->game_mode == CLIENT && !is_client_turn) {
            printf("[MOVE] REFUSÉ - Pas le tour du client (tour %d)\n", game->turn);
            return;
        }

        if (game->game_mode == CLIENT && g_client_socket >= 0 && is_client_turn) {
            /* CLIENT : applique son coup localement et l'envoie au serveur */
            printf("[MOVE] CLIENT joue son tour %d\n", game->turn);
            game->selected_tile[0] = src_r;
            game->selected_tile[1] = src_c;
            update_board(game, dst_r, dst_c);
            display_request_redraw();
            
            /* Envoie au serveur */
            send_message(g_client_socket, move);

        } else if (game->game_mode == SERVER && g_server_client_socket >= 0 && is_server_turn) {
            /* SERVEUR : applique son coup localement et l'envoie au client */
            printf("[MOVE] SERVEUR joue son tour %d\n", game->turn);
            game->selected_tile[0] = src_r;
            game->selected_tile[1] = src_c;
            update_board(game, dst_r, dst_c);
            display_request_redraw();
            
            /* Envoie au client */
            send_message_to_client(g_server_client_socket, move);
        }
    }
}

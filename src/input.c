#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>

#include "input.h"
#include "game.h"
#include "display.h"
#include "move_util.h"
#include "algo.h"

// Structure pour gérer l'IA de façon asynchrone
typedef struct {
    Game *game;
} AITask;

/**
 * Callback pour exécuter l'IA de façon asynchrone
 */
gboolean ai_delayed_callback(gpointer data) {
    AITask *task = (AITask*)data;
    Game *game = task->game;
    
    if (game->won != NOT_PLAYER) {
        g_free(task);
        return G_SOURCE_REMOVE;
    }
    
    int is_ai_turn = 0;
    
    // Déterminer si c'est encore le tour de l'IA
    if (game->game_mode == LOCAL && (game->turn % 2 == 1)) {
        is_ai_turn = 1;
    } else if (game->game_mode == SERVER && (game->turn % 2 == 1)) {
        is_ai_turn = 1;
    } else if (game->game_mode == CLIENT && (game->turn % 2 == 0)) {
        is_ai_turn = 1;
    }
    
    if (is_ai_turn) {
        if (game->game_mode == LOCAL) {
            ai_next_move(game);
            display_request_redraw();
        } else {
            ai_network_move(game);
        }
    }
    
    g_free(task);
    return G_SOURCE_REMOVE;
}
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
    
    const char* mode_name = (game->game_mode == SERVER) ? "SERVER" : "CLIENT";
    const char* player_name = (game->game_mode == SERVER) ? "P2 (Rouge)" : "P1 (Bleu)";
    
    printf("[AI] IA %s (%s) calcule son prochain coup...\n", mode_name, player_name);
    
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
    
    printf("[AI] IA %s joue: %s (de %c%c à %c%c)\n", 
           mode_name, move, move[0], move[1], move[2], move[3]);
    
    // Envoyer le mouvement selon le mode AVANT de l'appliquer localement
    if (game->game_mode == SERVER && g_server_client_socket >= 0) {
        printf("[AI] Envoi du mouvement au client...\n");
        send_message_to_client(g_server_client_socket, move);
    } else if (game->game_mode == CLIENT && g_client_socket >= 0) {
        printf("[AI] Envoi du mouvement au serveur...\n");
        send_message(g_client_socket, move);
    }
    
    // Appliquer le mouvement localement APRÈS l'envoi réseau
    game->selected_tile[0] = best_move.src_row;
    game->selected_tile[1] = best_move.src_col;
    update_board(game, best_move.dst_row, best_move.dst_col);
    
    // Redessiner APRÈS l'application locale
    display_request_redraw();
}

/**
 * Vérifier si l'IA doit commencer à jouer au début de la partie
 * @param game Pointeur vers la structure de jeu
 * @return void
 */
void check_ai_initial_move(Game *game) {
    if (!game->is_ai || game->won != NOT_PLAYER) return;
    
    // L'IA commence si:
    // - En mode client et c'est le tour 0 (client = P1 = tours pairs = bleu)
    // - En mode serveur et c'est le tour 1 (serveur = P2 = tours impairs = rouge)
    // - En mode local et c'est le tour 0 (joueur 1 commence)
    
    int should_start = 0;
    
    if (game->turn == 0 && game->game_mode == CLIENT) {
        // Client = P1 = Bleu = commence en premier (tour 0)
        should_start = 1;
        printf("[AI] IA client (P1/Bleu) commence la partie\n");
    } else if (game->turn == 0 && game->game_mode == LOCAL) {
        // En mode local, vérifier si l'IA doit jouer au premier tour
        should_start = 0; // L'IA est joueur 2 en local, ne commence pas
        printf("[AI] Mode local: humain commence, IA attendra son tour\n");
    }
    
    if (should_start) {
        // Délai réduit pour une meilleure réactivité
        usleep(500000); // 0.5 seconde au lieu de 1 seconde
        
        if (game->game_mode == CLIENT) {
            ai_network_move(game);
        } else if (game->game_mode == LOCAL) {
            check_ai_turn(game);
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
        // En mode serveur, l'IA joue le serveur = P2 (tours impairs)
        is_ai_turn = 1;
    } else if (game->game_mode == CLIENT && (game->turn % 2 == 0)) {
        // En mode client, l'IA joue le client = P1 (tours pairs)
        is_ai_turn = 1;
    }
    
    if (is_ai_turn) {
        printf("[AI] C'est le tour de l'IA (tour %d, mode %s)\n", 
               game->turn, 
               game->game_mode == LOCAL ? "LOCAL" : 
               game->game_mode == SERVER ? "SERVER" : "CLIENT");
        
        // Programmer l'IA pour qu'elle joue après un court délai
        // permettant à l'interface de se redessiner
        AITask *task = g_new0(AITask, 1);
        task->game = game;
        
        // Délai minimal pour permettre le redraw de l'interface
        g_timeout_add(50, ai_delayed_callback, task); // 50ms
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
    // Validation basique des coordonnées
    if (src_r < 0 || src_r >= GRID_SIZE || src_c < 0 || src_c >= GRID_SIZE ||
        dst_r < 0 || dst_r >= GRID_SIZE || dst_c < 0 || dst_c >= GRID_SIZE) {
        printf("[INPUT] Coordonnées invalides: src(%d,%d) dst(%d,%d)\n", src_r, src_c, dst_r, dst_c);
        return;
    }
    
    // Vérifier qu'il y a bien une pièce à la source
    if (game->board[src_r][src_c] == P_NONE) {
        printf("[INPUT] Aucune pièce à la source (%d,%d)\n", src_r, src_c);
        return;
    }
    
    // Vérifier que le mouvement est valide avec la logique du jeu
    if (!is_move_legal(game, src_r, src_c, dst_r, dst_c)) {
        printf("[INPUT] Mouvement invalide de (%d,%d) vers (%d,%d)\n", src_r, src_c, dst_r, dst_c);
        return;
    }

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
            } else if (game->game_mode == CLIENT && is_client_turn) {
                printf("[INPUT] IA contrôle le client, input humain bloqué\n");
                return;
            }
        }
        
        if (game->game_mode == SERVER && !is_server_turn) {
            printf("[MOVE] REFUSÉ - Pas le tour du serveur (tour %d)\n", game->turn);
            return;
        } else if (game->game_mode == CLIENT && !is_client_turn) {
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

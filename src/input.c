/**
 * @file input.c
 * @brief Gestion des entrées utilisateur et de l'intelligence artificielle
 * 
 * Ce fichier contient toutes les fonctions liées à la gestion des entrées dans le jeu, incluant :
 * - La gestion des clics souris pour les mouvements des joueurs humains
 * - L'exécution asynchrone de l'intelligence artificielle
 * - La coordination entre les modes de jeu (local, client, serveur)
 * - La validation et l'application des mouvements selon les règles du jeu
 * - La gestion des tours et de la synchronisation réseau
 * 
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>


#include "input.h"
#include "game.h"
#include "display_gtk.h"
#include "move_util.h"
#include "const.h"
#include "algo.h"
#include "logging.h"

/**
 * @struct AITask
 * @brief Structure de contexte pour l'exécution asynchrone de l'IA
 * 
 * Cette structure contient les informations nécessaires pour exécuter
 * l'intelligence artificielle de manière asynchrone via les callbacks GTK.
 */
typedef struct {
    Game *game;   /**< Pointeur vers la structure de jeu principale */
} AITask;

/**
 * @brief Callback GTK pour l'exécution différée de l'intelligence artificielle
 * 
 * Cette fonction est appelée de manière asynchrone par GTK pour permettre à l'IA
 * de jouer son coup sans bloquer l'interface utilisateur. Elle vérifie si c'est
 * toujours le tour de l'IA et exécute le mouvement approprié selon le mode de jeu.
 * 
 * @param data Pointeur vers la structure AITask contenant le contexte
 * @return gboolean G_SOURCE_REMOVE pour supprimer le callback après exécution
 */
gboolean ai_delayed_callback(gpointer data) {
    AITask *task = (AITask*)data;
    Game *game = task->game;
    
    // Vérification si le jeu est terminé
    if (game->won != NOT_PLAYER) {
        g_free(task);
        return G_SOURCE_REMOVE;
    }
    
    int is_ai_turn = 0;
    
    // Détermination si c'est encore le tour de l'IA
    if ((game->game_mode == LOCAL || game->game_mode == SERVER) && current_player_turn(game) == P2) {
        is_ai_turn = 1;
    } else if (game->game_mode == CLIENT && current_player_turn(game) == P1) {
        is_ai_turn = 1;
    }
    
    // Exécution du mouvement IA selon le mode de jeu
    if (is_ai_turn) {
        if (game->game_mode == LOCAL) {
            ai_next_move(game);
            display_request_redraw();
        } else {
            ai_network_move(game);
        }
    }
    
    // Libération de la mémoire et suppression du callback
    g_free(task);
    return G_SOURCE_REMOVE;
}

/* Déclarations externes pour les fonctions réseau */
extern int g_client_socket;                                         /**< Socket client global */
void send_message(int client_socket, const char *move4);            /**< Envoi message au serveur */

extern int g_server_client_socket;                                  /**< Socket serveur global */
void send_message_to_client(int server_socket, const char *move4);  /**< Envoi message au client */

/**
 * @brief Exécute un mouvement de l'IA en mode réseau
 * 
 * Cette fonction calcule le meilleur coup pour l'IA en utilisant l'algorithme minimax,
 * convertit le mouvement au format réseau, l'envoie au joueur distant, puis l'applique
 * localement. Elle gère différemment les modes serveur et client.
 * 
 * @param game Pointeur vers la structure de jeu principale
 * @return void
 */
void ai_network_move(Game *game) {
    // Vérification si le jeu est terminé
    if (game->won != NOT_PLAYER) return;
    
    // Identification du mode et du joueur pour les logs
    const char* mode_name = (game->game_mode == SERVER) ? "SERVER" : "CLIENT";
    const char* player_name = (game->game_mode == SERVER) ? "P2 (Rouge)" : "P1 (Bleu)";
    
    LOG_INFO_MSG("[AI] IA %s (%s) calcule son prochain coup...", mode_name, player_name);
    
    // Calcul du meilleur mouvement avec l'algorithme minimax
    Game copy = *game;
    copy.is_ai = 0; // Prévention de la récursion dans l'IA
    Move best_move = minimax_best_move(&copy, DEPTH);
    
    // Validation du mouvement calculé
    if (best_move.src_row < 0 || best_move.src_col < 0) {
        LOG_INFO_MSG("[AI] Aucun coup valide trouvé");
        return;
    }
    
    // Conversion du mouvement au format réseau (ex: "A1B2")
    char move[5];
    move[0] = COLS_MAP[best_move.src_col];
    move[1] = (char)('9' - best_move.src_row);
    move[2] = COLS_MAP[best_move.dst_col];
    move[3] = (char)('9' - best_move.dst_row);
    move[4] = '\0';
    
    LOG_INFO_MSG("[AI] IA %s joue: %s (de %c%c à %c%c)", mode_name, move, move[0], move[1], move[2], move[3]);
    
    // Envoi du mouvement au joueur distant AVANT application locale
    if (game->game_mode == SERVER && g_server_client_socket >= 0) {
        LOG_INFO_MSG("[AI] Envoi du mouvement au client...");
        send_message_to_client(g_server_client_socket, move);
    } else if (game->game_mode == CLIENT && g_client_socket >= 0) {
        LOG_INFO_MSG("[AI] Envoi du mouvement au serveur...");
        send_message(g_client_socket, move);
    }
    
    // Application du mouvement localement APRÈS l'envoi réseau
    game->selected_tile[0] = best_move.src_row;
    game->selected_tile[1] = best_move.src_col;
    update_board(game, best_move.dst_row, best_move.dst_col);
    
    // Demande de redessinage de l'interface
    display_request_redraw();
}

/**
 * @brief Vérifie si l'IA doit effectuer le premier mouvement de la partie
 * 
 * Cette fonction détermine si l'intelligence artificielle doit commencer à jouer
 * au début d'une nouvelle partie selon le mode de jeu et la configuration.
 * Elle gère les différents scénarios de démarrage pour chaque mode.
 * 
 * @param game Pointeur vers la structure de jeu principale
 * @return void
 */
void check_ai_initial_move(Game *game) {
    // Vérification des conditions préalables
    if (!game->is_ai || game->won != NOT_PLAYER) return;
    
    // Détermination si l'IA doit commencer selon le mode de jeu:
    // - Client: IA = P1 = Bleu = commence au tour 0 (tours pairs)
    // - Serveur: IA = P2 = Rouge = commence au tour 1 (tours impairs)
    // - Local: IA = P2 = ne commence jamais (joueur humain commence)
    
    int should_start = 0;
    
    if (game->turn == 0 && game->game_mode == CLIENT) {
        // Mode client: IA joue P1 (Bleu) et commence en premier
        // client_first_move(game);
        should_start = 1;
        LOG_INFO_MSG("[AI] IA client (P1/Bleu) commence la partie");
    } else if (game->turn == 0 && game->game_mode == LOCAL) {
        // Mode local: joueur humain commence, IA attendra son tour
        should_start = 0;
        LOG_INFO_MSG("[AI] Mode local: humain commence, IA attendra son tour");
    }
    
    // Exécution du premier mouvement si nécessaire
    if (should_start) {
        // Délai réduit pour une meilleure réactivité utilisateur
        usleep(500000); // 0.5 seconde de pause
        if (game->game_mode == CLIENT) {
            ai_network_move(game);
        } else if (game->game_mode == LOCAL) {
            check_ai_turn(game);
        }
    }
}

/**
 * @brief Vérifie si l'IA doit jouer après un changement d'état du jeu
 * 
 * Cette fonction est appelée après chaque mouvement pour déterminer si c'est
 * maintenant le tour de l'intelligence artificielle. Elle programme l'exécution
 * asynchrone de l'IA via un callback GTK pour éviter de bloquer l'interface.
 * 
 * @param game Pointeur vers la structure de jeu principale
 * @return void
 */
void check_ai_turn(Game *game) {
    // Vérification des conditions préalables
    if (!game->is_ai || game->won != NOT_PLAYER) return;
    
    int is_ai_turn = 0;
    
    // Détermination du tour de l'IA selon le mode de jeu
    if ((game->game_mode == LOCAL || game->game_mode == SERVER) && (current_player_turn(game) == P2)) {
        // Mode local: IA = joueur 2 (tours impairs)
        // Mode serveur: IA = serveur = P2 (tours impairs)
        is_ai_turn = 1;
    } else if (game->game_mode == CLIENT && (current_player_turn(game) == P1)) {
        // Mode client: IA = client = P1 (tours pairs)
        is_ai_turn = 1;
    }
    
    // Programmation de l'exécution asynchrone de l'IA
    if (is_ai_turn) {
        LOG_INFO_MSG("[AI] C'est le tour de l'IA (tour %d, mode %s)",
               game->turn, 
               game->game_mode == LOCAL ? "LOCAL" : 
               game->game_mode == SERVER ? "SERVER" : "CLIENT");
        
        // Création du contexte pour le callback asynchrone
        AITask *task = g_new0(AITask, 1);
        task->game = game;
        
        // Programmation de l'IA avec délai minimal pour le redraw
        g_timeout_add(50, ai_delayed_callback, task); // 50ms de délai
    }
}

/**
 * @brief Traite un mouvement décidé par l'utilisateur (humain)
 * 
 * Cette fonction est appelée quand un joueur humain a sélectionné une pièce source
 * et une destination via l'interface graphique. Elle valide le mouvement, vérifie
 * les permissions selon le mode de jeu et l'état de l'IA, puis applique le mouvement
 * localement et/ou l'envoie via le réseau selon le contexte.
 * 
 * @param game Pointeur vers la structure de jeu principale
 * @param src_r Ligne de la pièce source (0-8)
 * @param src_c Colonne de la pièce source (0-8)
 * @param dst_r Ligne de destination (0-8)
 * @param dst_c Colonne de destination (0-8)
 * @return void
 */
void on_user_move_decided(Game *game, int src_r, int src_c, int dst_r, int dst_c) {
    // Validation des coordonnées d'entrée
    if (src_r < 0 || src_r >= GRID_SIZE || src_c < 0 || src_c >= GRID_SIZE ||
        dst_r < 0 || dst_r >= GRID_SIZE || dst_c < 0 || dst_c >= GRID_SIZE) {
        LOG_INFO_MSG("[INPUT] Coordonnées invalides: src(%d,%d) dst(%d,%d)", src_r, src_c, dst_r, dst_c);
        return;
    }
    
    // Vérification de la présence d'une pièce à la source
    if (game->board[src_r][src_c] == P_NONE) {
        LOG_INFO_MSG("[INPUT] Aucune pièce à la source (%d,%d)", src_r, src_c);
        return;
    }
    
    // Validation de la légalité du mouvement selon les règles du jeu
    if (!is_move_legal(game, src_r, src_c, dst_r, dst_c)) {
        LOG_INFO_MSG("[INPUT] Mouvement invalide de (%d,%d) vers (%d,%d)", src_r, src_c, dst_r, dst_c);
        return;
    }

    // Préparation du message de mouvement au format réseau
    char move[5];
    move[0] = COLS_MAP[src_c];
    move[1] = (char)('9' - src_r); // Conversion: index 0 → ligne 9, index 8 → ligne 1
    move[2] = COLS_MAP[dst_c];
    move[3] = (char)('9' - dst_r); // Conversion: index 0 → ligne 9, index 8 → ligne 1
    move[4] = '\0';

    // Gestion selon le mode de jeu
    if (game->game_mode == LOCAL) {
        // Mode local: vérification du contrôle par l'IA
        if (game->is_ai && (current_player_turn(game) == P2)) {
            LOG_INFO_MSG("[INPUT] IA contrôle le joueur 2, input humain bloqué");
            return;
        }
        // Application directe du mouvement en mode local
        game->selected_tile[0] = src_r;
        game->selected_tile[1] = src_c;
        update_board(game, dst_r, dst_c);
        display_request_redraw();
    } else {
        // Mode réseau: validation des tours et permissions
        LOG_INFO_MSG("[MOVE] Tentative coup: %s (Tour %d)", move, game->turn);

        // Détermination du joueur actuel
        int is_server_turn = (current_player_turn(game) == P2);  // Tours impairs = serveur (rouge)
        int is_client_turn = (current_player_turn(game) == P1);  // Tours pairs = client (bleu)
        
        // Blocage de l'input humain si l'IA contrôle ce joueur
        if (game->is_ai) {
            if (game->game_mode == SERVER && is_server_turn) {
                LOG_INFO_MSG("[INPUT] IA contrôle le serveur, input humain bloqué");
                return;
            } else if (game->game_mode == CLIENT && is_client_turn) {
                LOG_INFO_MSG("[INPUT] IA contrôle le client, input humain bloqué");
                return;
            }
        }
        
        // Validation du tour selon le mode de jeu
        if (game->game_mode == SERVER && !is_server_turn) {
            LOG_INFO_MSG("[MOVE] REFUSÉ - Pas le tour du serveur (tour %d)", game->turn);
            return;
        } else if (game->game_mode == CLIENT && !is_client_turn) {
            LOG_INFO_MSG("[MOVE] REFUSÉ - Pas le tour du client (tour %d)", game->turn);
            return;
        }

        // Traitement des mouvements selon le rôle réseau
        if (game->game_mode == CLIENT && g_client_socket >= 0 && is_client_turn) {
            // Client: application locale puis envoi au serveur
            LOG_INFO_MSG("[MOVE] CLIENT joue son tour %d", game->turn);
            game->selected_tile[0] = src_r;
            game->selected_tile[1] = src_c;
            update_board(game, dst_r, dst_c);
            display_request_redraw();
            
            // Transmission du mouvement au serveur
            send_message(g_client_socket, move);

        } else if (game->game_mode == SERVER && g_server_client_socket >= 0 && is_server_turn) {
            // Serveur: application locale puis envoi au client
            LOG_INFO_MSG("[MOVE] SERVEUR joue son tour %d", game->turn);
            game->selected_tile[0] = src_r;
            game->selected_tile[1] = src_c;
            update_board(game, dst_r, dst_c);
            display_request_redraw();
            
            // Transmission du mouvement au client
            send_message_to_client(g_server_client_socket, move);
        }
    }
}

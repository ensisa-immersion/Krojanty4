
/**
 * @file main.c
 * @brief Point d'entrée principal de l'application de jeu
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 *
 * Ce fichier gère le lancement du jeu, le parsing des arguments,
 * l'initialisation du mode (local, serveur, client), la gestion de l'IA,
 * et le démarrage de l'interface graphique GTK.
 *
 * Modes supportés :
 * - Local (2 joueurs sur la même machine, avec ou sans IA)
 * - Serveur (host + joueur local, thread serveur séparé)
 * - Client (connexion à un serveur distant)
 *
 * Utilisation :
 *   ./game [-ia] -l
 *   ./game [-ia] -s <port>
 *   ./game [-ia] -c <ip:port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "game.h"
#include "display_gtk.h"
#include "client.h"
#include "server.h"
#include "algo.h"
#include "input.h"


/**
 * @struct ServerData
 * @brief Structure de passage de données pour le thread serveur
 */
typedef struct {
    Game *game; /**< Pointeur vers la structure de jeu */
    int port;   /**< Port TCP d'écoute */
} ServerData;


/**
 * @brief Fonction de thread pour lancer le serveur en mode hôte
 *
 * Cette fonction est exécutée dans un thread séparé pour ne pas bloquer la GUI.
 * Elle lance le serveur sur le port spécifié et libère la mémoire à la fin.
 *
 * @param arg Pointeur vers une structure ServerData
 * @return void*
 */
void* run_server_thread(void* arg) {
    ServerData *data = (ServerData*)arg;
    run_server_host(data->game, data->port);
    free(data);
    return NULL;
}


/**
 * @brief Point d'entrée principal du programme
 *
 * Gère le parsing des arguments, l'initialisation du jeu, le choix du mode,
 * la gestion de l'IA, la connexion réseau, et le lancement de l'interface GTK.
 *
 * @param argc Nombre d'arguments de la ligne de commande
 * @param argv Tableau des arguments de la ligne de commande
 * @return int Code de retour du programme
 */
int main(int argc, char *argv[]) {

    Game game;
    int ai_enabled = 0;
    
    // Check for -ia flag in arguments and filter it out
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-ia") == 0) {
            ai_enabled = 1;
            // Shift remaining arguments left to remove -ia
            for (int j = i; j < argc - 1; j++) {
                argv[j] = argv[j + 1];
            }
            argc--; // Reduce argument count
            i--; // Check the same position again in case of multiple -ia
        }
    }

    if (argc == 1 || (argc >= 2 && strcmp(argv[1], "-l") == 0)) {
        // Mode LOCAL (2 joueurs sur la même machine)
        printf("Démarrage en mode local%s...\n", ai_enabled ? " avec IA" : "");
        game = init_game(LOCAL, ai_enabled);
    }
    else if (argc >= 2 && strcmp(argv[1], "-s") == 0 && argc >= 3) {
        // Mode SERVEUR (host + player)
        int port = atoi(argv[2]);
        printf("Démarrage du serveur sur le port %d%s...\n", port, ai_enabled ? " avec IA" : "");
        game = init_game(SERVER, ai_enabled);

        // Lance le serveur dans un thread séparé pour ne pas bloquer la GUI
        pthread_t server_thread;
        ServerData *server_data = malloc(sizeof(ServerData));
        server_data->game = &game;
        server_data->port = port;

        if (pthread_create(&server_thread, NULL, run_server_thread, server_data) != 0) {
            fprintf(stderr, "[SERVER] Échec du lancement du thread serveur.\n");
            free(server_data);
            return 1;
        }
        pthread_detach(server_thread);
    }
    else if (argc >= 2 && strcmp(argv[1], "-c") == 0 && argc >= 3) {
        // Mode CLIENT
        char *sep = strchr(argv[2], ':');
        if (!sep) {
            fprintf(stderr, "Format invalide: utilisez -c ip:port\n");
            return 1;
        }

        *sep = '\0';
        const char *addr = argv[2];
        int port = atoi(sep + 1);

        printf("Connexion au serveur %s:%d%s...\n", addr, port, ai_enabled ? " avec IA" : "");
        game = init_game(CLIENT, ai_enabled);

        if (connect_to_server(addr, port) < 0) {
            fprintf(stderr, "[CLIENT] Impossible de se connecter.\n");
            return 1;
        }
        start_client_rx(&game);
    }

    if (ai_enabled) {
        check_ai_initial_move(&game);
    }

    return initialize_display(0, NULL, &game);

    fprintf(stderr, "Usage: %s [-ia] -l | [-ia] -s <port> | [-ia] -c <ip:port>\n", argv[0]);
    return 1;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "game.h"
#include "display.h"
#include "client.h"
#include "server.h"

typedef struct {
    Game *game;
    int port;
} ServerData;

void* run_server_thread(void* arg) {
    ServerData *data = (ServerData*)arg;
    run_server_host(data->game, data->port);
    free(data);
    return NULL;
}

int main(int argc, char *argv[]) {

    Game game;
    int ai_enabled = 0;
    
    // Check for -ai flag in arguments and filter it out
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-ia") == 0) {
            ai_enabled = 1;
            // Shift remaining arguments left to remove -ai
            for (int j = i; j < argc - 1; j++) {
                argv[j] = argv[j + 1];
            }
            argc--; // Reduce argument count
            i--; // Check the same position again in case of multiple -ai
        }
    }

    if (argc == 1 || (argc >= 2 && strcmp(argv[1], "-l") == 0)) {
        // Mode LOCAL (2 joueurs sur la même machine)
        printf("Démarrage en mode local%s...\n", ai_enabled ? " avec IA" : "");
        game = init_game(LOCAL, ai_enabled);
        // return initialize_display(0, NULL, &game);
    }

    if (strcmp(argv[1], "-s") == 0 && argc >= 3) {
        // Mode SERVEUR (host + player)
        int port = atoi(argv[2]);
        printf("Démarrage du serveur sur le port %d%s...\n", port, ai_enabled ? " avec IA" : "");
        Game game = init_game(SERVER, ai_enabled);

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

        /* // Lance immédiatement l'UI GTK pour le serveur (joueur host)
        char *gtk_argv[] = { argv[0], NULL };
        return initialize_display(1, gtk_argv, &game); */
    }

    if (strcmp(argv[1], "-c") == 0 && argc >= 3) {
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
        Game game = init_game(CLIENT, ai_enabled);

        if (connect_to_server(addr, port) < 0) {
            fprintf(stderr, "[CLIENT] Impossible de se connecter.\n");
            return 1;
        }
        start_client_rx(&game);

        /* // Lancement de GTK côté client
        char *gtk_argv[] = { argv[0], NULL };
        return initialize_display(1, gtk_argv, &game); */
    }

    return initialize_display(0, NULL, &game);

    fprintf(stderr, "Usage: %s [-ai] -l | [-ai] -s <port> | [-ai] -c <ip:port>\n", argv[0]);
    return 1;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

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
    if (argc == 1 || (argc >= 2 && strcmp(argv[1], "-l") == 0)) {
        // Mode LOCAL (2 joueurs sur la même machine)
        printf("Démarrage en mode local...\n");
        Game game = init_game(LOCAL, 0);
        return initialize_display(0, NULL, &game);
    }

    if (strcmp(argv[1], "-s") == 0 && argc >= 3) {
        // Mode SERVEUR (host + player)
        int port = atoi(argv[2]);
        printf("Démarrage du serveur sur le port %d...\n", port);
        Game game = init_game(SERVER, 0);

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

        // Lance immédiatement l'UI GTK pour le serveur (joueur host)
        char *gtk_argv[] = { argv[0], NULL };
        return initialize_display(1, gtk_argv, &game);
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

        printf("Connexion au serveur %s:%d...\n", addr, port);
        Game game = init_game(CLIENT, 0);

        if (connect_to_server(addr, port) < 0) {
            fprintf(stderr, "[CLIENT] Impossible de se connecter.\n");
            return 1;
        }
        start_client_rx(&game);

        // Lancement de GTK côté client
        char *gtk_argv[] = { argv[0], NULL };
        return initialize_display(1, gtk_argv, &game);
    }

    fprintf(stderr, "Usage: %s -l | -s <port> | -c <ip:port>\n", argv[0]);
    return 1;
}

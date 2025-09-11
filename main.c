#include <string.h>
#include <pthread.h>

#include "./include/display.h"
#include "./include/game.h"
#include "./include/server.h"
#include "./include/client.h"

// gcc $( pkg-config --cflags gtk4 ) -o bin/krojanty.exe src/*.c $( pkg-config --libs gtk4 ) -Iinclude
// gcc `pkg-config --cflags gtk4` main.c src/*.c -o bin/krojanty.exe `pkg-config --libs gtk4` -Iinclude

int main (int argc, char **argv) {
    // Initializes game
    Game game;

    // Checks if AI
    int is_artificial_intelligence = 0;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-ia") == 0) is_artificial_intelligence = 1;
    }

    // Choose game mode
    for (int j = 0; j < argc; j++) {
        if (strcmp(argv[j], "-l") == 0) {

            game = init_game(LOCAL, is_artificial_intelligence);

        } else if (strcmp(argv[j], "-s") == 0) {

            int port = atoi(argv[2]);
            if (port <= 0 || port > 65535)
            {
                printf("Port invalide. Utilisez un port entre 1 et 65535.\n");
                return 1;
            }

            printf("Démarrage du serveur sur le port %d...\n", port);
            printf("Serveur lancé (simulation, code réel en commentaire)\n");

            server(port);

        } else if (strcmp(argv[j], "-c") == 0) {
             // Mode client
            char *ip_port = argv[2];
            char *delimiter = strchr(ip_port, ':');

            if (delimiter == NULL)
            {
                printf("Format incorrect. Utilisez <adresse_ip>:<port>\n");
                return 1;
            }

            *delimiter = '\0';
            const char *ip_address = ip_port;
            int port = atoi(delimiter + 1);

            if (port <= 0 || port > 65535)
            {
                printf("Port invalide. Utilisez un port entre 1 et 65535.\n");
                return 1;
            }

            printf("Démarrage du client...\n");
            int socket = client(ip_address, port);

            send_message(socket, "B2:A2");

        } else {

            game = init_game(LOCAL, is_artificial_intelligence);

        }
    }

    // Initializes display and also initializes click listener
    initialize_display(0, NULL, &game);

    return 0;
}

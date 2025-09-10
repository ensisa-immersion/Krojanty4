#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "include/server.h"
#include "include/client.h"
#include "include/display.h"
#include "include/game.h"

/**
 * Lance le code principal du serveur ou du client.
 * @param argc Nombre d'arguments de la ligne de commande.
 * @param argv Tableau des arguments de la ligne de commande.
 */
int main(int argc, char *argv[])
{
    // Vérification des arguments
    if (argc == 2 && strcmp(argv[1], "-l") == 0) {
        Game game = init_game();
        initialize_display(argc, argv, &game);
        printf("Démarrage du jeu en mode local...\n");
        // Boucle principale du jeu local
        while (!is_game_over(&game)) {
            update_display(&game);
            handle_local_input(&game);
        }
        printf("Partie terminée.\n");
        return 0;
    } else if (argc == 3 && strcmp(argv[1], "-c") == 0) {
        // Mode client
        char *ip_port = argv[2];
        char *delimiter = strchr(ip_port, ':');
        if (delimiter == NULL) {
            printf("Format incorrect. Utilisez <adresse_ip>:<port>\n");
            return 1;
        }
        *delimiter = '\0';
        const char *ip_address = ip_port;
        int port = atoi(delimiter + 1);
        if (port <= 0 || port > 65535) {
            printf("Port invalide. Utilisez un port entre 1 et 65535.\n");
            return 1;
        }
        printf("Démarrage du client...\n");
        Game game = init_game();
        initialize_display(argc, argv, &game);
        int socket = client(ip_address, port);
        send_message(socket, "B2:A2");
        return 0;
    } else if (argc == 3 && strcmp(argv[1], "-s") == 0) {
        // Mode serveur
        int port = atoi(argv[2]);
        if (port <= 0 || port > 65535) {
            printf("Port invalide. Utilisez un port entre 1 et 65535.\n");
            return 1;
        }
        printf("Démarrage du serveur sur le port %d...\n", port);
        Game game = init_game();
        initialize_display(argc, argv, &game);
        ServerConfig config = {
            .port = port,
            .game = &game
        };
        pthread_t server_thread;
        pthread_create(&server_thread, NULL, start_server, &config);
        printf("Serveur lancé en arrière-plan\n");
        return 0;
    } else {
        printf("Usage : %s -l | -c <adresse_ip>:<port> | -s <port>\n", argv[0]);
        printf("Exemple client : %s -c 127.0.0.1:8080\n", argv[0]);
        printf("Exemple serveur : %s -s 8080\n", argv[0]);
        printf("Exemple local : %s -l\n", argv[0]);
        return 1;
    }
}

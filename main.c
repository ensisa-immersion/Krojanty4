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
    if (argc != 3 || (strcmp(argv[1], "-c") != 0 && strcmp(argv[1], "-s") != 0))
    {
        printf("Usage : %s -c <adresse_ip>:<port> | -s <port>\n", argv[0]);
        printf("Exemple client : %s -c 127.0.0.1:8080\n", argv[0]);
        printf("Exemple serveur : %s -s 8080\n", argv[0]);
        return 1;
    }

    Game game = init_game();               // Initializes game_1
    initialize_display(argc, argv, &game); // Initializes display and also initializes click listener

    if (strcmp(argv[1], "-c") == 0)
    {
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
    }
    else if (strcmp(argv[1], "-s") == 0)
    {
        // Mode serveur
        int port = atoi(argv[2]);
        if (port <= 0 || port > 65535)
        {
            printf("Port invalide. Utilisez un port entre 1 et 65535.\n");
            return 1;
        }

        printf("Démarrage du serveur sur le port %d...\n", port);
        
        // Créer la config du serveur
        ServerConfig config = {
            .port = port,
            .game = &game
        };
        
        // Lancer le serveur dans un thread
        pthread_t server_thread;
        pthread_create(&server_thread, NULL, start_server, &config);
        
        printf("Serveur lancé en arrière-plan\n");
    }

    return 0;

    
}

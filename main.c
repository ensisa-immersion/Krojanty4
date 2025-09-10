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

    Game game = init_game();
    initialize_display(argc, argv, &game);

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
        printf("Serveur lancé (simulation, code réel en commentaire)\n");

        server(port);
    }
    else {
        printf("Argument inconnu.");
        return 1;
    }

    return 0;
}

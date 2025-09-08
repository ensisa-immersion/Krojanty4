#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/client.h"

/**
 * Lance le code principal du serveur.
 * @param argc Nombre d'arguments de la ligne de commande.
 * @param argv Tableau des arguments de la ligne de commande.
 */
int main(int argc, char *argv[]) {
    // Vérification des arguments
    if (argc != 3 || strcmp(argv[1], "-c") != 0) {
        printf("Usage : %s -c <adresse_ip>:<port>\n", argv[0]);
        printf("Exemple : %s -c 127.0.0.1:8080\n", argv[0]);
        return 1;
    }

    // Récupération de la chaîne ip:port
    char *ip_port = argv[2];
    char *delimiter = strchr(ip_port, ':');

    if (delimiter == NULL) {
        printf("Format incorrect. Utilisez <adresse_ip>:<port>\n");
        return 1;
    }

    // Extraction de l'adresse IP et du port
    *delimiter = '\0';
    const char *ip_address = ip_port;
    int port = atoi(delimiter + 1);

    // Vérification de la validité du port
    if (port <= 0 || port > 65535) {
        printf("Port invalide. Utilisez un port entre 1 et 65535.\n");
        return 1;
    }

    // Démarrage du client
    printf("Démarrage du client...\n");
    client("10.171.251.235", 5555);

    return 0;
}

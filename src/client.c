#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "../include/client.h"

/**
 * Fonction client qui permet de se connecter à un serveur donné par son adresse IP et son port.
 * @param ip_address Adresse IP du serveur.
 * @param port Port du serveur.
 * @return 0 en cas de succès, -1 en cas d'erreur.
 */
int client(const char *ip_address, int port) {
    // Création d'un socket client
    int client_socket;
    client_socket = socket(AF_INET, SOCK_STREAM, 0); // Initialisation du socket en utilisant l'IPv4 (AF_INET) et le protocole TCP (SOCK_STREAM)

    // Configuration de l'adresse du serveur auquel se connecter
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // Utilisation du protocole IPv4
    server_address.sin_port = htons(port); // Conversion du port en format réseau (host-to-network-short)
    server_address.sin_addr.s_addr = inet_addr(ip_address); // Conversion de l'adresse IP en format binaire

    // Établissement de la connexion au serveur
    int connection_status = connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if (connection_status == -1) fprintf(stderr, "Erreur lors de l'établissement de la connexion au serveur\n\n"); // Affichage d'un message d'erreur si la connexion échoue

    // Réception des données envoyées par le serveur
    char server_response[256]; // Buffer pour stocker la réponse
    recv(client_socket, &server_response, sizeof(server_response), 0); // Attente et réception des données

    // Affiche le résultat
    printf("Message du serveur : %s\n", server_response);

    // Met fin à la connexion et ferme le socket
    close(client_socket);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <pthread.h>

#include "../include/client.h"

void* receive_message(int client_socket) {
    // Réception des données envoyées par le serveur
    char server_response[256];
    while (1) { // Boucle infinie pour recevoir en continu
        ssize_t bytes_received = recv(client_socket, server_response, sizeof(server_response) - 1, 0);
        if (bytes_received <= 0) {
            perror("Erreur lors de la réception ou connexion fermée");
            break;
        }
        server_response[bytes_received] = '\0'; // Terminaison de la chaîne
        printf("Message du serveur : %s\n", server_response);
    }

    printf("je sors de la boucle");

    return NULL;
}

// Envoi d'un message au serveur
void send_message(int client_socket, char *message) {
    send(client_socket, message, sizeof(message), 0); // Envoi du message au serveur
    printf("Message envoyé au serveur : %s\n", message);
}

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

    char message[] = "B2:A2";
    send_message(client_socket, message);

    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_message(client_socket), NULL);
    //pthread_join(receive_thread, NULL);

    // Met fin à la connexion et ferme le socket
    //close(client_socket);
    return 0;
}
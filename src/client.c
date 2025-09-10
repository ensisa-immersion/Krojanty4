#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#include "display.h"
#include "game.h"

void *start_gui()
{
    Game game = init_game();
    initialize_display(0, NULL, &game);
    return NULL;
}

void *receive_message(int client_socket)
{
    // Réception des données envoyées par le serveur
    char server_response[256];
    while (1)
    { // Boucle infinie pour recevoir en continu
        ssize_t bytes_received = recv(client_socket, server_response, sizeof(server_response) - 1, 0);
        if (bytes_received <= 0)
        {
            perror("Erreur lors de la réception ou connexion fermée");
            break;
        }
        server_response[bytes_received] = '\0'; // Terminaison de la chaîne
        printf("Message du serveur : %s\n", server_response);
    }

    return NULL;
}

// Envoi d'un message au serveur
void *send_message(int client_socket, char *message)
{
    ssize_t bytes_sent = send(client_socket, message, strlen(message), 0); // Envoi du message au serveur
    if (bytes_sent == -1)
    {
        perror("Erreur lors de l'envoi du message");
    }
    else
    {
        printf("Message envoyé au serveur : %s\n", message);
    }
    return NULL;
}

int client(const char *ip_address, int port)
{
    // Création d'un socket client
    int client_socket;
    client_socket = socket(AF_INET, SOCK_STREAM, 0); // Initialisation du socket en utilisant l'IPv4 (AF_INET) et le protocole TCP (SOCK_STREAM)

    // Configuration de l'adresse du serveur auquel se connecter
    struct sockaddr_in server_address = {
        .sin_family = AF_INET,                   // Utilisation du protocole IPv4
        .sin_port = htons(port),                 // Conversion du port en format réseau (host-to-network-short)
        .sin_addr.s_addr = inet_addr(ip_address) // Conversion de l'adresse IP en format binaire
    };

    // Établissement de la connexion au serveur et gestion des erreurs
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("Erreur de connexion");
        close(client_socket);
        return -1;
    }

    pthread_t game_thread, receive_thread;

    pthread_create(&game_thread, NULL, start_gui, NULL);
    pthread_detach(game_thread);
    pthread_join(game_thread, NULL);

    pthread_create(&receive_thread, NULL, receive_message(client_socket), NULL);
    pthread_detach(receive_thread);
    pthread_join(receive_thread, NULL);

    return 0;
}

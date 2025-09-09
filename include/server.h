#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>

#define PORT_SERVEUR 5555
#define TAILLE_BUFFER 64

/* ----- Initialisation de l'adresse et de la socket ----- */
void initialiser_adresse(struct sockaddr_in *adresse, int port);
int initialiser_socket(struct sockaddr_in *adresse, int max_conn);
int accepter_client(int sock);

/* ----- Validation des coups ----- */
int coord_valide(char col, char row);
int entree_valide(const char *buffer);
int deplacement_valide(const char *buffer);

/* ----- Communication avec le client ----- */
void envoyer_message_client(int fd, const char *msg);
int lire_message_client(int fd, char *buffer, int max_size);

/* ----- Joueur 1 : fonction appelée depuis GUI ----- */
int joueur1_joue(int client_fd, const char *coup, int *tour);

#endif // SERVER_H

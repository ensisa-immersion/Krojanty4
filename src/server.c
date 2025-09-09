#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define PORT_SERVEUR 5555
#define TAILLE_BUFFER 64

/* ----- Initialisation de l'adresse et de la socket ----- */
void initialiser_adresse(struct sockaddr_in *adresse, int port)
{
    adresse->sin_family = AF_INET;
    adresse->sin_addr.s_addr = INADDR_ANY;
    adresse->sin_port = htons(port);
}

int initialiser_socket(struct sockaddr_in *adresse, int max_conn)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int option = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1)
    {
        perror("setsockopt");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (bind(sock, (struct sockaddr *)adresse, sizeof(*adresse)) != 0)
    {
        perror("bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (listen(sock, max_conn) == -1)
    {
        perror("listen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    fcntl(sock, F_SETFL, O_NONBLOCK);
    return sock;
}

int accepter_client(int sock)
{
    struct sockaddr_in adr;
    socklen_t taille = sizeof(adr);
    int cli = accept(sock, (struct sockaddr *)&adr, &taille);
    if (cli != -1)
    {
        fcntl(cli, F_SETFL, O_NONBLOCK);
        printf("[DEBUG] Client connecté (fd=%d) depuis %s:%d\n",
               cli, inet_ntoa(adr.sin_addr), ntohs(adr.sin_port));
    }
    return cli;
}

/* ----- Validation des coups ----- */
int coord_valide(char col, char row)
{
    return (col >= 'A' && col <= 'I' && row >= '1' && row <= '9');
}

int entree_valide(const char *buffer)
{
    int len = strlen(buffer);
    while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r'))
    {
        len--;
    }
    if (len != 5 || buffer[2] != ':')
        return 0;
    return coord_valide(buffer[0], buffer[1]) && coord_valide(buffer[3], buffer[4]);
}

int deplacement_valide(const char *buffer)
{
    int col1 = buffer[0] - 'A';
    int row1 = buffer[1] - '1';
    int col2 = buffer[3] - 'A';
    int row2 = buffer[4] - '1';

    if ((col1 != col2) && (row1 != row2))
        return 0; // diagonal interdit
    if (col1 == col2 && row1 == row2)
        return 0; // pas de mouvement
    return 1;
}

/* ----- Communication avec le client ----- */
void envoyer_message_client(int fd, const char *msg)
{
    if (fd == -1)
        return;

    size_t total_sent = 0;
    size_t msg_len = strlen(msg);

    while (total_sent < msg_len)
    {
        ssize_t sent = send(fd, msg + total_sent, msg_len - total_sent, 0);
        if (sent <= 0)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                perror("send");
                break;
            }
            usleep(1000);
            continue;
        }
        total_sent += sent;
    }
}

int lire_message_client(int fd, char *buffer, int max_size)
{
    int recu = recv(fd, buffer, max_size - 1, 0);
    if (recu > 0)
    {
        buffer[recu] = '\0';
        int len = strlen(buffer);
        while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r'))
        {
            buffer[--len] = '\0';
        }
        return len > 0 ? len : 0;
    }
    return recu;
}

/* ----- Joueur 1 : fonction appelée depuis GUI ----- */
int joueur1_joue(int client_fd, const char *coup, int *tour)
{
    if (!entree_valide(coup))
    {
        printf("[ERREUR] Coup invalide (format)\n");
        return 0;
    }
    if (!deplacement_valide(coup))
    {
        printf("[ERREUR] Coup invalide (déplacement)\n");
        return 0;
    }

    printf("[OK] Joueur 1 a joué: %s\n", coup);

    if (client_fd != -1)
    {
        char msg[128];
        snprintf(msg, sizeof(msg), "Joueur 1 a joué: %s\nVotre tour!\n", coup);
        envoyer_message_client(client_fd, msg);
    }

    *tour = 1; // Passe au joueur 2
    return 1;
}

/* ----- Boucle principale ----- */
int server(void)
{
    int client_joueur2 = -1;
    int partie_en_cours = 1;
    int tour = 0; // 0 = Joueur 1, 1 = Joueur 2

    struct sockaddr_in adr_serv;
    initialiser_adresse(&adr_serv, PORT_SERVEUR);
    int sock_serv = initialiser_socket(&adr_serv, 1);

    printf("=== SERVEUR DE JEU - JOUEUR 1 ===\n");
    printf("[INFO] Serveur en écoute sur le port %d\n", PORT_SERVEUR);
    printf("[INFO] Attente de la connexion du joueur 2...\n");

    // Attente connexion joueur 2
    while (client_joueur2 == -1)
    {
        int cli = accepter_client(sock_serv);
        if (cli != -1)
        {
            client_joueur2 = cli;
            envoyer_message_client(client_joueur2, "Bienvenue! Vous êtes le joueur 2.\n");
            printf("[INFO] Joueur 2 connecté! La partie peut commencer.\n");
        }
        usleep(100000);
    }

    char buffer[TAILLE_BUFFER + 1];

    // --- Test sans GUI : joueur 1 joue une seule fois ---
    joueur1_joue(client_joueur2, "A2:A3", &tour);

    // Boucle de jeu
    while (partie_en_cours)
    {
        if (tour == 0)
        {
            // Tour du joueur 1
            // Attente de l'appel depuis la GUI
        }
        else
        {
            // Tour du joueur 2
            int recu = lire_message_client(client_joueur2, buffer, TAILLE_BUFFER);

            if (recu > 0)
            {
                printf("[INFO] Joueur 2 a joué: %s\n", buffer);

                if (!entree_valide(buffer))
                {
                    envoyer_message_client(client_joueur2, "Format invalide! Utilisez A2:A3\n");
                }
                else if (!deplacement_valide(buffer))
                {
                    envoyer_message_client(client_joueur2, "Déplacement invalide!\n");
                }
                else
                {
                    envoyer_message_client(client_joueur2, "Coup accepté! Attendez...\n");
                    printf("[OK] Coup joueur 2 accepté.\n");
                    tour = 0; // Retour au joueur 1
                }
            }
            else if (recu == 0)
            {
                printf("[WARN] Joueur 2 s'est déconnecté.\n");
                close(client_joueur2);
                client_joueur2 = -1;
            }
            else if (recu < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
            {
                perror("recv");
                close(client_joueur2);
                client_joueur2 = -1;
            }
        }
        usleep(50000);
    }

    if (client_joueur2 != -1)
        close(client_joueur2);
    close(sock_serv);
    return 0;
}

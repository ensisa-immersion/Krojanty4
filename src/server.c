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
#define NB_JOUEURS 2
#define TAILLE_BUFFER 64

void initialiser_adresse(struct sockaddr_in *adresse, int port) {
    adresse->sin_family = AF_INET;
    adresse->sin_addr.s_addr = INADDR_ANY;
    adresse->sin_port = htons(port);
    printf("[DEBUG] Adresse initialisee pour le port %d\n", port);
}

int initialiser_socket(struct sockaddr_in *adresse, int max_conn) {
    printf("[DEBUG] Creation de la socket sur le port %d...\n", ntohs(adresse->sin_port));
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    int option = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if (bind(sock, (struct sockaddr *)adresse, sizeof(*adresse)) != 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    listen(sock, max_conn);
    fcntl(sock, F_SETFL, O_NONBLOCK);
    printf("[DEBUG] Socket ecoute sur le port %d\n", ntohs(adresse->sin_port));
    return sock;
}

int accepter_client(int sock) {
    struct sockaddr_in adr;
    socklen_t taille = sizeof(adr);
    int cli = accept(sock, (struct sockaddr*)&adr, &taille);
    if (cli != -1) {
        printf("[DEBUG] Nouvelle connexion acceptee (fd=%d) depuis %s:%d\n",
            cli, inet_ntoa(adr.sin_addr), ntohs(adr.sin_port));
    }
    return cli;
}

int coord_valide(char col, char row) {
    return (col >= 'A' && col <= 'I' && row >= '1' && row <= '9');
}

int entree_valide(const char *buffer) {
    // Format attendu: "A2:A3"
    if (strlen(buffer) != 5 || buffer[2] != ':')
        return 0;
    return coord_valide(buffer[0], buffer[1]) && coord_valide(buffer[3], buffer[4]);
}

void debug_send(int sock, const char *msg) {
    if (sock != -1) send(sock, msg, strlen(msg), 0);
}

int main(void) {
    int joueurs[NB_JOUEURS] = {-1, -1};
    int sock_debug = -1;

    struct sockaddr_in adr_serv;
    initialiser_adresse(&adr_serv, PORT_SERVEUR);

    int sock_serv = initialiser_socket(&adr_serv, NB_JOUEURS + 1);

    printf("[INFO] Serveur sur %d\n", PORT_SERVEUR);

    int nb_connectes = 0;
    printf("[INFO] Attente de connexion des joueurs et du client debug...\n");

    while (nb_connectes < NB_JOUEURS || sock_debug == -1) {
        int cli = accepter_client(sock_serv);
        if (cli != -1) {
            char buffer[TAILLE_BUFFER + 1] = {0};
            int recu = recv(cli, buffer, TAILLE_BUFFER, 0);
            buffer[recu > 0 ? recu : 0] = '\0';

            printf("[DEBUG] Message recu lors de la connexion: '%s'\n", buffer);

            if (sock_debug == -1 && strstr(buffer, "DEBUG") != NULL) {
                sock_debug = cli;
                printf("[INFO] Client debug connecte (fd=%d)\n", cli);
                debug_send(sock_debug, "[DEBUG] Client debug connecte\n");
            } else if (nb_connectes < NB_JOUEURS) {
                joueurs[nb_connectes++] = cli;
                char msg[] = "Connecte au serveur de jeu 9x9\n";
                send(cli, msg, strlen(msg), 0);
                printf("[INFO] Joueur %d connecte (fd=%d)\n", nb_connectes, cli);
                char dbgmsg[64];
                snprintf(dbgmsg, sizeof(dbgmsg), "[DEBUG] Joueur %d connecte\n", nb_connectes);
                debug_send(sock_debug, dbgmsg);
            } else {
                // Trop de connexions
                char msg[] = "Serveur complet\n";
                send(cli, msg, strlen(msg), 0);
                close(cli);
                printf("[WARN] Connexion refusee, serveur complet (fd=%d)\n", cli);
            }
        }
        usleep(100000);
    }
    printf("[INFO] Les deux joueurs sont connectes. Debut de la partie.\n");
    debug_send(sock_debug, "[DEBUG] Les deux joueurs sont connectes. Debut de la partie.\n");

    int tour = 0;
    char buffer[TAILLE_BUFFER + 1];
    while (1) {
        int sj = joueurs[tour];
        printf("[INFO] Attente du coup du joueur %d (fd=%d)...\n", tour + 1, sj);
        int recu = recv(sj, buffer, TAILLE_BUFFER, 0);
        if (recu > 0) {
            buffer[recu] = '\0';
            printf("[INFO] Recu du joueur %d: %s\n", tour + 1, buffer);
            char dbgmsg[128];
            snprintf(dbgmsg, sizeof(dbgmsg), "[DEBUG] Joueur %d a joue: %s\n", tour + 1, buffer);
            debug_send(sock_debug, dbgmsg);

            if (entree_valide(buffer)) {
                int autre = (tour + 1) % NB_JOUEURS;
                char msg_adv[TAILLE_BUFFER + 32];
                snprintf(msg_adv, sizeof(msg_adv), "Adversaire a joue: %s\n", buffer);
                send(joueurs[autre], msg_adv, strlen(msg_adv), 0);
                printf("[INFO] Coup valide du joueur %d, transmis a l'adversaire.\n", tour + 1);
                debug_send(sock_debug, "[DEBUG] Coup valide\n");
            } else {
                char msg_err[] = "Format ou coordonnees invalides. Exemple: A2:A3 (A-I, 1-9)\n";
                send(sj, msg_err, strlen(msg_err), 0);
                printf("[WARN] Coup invalide du joueur %d: %s\n", tour + 1, buffer);
                debug_send(sock_debug, "[DEBUG] Coup invalide\n");
            }
            tour = (tour + 1) % NB_JOUEURS;
        }
        usleep(100000);
    }
    close(sock_serv);
    return 0;
}

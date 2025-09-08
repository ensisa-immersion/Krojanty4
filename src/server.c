#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>

/**
 * Port d'écoute du serveur.
 */
#define PORT_SERVEUR 5555
/**
 * Nombre de joueurs maximum.
 */
#define NB_JOUEURS 2
/**
 * Taille maximale du buffer pour les messages.
 */
#define TAILLE_BUFFER 64

/**
 * Initialise une adresse sockaddr_in pour le serveur.
 * @param adresse Pointeur vers la structure sockaddr_in a initialiser.
 * @param port Port sur lequel le serveur écoute.
 */
void initialiser_adresse(struct sockaddr_in *adresse, int port) {
    adresse->sin_family = AF_INET;
    adresse->sin_addr.s_addr = INADDR_ANY;
    adresse->sin_port = htons(port);
    printf("[DEBUG] Adresse initialisee pour le port %d\n", port);
}

/**
 * Initialise un socket serveur.
 * @param adresse Pointeur vers la structure sockaddr_in contenant l'adresse du serveur.
 * @param max_conn Nombre maximum de connexions.
 * @return Le serveur socket
 */
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

/**
 * Accepte une connexion entrante.
 * @param sock Socket serveur.
 * @return Le socket client, -1 en cas d'erreur.
 */
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

/**
 * Valide si les coordonnées sont dans le format attendu.
 * @param col Colonne (A-I).
 * @param row Ligne (1-9).
 * @return 1 si valide, 0 sinon.
 */
int coord_valide(char col, char row) {
    return (col >= 'A' && col <= 'I' && row >= '1' && row <= '9');
}

/**
 * Valide l'entrée du joueur.
 * @param buffer Chaîne de caractères contenant l'entrée.
 * @return 1 si l'entrée est valide, 0 sinon.
 */
int entree_valide(const char *buffer) {
    // Format attendu: "A2:A3"
    if (strlen(buffer) != 5 || buffer[2] != ':')
        return 0;
    return coord_valide(buffer[0], buffer[1]) && coord_valide(buffer[3], buffer[4]);
}


/**
 * Vérifie si le déplacement est diagonal (interdit).
 * @param buffer Chaîne de caractères contenant l'entrée.
 * @return 1 si le déplacement n'est pas diagonal, 0 sinon.
 */
int deplacement_diagonale_interdit(const char *buffer) {
    int col1 = buffer[0] - 'A';
    int row1 = buffer[1] - '1';
    int col2 = buffer[3] - 'A';
    int row2 = buffer[4] - '1';
    // Déplacement diagonal si les deux coordonnées changent
    return !((col1 != col2) && (row1 != row2));
}

/**
 * Envoie un message à un client.
 * @param fd Descripteur de fichier du client.
 * @param msg Message à envoyer.
 */
void envoyer_message(int fd, const char *msg) {
    if (fd != -1) {
        send(fd, msg, strlen(msg), 0);
    }
}

/**
 * Point d'entrée principal du serveur.
 * @return Code de sortie.
 */
int main(void) {
    int joueurs[NB_JOUEURS] = {-1, -1};

    struct sockaddr_in adr_serv;
    initialiser_adresse(&adr_serv, PORT_SERVEUR);

    int sock_serv = initialiser_socket(&adr_serv, NB_JOUEURS);

    printf("[INFO] Serveur sur %d\n", PORT_SERVEUR);

    printf("[INFO] Attente de connexion des joueurs...\n");

    while (1) {
        // Acceptation des nouveaux clients si une place est libre
        int cli = accepter_client(sock_serv);
        if (cli != -1) {
            char buffer[TAILLE_BUFFER + 1] = {0};
            int recu = recv(cli, buffer, TAILLE_BUFFER, 0);
            buffer[recu > 0 ? recu : 0] = '\0';

            printf("[DEBUG] Message recu lors de la connexion: '%s'\n", buffer);

            int place = -1;
            for (int i = 0; i < NB_JOUEURS; i++) {
                if (joueurs[i] == -1) {
                    place = i;
                    break;
                }
            }
            if (place != -1) {
                joueurs[place] = cli;
                char msg[] = "Connecte au serveur de jeu 9x9\n";
                envoyer_message(cli, msg);
                printf("[INFO] Joueur %d connecte (fd=%d)\n", place + 1, cli);
            } else {
                char msg[] = "Serveur complet\n";
                envoyer_message(cli, msg);
                close(cli);
                printf("[WARN] Connexion refusee, serveur complet (fd=%d)\n", cli);
            }
        }

        // Si les deux joueurs sont connectés, on commence la partie
        if (joueurs[0] != -1 && joueurs[1] != -1) {
            printf("[INFO] Les deux joueurs sont connectes. Debut de la partie.\n");
            break;
        }
        usleep(100000);
    }

    int tour = 0;
    char buffer[TAILLE_BUFFER + 1];
    while (1) {
        int sj = joueurs[tour];
        if (sj == -1) {
            // Attente de reconnexion du joueur
            printf("[INFO] Joueur %d deconnecte, attente de reconnexion...\n", tour + 1);
            usleep(100000);
            continue;
        }
        printf("[INFO] Attente du coup du joueur %d (fd=%d)...\n", tour + 1, sj);
        int recu = recv(sj, buffer, TAILLE_BUFFER, 0);
        if (recu > 0) {
            buffer[recu] = '\0';
            printf("[INFO] Recu du joueur %d: %s\n", tour + 1, buffer);

            if (!entree_valide(buffer)) {
                char msg_err[] = "Format ou coordonnees invalides. Exemple: A2:A3 (A-I, 1-9)\n";
                envoyer_message(sj, msg_err);
                printf("[WARN] Coup invalide du joueur %d: %s\n", tour + 1, buffer);
            } else if (!deplacement_diagonale_interdit(buffer)) {
                char msg_err[] = "Deplacement diagonal interdit. Seuls les deplacements horizontaux ou verticaux sont autorises.\n";
                envoyer_message(sj, msg_err);
                printf("[WARN] Deplacement diagonal interdit pour joueur %d: %s\n", tour + 1, buffer);
            } else {
                int autre = (tour + 1) % NB_JOUEURS;
                char msg_adv[TAILLE_BUFFER + 32];
                snprintf(msg_adv, sizeof(msg_adv), "Adversaire a joue: %s\n", buffer);
                envoyer_message(joueurs[autre], msg_adv);
                printf("[INFO] Coup valide du joueur %d, transmis a l'adversaire.\n", tour + 1);
                tour = autre;
            }
        } else if (recu == 0) {
            // Déconnexion du joueur
            printf("[WARN] Joueur %d (fd=%d) deconnecte.\n", tour + 1, sj);
            close(sj);
            joueurs[tour] = -1;
        }
        usleep(100000);
    }
    close(sock_serv);
    return 0;
}

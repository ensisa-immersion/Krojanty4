#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <poll.h>

#define PORT_SERVEUR 5555
#define TAILLE_BUFFER 64

void initialiser_adresse(struct sockaddr_in *adresse, int port) {
    adresse->sin_family = AF_INET;
    adresse->sin_addr.s_addr = INADDR_ANY;
    adresse->sin_port = htons(port);
}

int initialiser_socket(struct sockaddr_in *adresse, int max_conn) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) { 
        perror("socket"); 
        exit(EXIT_FAILURE); 
    }

    int option = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1) {
        perror("setsockopt");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    if (bind(sock, (struct sockaddr *)adresse, sizeof(*adresse)) != 0) {
        perror("bind"); 
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    if (listen(sock, max_conn) == -1) {
        perror("listen");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    fcntl(sock, F_SETFL, O_NONBLOCK);
    return sock;
}

int accepter_client(int sock) {
    struct sockaddr_in adr;
    socklen_t taille = sizeof(adr);
    int cli = accept(sock, (struct sockaddr*)&adr, &taille);
    if (cli != -1) {
        fcntl(cli, F_SETFL, O_NONBLOCK);
        printf("[DEBUG] Client connecté (fd=%d) depuis %s:%d\n",
               cli, inet_ntoa(adr.sin_addr), ntohs(adr.sin_port));
    }
    return cli;
}

int coord_valide(char col, char row) {
    return (col >= 'A' && col <= 'I' && row >= '1' && row <= '9');
}

int entree_valide(const char *buffer) {
    int len = strlen(buffer);
    while (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r')) {
        len--;
    }
    
    if (len != 5 || buffer[2] != ':') return 0;
    return coord_valide(buffer[0], buffer[1]) && coord_valide(buffer[3], buffer[4]);
}

int deplacement_valide(const char *buffer) {
    int col1 = buffer[0] - 'A';
    int row1 = buffer[1] - '1';
    int col2 = buffer[3] - 'A';
    int row2 = buffer[4] - '1';
    
    if ((col1 != col2) && (row1 != row2)) {
        return 0; // diagonal interdit
    }
    
    if (col1 == col2 && row1 == row2) {
        return 0; // pas de mouvement
    }
    
    return 1;
}

void envoyer_message_client(int fd, const char *msg) {
    if (fd == -1) return;
    
    printf("[DEBUG] Envoi au client: '%s'", msg);
    size_t total_sent = 0;
    size_t msg_len = strlen(msg);
    
    while (total_sent < msg_len) {
        ssize_t sent = send(fd, msg + total_sent, msg_len - total_sent, 0);
        if (sent <= 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("send");
                break;
            }
            usleep(1000);
            continue;
        }
        total_sent += sent;
    }
}

int lire_message_client(int fd, char *buffer, int max_size) {
    int recu = recv(fd, buffer, max_size - 1, 0);
    if (recu > 0) {
        buffer[recu] = '\0';
        // Enlever les retours à la ligne
        int len = strlen(buffer);
        while (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r')) {
            buffer[--len] = '\0';
        }
        return len > 0 ? len : 0;
    }
    return recu;
}

int lire_entree_serveur(char *buffer, int max_size) {
    // Vérifier si il y a une entrée disponible sur stdin
    struct pollfd pfd;
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;
    
    int ret = poll(&pfd, 1, 0); // Non-bloquant
    if (ret > 0 && (pfd.revents & POLLIN)) {
        if (fgets(buffer, max_size, stdin) != NULL) {
            // Enlever le \n
            int len = strlen(buffer);
            if (len > 0 && buffer[len-1] == '\n') {
                buffer[len-1] = '\0';
                len--;
            }
            return len;
        }
    }
    return 0; // Pas d'entrée disponible
}

int main(void) {
    int client_joueur2 = -1;
    int partie_en_cours = 0;
    int tour = 0; // 0 = Serveur (Joueur 1), 1 = Client (Joueur 2)

    struct sockaddr_in adr_serv;
    initialiser_adresse(&adr_serv, PORT_SERVEUR);
    int sock_serv = initialiser_socket(&adr_serv, 1);

    printf("=== SERVEUR DE JEU - VOUS ÊTES LE JOUEUR 1 ===\n");
    printf("[INFO] Serveur en écoute sur le port %d\n", PORT_SERVEUR);
    printf("[INFO] Attente de la connexion du joueur 2...\n");

    char buffer[TAILLE_BUFFER + 1];

    // Attendre la connexion du joueur 2
    while (client_joueur2 == -1) {
        int cli = accepter_client(sock_serv);
        if (cli != -1) {
            client_joueur2 = cli;
            envoyer_message_client(client_joueur2, "Bienvenue! Vous êtes le joueur 2.\n");
            printf("[INFO] Joueur 2 connecté! La partie peut commencer.\n");
            partie_en_cours = 1;
        }
        usleep(100000);
    }

    printf("\n=== DÉBUT DE LA PARTIE ===\n");
    printf("Format des coups: A2:A3 (de A2 vers A3)\n");
    printf("C'est votre tour (Joueur 1)! Entrez votre coup: ");
    fflush(stdout);
    
    envoyer_message_client(client_joueur2, "La partie commence! Attendez votre tour.\n");

    // Boucle de jeu principale
    while (partie_en_cours) {
        if (tour == 0) {
            // Tour du serveur (Joueur 1)
            int entree_recue = lire_entree_serveur(buffer, TAILLE_BUFFER);
            
            if (entree_recue > 0) {
                printf("\n[INFO] Vous avez joué: %s\n", buffer);
                
                if (!entree_valide(buffer)) {
                    printf("[ERREUR] Format invalide! Utilisez: A2:A3\n");
                    printf("Votre tour (Joueur 1): ");
                    fflush(stdout);
                } else if (!deplacement_valide(buffer)) {
                    printf("[ERREUR] Déplacement invalide! (pas de diagonal/mouvement nul)\n");
                    printf("Votre tour (Joueur 1): ");
                    fflush(stdout);
                } else {
                    // Coup valide
                    printf("[OK] Coup accepté!\n");
                    
                    // Notifier le client
                    if (client_joueur2 != -1) {
                        char msg_client[128];
                        snprintf(msg_client, sizeof(msg_client), 
                                "Joueur 1 a joué: %s\nVotre tour!\n", buffer);
                        envoyer_message_client(client_joueur2, msg_client);
                    }
                    
                    // Passer au tour suivant
                    tour = 1;
                    printf("Attente du coup du joueur 2...\n");
                }
            }
        } else {
            // Tour du client (Joueur 2)
            if (client_joueur2 == -1) {
                printf("[WARN] Joueur 2 déconnecté, attente de reconnexion...\n");
                
                int cli = accepter_client(sock_serv);
                if (cli != -1) {
                    client_joueur2 = cli;
                    envoyer_message_client(client_joueur2, "Reconnecté! C'est votre tour.\n");
                    printf("[INFO] Joueur 2 reconnecté.\n");
                }
                usleep(100000);
                continue;
            }

            int recu = lire_message_client(client_joueur2, buffer, TAILLE_BUFFER);
            
            if (recu > 0) {
                printf("[INFO] Joueur 2 a joué: %s\n", buffer);
                
                if (!entree_valide(buffer)) {
                    envoyer_message_client(client_joueur2, "Format invalide! Utilisez: A2:A3\n");
                } else if (!deplacement_valide(buffer)) {
                    envoyer_message_client(client_joueur2, "Déplacement invalide!\n");
                } else {
                    // Coup valide
                    envoyer_message_client(client_joueur2, "Coup accepté! Attendez...\n");
                    
                    printf("[OK] Coup du joueur 2 accepté.\n");
                    printf("C'est votre tour (Joueur 1)! Entrez votre coup: ");
                    fflush(stdout);
                    
                    // Passer au tour suivant
                    tour = 0;
                }
            } else if (recu == 0) {
                printf("[WARN] Joueur 2 s'est déconnecté.\n");
                close(client_joueur2);
                client_joueur2 = -1;
            } else if (recu < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recv");
                printf("[ERROR] Erreur avec le joueur 2.\n");
                close(client_joueur2);
                client_joueur2 = -1;
            }
        }
        
        usleep(50000); // 50ms pour éviter une boucle trop rapide
    }

    // Nettoyage
    if (client_joueur2 != -1) {
        close(client_joueur2);
    }
    close(sock_serv);
    return 0;
}
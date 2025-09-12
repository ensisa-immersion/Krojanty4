#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "game.h"
#include "display.h"
#include "netutil.h"
#include "move_util.h"

/* Petit serveur 1v1: accepte 2 clients, relaye les coups (4 octets) */

/* Socket du client connecté (pour que le serveur puisse lui envoyer ses coups) */
int g_server_client_socket = -1;

/* Fonction pour envoyer un coup du serveur au client */
void send_message_to_client(int server_socket, const char *move4) {
    if (!move4 || strlen(move4) != 4) {
        fprintf(stderr, "[SERVER] move invalide (attendu 4 chars)\n");
        return;
    }
    if (server_socket < 0) {
        fprintf(stderr, "[SERVER] socket client invalide\n");
        return;
    }

    if (send_all(server_socket, move4, 4) == -1) {
        perror("[SERVER] send_all au client");
    } else {
        printf("[SERVER] Envoyé au client: %.4s\n", move4);
    }
}

typedef struct {
    int me_sock;
    int other_sock;
    Game *game;     /* si serveur-host applique aussi localement, utiliser post_move_to_gtk */
} SrvRxCtx;

static void *server_client_rx(void *arg) {
    SrvRxCtx *ctx = (SrvRxCtx*)arg;
    int me = ctx->me_sock, other = ctx->other_sock;
    Game *game = ctx->game;
    free(ctx);

    char m[4];
    for (;;) {
        int r = read_exact(me, m, 4);
        if (r == 1) {
            printf("[SERVER] Reçu coup client: %c%c%c%c\n", m[0], m[1], m[2], m[3]);

            /* Le serveur applique le coup reçu du client (joueur 2) */
            if (game) {
                printf("[SERVER] Application coup client sur interface serveur\n");
                post_move_to_gtk(game, m);
            }

            /* Relais au pair (pour mode 2 clients) - pour l'instant non utilisé */
            if (other >= 0) {
                if (send_all(other, m, 4) == -1) {
                    perror("[SERVER] send_all to peer");
                }
            }

        } else if (r == 0) {
            printf("[SERVER] Client fermé.\n");
            break;
        } else {
            perror("[SERVER] recv");
            break;
        }
    }

    close(me);
    if (g_server_client_socket == me) {
        g_server_client_socket = -1;
    }
    return NULL;
}

static int create_listen_socket(int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return -1; }

    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in adr;
    memset(&adr, 0, sizeof(adr));
    adr.sin_family = AF_INET;
    adr.sin_addr.s_addr = INADDR_ANY;
    adr.sin_port = htons(port);

    if (bind(s, (struct sockaddr*)&adr, sizeof(adr)) < 0) {
        perror("bind"); close(s); return -1;
    }
    if (listen(s, 2) < 0) {
        perror("listen"); close(s); return -1;
    }
    printf("[SERVER] Écoute sur 0.0.0.0:%d\n", port);
    return s;
}

/* Lance un match 1v1: attend 2 clients, crée 2 threads RX croisés */
int run_server_1v1(Game *game, int port) {
    int ls = create_listen_socket(port);
    if (ls < 0) return -1;

    struct sockaddr_in cli;
    socklen_t clen = sizeof(cli);

    printf("[SERVER] En attente du Client A…\n");
    int a = accept(ls, (struct sockaddr*)&cli, &clen);
    if (a < 0) { perror("accept A"); close(ls); return -1; }
    printf("[SERVER] Client A connecté.\n");

    printf("[SERVER] En attente du Client B…\n");
    int b = accept(ls, (struct sockaddr*)&cli, &clen);
    if (b < 0) { perror("accept B"); close(a); close(ls); return -1; }
    printf("[SERVER] Client B connecté.\n");

    /* Démarre RX pour A (envoie à B) */
    pthread_t thA, thB;

    SrvRxCtx *cA = (SrvRxCtx*)malloc(sizeof(*cA));
    cA->me_sock = a; cA->other_sock = b; cA->game = game;
    if (pthread_create(&thA, NULL, server_client_rx, cA) != 0) {
        perror("pthread_create A");
        free(cA); close(a); close(b); close(ls);
        return -1;
    }
    pthread_detach(thA);

    /* Démarre RX pour B (envoie à A) */
    SrvRxCtx *cB = (SrvRxCtx*)malloc(sizeof(*cB));
    cB->me_sock = b; cB->other_sock = a; cB->game = game;
    if (pthread_create(&thB, NULL, server_client_rx, cB) != 0) {
        perror("pthread_create B");
        free(cB); close(a); close(b); close(ls);
        return -1;
    }
    pthread_detach(thB);

    /* Le serveur garde l’écoute ouverte (ou pas selon ton design) */
    /* Ici on ferme l’écoute pour un match 1v1 fixe */
    close(ls);
    printf("[SERVER] Match lancé. RX threads actifs.\n");
    return 0;
}

/* Version simplifiée: serveur = joueur 1, attend 1 client = joueur 2 */
int run_server_host(Game *game, int port) {
    int ls = create_listen_socket(port);
    if (ls < 0) return -1;

    struct sockaddr_in cli;
    socklen_t clen = sizeof(cli);

    printf("[SERVER] En attente d'un client…\n");
    int client_sock = accept(ls, (struct sockaddr*)&cli, &clen);
    if (client_sock < 0) { perror("accept client"); close(ls); return -1; }
    printf("[SERVER] Client connecté. Vous pouvez jouer!\n");

    /* Sauvegarde le socket du client pour pouvoir lui envoyer des coups */
    g_server_client_socket = client_sock;

    /* Démarre RX pour le client */
    pthread_t client_thread;
    SrvRxCtx *client_ctx = (SrvRxCtx*)malloc(sizeof(*client_ctx));
    client_ctx->me_sock = client_sock;
    client_ctx->other_sock = -1; // Le serveur ne relaye pas, il traite directement
    client_ctx->game = game;

    if (pthread_create(&client_thread, NULL, server_client_rx, client_ctx) != 0) {
        perror("pthread_create client");
        free(client_ctx); close(client_sock); close(ls);
        g_server_client_socket = -1;
        return -1;
    }
    pthread_detach(client_thread);

    close(ls);
    printf("[SERVER] Match lancé. Thread RX client actif.\n");
    return 0;
}

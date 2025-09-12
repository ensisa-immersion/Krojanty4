#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "game.h"
#include "display.h"
#include "input.h"
#include "netutil.h"
#include "move_util.h"

/* Socket client global (si déjà déclaré ailleurs, garde une seule def) */
int g_client_socket = -1;

/* ===== Connexion au serveur ===== */
int connect_to_server(const char *ip, int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return -1; }

    struct sockaddr_in adr;
    memset(&adr, 0, sizeof(adr));
    adr.sin_family = AF_INET;
    adr.sin_port   = htons(port);
    if (inet_pton(AF_INET, ip, &adr.sin_addr) != 1) {
        perror("inet_pton");
        close(s);
        return -1;
    }
    if (connect(s, (struct sockaddr*)&adr, sizeof(adr)) < 0) {
        perror("connect");
        close(s);
        return -1;
    }
    g_client_socket = s;
    printf("[CLIENT] Connecté à %s:%d\n", ip, port);
    return s;
}

/* ===== Envoi d’un coup (4 octets) ===== */
void send_message(int client_socket, const char *move4) {
    if (!move4 || strlen(move4) != 4) {
        fprintf(stderr, "[CLIENT] move invalide (attendu 4 chars)\n");
        return;
    }
    if (client_socket < 0) { fprintf(stderr, "[CLIENT] socket invalide\n"); return; }

    if (send_all(client_socket, move4, 4) == -1) {
        perror("[CLIENT] send_all");
    } else {
        printf("[CLIENT] Envoyé: %.4s\n", move4);
    }
}

/* ===== Thread de réception client ===== */
typedef struct { int sock; Game *game; } ClientRxCtx;

static void *client_rx_thread(void *arg) {
    ClientRxCtx *ctx = (ClientRxCtx*)arg;
    int s = ctx->sock;
    Game *game = ctx->game;
    free(ctx);

    char m[4];
    for (;;) {
        int r = read_exact(s, m, 4);
        if (r == 1) {
            printf("[CLIENT] Reçu coup serveur: %c%c%c%c\n", m[0], m[1], m[2], m[3]);

            /* Le client applique le coup reçu du serveur (joueur 1) */
            printf("[CLIENT] Application coup serveur sur interface client\n");
            post_move_to_gtk(game, m);

        } else if (r == 0) {
            printf("[CLIENT] Serveur fermé proprement.\n");
            break;
        } else {
            perror("[CLIENT] recv");
            break;
        }
    }
    close(s);
    if (g_client_socket == s) g_client_socket = -1;
    return NULL;
}

/* Démarrage du RX (à appeler après connect) */
int start_client_rx(Game *game) {
    if (g_client_socket < 0) return -1;
    pthread_t th;
    ClientRxCtx *c = (ClientRxCtx*)malloc(sizeof(*c));
    if (!c) return -1;
    c->sock = g_client_socket;
    c->game = game;
    if (pthread_create(&th, NULL, client_rx_thread, c) != 0) {
        perror("pthread_create");
        free(c);
        return -1;
    }
    pthread_detach(th);
    return 0;
}

/* (optionnel) Fermeture propre */
void client_close(void) {
    if (g_client_socket >= 0) {
        close(g_client_socket);
        g_client_socket = -1;
    }
}

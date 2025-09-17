/**
 * @file server.c
 * @brief Implémentation du serveur réseau pour le jeu
 * 
 * Ce fichier contient toutes les fonctions liées au serveur réseau du jeu, incluant :
 * - La création et gestion des sockets d'écoute TCP
 * - L'acceptation et la gestion des connexions clients multiples
 * - Le relais des mouvements entre clients (mode 1v1)
 * - La gestion du mode serveur-host (serveur + interface locale)
 * - La gestion des threads de communication réseau par client
 * - L'application locale des mouvements reçus des clients
 * 
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 */

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
#include "input.h"

/** @brief Socket global du client connecté (pour que le serveur puisse lui envoyer ses coups) */
int g_server_client_socket = -1;

/**
 * @brief Envoie un mouvement au client connecté via le socket TCP
 * 
 * Cette fonction transmet un mouvement de 4 caractères au client connecté.
 * Elle vérifie la validité du message et la disponibilité du socket avant
 * l'envoi. Utilise send_all() pour garantir l'envoi complet des données.
 * 
 * @param server_socket Socket de connexion au client (g_server_client_socket)
 * @param move4 Chaîne de 4 caractères représentant le mouvement (ex: "a1b2")
 * @return void
 */
void send_message_to_client(int server_socket, const char *move4) {
    // Validation du format du mouvement
    if (!move4 || strlen(move4) != 4) {
        fprintf(stderr, "[SERVER] move invalide (attendu 4 chars)\n");
        return;
    }
    
    // Vérification de la validité du socket
    if (server_socket < 0) {
        fprintf(stderr, "[SERVER] socket client invalide\n");
        return;
    }

    // Envoi du mouvement avec gestion d'erreur
    if (send_all(server_socket, move4, 4) == -1) {
        perror("[SERVER] send_all au client");
    } else {
        printf("[SERVER] Envoyé au client: %.4s\n", move4);
    }
}

/**
 * @struct SrvRxCtx
 * @brief Structure de contexte pour les threads de réception du serveur
 * 
 * Cette structure contient les informations nécessaires pour les threads
 * qui écoutent les messages des clients connectés et gèrent le relais.
 */
typedef struct {
    int me_sock;      /**< Socket de communication avec ce client */
    int other_sock;   /**< Socket du client pair (pour relais en mode 1v1) */
    Game *game;       /**< Pointeur vers la structure de jeu (si serveur-host applique localement) */
} SrvRxCtx;

/**
 * @brief Thread de réception des mouvements d'un client
 * 
 * Cette fonction s'exécute dans un thread séparé pour chaque client connecté.
 * Elle écoute en permanence les messages du client assigné. Quand un mouvement
 * de 4 caractères est reçu, il peut être :
 * - Appliqué sur l'interface locale (si game non NULL, mode serveur-host)
 * - Relayé vers un autre client (si other_sock >= 0, mode 1v1)
 * Le thread se termine proprement en cas de déconnexion du client.
 * 
 * @param arg Pointeur vers la structure SrvRxCtx contenant le contexte
 * @return void* NULL à la fin de l'exécution du thread
 */
static void *server_client_rx(void *arg) {
    // Récupération du contexte et libération de la mémoire allouée
    SrvRxCtx *ctx = (SrvRxCtx*)arg;
    int me = ctx->me_sock, other = ctx->other_sock;
    Game *game = ctx->game;
    free(ctx);

    char m[4]; // Buffer pour recevoir les mouvements de 4 caractères
    
    // Boucle principale de réception
    for (;;) {
        // Lecture exacte de 4 caractères depuis le client
        int r = read_exact(me, m, 4);
        
        if (r == 1) {
            // Mouvement reçu avec succès
            printf("[SERVER] Reçu coup client: %c%c%c%c\n", m[0], m[1], m[2], m[3]);

            // Application locale du coup reçu (P1 = Bleu = tours pairs) si mode serveur-host
            if (game) {
                printf("[SERVER] Application coup client (P1/Bleu) sur interface serveur\n");
                post_move_to_gtk(game, m);
            }

            // Relais vers l'autre client (pour mode 2 clients 1v1)
            if (other >= 0) {
                if (send_all(other, m, 4) == -1) {
                    perror("[SERVER] send_all to peer");
                }
            }

        } else if (r == 0) {
            // Client fermé proprement
            printf("[SERVER] Client fermé.\n");
            break;
        } else {
            // Erreur de réception
            perror("[SERVER] recv");
            break;
        }
    }

    // Nettoyage et fermeture du socket à la fin du thread
    close(me);
    if (g_server_client_socket == me) {
        g_server_client_socket = -1;
    }
    return NULL;
}

/**
 * @brief Crée et configure un socket TCP en mode écoute
 * 
 * Cette fonction crée un socket TCP, configure l'option SO_REUSEADDR
 * pour éviter les erreurs "Address already in use", bind le socket sur
 * toutes les interfaces (INADDR_ANY) au port spécifié, et le met en écoute.
 * 
 * @param port Numéro de port sur lequel écouter (1-65535)
 * @return int Socket en écoute en cas de succès, -1 en cas d'erreur
 */
static int create_listen_socket(int port) {
    // Création du socket TCP
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return -1; }

    // Configuration de l'option SO_REUSEADDR pour éviter "Address already in use"
    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Configuration de l'adresse d'écoute (toutes les interfaces)
    struct sockaddr_in adr;
    memset(&adr, 0, sizeof(adr));
    adr.sin_family = AF_INET;
    adr.sin_addr.s_addr = INADDR_ANY;
    adr.sin_port = htons(port);

    // Liaison du socket à l'adresse et au port
    if (bind(s, (struct sockaddr*)&adr, sizeof(adr)) < 0) {
        perror("bind"); close(s); return -1;
    }
    
    // Mise en écoute avec une file d'attente de 2 connexions
    if (listen(s, 2) < 0) {
        perror("listen"); close(s); return -1;
    }
    
    printf("[SERVER] Écoute sur 0.0.0.0:%d\n", port);
    return s;
}

/**
 * @brief Lance un serveur en mode 1v1 (relais entre 2 clients)
 * 
 * Cette fonction démarre un serveur qui accepte exactement 2 connexions
 * clientes et crée un système de relais bidirectionnel entre elles.
 * Chaque mouvement reçu d'un client est automatiquement transmis à l'autre.
 * Deux threads de réception sont créés pour gérer les communications
 * simultanées dans les deux sens.
 * 
 * @param game Pointeur vers la structure de jeu (peut être NULL si pas d'interface locale)
 * @param port Numéro de port sur lequel écouter
 * @return int 0 en cas de succès, -1 en cas d'erreur
 */
int run_server_1v1(Game *game, int port) {
    // Création du socket d'écoute
    int ls = create_listen_socket(port);
    if (ls < 0) return -1;

    struct sockaddr_in cli;
    socklen_t clen = sizeof(cli);

    // Acceptation de la première connexion (Client A)
    printf("[SERVER] En attente du Client A…\n");
    int a = accept(ls, (struct sockaddr*)&cli, &clen);
    if (a < 0) { perror("accept A"); close(ls); return -1; }
    printf("[SERVER] Client A connecté.\n");

    // Acceptation de la seconde connexion (Client B)
    printf("[SERVER] En attente du Client B…\n");
    int b = accept(ls, (struct sockaddr*)&cli, &clen);
    if (b < 0) { perror("accept B"); close(a); close(ls); return -1; }
    printf("[SERVER] Client B connecté.\n");

    // Création et lancement du thread de réception pour le Client A (relais vers B)
    pthread_t thA, thB;

    SrvRxCtx *cA = (SrvRxCtx*)malloc(sizeof(*cA));
    cA->me_sock = a; cA->other_sock = b; cA->game = game;
    if (pthread_create(&thA, NULL, server_client_rx, cA) != 0) {
        perror("pthread_create A");
        free(cA); close(a); close(b); close(ls);
        return -1;
    }
    pthread_detach(thA);

    // Création et lancement du thread de réception pour le Client B (relais vers A)
    SrvRxCtx *cB = (SrvRxCtx*)malloc(sizeof(*cB));
    cB->me_sock = b; cB->other_sock = a; cB->game = game;
    if (pthread_create(&thB, NULL, server_client_rx, cB) != 0) {
        perror("pthread_create B");
        free(cB); close(a); close(b); close(ls);
        return -1;
    }
    pthread_detach(thB);

    // Fermeture du socket d'écoute (plus besoin d'accepter de nouvelles connexions)
    close(ls);
    printf("[SERVER] Match lancé. RX threads actifs.\n");
    return 0;
}

/**
 * @brief Lance un serveur en mode host (1 client + interface locale)
 * 
 * Cette fonction démarre un serveur qui accepte une seule connexion cliente
 * et maintient une interface graphique locale. Le serveur peut à la fois :
 * - Recevoir et appliquer les mouvements du client distant sur l'interface locale
 * - Envoyer ses propres mouvements au client via g_server_client_socket
 * Un seul thread de réception est créé pour écouter le client connecté.
 * 
 * @param game Pointeur vers la structure de jeu (doit être non NULL pour appliquer les coups reçus)
 * @param port Numéro de port sur lequel écouter
 * @return int 0 en cas de succès, -1 en cas d'erreur
 */
int run_server_host(Game *game, int port) {
    // Création du socket d'écoute
    int ls = create_listen_socket(port);
    if (ls < 0) return -1;

    struct sockaddr_in cli;
    socklen_t clen = sizeof(cli);

    // Acceptation d'une seule connexion cliente
    printf("[SERVER] En attente d'un client…\n");
    int client_sock = accept(ls, (struct sockaddr*)&cli, &clen);
    if (client_sock < 0) { perror("accept client"); close(ls); return -1; }
    printf("[SERVER] Client connecté. Vous pouvez jouer!\n");

    // Sauvegarde du socket client dans la variable globale pour envoi des coups
    g_server_client_socket = client_sock;

    // Création et lancement du thread de réception pour le client
    pthread_t client_thread;
    SrvRxCtx *client_ctx = (SrvRxCtx*)malloc(sizeof(*client_ctx));
    client_ctx->me_sock = client_sock;
    client_ctx->other_sock = -1; // Pas de relais, juste application locale
    client_ctx->game = game;

    if (pthread_create(&client_thread, NULL, server_client_rx, client_ctx) != 0) {
        perror("pthread_create client");
        free(client_ctx); close(client_sock); close(ls);
        g_server_client_socket = -1;
        return -1;
    }
    pthread_detach(client_thread);

    // Fermeture du socket d'écoute (plus besoin d'accepter de nouvelles connexions)
    close(ls);
    printf("[SERVER] Match lancé. Thread RX client actif.\n");
    return 0;
}

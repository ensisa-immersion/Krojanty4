/**
 * @file client.c
 * @brief Implémentation du client réseau pour le jeu
 * 
 * Ce fichier contient toutes les fonctions liées au client réseau du jeu, incluant :
 * - La connexion au serveur de jeu distant
 * - L'envoi des mouvements du joueur local vers le serveur
 * - La réception et l'application des mouvements du joueur distant
 * - La gestion des threads de communication réseau
 * - La fermeture propre des connexions
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
#include "input.h"
#include "netutil.h"
#include "move_util.h"

/** @brief Socket global de connexion au serveur */
int g_client_socket = -1;

/**
 * @brief Établit une connexion TCP au serveur de jeu
 * 
 * Cette fonction crée un socket TCP, configure l'adresse du serveur
 * et établit une connexion. Elle sauvegarde le socket global pour
 * les opérations ultérieures de communication.
 * 
 * @param ip Adresse IP du serveur (format string, ex: "127.0.0.1")
 * @param port Numéro de port du serveur (1-65535)
 * @return int Socket connecté en cas de succès, -1 en cas d'erreur
 */
int connect_to_server(const char *ip, int port) {
    // Création du socket TCP
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return -1; }

    // Configuration de l'adresse du serveur
    struct sockaddr_in adr;
    memset(&adr, 0, sizeof(adr));
    adr.sin_family = AF_INET;
    adr.sin_port   = htons(port);
    if (inet_pton(AF_INET, ip, &adr.sin_addr) != 1) {
        perror("inet_pton");
        close(s);
        return -1;
    }
    
    // Établissement de la connexion
    if (connect(s, (struct sockaddr*)&adr, sizeof(adr)) < 0) {
        perror("connect");
        close(s);
        return -1;
    }
    
    // Sauvegarde du socket global et confirmation de connexion
    g_client_socket = s;
    printf("[CLIENT] Connecté à %s:%d\n", ip, port);
    return s;
}

/**
 * @brief Envoie un mouvement au serveur via le socket TCP
 * 
 * Cette fonction transmet un mouvement de 4 caractères au serveur de jeu.
 * Elle vérifie la validité du message et la disponibilité du socket avant
 * l'envoi. Utilise send_all() pour garantir l'envoi complet des données.
 * 
 * @param client_socket Socket de connexion au serveur
 * @param move4 Chaîne de 4 caractères représentant le mouvement (ex: "a1b2")
 * @return void
 */
void send_message(int client_socket, const char *move4) {
    // Validation du format du mouvement
    if (!move4 || strlen(move4) != 4) {
        fprintf(stderr, "[CLIENT] move invalide (attendu 4 chars)\n");
        return;
    }
    
    // Vérification de la validité du socket
    if (client_socket < 0) { 
        fprintf(stderr, "[CLIENT] socket invalide\n"); 
        return; 
    }

    // Envoi du mouvement avec gestion d'erreur
    if (send_all(client_socket, move4, 4) == -1) {
        perror("[CLIENT] send_all");
    } else {
        printf("[CLIENT] Envoyé: %.4s\n", move4);
    }
}

/**
 * @struct ClientRxCtx
 * @brief Structure de contexte pour le thread de réception
 * 
 * Cette structure contient les informations nécessaires pour le thread
 * qui écoute en permanence les messages du serveur distant.
 */
typedef struct { 
    int sock;     /**< Socket de communication avec le serveur */
    Game *game;   /**< Pointeur vers la structure de jeu principale */
} ClientRxCtx;

/**
 * @brief Thread de réception des mouvements du serveur
 * 
 * Cette fonction s'exécute dans un thread séparé et écoute en permanence
 * les messages du serveur. Quand un mouvement de 4 caractères est reçu,
 * il est automatiquement appliqué à l'interface graphique du jeu local.
 * Le thread se termine proprement en cas de déconnexion du serveur.
 * 
 * @param arg Pointeur vers la structure ClientRxCtx contenant le contexte
 * @return void* NULL à la fin de l'exécution du thread
 */
static void *client_rx_thread(void *arg) {
    // Récupération du contexte et libération de la mémoire allouée
    ClientRxCtx *ctx = (ClientRxCtx*)arg;
    int s = ctx->sock;
    Game *game = ctx->game;
    free(ctx);

    char m[4]; // Buffer pour recevoir les mouvements de 4 caractères
    
    // Boucle principale de réception
    for (;;) {
        // Lecture exacte de 4 caractères depuis le serveur
        int r = read_exact(s, m, 4);
        
        if (r == 1) {
            // Mouvement reçu avec succès
            printf("[CLIENT] Reçu coup serveur: %c%c%c%c\n", m[0], m[1], m[2], m[3]);

            // Application du coup reçu du serveur (P2 = Rouge = tours impairs)
            printf("[CLIENT] Application coup serveur (P2/Rouge) sur interface client\n");
            post_move_to_gtk(game, m);

        } else if (r == 0) {
            // Serveur fermé proprement
            printf("[CLIENT] Serveur fermé proprement.\n");
            break;
        } else {
            // Erreur de réception
            perror("[CLIENT] recv");
            break;
        }
    }
    
    // Nettoyage et fermeture du socket à la fin du thread
    close(s);
    if (g_client_socket == s) g_client_socket = -1;
    return NULL;
}

/**
 * @brief Démarre le thread de réception des messages du serveur
 * 
 * Cette fonction crée et lance un thread détaché qui écoutera en permanence
 * les messages du serveur. Le thread applique automatiquement les mouvements
 * reçus sur l'interface graphique du jeu local. La fonction alloue et configure
 * le contexte nécessaire au thread.
 * 
 * @param game Pointeur vers la structure de jeu principale
 * @return int 0 en cas de succès, -1 en cas d'erreur
 */
int start_client_rx(Game *game) {
    // Vérification de la validité du socket global
    if (g_client_socket < 0) return -1;
    
    // Allocation et configuration du contexte du thread
    pthread_t th;
    ClientRxCtx *c = (ClientRxCtx*)malloc(sizeof(*c));
    if (!c) return -1;
    c->sock = g_client_socket;
    c->game = game;
    
    // Création et détachement du thread de réception
    if (pthread_create(&th, NULL, client_rx_thread, c) != 0) {
        perror("pthread_create");
        free(c);
        return -1;
    }
    pthread_detach(th);
    return 0;
}

/**
 * @brief Ferme la connexion au serveur et libère les ressources
 * 
 * Cette fonction ferme proprement le socket de connexion au serveur
 * et remet à zéro la variable globale. Elle doit être appelée lors
 * de la fermeture de l'application ou en cas de déconnexion volontaire.
 * 
 * @return void
 */
void client_close(void) {
    // Fermeture du socket et réinitialisation de la variable globale
    if (g_client_socket >= 0) {
        close(g_client_socket);
        g_client_socket = -1;
    }
}

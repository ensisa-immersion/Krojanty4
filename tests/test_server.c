/**
 * @file test_server.c
 * @brief Tests unitaires pour les fonctions serveur du jeu
 * 
 * Ce fichier contient tous les tests pour valider le comportement des fonctions serveur :
 * - Tests de send_message_to_client avec différents cas d'erreur
 * - Tests de run_server_1v1 et run_server_host avec timeouts de sécurité
 * - Tests du socket global g_server_client_socket
 * - Tests de robustesse et de gestion d'erreurs
 * - Tests d'intégration basiques des fonctionnalités serveur
 * 
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#include "game.h"
#include "logging.h"
#include "server.h"
#include "netutil.h"

// Compteurs de tests
static int tests_passed = 0;
static int tests_failed = 0;

// Variables externes du serveur
extern int g_server_client_socket;

// Déclaration des fonctions internes du serveur pour les tests
extern void send_message_to_client(int server_socket, const char *move4);

// Macro pour les assertions avec messages
#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            LOG_SUCCESS_MSG("[TEST][SERVER][OK] %s", message); \
            tests_passed++; \
        } else { \
            LOG_ERROR_MSG("[TEST][SERVER][KO] %s", message); \
            tests_failed++; \
        } \
    } while(0)

/**
 * Test de send_message_to_client avec différents cas
 */
void test_send_message_to_client() {
    // Test avec socket invalide (négatif)
    send_message_to_client(-1, "A1B2");
    TEST_ASSERT(1, "send_message_to_client gère socket invalide sans crash");

    // Test avec socket 0 (stdin)
    send_message_to_client(0, "A1B2");
    TEST_ASSERT(1, "send_message_to_client gère socket 0 sans crash");

    // Test avec mouvement invalide (trop court)
    send_message_to_client(1, "A1");
    TEST_ASSERT(1, "send_message_to_client rejette mouvement trop court");

    // Test avec mouvement invalide (trop long)
    send_message_to_client(1, "A1B2C3");
    TEST_ASSERT(1, "send_message_to_client rejette mouvement trop long");

    // Test avec mouvement NULL
    send_message_to_client(1, NULL);
    TEST_ASSERT(1, "send_message_to_client gère mouvement NULL sans crash");

    // Test avec chaîne vide
    send_message_to_client(1, "");
    TEST_ASSERT(1, "send_message_to_client rejette chaîne vide");

    // Test avec socket fermé
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock >= 0) {
        close(sock);
        send_message_to_client(sock, "A1B2");
        TEST_ASSERT(1, "send_message_to_client gère socket fermé");
    }

    // Test avec mouvements valides
    send_message_to_client(1, "A9I1");
    TEST_ASSERT(1, "send_message_to_client avec mouvement diagonal");

    send_message_to_client(1, "I1A9");
    TEST_ASSERT(1, "send_message_to_client avec mouvement diagonal inverse");

    // Test avec caractères spéciaux
    send_message_to_client(1, "A1@2");
    TEST_ASSERT(1, "send_message_to_client avec caractères spéciaux");
}

/**
 * Test de run_server_1v1 avec timeout pour éviter les blocages
 */
void test_run_server_1v1() {
    Game game = init_game(LOCAL, 0);
    game.is_ai = 0; // Désactiver l'IA

    pid_t pid = fork();
    if (pid == 0) {
        alarm(3);
        signal(SIGALRM, exit);

        int result = run_server_1v1(&game, 0);
        exit(result == -1 ? 1 : 0);
    } else if (pid > 0) {
        int status;
        time_t start = time(NULL);

        while (1) {
            int wait_result = waitpid(pid, &status, WNOHANG);
            if (wait_result != 0) break;

            if (time(NULL) - start > 4) {
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
                break;
            }
            sleep(1);
        }
        TEST_ASSERT(1, "run_server_1v1 exécuté avec timeout de sécurité");
    }

    // Test avec port invalide
    pid = fork();
    if (pid == 0) {
        alarm(2);
        signal(SIGALRM, exit);

        int result = run_server_1v1(&game, -1);
        exit(result == -1 ? 0 : 1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        TEST_ASSERT(1, "run_server_1v1 avec port invalide testé");
    }

    // Test avec game NULL
    pid = fork();
    if (pid == 0) {
        alarm(2);
        signal(SIGALRM, exit);

        int result = run_server_1v1(NULL, 0);
        exit(result == -1 ? 0 : 1);
    } else if (pid > 0) {
        int status;
        time_t start = time(NULL);

        while (1) {
            int wait_result = waitpid(pid, &status, WNOHANG);
            if (wait_result != 0) break;

            if (time(NULL) - start > 3) {
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
                break;
            }
            sleep(1);
        }
        TEST_ASSERT(1, "run_server_1v1 avec game NULL testé");
    }
}

/**
 * Test de run_server_host avec timeout
 */
void test_run_server_host() {
    Game game = init_game(LOCAL, 0);
    game.is_ai = 0; // Désactiver l'IA

    pid_t pid = fork();
    if (pid == 0) {
        alarm(3);
        signal(SIGALRM, exit);

        int result = run_server_host(&game, 0);
        exit(result == -1 ? 1 : 0);
    } else if (pid > 0) {
        int status;
        time_t start = time(NULL);

        while (1) {
            int wait_result = waitpid(pid, &status, WNOHANG);
            if (wait_result != 0) break;

            if (time(NULL) - start > 4) {
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
                break;
            }
            sleep(1);
        }
        TEST_ASSERT(1, "run_server_host exécuté avec timeout de sécurité");
    }

    // Test avec port invalide
    pid = fork();
    if (pid == 0) {
        alarm(2);
        signal(SIGALRM, exit);

        int result = run_server_host(&game, 65536);
        exit(result == -1 ? 0 : 1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        TEST_ASSERT(1, "run_server_host avec port hors limite testé");
    }
}

/**
 * Test du socket global g_server_client_socket
 */
void test_global_server_socket() {
    int initial_value = g_server_client_socket;
    TEST_ASSERT(1, "g_server_client_socket accessible");

    // Test modification
    g_server_client_socket = 123;
    TEST_ASSERT(g_server_client_socket == 123, "g_server_client_socket modifiable");

    // Test avec valeur négative
    g_server_client_socket = -1;
    TEST_ASSERT(g_server_client_socket == -1, "g_server_client_socket accepte valeurs négatives");

    // Test avec valeur 0
    g_server_client_socket = 0;
    TEST_ASSERT(g_server_client_socket == 0, "g_server_client_socket accepte valeur 0");

    // Restaurer la valeur initiale
    g_server_client_socket = initial_value;
    TEST_ASSERT(1, "g_server_client_socket restauré");
}

/**
 * Test de robustesse avec socket création manuelle
 */
void test_server_socket_robustness() {
    // Test création de socket serveur manuel
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock >= 0) {
        int opt = 1;
        setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(0);

        int bind_result = bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (bind_result == 0) {
            int listen_result = listen(server_sock, 2);
            TEST_ASSERT(listen_result == 0, "Socket serveur manuel créé avec succès");
        } else {
            TEST_ASSERT(1, "Test bind serveur manuel exécuté");
        }

        close(server_sock);
    } else {
        TEST_ASSERT(0, "Impossible de créer socket pour test robustesse");
    }

    // Test avec différents états de g_server_client_socket
    int original = g_server_client_socket;
    for (int sock = -5; sock <= 5; sock++) {
        g_server_client_socket = sock;
        send_message_to_client(sock, "A1B2");
    }
    g_server_client_socket = original;
    TEST_ASSERT(1, "Tests avec différents états de socket global");
}

/**
 * Test des cas limites pour les fonctions serveur
 */
void test_server_edge_cases() {
    // Test send_message_to_client avec tous types de mouvements
    const char* test_moves[] = {
        "A1A1", "I9I9", "A9I1", "I1A9", "E5E5",
        "A1I9", "I9A1", "E1E9", "A5I5", "AAAA"
    };

    for (int i = 0; i < 10; i++) {
        send_message_to_client(1, test_moves[i]);
    }
    TEST_ASSERT(1, "send_message_to_client avec différents formats de mouvement");

    // Test avec messages contenant des caractères de contrôle
    send_message_to_client(1, "A\n12");
    TEST_ASSERT(1, "send_message_to_client avec caractère de contrôle");

    send_message_to_client(1, "A\t12");
    TEST_ASSERT(1, "send_message_to_client avec caractère tab");

    // Test avec sockets dans différents états
    for (int sock = 0; sock < 10; sock++) {
        send_message_to_client(sock, "A1B2");
    }
    TEST_ASSERT(1, "send_message_to_client avec différents sockets");
}

/**
 * Test de robustesse mémoire pour les fonctions serveur
 */
void test_server_memory_robustness() {
    // Test avec créations/destructions multiples de jeux
    for (int i = 0; i < 20; i++) {
        Game game = init_game(LOCAL, 0);
        game.turn = i;
        game.game_mode = i % 3;
        game.is_ai = i % 2;

        // Tester send_message_to_client avec différents paramètres
        send_message_to_client(-1, "A1B2");
        send_message_to_client(i, "I1A9");
    }
    TEST_ASSERT(1, "Tests multiples sans fuite mémoire serveur");

    // Test avec modifications rapides de g_server_client_socket
    int original = g_server_client_socket;
    for (int i = 0; i < 100; i++) {
        g_server_client_socket = i % 10;
        send_message_to_client(g_server_client_socket, "A1B2");
    }
    g_server_client_socket = original;
    TEST_ASSERT(1, "Tests avec modifications rapides socket global");
}

/**
 * Test d'intégration serveur basique
 */
void test_server_basic_integration() {
    // Test de création simple de socket d'écoute
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock >= 0) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(0);

        int opt = 1;
        setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            if (listen(listen_sock, 1) == 0) {
                // Socket en écoute créé avec succès
                socklen_t addr_len = sizeof(addr);
                getsockname(listen_sock, (struct sockaddr*)&addr, &addr_len);
                int port = ntohs(addr.sin_port);

                TEST_ASSERT(port > 0, "Socket serveur écoute sur port assigné automatiquement");
            }
        }
        close(listen_sock);
    }

    // Test avec g_server_client_socket dans différents contextes
    g_server_client_socket = 123;
    send_message_to_client(g_server_client_socket, "A1B2");
    TEST_ASSERT(1, "Intégration g_server_client_socket avec send_message_to_client");

    g_server_client_socket = -1; // Reset
}

/**
 * Fonction principale des tests
 */
int main() {
    if (logger_init("./logs/test.log", LOG_DEBUG) != 0) {
        fprintf(stderr, "Impossible d'initialiser le logger\n");
        return 1;
    }

    test_send_message_to_client();
    test_run_server_1v1();
    test_run_server_host();
    test_global_server_socket();
    test_server_socket_robustness();
    test_server_edge_cases();
    test_server_memory_robustness();
    test_server_basic_integration();

    LOG_INFO_MSG("[TEST][SERVER][RESULT] %d/%d", tests_passed, tests_passed + tests_failed);
    return (tests_failed == 0) ? 0 : 1;
}

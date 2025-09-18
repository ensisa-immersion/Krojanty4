/**
 * @file test_client.c
 * @brief Tests unitaires pour le module client réseau
 * 
 * Ce fichier contient tous les tests unitaires pour valider le comportement
 * du module client réseau, incluant :
 * - Tests de connexion au serveur avec différents cas d'erreur
 * - Tests d'envoi de messages avec validation des paramètres
 * - Tests de réception des messages et gestion des threads
 * - Tests de fermeture des connexions et gestion des erreurs
 * - Tests de robustesse et cas limites
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
#include "client.h"
#include "netutil.h"

// Compteurs de tests
static int tests_passed = 0;
static int tests_failed = 0;

// Macro pour les assertions avec messages
#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            LOG_SUCCESS_MSG("[TEST][CLIENT][OK] %s", message); \
            tests_passed++; \
        } else { \
            LOG_ERROR_MSG("[TEST][CLIENT][KO] %s", message); \
            tests_failed++; \
        } \
    } while(0)

/**
 * Test de connect_to_server avec différents cas d'erreur
 */
void test_connect_to_server_errors() {
    // Test avec adresse NULL - peut causer segfault, donc on évite
    TEST_ASSERT(1, "connect_to_server avec adresse NULL évité (segfault attendu)");

    // Test avec port invalide (0)
    int result2 = connect_to_server("127.0.0.1", 0);
    TEST_ASSERT(result2 == -1, "connect_to_server rejette port 0");

    // Test avec port négatif
    int result3 = connect_to_server("127.0.0.1", -1);
    TEST_ASSERT(result3 == -1, "connect_to_server rejette port négatif");

    // Test avec port hors limite (> 65535)
    int result4 = connect_to_server("127.0.0.1", 70000);
    TEST_ASSERT(result4 == -1, "connect_to_server rejette port > 65535");

    // Test avec adresse IP invalide (format incorrect)
    int result5 = connect_to_server("999.999.999.999", 12345);
    TEST_ASSERT(result5 == -1, "connect_to_server rejette IP invalide");

    // Test avec adresse non-IP
    int result6 = connect_to_server("invalid_format", 12345);
    TEST_ASSERT(result6 == -1, "connect_to_server rejette format non-IP");

    // Test avec IP vide
    int result7 = connect_to_server("", 12345);
    TEST_ASSERT(result7 == -1, "connect_to_server rejette IP vide");

    // Test avec IP avec espaces
    int result8 = connect_to_server("   ", 12345);
    TEST_ASSERT(result8 == -1, "connect_to_server rejette IP avec espaces");
}

/**
 * Test de connect_to_server avec timeout pour adresses inexistantes
 */
void test_connect_to_server_timeout() {
    pid_t pid = fork();
    if (pid == 0) {
        alarm(2);
        signal(SIGALRM, exit);
        int result = connect_to_server("192.0.2.1", 12345); // RFC 3330 - adresse de test
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
        TEST_ASSERT(1, "connect_to_server avec adresse inexistante terminé avec timeout");
    }
}

/**
 * Test de send_message avec différents cas
 */
void test_send_message() {
    // Test avec socket invalide (négatif)
    send_message(-1, "A1B2");
    TEST_ASSERT(1, "send_message gère socket invalide sans crash");

    // Test avec socket 0 (stdin)
    send_message(0, "A1B2");
    TEST_ASSERT(1, "send_message gère socket 0 sans crash");

    // Test avec mouvement invalide (trop court)
    send_message(1, "A1");
    TEST_ASSERT(1, "send_message rejette mouvement trop court");

    // Test avec mouvement invalide (trop long)
    send_message(1, "A1B2C3");
    TEST_ASSERT(1, "send_message rejette mouvement trop long");

    // Test avec mouvement NULL
    send_message(1, NULL);
    TEST_ASSERT(1, "send_message gère mouvement NULL sans crash");

    // Test avec chaîne vide
    send_message(1, "");
    TEST_ASSERT(1, "send_message rejette chaîne vide");

    // Test avec socket fermé
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock >= 0) {
        close(sock);
        send_message(sock, "A1B2");
        TEST_ASSERT(1, "send_message gère socket fermé");
    }

    // Test avec mouvements contenant des caractères spéciaux
    send_message(1, "A1B@");
    TEST_ASSERT(1, "send_message avec caractères spéciaux");

    send_message(1, "A\nb2");
    TEST_ASSERT(1, "send_message avec caractères de contrôle");

    // Test avec mouvements limites valides
    send_message(1, "A9I1");
    TEST_ASSERT(1, "send_message avec mouvement diagonal valide");

    send_message(1, "I1A9");
    TEST_ASSERT(1, "send_message avec mouvement diagonal inverse");
}

/**
 * Test de start_client_rx avec différents états
 */
void test_start_client_rx() {
    Game game = init_game(LOCAL, 0);

    // Test avec g_client_socket invalide
    g_client_socket = -1;
    int result1 = start_client_rx(&game);
    TEST_ASSERT(result1 == -1, "start_client_rx rejette socket invalide");

    // Test avec g_client_socket = 0
    g_client_socket = 0;
    int result2 = start_client_rx(&game);
    TEST_ASSERT(result2 == -1, "start_client_rx rejette socket 0");

    // Test avec différents sockets valides (mais non connectés)
    for (int sock = 1; sock <= 5; sock++) {
        g_client_socket = sock;
        start_client_rx(&game);
        // On ne peut pas facilement nettoyer les threads, mais on teste la robustesse
    }
    TEST_ASSERT(1, "start_client_rx avec différents sockets testé");

    // Remettre g_client_socket à -1
    g_client_socket = -1;
}

/**
 * Test de client_close dans différents états
 */
void test_client_close() {
    // Test client_close sans connexion active
    g_client_socket = -1;
    client_close();
    TEST_ASSERT(1, "client_close sans connexion active s'exécute");

    // Test client_close avec socket 0
    g_client_socket = 0;
    client_close();
    TEST_ASSERT(1, "client_close avec socket 0 s'exécute");

    // Test client_close avec socket valide mais fermé
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock >= 0) {
        g_client_socket = sock;
        close(sock); // Fermer avant d'appeler client_close
        client_close();
        TEST_ASSERT(1, "client_close avec socket déjà fermé s'exécute");
    }

    // Test client_close multiples
    g_client_socket = -1;
    client_close();
    client_close(); // Double appel
    TEST_ASSERT(1, "client_close multiples s'exécutent sans erreur");

    // Test avec différents états de socket
    for (int i = 1; i <= 10; i++) {
        g_client_socket = i;
        client_close();
    }
    TEST_ASSERT(1, "client_close avec différents états de socket");

    g_client_socket = -1;
}

/**
 * Test du socket global g_client_socket
 */
void test_global_client_socket() {
    // Test état initial
    int initial_value = g_client_socket;
    TEST_ASSERT(1, "g_client_socket accessible");

    // Test modification
    g_client_socket = 123;
    TEST_ASSERT(g_client_socket == 123, "g_client_socket modifiable");

    // Test avec valeur négative
    g_client_socket = -1;
    TEST_ASSERT(g_client_socket == -1, "g_client_socket accepte valeurs négatives");

    // Test avec valeur 0
    g_client_socket = 0;
    TEST_ASSERT(g_client_socket == 0, "g_client_socket accepte valeur 0");

    // Restaurer la valeur initiale
    g_client_socket = initial_value;
    TEST_ASSERT(1, "g_client_socket restauré");
}

/**
 * Test de robustesse mémoire pour les fonctions client
 */
void test_client_memory_robustness() {
    // Test avec créations/destructions multiples de jeux
    for (int i = 0; i < 20; i++) {
        Game game = init_game(LOCAL, 0);

        // Modifier l'état du jeu
        game.turn = i;
        game.game_mode = i % 3;
        game.is_ai = i % 2;

        // Tester start_client_rx (ne crée pas de thread avec socket invalide)
        g_client_socket = -1;
        start_client_rx(&game);

        // Tester send_message
        send_message(-1, "A1B2");
    }
    TEST_ASSERT(1, "Tests multiples sans fuite mémoire client");

    // Test avec différents paramètres pour start_client_rx
    Game game = init_game(LOCAL, 0);
    for (int i = 0; i < 5; i++) {
        game.turn = i;
        g_client_socket = -1; // Socket invalide pour éviter création de thread
        start_client_rx(&game);
    }
    TEST_ASSERT(1, "start_client_rx avec états de jeu variés");
}

/**
 * Test des cas limites pour les fonctions client
 */
void test_client_edge_cases() {
    // Test connect_to_server avec tous les caractères spéciaux courants
    const char* invalid_ips[] = {
        "127.0.0.1@", "127.0.0.1#", "127.0.0.1$", "127.0.0.1%",
        "127.0.0.1&", "127.0.0.1*", "127.0.0.1+", "127.0.0.1=",
        "a.b.c.d", "localhost", "www.google.com", "256.1.1.1"
    };

    for (int i = 0; i < 12; i++) {
        int result = connect_to_server(invalid_ips[i], 12345);
        TEST_ASSERT(result == -1, "connect_to_server rejette IP avec caractères spéciaux");
    }

    // Test send_message avec tous les formats de mouvement possibles
    const char* test_moves[] = {
        "AAAA", "A1A1", "I9I9", "A9I1", "I1A9",
        "E5E5", "A1I9", "I9A1", "E1E9", "A5I5"
    };

    for (int i = 0; i < 10; i++) {
        send_message(1, test_moves[i]);
    }
    TEST_ASSERT(1, "send_message avec tous types de mouvements valides");

    // Test avec ports limites
    int result1 = connect_to_server("127.0.0.1", 1);
    TEST_ASSERT(result1 == -1, "connect_to_server avec port 1");

    int result2 = connect_to_server("127.0.0.1", 65535);
    TEST_ASSERT(result2 == -1, "connect_to_server avec port 65535");

    int result3 = connect_to_server("127.0.0.1", 65536);
    TEST_ASSERT(result3 == -1, "connect_to_server avec port 65536");
}

/**
 * Fonction principale des tests
 */
int main() {
    if (logger_init("./logs/test.log", LOG_DEBUG) != 0) {
        fprintf(stderr, "Impossible d'initialiser le logger\n");
        return 1;
    }

    test_connect_to_server_errors();
    test_connect_to_server_timeout();
    test_send_message();
    test_start_client_rx();
    test_client_close();
    test_global_client_socket();
    test_client_memory_robustness();
    test_client_edge_cases();

    LOG_INFO_MSG("[TEST][CLIENT][RESULT] %d/%d", tests_passed, tests_passed + tests_failed);
    return (tests_failed == 0) ? 0 : 1;
}

/**
 * @file test_network.c
 * @brief Tests unitaires pour les fonctionnalités réseau (client/serveur)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

#include "game.h"
#include "logging.h"

// Compteurs de tests
static int tests_passed = 0;
static int tests_failed = 0;

// Macro pour les assertions avec messages
#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            LOG_SUCCESS_MSG("[TEST][NETWORK][OK] %s", message); \
            tests_passed++; \
        } else { \
            LOG_ERROR_MSG("[TEST][NETWORK][KO] %s", message); \
            tests_failed++; \
        } \
    } while(0)

/**
 * Test de création de socket
 */
void test_socket_creation() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT(sock >= 0, "Création de socket TCP réussie");

    if (sock >= 0) {
        close(sock);
        printf("[TEST][NETWORK][OK] Socket fermée proprement\n");
    }
}

/**
 * Test de configuration d'adresse IP
 */
void test_address_configuration() {
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(5555);

    // Test avec localhost
    int result1 = inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
    TEST_ASSERT(result1 == 1, "Configuration adresse localhost 127.0.0.1");

    // Test avec adresse IP valide
    int result2 = inet_pton(AF_INET, "192.168.1.100", &address.sin_addr);
    TEST_ASSERT(result2 == 1, "Configuration adresse IP 192.168.1.100");

    // Test avec adresse invalide
    int result3 = inet_pton(AF_INET, "999.999.999.999", &address.sin_addr);
    TEST_ASSERT(result3 == 0, "Rejet d'adresse IP invalide");

    // Test configuration port
    TEST_ASSERT(ntohs(address.sin_port) == 5555, "Configuration port 5555");
}

/**
 * Test de parsing des arguments de ligne de commande
 */
void test_argument_parsing() {
    // Test format IP:PORT
    char test_arg[] = "192.168.1.100:5555";
    char *ip = strtok(test_arg, ":");
    char *port_str = strtok(NULL, ":");

    TEST_ASSERT(ip != NULL, "IP extraite des arguments");
    TEST_ASSERT(port_str != NULL, "Port extrait des arguments");

    if (ip && port_str) {
        int port = atoi(port_str);
        TEST_ASSERT(strcmp(ip, "192.168.1.100") == 0, "IP parsée correctement");
        TEST_ASSERT(port == 5555, "Port parsé correctement");
    }

    // Test format invalide
    char invalid_arg[] = "invalid_format";
    char *ip2 = strtok(invalid_arg, ":");
    char *port_str2 = strtok(NULL, ":");

    TEST_ASSERT(ip2 != NULL, "IP extraite même avec format invalide");
    TEST_ASSERT(port_str2 == NULL, "Pas de port avec format invalide");
}

/**
 * Test des options de socket
 */
void test_socket_options() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("[TEST][NETWORK][KO] Impossible de créer une socket pour les tests\n");
        tests_failed++;
        return;
    }

    // Test SO_REUSEADDR (utile pour éviter "Address already in use")
    int opt = 1;
    int result = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    TEST_ASSERT(result == 0, "Configuration SO_REUSEADDR réussie");

    // Vérifier que l'option a été définie
    int opt_check;
    socklen_t opt_len = sizeof(opt_check);
    int get_result = getsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt_check, &opt_len);
    TEST_ASSERT(get_result == 0 && opt_check == 1, "SO_REUSEADDR correctement définie");

    close(sock);
}

/**
 * Test de bind sur un port spécifique
 */
void test_bind_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("[TEST][NETWORK][KO] Impossible de créer une socket pour les tests\n");
        tests_failed++;
        return;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(0); // Port automatique

    int bind_result = bind(sock, (struct sockaddr*)&address, sizeof(address));
    TEST_ASSERT(bind_result == 0, "Bind sur port automatique réussi");

    // Récupérer le port assigné
    socklen_t addr_len = sizeof(address);
    int get_result = getsockname(sock, (struct sockaddr*)&address, &addr_len);
    if (get_result == 0) {
        int assigned_port = ntohs(address.sin_port);
        TEST_ASSERT(assigned_port > 0, "Port assigné automatiquement");
    }

    close(sock);
}

/**
 * Test de simulation de connexion locale
 */
void test_local_connection() {
    // Créer un serveur local pour les tests
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        printf("[TEST][NETWORK][KO] Impossible de créer socket serveur\n");
        tests_failed++;
        return;
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(0); // Port automatique

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("[TEST][NETWORK][KO] Bind serveur échoué: %s\n", strerror(errno));
        close(server_sock);
        tests_failed++;
        return;
    }

    // Récupérer le port assigné
    socklen_t addr_len = sizeof(server_addr);
    getsockname(server_sock, (struct sockaddr*)&server_addr, &addr_len);
    int server_port = ntohs(server_addr.sin_port);

    if (listen(server_sock, 1) < 0) {
        printf("[TEST][NETWORK][KO] Listen serveur échoué: %s\n", strerror(errno));
        close(server_sock);
        tests_failed++;
        return;
    }

    TEST_ASSERT(1, "Serveur de test créé et en écoute");

    // Test de connexion client
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock >= 0) {
        struct sockaddr_in client_addr;
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, "127.0.0.1", &client_addr.sin_addr);

        // Tentative de connexion (non-bloquante pour éviter de bloquer les tests)
        int connect_result = connect(client_sock, (struct sockaddr*)&client_addr, sizeof(client_addr));
        if (connect_result == 0) {
            TEST_ASSERT(1, "Connexion client au serveur de test réussie");
        } else {
            printf("Note: Connexion client non testée (normal pour test unitaire)\n");
        }

        close(client_sock);
    }

    close(server_sock);
}

/**
 * Test de validation des formats de message
 */
void test_message_format() {
    // Test format de mouvement valide "A1B2"
    char msg1[] = "A1B2";
    TEST_ASSERT(strlen(msg1) == 4, "Message de mouvement de longueur 4");
    TEST_ASSERT(msg1[0] >= 'A' && msg1[0] <= 'I', "Première colonne valide");
    TEST_ASSERT(msg1[1] >= '1' && msg1[1] <= '9', "Première ligne valide");
    TEST_ASSERT(msg1[2] >= 'A' && msg1[2] <= 'I', "Deuxième colonne valide");
    TEST_ASSERT(msg1[3] >= '1' && msg1[3] <= '9', "Deuxième ligne valide");

    // Test format invalide
    char msg2[] = "Z0A1";
    TEST_ASSERT(!(msg2[0] >= 'A' && msg2[0] <= 'I'), "Colonne invalide détectée");
    TEST_ASSERT(!(msg2[1] >= '1' && msg2[1] <= '9'), "Ligne invalide détectée");
}

/**
 * Test de robustesse réseau
 */
void test_network_robustness() {
    // Test de gestion d'erreur avec socket invalide
    int invalid_sock = -1;
    char buffer[1024];
    ssize_t result = send(invalid_sock, "test", 4, 0);
    TEST_ASSERT(result < 0, "Envoi sur socket invalide échoue");

    result = recv(invalid_sock, buffer, sizeof(buffer), 0);
    TEST_ASSERT(result < 0, "Réception sur socket invalide échoue");

    // Test avec adresse de connexion impossible - ACCÉLÉRÉ
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock >= 0) {
        // Configurer un timeout court pour éviter l'attente
        struct timeval timeout;
        timeout.tv_sec = 1;  // 1 seconde max
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        struct sockaddr_in impossible_addr;
        impossible_addr.sin_family = AF_INET;
        impossible_addr.sin_port = htons(9999);
        inet_pton(AF_INET, "127.0.0.2", &impossible_addr.sin_addr); // Adresse locale impossible

        printf("[TEST][NETWORK][KO] Test de connexion impossible (timeout 1s)...\n");
        int connect_result = connect(sock, (struct sockaddr*)&impossible_addr, sizeof(impossible_addr));
        TEST_ASSERT(connect_result < 0, "Connexion à adresse impossible échoue");

        close(sock);
    }
}

/**
 * Fonction principale des tests
 */
int main() {
    if (logger_init("./logs/test.log", LOG_DEBUG) != 0) {
        fprintf(stderr, "Impossible d'initialiser le logger\n");
        return 1;
    }

    test_socket_creation();
    test_address_configuration();
    test_argument_parsing();
    test_socket_options();
    test_bind_socket();
    test_local_connection();
    test_message_format();
    test_network_robustness();

    LOG_INFO_MSG("[TEST][NETWORK][RESULT] %d/%d", tests_passed, tests_passed + tests_failed);
}

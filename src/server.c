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
#define JOUEUR_1 0
#define JOUEUR_2 1

typedef struct {
    int fd;
    int connecte;
    char nom[32];
} Joueur;

typedef enum {
    ETAT_ATTENTE_JOUEURS,
    ETAT_PARTIE_EN_COURS,
    ETAT_ATTENTE_RECONNEXION
} EtatServeur;

// Variables globales
Joueur joueurs[NB_JOUEURS];
EtatServeur etat_serveur = ETAT_ATTENTE_JOUEURS;
int tour_actuel = JOUEUR_1;

// Prototypes
void initialiser_serveur();
int creer_socket_serveur();
void accepter_nouveau_joueur(int sock_serveur);
void traiter_message_joueur(int index_joueur, const char* message);
void demander_coup(int index_joueur);
void envoyer_message(int fd, const char* message);
int valider_coup(const char* coup);
void diffuser_coup(int joueur_origine, const char* coup);
void gerer_deconnexion(int index_joueur);
void attendre_reconnexion();
void nettoyer_serveur();

int main() {
    printf("=== SERVEUR D'ECHECS MINIMAL ===\n");
    printf("Port: %d\n", PORT_SERVEUR);
    
    initialiser_serveur();
    int sock_serveur = creer_socket_serveur();
    
    printf("Serveur démarré. En attente des joueurs...\n\n");
    
    while (1) {
        switch (etat_serveur) {
            case ETAT_ATTENTE_JOUEURS:
                accepter_nouveau_joueur(sock_serveur);
                break;
                
            case ETAT_PARTIE_EN_COURS:
                demander_coup(tour_actuel);
                break;
                
            case ETAT_ATTENTE_RECONNEXION:
                attendre_reconnexion();
                accepter_nouveau_joueur(sock_serveur);
                break;
        }
        usleep(100000); // 100ms
    }
    
    nettoyer_serveur();
    close(sock_serveur);
    return 0;
}

void initialiser_serveur() {
    for (int i = 0; i < NB_JOUEURS; i++) {
        joueurs[i].fd = -1;
        joueurs[i].connecte = 0;
        memset(joueurs[i].nom, 0, sizeof(joueurs[i].nom));
    }
}

int creer_socket_serveur() {
    struct sockaddr_in adresse;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sock == -1) {
        perror("Erreur création socket");
        exit(EXIT_FAILURE);
    }
    
    int option = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    
    adresse.sin_family = AF_INET;
    adresse.sin_addr.s_addr = INADDR_ANY;
    adresse.sin_port = htons(PORT_SERVEUR);
    
    if (bind(sock, (struct sockaddr*)&adresse, sizeof(adresse)) != 0) {
        perror("Erreur bind");
        exit(EXIT_FAILURE);
    }
    
    if (listen(sock, NB_JOUEURS) != 0) {
        perror("Erreur listen");
        exit(EXIT_FAILURE);
    }
    
    // Socket non-bloquant pour accept
    fcntl(sock, F_SETFL, O_NONBLOCK);
    
    return sock;
}

void accepter_nouveau_joueur(int sock_serveur) {
    struct sockaddr_in adresse_client;
    socklen_t taille = sizeof(adresse_client);
    
    int nouveau_fd = accept(sock_serveur, (struct sockaddr*)&adresse_client, &taille);
    
    if (nouveau_fd == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Erreur accept");
        }
        return;
    }
    
    printf("Nouvelle connexion depuis %s:%d (fd=%d)\n", 
           inet_ntoa(adresse_client.sin_addr), 
           ntohs(adresse_client.sin_port), 
           nouveau_fd);
    
    // Recevoir le message d'identification du client
    char buffer[TAILLE_BUFFER + 1] = {0};
    int recu = recv(nouveau_fd, buffer, TAILLE_BUFFER, 0);
    
    if (recu <= 0) {
        printf("Erreur réception message d'identification\n");
        close(nouveau_fd);
        return;
    }
    
    buffer[recu] = '\0';
    // Supprimer le \n final si présent
    if (buffer[strlen(buffer)-1] == '\n') {
        buffer[strlen(buffer)-1] = '\0';
    }
    
    printf("Message d'identification reçu: '%s'\n", buffer);
    
    // Chercher une place libre ou reconnexion
    int place = -1;
    for (int i = 0; i < NB_JOUEURS; i++) {
        // Reconnexion d'un joueur existant
        if (strlen(joueurs[i].nom) > 0 && 
            strncmp(joueurs[i].nom, buffer, strlen(buffer)) == 0) {
            place = i;
            joueurs[i].fd = nouveau_fd;
            joueurs[i].connecte = 1;
            envoyer_message(nouveau_fd, "RECONNECTE: Bienvenue de retour!\n");
            printf("Joueur %d (%s) reconnecté\n", i + 1, joueurs[i].nom);
            break;
        }
        // Nouvelle place libre
        else if (!joueurs[i].connecte && place == -1) {
            place = i;
        }
    }
    
    // Nouveau joueur
    if (place != -1 && strlen(joueurs[place].nom) == 0) {
        joueurs[place].fd = nouveau_fd;
        joueurs[place].connecte = 1;
        strncpy(joueurs[place].nom, buffer, sizeof(joueurs[place].nom) - 1);
        
        char message_bienvenue[128];
        snprintf(message_bienvenue, sizeof(message_bienvenue), 
                "BIENVENUE: Vous êtes le joueur %d (%s)\n", place + 1, buffer);
        envoyer_message(nouveau_fd, message_bienvenue);
        
        printf("Nouveau joueur %d: %s (fd=%d)\n", place + 1, buffer, nouveau_fd);
    }
    // Serveur complet
    else if (place == -1) {
        envoyer_message(nouveau_fd, "ERREUR: Serveur complet\n");
        close(nouveau_fd);
        printf("Connexion refusée - serveur complet\n");
        return;
    }
    
    // Vérifier si on peut commencer la partie
    int joueurs_connectes = 0;
    for (int i = 0; i < NB_JOUEURS; i++) {
        if (joueurs[i].connecte) joueurs_connectes++;
    }
    
    if (joueurs_connectes == NB_JOUEURS && etat_serveur == ETAT_ATTENTE_JOUEURS) {
        printf("\n=== DEBUT DE LA PARTIE ===\n");
        etat_serveur = ETAT_PARTIE_EN_COURS;
        
        // Notifier les joueurs
        envoyer_message(joueurs[JOUEUR_1].fd, "PARTIE: La partie commence! Vous avez les blancs.\n");
        envoyer_message(joueurs[JOUEUR_2].fd, "PARTIE: La partie commence! Vous avez les noirs.\n");
        
        tour_actuel = JOUEUR_1; // Les blancs commencent
    }
    else if (etat_serveur == ETAT_ATTENTE_RECONNEXION && joueurs_connectes == NB_JOUEURS) {
        etat_serveur = ETAT_PARTIE_EN_COURS;
        printf("Tous les joueurs sont reconnectés. Reprise de la partie.\n");
    }
}

void demander_coup(int index_joueur) {
    if (!joueurs[index_joueur].connecte) {
        printf("Joueur %d déconnecté, passage en mode attente\n", index_joueur + 1);
        etat_serveur = ETAT_ATTENTE_RECONNEXION;
        return;
    }
    
    // Envoyer la demande de coup
    char message[64];
    snprintf(message, sizeof(message), "VOTRE_TOUR: Entrez votre coup (format A2:A3)\n");
    envoyer_message(joueurs[index_joueur].fd, message);
    
    // Attendre la réponse
    char buffer[TAILLE_BUFFER + 1] = {0};
    int recu = recv(joueurs[index_joueur].fd, buffer, TAILLE_BUFFER, MSG_DONTWAIT);
    
    if (recu > 0) {
        buffer[recu] = '\0';
        // Supprimer le \n final
        if (buffer[strlen(buffer)-1] == '\n') {
            buffer[strlen(buffer)-1] = '\0';
        }
        traiter_message_joueur(index_joueur, buffer);
    }
    else if (recu == 0) {
        // Joueur déconnecté
        gerer_deconnexion(index_joueur);
    }
    else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        printf("Erreur recv pour joueur %d\n", index_joueur + 1);
        gerer_deconnexion(index_joueur);
    }
}

void traiter_message_joueur(int index_joueur, const char* message) {
    printf("Joueur %d (%s): %s\n", index_joueur + 1, joueurs[index_joueur].nom, message);
    
    if (valider_coup(message)) {
        printf("Coup valide: %s\n", message);
        
        // Envoyer confirmation au joueur
        envoyer_message(joueurs[index_joueur].fd, "COUP_ACCEPTE: Coup joué avec succès\n");
        
        // Diffuser le coup à l'adversaire
        diffuser_coup(index_joueur, message);
        
        // Passer au joueur suivant
        tour_actuel = (tour_actuel + 1) % NB_JOUEURS;
    }
    else {
        printf("Coup invalide: %s\n", message);
        envoyer_message(joueurs[index_joueur].fd, "COUP_INVALIDE: Format incorrect (exemple: A2:A3)\n");
    }
}

int valider_coup(const char* coup) {
    // Vérifier le format: X#:X# (ex: A2:A3)
    if (strlen(coup) != 5 || coup[2] != ':') {
        return 0;
    }
    
    // Vérifier les coordonnées
    char col1 = coup[0], row1 = coup[1];
    char col2 = coup[3], row2 = coup[4];
    
    if (col1 < 'A' || col1 > 'H' || col2 < 'A' || col2 > 'H' ||
        row1 < '1' || row1 > '8' || row2 < '1' || row2 > '8') {
        return 0;
    }
    
    // Vérifier que ce n'est pas la même case
    if (col1 == col2 && row1 == row2) {
        return 0;
    }
    
    return 1;
}

void diffuser_coup(int joueur_origine, const char* coup) {
    int adversaire = (joueur_origine + 1) % NB_JOUEURS;
    
    if (joueurs[adversaire].connecte) {
        char message[128];
        snprintf(message, sizeof(message), 
                "COUP_ADVERSAIRE: %s a joué %s\n", 
                joueurs[joueur_origine].nom, coup);
        envoyer_message(joueurs[adversaire].fd, message);
    }
}

void gerer_deconnexion(int index_joueur) {
    printf("Déconnexion du joueur %d (%s)\n", index_joueur + 1, joueurs[index_joueur].nom);
    
    if (joueurs[index_joueur].fd != -1) {
        close(joueurs[index_joueur].fd);
    }
    
    joueurs[index_joueur].fd = -1;
    joueurs[index_joueur].connecte = 0;
    // On garde le nom pour permettre la reconnexion
    
    etat_serveur = ETAT_ATTENTE_RECONNEXION;
    
    // Notifier l'autre joueur
    int autre = (index_joueur + 1) % NB_JOUEURS;
    if (joueurs[autre].connecte) {
        char message[128];
        snprintf(message, sizeof(message), 
                "ATTENTE: %s s'est déconnecté. En attente de reconnexion...\n", 
                joueurs[index_joueur].nom);
        envoyer_message(joueurs[autre].fd, message);
    }
}

void attendre_reconnexion() {
    // Cette fonction est appelée en boucle jusqu'à ce qu'un joueur se reconnecte
    // Le traitement se fait dans accepter_nouveau_joueur()
}

void envoyer_message(int fd, const char* message) {
    if (fd != -1) {
        send(fd, message, strlen(message), 0);
        printf("→ [fd=%d]: %s", fd, message);
    }
}

void nettoyer_serveur() {
    for (int i = 0; i < NB_JOUEURS; i++) {
        if (joueurs[i].fd != -1) {
            close(joueurs[i].fd);
        }
    }
}
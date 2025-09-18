/**
 * @file netutil.c
 * @brief Implémentation des utilitaires réseau robustes pour communications TCP fiables
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 * 
 * Ce fichier contient l'implémentation des utilitaires réseau fondamentaux pour assurer des
 * communications TCP robustes et fiables, incluant :
 * - La gestion automatique des envois fragmentés avec send_all
 * - La lecture garantie de données complètes avec read_exact
 * - La gestion transparente des interruptions système (EINTR)
 * - La protection contre les envois/réceptions partiels
 * - La gestion d'erreurs standardisée avec codes de retour explicites
 */

#include "netutil.h"

// ============================================================================
// FONCTIONS D'ENVOI ROBUSTE
// ============================================================================

/**
 * @brief Envoie toutes les données sur un socket de manière garantie
 * 
 * Cette fonction robuste garantit l'envoi complet des données sur un socket TCP,
 * en gérant automatiquement les envois fragmentés qui peuvent survenir avec
 * la fonction send() standard. Elle continue les envois jusqu'à ce que toutes
 * les données soient transmises, en gérant les interruptions système (EINTR)
 * et en maintenant un offset interne pour suivre la progression.
 * 
 * @param fd Descripteur de fichier du socket TCP connecté
 * @param buf Pointeur vers les données à envoyer
 * @param len Nombre d'octets à envoyer (doit être > 0)
 * @return int 0 en cas de succès complet, -1 en cas d'erreur (errno défini)
 */
int send_all(int fd, const void *buf, size_t len) {
    const char *p = (const char*)buf;  // Pointeur pour parcourir les données
    size_t off = 0;                    // Offset des données déjà envoyées
    
    // Boucle jusqu'à envoi complet de toutes les données
    while (off < len) {
        ssize_t n = send(fd, p + off, len - off, 0);
        if (n < 0) {
            // Gestion des interruptions système : recommencer l'envoi
            if (errno == EINTR) continue;
            return -1;  // Erreur critique : arrêt avec code d'erreur
        }
        off += (size_t)n;  // Mise à jour de l'offset avec les données envoyées
    }
    return 0;  // Succès : toutes les données ont été envoyées
}

// ============================================================================
// FONCTIONS DE RÉCEPTION ROBUSTE
// ============================================================================

/**
 * @brief Lit exactement le nombre d'octets spécifié sur un socket
 * 
 * Cette fonction garantit la lecture complète des données demandées sur un socket TCP,
 * en gérant automatiquement les réceptions fragmentées et les interruptions système.
 * Elle est essentielle pour les protocoles qui nécessitent des messages de taille fixe,
 * comme le protocole de mouvement 4-caractères utilisé dans ce projet. La fonction
 * distingue clairement entre une fermeture propre du socket et une erreur réseau.
 * 
 * @param fd Descripteur de fichier du socket TCP connecté
 * @param buf Pointeur vers le buffer de réception (doit être alloué)
 * @param len Nombre exact d'octets à lire (doit être > 0)
 * @return int 1 = succès complet, 0 = socket fermé proprement par le pair, -1 = erreur (errno défini)
 */
int read_exact(int fd, void *buf, size_t len) {
    char *p = (char*)buf;  // Pointeur pour parcourir le buffer de réception
    size_t off = 0;        // Offset des données déjà reçues
    
    // Boucle jusqu'à réception complète de toutes les données demandées
    while (off < len) {
        ssize_t n = recv(fd, p + off, len - off, 0);
        
        // Détection de fermeture propre du socket par le pair
        if (n == 0) return 0;
        
        if (n < 0) {
            // Gestion des interruptions système : recommencer la réception
            if (errno == EINTR) continue;
            return -1;  // Erreur réseau critique
        }
        off += (size_t)n;  // Mise à jour de l'offset avec les données reçues
    }
    return 1;  // Succès : toutes les données demandées ont été reçues
}

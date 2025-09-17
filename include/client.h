/**
 * @file client.h
 * @brief Interface client pour la connexion au serveur de jeu
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 * 
 * Ce fichier contient les fonctions et variables pour la gestion du client :
 * - Connexion au serveur
 * - Réception des messages
 * - Envoi des coups
 * - Fermeture de la connexion
 */

#ifndef CLIENT_H
#define CLIENT_H

#include "game.h"

/**
 * @brief Socket global du client pour la connexion au serveur
 */
extern int g_client_socket;

/**
 * @brief Établit une connexion TCP avec le serveur de jeu
 * 
 * @param ip Adresse IP du serveur (format string)
 * @param port Port du serveur
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int connect_to_server(const char *ip, int port);

/**
 * @brief Démarre le thread de réception des messages du serveur
 * 
 * Lance un thread qui écoute en continu les messages entrants du serveur
 * et met à jour l'état du jeu en conséquence.
 * 
 * @param game Pointeur vers la structure de jeu à mettre à jour
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int start_client_rx(Game *game);

/**
 * @brief Envoie un message/coup au serveur
 * 
 * Transmet un coup de jeu au serveur via le socket client.
 * 
 * @param client_socket Socket de connexion au serveur
 * @param move4 Chaîne représentant le coup au format protocole (4 caractères)
 */
void send_message(int client_socket, const char *move4);

/**
 * @brief Ferme proprement la connexion client
 * 
 * Libère les ressources et ferme le socket de connexion au serveur.
 */
void client_close(void);

#endif

/**
 * @file server.h
 * @brief Interface serveur pour les parties multijoueurs en réseau
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 * 
 * Ce fichier contient les déclarations pour le serveur de jeu multijoueur, incluant :
 * - La gestion des connexions TCP entrantes pour les parties en réseau
 * - Le serveur 1v1 pour les parties entre deux clients distants
 * - Le serveur hôte pour les parties où le serveur participe activement
 * - La synchronisation des mouvements entre les joueurs connectés
 * - La gestion des threads pour les communications bidirectionnelles
 * - Le protocole de communication standardisé pour les mouvements
 */

#ifndef SERVER_H
#define SERVER_H

#include "game.h"

// ============================================================================
// FONCTIONS DE SERVEUR MULTIJOUEUR
// ============================================================================

/**
 * @brief Lance un serveur pour partie 1v1 entre deux clients distants
 * 
 * Cette fonction démarre un serveur TCP qui attend deux connexions clientes
 * pour organiser une partie entre deux joueurs distants. Le serveur agit
 * comme un relais neutre, transmettant les mouvements d'un client à l'autre
 * sans participer au jeu. Il gère la synchronisation des tours et assure
 * la cohérence de l'état de jeu entre les deux participants.
 * 
 * @param game Pointeur vers la structure de jeu à utiliser comme référence
 * @param port Numéro de port TCP sur lequel écouter (1-65535)
 * @return int 0 en cas de succès, -1 en cas d'erreur de serveur
 */
int run_server_1v1(Game *game, int port);

/**
 * @brief Lance un serveur où l'hôte participe activement à la partie
 * 
 * Cette fonction démarre un serveur TCP qui attend une connexion cliente
 * pour une partie où le serveur lui-même joue contre le client connecté.
 * Le serveur gère à la fois l'interface locale (affichage, entrées utilisateur)
 * et la communication réseau avec le client distant. Il synchronise les
 * mouvements locaux avec le client et applique les mouvements reçus.
 * 
 * @param game Pointeur vers la structure de jeu locale du serveur
 * @param port Numéro de port TCP sur lequel écouter (1-65535)
 * @return int 0 en cas de succès, -1 en cas d'erreur de serveur
 */
int run_server_host(Game *game, int port);

#endif

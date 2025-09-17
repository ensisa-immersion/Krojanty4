/**
 * @file move_util.h
 * @brief Utilitaires de conversion et transmission des mouvements
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 * 
 * Ce fichier contient les utilitaires pour la gestion des mouvements entre
 * les différents modules du jeu, incluant :
 * - La conversion entre formats de coordonnées (lettres/nombres, indices)
 * - La transmission thread-safe des mouvements vers l'interface GTK
 * - Les structures de données pour l'encapsulation des mouvements
 * - L'intégration avec la boucle d'événements GTK via g_idle_add
 * - Les fonctions de mapping et de validation des coordonnées
 */

#ifndef MOVE_UTIL_H
#define MOVE_UTIL_H

#include <gtk/gtk.h>
#include "game.h"

/**
 * @struct MoveTask
 * @brief Structure d'encapsulation pour les mouvements thread-safe
 * 
 * Cette structure contient toutes les informations nécessaires pour
 * transmettre un mouvement de manière sécurisée entre threads, notamment
 * depuis les threads réseau vers le thread principal GTK.
 */
typedef struct {
    Game *game;  /**< Pointeur vers la structure de jeu à modifier */
    int sr;      /**< Ligne source du mouvement (0-8) */
    int sc;      /**< Colonne source du mouvement (0-8) */
    int dr;      /**< Ligne destination du mouvement (0-8) */
    int dc;      /**< Colonne destination du mouvement (0-8) */
} MoveTask;

/**
 * @brief Table de correspondance pour la conversion colonnes lettres/indices
 * 
 * Cette constante globale fournit le mapping entre les lettres de colonnes
 * (a-i) utilisées dans le protocole réseau et les indices numériques (0-8)
 * utilisés en interne par le moteur de jeu.
 */
extern const char COLS_MAP[10];

// ============================================================================
// FONCTIONS DE CONVERSION DE COORDONNÉES
// ============================================================================

/**
 * @brief Convertit une lettre de colonne en indice numérique
 * 
 * Cette fonction transforme les lettres de colonnes du protocole réseau
 * ('a' à 'i') en indices numériques correspondants (0 à 8) utilisés
 * par les structures internes du jeu.
 * 
 * @param L Lettre de colonne à convertir ('a' à 'i')
 * @return int Indice de colonne (0-8), ou -1 si lettre invalide
 */
int col_from_letter(char L);

// ============================================================================
// FONCTIONS DE TRANSMISSION THREAD-SAFE
// ============================================================================

/**
 * @brief Callback GTK pour l'application des mouvements dans le thread principal
 * 
 * Cette fonction est exécutée dans le thread GTK principal via le mécanisme
 * g_idle_add. Elle applique de manière sécurisée un mouvement encapsulé dans
 * une structure MoveTask, garantissant la cohérence de l'interface graphique.
 * 
 * @param data Pointeur vers une structure MoveTask contenant le mouvement
 * @return gboolean G_SOURCE_REMOVE pour indiquer la fin de l'exécution
 */
gboolean apply_move_idle(gpointer data);

/**
 * @brief Transmet un mouvement réseau vers l'interface GTK
 * 
 * Cette fonction principale convertit un mouvement reçu du réseau (format
 * 4 caractères comme "a1b2") vers le format interne, puis l'envoie de manière
 * thread-safe vers le thread GTK pour application sur l'interface graphique.
 * Elle gère automatiquement la conversion de format et l'allocation mémoire.
 * 
 * @param game Pointeur vers la structure de jeu à modifier
 * @param m Tableau de 4 caractères représentant le mouvement (ex: "a1b2")
 * @return void
 */
void post_move_to_gtk(Game *game, const char m[4]);

#endif 

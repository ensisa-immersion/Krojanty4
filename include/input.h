/**
 * @file input.h
 * @brief Interface de gestion des entrées utilisateur et contrôle de l'IA
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 * 
 * Ce fichier contient les déclarations pour la gestion des interactions utilisateur
 * et le contrôle de l'intelligence artificielle, incluant :
 * - Le traitement des mouvements décidés par l'utilisateur
 * - La détection et gestion des clics sur l'interface graphique
 * - La coordination entre les mouvements humains et de l'IA
 * - L'intégration des mouvements IA dans les parties réseau
 * - La logique de contrôle des tours et initiatives
 */

#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED

#include "game.h"

// ============================================================================
// GESTION DES MOUVEMENTS UTILISATEUR
// ============================================================================

/**
 * @brief Traite un mouvement décidé par l'utilisateur
 * 
 * Cette fonction centrale gère l'exécution d'un mouvement choisi par le joueur.
 * Elle valide le mouvement, l'applique au plateau, gère les captures éventuelles,
 * et coordonne les actions post-mouvement (vérification de victoire, transmission
 * réseau, activation de l'IA adverse).
 * 
 * @param game Pointeur vers la structure de jeu à modifier
 * @param src_r Ligne de la case source du mouvement
 * @param src_c Colonne de la case source du mouvement
 * @param dst_r Ligne de la case destination du mouvement
 * @param dst_c Colonne de la case destination du mouvement
 * @return void
 */
void on_user_move_decided(Game *game, int src_r, int src_c, int dst_r, int dst_c);

/**
 * @brief Configure la détection des clics sur la zone de dessin
 * 
 * Cette fonction établit la connexion entre les événements de clic de souris
 * sur l'interface graphique et les fonctions de traitement correspondantes.
 * Elle configure les callbacks GTK nécessaires pour capturer les interactions
 * utilisateur avec le plateau de jeu.
 * 
 * @param drawing_area Widget GTK de la zone de dessin du plateau
 * @param game Pointeur vers la structure de jeu associée
 * @return void
 */
void detect_click(GtkWidget *drawing_area, Game *game);

// ============================================================================
// GESTION DE L'INTELLIGENCE ARTIFICIELLE
// ============================================================================

/**
 * @brief Exécute un mouvement de l'IA en mode réseau
 * 
 * Cette fonction spécialisée gère les mouvements de l'IA dans le contexte
 * d'une partie en réseau. Elle coordonne le calcul du mouvement optimal,
 * son application locale, et sa transmission au joueur distant via le
 * protocole réseau établi.
 * 
 * @param game Pointeur vers la structure de jeu en mode réseau
 * @return void
 */
void ai_network_move(Game *game);

/**
 * @brief Vérifie si l'IA doit effectuer le premier mouvement
 * 
 * Cette fonction détermine si l'intelligence artificielle doit prendre
 * l'initiative du premier coup selon la configuration de la partie
 * (mode de jeu, rôle de l'IA, règles d'ouverture). Elle est appelée
 * lors de l'initialisation d'une nouvelle partie.
 * 
 * @param game Pointeur vers la structure de jeu initialisée
 * @return void
 */
void check_ai_initial_move(Game *game);

/**
 * @brief Vérifie si c'est au tour de l'IA de jouer
 * 
 * Cette fonction examine l'état actuel du jeu pour déterminer si l'IA
 * doit prendre son tour. Elle prend en compte le numéro de tour, l'état
 * de la partie, le mode de jeu, et déclenche automatiquement le calcul
 * et l'exécution du mouvement IA si nécessaire.
 * 
 * @param game Pointeur vers la structure de jeu à examiner
 * @return void
 */
void check_ai_turn(Game *game);

#endif // INPUT_H_INCLUDED

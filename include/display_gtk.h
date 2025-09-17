/**
 * @file display.h
 * @brief Interface d'affichage graphique avec GTK pour le jeu
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 * 
 * Ce fichier contient les déclarations pour l'interface graphique du jeu, incluant :
 * - L'initialisation de l'interface GTK
 * - Les fonctions de dessin du plateau de jeu
 * - Les callbacks pour les événements graphiques
 * - Les fonctions de rafraîchissement de l'affichage
 * - L'intégration avec la boucle principale GTK
 */

#ifndef DISPLAY_H_INCLUDED
#define DISPLAY_H_INCLUDED

#include <gtk/gtk.h>
#include "game.h"
#include "const.h"

/**
 * @brief Fonction de callback pour le dessin du plateau de jeu
 * 
 * Cette fonction est appelée automatiquement par GTK lorsque la zone de dessin
 * doit être rafraîchie. Elle utilise Cairo pour dessiner le plateau, les pièces,
 * les cases sélectionnées et les mouvements possibles.
 * 
 * @param area Pointeur vers la zone de dessin GTK
 * @param cr Contexte Cairo pour les opérations de dessin
 * @param width Largeur de la zone de dessin en pixels
 * @param height Hauteur de la zone de dessin en pixels
 * @param user_data Données utilisateur (structure Game*)
 * @return void
 */
void draw_callback(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data);

/**
 * @brief Fonction d'activation de l'application GTK
 * 
 * Cette fonction est appelée lors du démarrage de l'application GTK.
 * Elle configure la fenêtre principale, crée les widgets nécessaires
 * et établit les connexions entre les signaux et les callbacks.
 * 
 * @param app Pointeur vers l'application GTK
 * @param user_data Données utilisateur passées à l'application
 * @return void
 */
void activate (GtkApplication *app, gpointer user_data);

/**
 * @brief Initialise le système d'affichage graphique
 * 
 * Cette fonction configure et démarre l'interface graphique GTK pour le jeu.
 * Elle crée l'application GTK, configure les paramètres d'affichage et lance
 * la boucle principale d'événements graphiques.
 * 
 * @param argc Nombre d'arguments de la ligne de commande
 * @param argv Tableau des arguments de la ligne de commande
 * @param game Pointeur vers la structure de jeu à afficher
 * @return int Code de retour de l'application GTK (0 = succès)
 */
int initialize_display(int argc, char** argv, Game* game);

/**
 * @brief Demande un rafraîchissement de l'affichage depuis un thread externe
 * 
 * Cette fonction permet aux threads réseau ou autres de demander une mise à jour
 * de l'affichage graphique de manière thread-safe. Elle utilise les mécanismes
 * GTK appropriés pour synchroniser les opérations graphiques.
 * 
 * @return void
 */
void display_request_redraw(void);

#endif // DISPLAY_H_INCLUDED

#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED

#include "game.h"

/* fonction pour traiter un mouvement décidé par l'utilisateur */
void on_user_move_decided(Game *game, int src_r, int src_c, int dst_r, int dst_c);

/* branchement du contrôleur */
void detect_click(GtkWidget *drawing_area, Game *game);

/* fonction pour que l'IA joue en mode réseau */
void ai_network_move(Game *game);

/* fonction pour vérifier si l'IA doit commencer */
void check_ai_initial_move(Game *game);

/* fonction pour vérifier si l'IA doit jouer après changement d'état */
void check_ai_turn(Game *game);

#endif // INPUT_H_INCLUDED
